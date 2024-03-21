#include "syscall.h"
#include "rpi.h"
#include "traincontrol.h"
#include "nameserver.h"
#include "track_data.h"
#include "heap.h"
#include "pathfinding.h"
#include "util.h"
#include "trainserver.h"

NavPath *hns_init(NavPath hns[], size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            hns[i].next = hns+i+1;
        }else{
            hns[i].next = NULL;
        }
        hns[i].sensor_path.num_sensors = 0;
    }
    return hns;
}
NavPath *getNextFreeHeapNode(NavPath **nextFreeHeapNode){
    if(*nextFreeHeapNode==NULL){
        return NULL;
    }
    NavPath *hn = *nextFreeHeapNode;
    *nextFreeHeapNode = (*nextFreeHeapNode)->next;
    return hn;
}
void reclaimHeapNode(NavPath **nextFreeHeapNode, NavPath *hn){
    hn->sensor_path.num_sensors = 0;
    hn->next = *nextFreeHeapNode;
    *nextFreeHeapNode = hn;
}

int heap_node_cmp(NavPath *n1, NavPath *n2) {
    if (n1->dist < n2->dist) {
        return -1;
    } else if (n1->dist > n2->dist) {
        return 1;
    }
    return 0;
}

int get_next_sensor(uint8_t src, char switch_states[], track_node track[], int *switch_err_sensor, int *next_switch, uint16_t *dist){
    track_node *cur_track_node;
    int cur_sensor = src;
    if(dist!=NULL){
        *dist = 0;
    }

    int branch_index;
    uint8_t dir = 0;
    cur_track_node = track + cur_sensor;
    if(switch_err_sensor!=NULL){
        *switch_err_sensor = -1;
    }
    if(next_switch!=NULL){
        *next_switch = -1;
    }
    while(cur_track_node->type!=NODE_SENSOR || cur_sensor==src){
        if((cur_track_node->type == NODE_SENSOR && cur_sensor==src) || cur_track_node->type == NODE_MERGE){
            cur_sensor = cur_track_node->edge[0].dest - track;
            if(dist!=NULL){
                *dist += cur_track_node->edge[0].dist;
            }
        }else if(cur_track_node->type == NODE_BRANCH){
            branch_index = cur_track_node->num-1;
            if(branch_index>=152) branch_index -= 134;
            if(switch_states[branch_index]=='S') dir = 0;
            else if(switch_states[branch_index]=='C')dir = 1;
            else{
                // uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown switch v %u %u %s %u\0338", cur_track_node->num, switch_states[branch_index], switch_states, switch_states[19]);
                return -3;
            } 
            if(next_switch!=NULL){
                *next_switch = cur_track_node->num;
            }
            if(switch_err_sensor!=NULL){
                *switch_err_sensor = cur_track_node->edge[1-dir].dest - track;
            }
            cur_sensor = cur_track_node->edge[dir].dest - track;
            if(dist!=NULL){
                *dist += cur_track_node->edge[dir].dist;
            }
        }else if(cur_track_node->type == NODE_EXIT){
            return -1;
        }else{
            return -2;
        }
        cur_track_node = track + cur_sensor;
    }
    return cur_sensor;
}

void add_to_sensor_path(NavPath *cur_node, uint8_t sensor, uint16_t dist, uint8_t reverse){
    uint8_t num_sensors = cur_node->sensor_path.num_sensors;
    cur_node->sensor_path.sensors[num_sensors] = sensor;
    cur_node->sensor_path.dists[num_sensors] = dist;
    cur_node->sensor_path.does_reverse[num_sensors] = reverse;
    // cur_node->sensor_path.speeds[num_sensors] = 255;
    cur_node->sensor_path.scs[0][num_sensors].switch_num = 255;
    cur_node->sensor_path.scs[1][num_sensors].switch_num = 255;
    cur_node->sensor_path.num_sensors += 1;
}

void get_speeds_from_reverse(NavPath *cur_node){
    for(int i = 0; i<cur_node->sensor_path.num_sensors; i++){
        cur_node->sensor_path.speeds[i] = 255;
        if(i==0 && cur_node->sensor_path.does_reverse[0]==0 && cur_node->sensor_path.does_reverse[1]==0){
            cur_node->sensor_path.speeds[0] = 14;
        }
        if(i>0 && cur_node->sensor_path.does_reverse[i-1]==1){
            cur_node->sensor_path.speeds[i] = 14;
        }
        if(i+2<cur_node->sensor_path.num_sensors && cur_node->sensor_path.does_reverse[i+2]==1){
            cur_node->sensor_path.speeds[i] = 8;
        }
        if(i+1<cur_node->sensor_path.num_sensors && cur_node->sensor_path.does_reverse[i+1]==1){
            cur_node->sensor_path.speeds[i] = 4;
        }
        if(i==cur_node->sensor_path.num_sensors-1){
            cur_node->sensor_path.speeds[i] = 255;
        }
    }
}

NavPath *dijkstra(uint8_t src, uint8_t dest, track_node *track, int track_len, char switch_states[], NavPath **nextFreeHeapNode){

    //TODO: start pathfinding maybe 2/3 sensors after so the train doesn't go off course
    /* 
    design decisions:
    using O(ElogV) dijkstra
    storing switches instead of nodes to save space
    storing switches in order or switch to make sure they all switch in time
    */

    Heap heap;
    NavPath *heap_node_ptrs[200];
    heap_init(&heap, heap_node_ptrs, 150, heap_node_cmp);

    uint32_t min_dist[track_len];
    for(int i = 0; i<track_len; i++){
        min_dist[i] = 2000000000;
    }
    min_dist[src] = 0;

    uint16_t reverse_cost = 400;

    int new_dist;
    int new_dest;
    int cur_dist;
    NavPath *cur_node, *next_node;
    track_node *cur_track_node;

    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = src;
    cur_node->dist = 0;
    cur_node->sensor_path.num_sensors = 0;
    cur_node->sensor_path.initial_scs[0].switch_num = 255;
    cur_node->sensor_path.initial_scs[1].switch_num = 255;
    heap_push(&heap, cur_node);

    cur_track_node = track + src;
    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = cur_track_node->reverse - track;
    cur_node->dist = reverse_cost;
    cur_node->sensor_path.num_sensors = 0;
    cur_node->sensor_path.initial_scs[0].switch_num = 255;
    cur_node->sensor_path.initial_scs[1].switch_num = 255;

    add_to_sensor_path(cur_node, src, 0, 1);
    // cur_node->sensor_path.sensors[0] = src;
    // cur_node->sensor_path.dists[0] = 0;
    // cur_node->sensor_path.does_reverse[0] = 1;
    // cur_node->sensor_path.num_sensors += 1;
    heap_push(&heap, cur_node);

    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kreverse: %u\0338", cur_node->node_index);
    // for(;;){}

    NavPath *res; 
    char dirs[2] = {'S', 'C'};

    int tmp_count = 0;

    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        cur_dist = cur_node->dist;

        cur_track_node = track + cur_node->node_index;
        if(cur_track_node->type == NODE_SENSOR){

            add_to_sensor_path(cur_node, cur_node->node_index, cur_node->dist, 0);
            // cur_node->sensor_path.sensors[cur_node->sensor_path.num_sensors] = cur_node->node_index;
            // cur_node->sensor_path.dists[cur_node->sensor_path.num_sensors] = cur_node->dist;
            // cur_node->sensor_path.does_reverse[cur_node->sensor_path.num_sensors] = 0;
            // cur_node->sensor_path.scs[0][cur_node->sensor_path.num_sensors].switch_num = 255;
            // cur_node->sensor_path.scs[1][cur_node->sensor_path.num_sensors].switch_num = 255;
            // cur_node->sensor_path.num_sensors += 1;
        }

        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[Knext node, %d, sensor[0]: %u\0338", 15+tmp_count, cur_node->node_index, cur_node->sensor_path.sensors[0]);
        // tmp_count += 1;

        if(cur_node->node_index == dest){
            // for(;;){}
            res = cur_node;
            get_speeds_from_reverse(res);
            while(heap.length!=0){
                cur_node = heap_pop(&heap);
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
            // uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
            return res;
        }else if((cur_track_node->reverse-track) == dest){
            cur_node->sensor_path.does_reverse[cur_node->sensor_path.num_sensors-1] = 1;
            // if(cur_node->sensor_path.num_sensors>2){
            //     cur_node->sensor_path.speeds[cur_node->sensor_path.num_sensors-3] = 4;
            // }

            // add_to_sensor_path(cur_node, dest, cur_node->dist, 0);
            // cur_node->sensor_path.sensors[cur_node->sensor_path.num_sensors] = dest;
            // cur_node->sensor_path.dists[cur_node->sensor_path.num_sensors] = cur_node->dist;
            // cur_node->sensor_path.does_reverse[cur_node->sensor_path.num_sensors] = 0;
            // cur_node->sensor_path.scs[0][cur_node->sensor_path.num_sensors].switch_num = 255;
            // cur_node->sensor_path.scs[1][cur_node->sensor_path.num_sensors].switch_num = 255;
            // cur_node->sensor_path.num_sensors += 1;
            cur_node->node_index = dest;
            cur_node->dist += reverse_cost;
            heap_push(&heap, cur_node);
            continue;
        }
        
        if(cur_track_node->type == NODE_SENSOR){
            new_dist = cur_dist + cur_track_node->edge[0].dist;
            new_dest = cur_track_node->edge[0].dest - track;
            if(min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                cur_node->dist = new_dist;
                cur_node->node_index = new_dest;
                heap_push(&heap, cur_node);
            }else{
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
        }else if(cur_track_node->type == NODE_MERGE){
            NavPath *next_node;
            next_node = getNextFreeHeapNode(nextFreeHeapNode);
            memcpy(&(next_node->sensor_path.initial_scs), &(cur_node->sensor_path.initial_scs), sizeof(SwitchChange)*2);
            memcpy(&(next_node->sensor_path.sensors), &(cur_node->sensor_path.sensors), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
            memcpy(&(next_node->sensor_path.dists), &(cur_node->sensor_path.dists), sizeof(uint16_t)*cur_node->sensor_path.num_sensors);
            memcpy(&(next_node->sensor_path.does_reverse), &(cur_node->sensor_path.does_reverse), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
            memcpy(&(next_node->sensor_path.scs[0]), &(cur_node->sensor_path.scs[0]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
            memcpy(&(next_node->sensor_path.scs[1]), &(cur_node->sensor_path.scs[1]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
            next_node->sensor_path.num_sensors = cur_node->sensor_path.num_sensors;

            // TODO: maybe don't need to go to next sensor to reverse if close
            uint8_t prev_sensor = cur_node->sensor_path.sensors[cur_node->sensor_path.num_sensors-1];
            uint16_t dist_bt_sensors;
            uint8_t next_sensor = get_next_sensor(prev_sensor, switch_states, track, NULL, NULL, &dist_bt_sensors);
            if(next_sensor==dest){
                add_to_sensor_path(next_node, next_sensor, next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors, 0);
                // next_node->sensor_path.sensors[next_node->sensor_path.num_sensors] = next_sensor;
                // next_node->sensor_path.dists[next_node->sensor_path.num_sensors] = next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors;
                // next_node->sensor_path.does_reverse[next_node->sensor_path.num_sensors] = 0;
                // next_node->sensor_path.scs[0][next_node->sensor_path.num_sensors].switch_num = 255;
                // next_node->sensor_path.scs[1][next_node->sensor_path.num_sensors].switch_num = 255;
                // next_node->sensor_path.num_sensors += 1;
                
                res = next_node;
                get_speeds_from_reverse(res);
                while(heap.length!=0){
                    cur_node = heap_pop(&heap);
                    reclaimHeapNode(nextFreeHeapNode, cur_node);
                }
                // uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
                return res;
            }

            add_to_sensor_path(next_node, next_sensor, next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors, 1);
            // next_node->sensor_path.sensors[next_node->sensor_path.num_sensors] = next_sensor;
            // next_node->sensor_path.dists[next_node->sensor_path.num_sensors] = next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors;
            // next_node->sensor_path.does_reverse[next_node->sensor_path.num_sensors] = 1;
            // next_node->sensor_path.scs[0][next_node->sensor_path.num_sensors].switch_num = 255;
            // next_node->sensor_path.scs[1][next_node->sensor_path.num_sensors].switch_num = 255;
            // next_node->sensor_path.num_sensors += 1;

            new_dest = (track+next_sensor)->reverse - track;
            new_dist = next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+reverse_cost;

            if(min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                next_node->dist = new_dist;
                next_node->node_index = new_dest;
                heap_push(&heap, next_node);
            }else{
                reclaimHeapNode(nextFreeHeapNode, next_node);
            }

            new_dist = cur_dist + cur_track_node->edge[0].dist;
            new_dest = cur_track_node->edge[0].dest - track;
            if(min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                cur_node->dist = new_dist;
                cur_node->node_index = new_dest;
                heap_push(&heap, cur_node);
            }else{
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
        }else if(cur_track_node->type == NODE_BRANCH){
            NavPath **hns[2];
            hns[0] = getNextFreeHeapNode(nextFreeHeapNode);
            hns[1] = getNextFreeHeapNode(nextFreeHeapNode);
            //TODO: can make more efficient by using the cur node
            for(int i = 0; i<2; i++){
                new_dist = cur_dist + cur_track_node->edge[i].dist;
                new_dest = cur_track_node->edge[i].dest - track;
                next_node = hns[i];
                if(min_dist[new_dest] > new_dist){
                    min_dist[new_dest] = new_dist;
                    next_node->dist = new_dist;
                    next_node->node_index = new_dest;
                    next_node->sensor_path.num_sensors = cur_node->sensor_path.num_sensors;
                    memcpy(&(next_node->sensor_path.initial_scs), &(cur_node->sensor_path.initial_scs), sizeof(SwitchChange)*2);
                    memcpy(&(next_node->sensor_path.sensors), &(cur_node->sensor_path.sensors), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
                    memcpy(&(next_node->sensor_path.dists), &(cur_node->sensor_path.dists), sizeof(uint16_t)*cur_node->sensor_path.num_sensors);
                    memcpy(&(next_node->sensor_path.does_reverse), &(cur_node->sensor_path.does_reverse), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
                    memcpy(&(next_node->sensor_path.scs[0]), &(cur_node->sensor_path.scs[0]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
                    memcpy(&(next_node->sensor_path.scs[1]), &(cur_node->sensor_path.scs[1]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
                    // for(int u = 0; u<cur_node->sensor_path.num_sensors; u++){
                    //     next_node->sensor_path.sensors[u] = cur_node->sensor_path.sensors[u];
                    //     next_node->sensor_path.dists[i] = cur_node->sensor_path.dists[i];
                    // }

                    // for(int i = 0; i<cur_node->num_switches; i++){
                    //     next_node->switches[i].switch_num=cur_node->switches[i].switch_num;
                    //     next_node->switches[i].dir=cur_node->switches[i].dir;
                    // }
                    SwitchChange *sc_to_change;
                    if(cur_node->sensor_path.num_sensors==1){
                        sc_to_change = next_node->sensor_path.initial_scs;
                    }else{
                        sc_to_change = next_node->sensor_path.scs[0] + (next_node->sensor_path.num_sensors-2);
                    }
                    sc_to_change->switch_num = cur_track_node->num;
                    sc_to_change->dir = dirs[i];

                    sc_to_change = next_node->sensor_path.scs[1] + (next_node->sensor_path.num_sensors-1);;
                    if(cur_track_node->num == 156 && i==1){
                        sc_to_change->switch_num = 155;
                        sc_to_change->dir = 'S';
                        // next_node->switches[next_node->num_switches].switch_num = 155;
                        // next_node->switches[next_node->num_switches].dir = 'S';
                        // next_node->num_switches += 1;
                    }else if(cur_track_node->num == 154 && i==1){
                        sc_to_change->switch_num = 153;
                        sc_to_change->dir = 'S';
                        // next_node->switches[next_node->num_switches].switch_num = 153;
                        // next_node->switches[next_node->num_switches].dir = 'S';
                        // next_node->num_switches += 1;
                    }
                    heap_push(&heap, next_node);
                }else{
                    reclaimHeapNode(nextFreeHeapNode, next_node);
                }
            }
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        }else if(cur_track_node->type == NODE_EXIT){
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        }else{
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpathfinding unexpected node type: %d\0338", cur_node->node_index);
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        }
    }
    return NULL;
}

void path_finding(){

    RegisterAs("pathfind\0");

    int cout = WhoIs("cout\0");
    // int mio = WhoIs("mio\0");
    int switch_tid = WhoIs("switch\0");
    int clock = WhoIs("clock\0");
    int train_server_tid = WhoIs("trainserver\0");

    // generate track data
    track_node track[TRACK_MAX];
    // size_t n = TRACK_MAX; 
    // for (char* it = (char*)track; n > 0; --n) *it++ = 0;
    // memset(track, 0, sizeof(track_node));

    // uart_printf(CONSOLE, "\033[30;1H\033[Ktracknode size %u", sizeof(track_node));
    init_tracka(track);

    // uint8_t edge_available[TRACK_MAX][2];

    PathMessage pm;
    int tid;
    int intended_send_len;
    int intended_reply_len;
    int switch_num;

    int res;
    int cur_pos = -2;
    int start_sensor;
    NewSensorInfo nsi;
    
    char switch_states[23];
    switch_states[22] = '\0';
    res = get_switches_setup(switch_tid, switch_states);
    if(res<0){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get switches setup\0338");
        for(;;){}
    }
    // uart_printf(CONSOLE, "\0337\033[34;1H\033[Kswitches: %s\0338", switch_states);

    TrainServerMsg tsm;
    tsm.type = TRAIN_SERVER_NAV_PATH;
    // uint8_t next_sensor;

    NavPath heap_nodes[150];
    NavPath *nextFreeHeapNode = heap_nodes;
    nextFreeHeapNode = hns_init(heap_nodes, 150);
    NavPath *path;

    for(;;){
        // uart_printf(CONSOLE, "\0337\033[55;1H\033[Kpathfinding before receive %d\0338", Time(clock));
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        // if(intended_send_len!=sizeof(pm)){
        //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpath finding received unknown object\0338");
        //     continue;
        // }
        if(pm.type == PATH_SWITCH_CHANGE){
            Reply(tid, NULL, 0);
            for(int i = 0; i<22; i++) switch_states[i] = pm.switches[i];
        }else if(pm.type==PATH_PF){

            Reply(tid, NULL, 0);
            path = dijkstra(pm.arg1, pm.dest, track, TRACK_MAX, switch_states, &nextFreeHeapNode);
            if(path==NULL){
                Puts(cout, 0, "\0337\033[18;1H\033[KDidn't find a route\0338");
                continue;
            }

            // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K initial sc: %u %u\0338", 39, path->sensor_path.initial_scs[0].switch_num, path->sensor_path.initial_scs[0].dir);
            // for(int i = 0; i<path->sensor_path.num_sensors; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u, dist: %u, reverse: %u, switch: %u %u\0338", 40+i, path->sensor_path.sensors[i], path->sensor_path.dists[i], path->sensor_path.does_reverse[i], path->sensor_path.scs[0][i].switch_num, path->sensor_path.scs[0][i].dir);
            // }
            // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K\0338", 40+path->sensor_path.num_sensors);

            memcpy(tsm.data, &(path->sensor_path), sizeof(SensorPath));
            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsg), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind sent sensor path and received unexpected rplen\0338");
            }

            reclaimHeapNode(nextFreeHeapNode, path);
        }else if(pm.type==PATH_NAV){
            Reply(tid, NULL, 0);
                
            cur_pos = pm.arg1;
            if(cur_pos<0 || cur_pos>80){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Knav invalid cur pos\0338");
                continue;
            }
            start_sensor = get_next_sensor(cur_pos, switch_states, track, NULL, NULL, NULL);

            if(start_sensor<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get start node\0338");
                continue;
            }
            path = dijkstra(start_sensor, pm.dest, track, TRACK_MAX, switch_states, &nextFreeHeapNode);
            if(path==NULL){
                uart_printf(CONSOLE, "\0337\033[18;1H\033[KDidn't find a route\0338");
                continue;
            }

            // for(int i = 0; i<path->sensor_path.num_sensors; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u %u\0338", 40+i, path->sensor_path.sensors[i], path->sensor_path.dists[i]);
            // }
            // uart_printf(CONSOLE, "\0337\033[19;1H\033[Kswitch changes, %d\0338", path->num_switches);
            // for(int i = 0; i<path->num_switches; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[Kswitch, %d %u\0338", 20+i, path->switches[i].switch_num, path->switches[i].dir);
            // }

            // change_switches_cmd(switch_tid, path->switches, path->num_switches);
            //TODO: maybe make this send size also dynamic depending on num of sensors
            memcpy(tsm.data, &(path->sensor_path), sizeof(SensorPath));

            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsg), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind sent sensor path and received unexpected rplen\0338");
            }
            reclaimHeapNode(nextFreeHeapNode, path);
            cur_pos = -2;
        
        }else if(pm.type==PATH_TRACK_CHANGE){
            Reply(tid, NULL, 0);
            if(pm.arg1 == 'a'){
                init_tracka(track);
            }else if(pm.arg1 == 'b'){
                init_trackb(track);
            }else{
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpathfind track change invalid track\0338");
            }
        }else if(pm.type==PATH_NEXT_SENSOR){
            cur_pos = pm.arg1;
            new_printf(cout, 0, "\033[63;1H\033[Kcur_pos: %u", cur_pos);

            nsi.next_sensor_switch_err = -3;
            nsi.next_sensor = get_next_sensor(cur_pos, switch_states, track, &(nsi.next_sensor_switch_err), NULL, NULL);
            nsi.next_next_sensor = get_next_sensor(nsi.next_sensor, switch_states, track, NULL, &(nsi.switch_after_next_sensor), NULL);
            nsi.reverse_sensor = (track+cur_pos)->reverse - track;
            // uart_printf(CONSOLE, "\0337\033[35;1H\033[Knext sensor %u %d\0338", cur_pos, res);
            // uart_printf(CONSOLE, "\0337\033[20;1H\033[KNext sensor: %u %u\0338", skipped_sensors.sensors[0], skipped_sensors.sensors[1]);
            new_printf(cout, 0, "\033[64;1H\033[Kcur_pos: %u, reverse sensor: %u", cur_pos, nsi.reverse_sensor);
            Reply(tid, &nsi, sizeof(NewSensorInfo));
        }else{
            Reply(tid, NULL, 0);
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown pathfind command\0338");
            continue;
        }
    }
}
