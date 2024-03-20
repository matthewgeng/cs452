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
        hns[i].num_switches = 0;
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
    hn->num_switches = 0;
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

NavPath *dijkstra(uint8_t src, uint8_t dest, track_node *track, int track_len, NavPath **nextFreeHeapNode){

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

    int new_dist;
    int new_dest;
    int cur_dist;
    NavPath *cur_node, *next_node;
    track_node *cur_track_node;

    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = src;
    cur_node->dist = 0;
    cur_node->num_switches = 0;
    cur_node->sensor_path.num_sensors = 0;
    heap_push(&heap, cur_node);

    // uart_printf(CONSOLE, "\0337\033[17;1H\033[Kswitches setup %u %u\0338", src, dest);
    // for(;;){}

    NavPath *res; 
    char dirs[2] = {'S', 'C'};

    int tmp_count = 0;

    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[Knext node, %d %d\0338", 20+tmp_count, cur_node->node_index, cur_node->dist);
        // tmp_count += 1;
        cur_dist = cur_node->dist;

        cur_track_node = track + cur_node->node_index;
        if(cur_node->node_index != src && cur_track_node->type == NODE_SENSOR){
            cur_node->sensor_path.sensors[cur_node->sensor_path.num_sensors] = cur_node->node_index;
            cur_node->sensor_path.dists[cur_node->sensor_path.num_sensors] = cur_node->dist;
            cur_node->sensor_path.num_sensors += 1;
        }

        if(cur_node->node_index == dest){
            // for(;;){}
            res = cur_node;
            while(heap.length!=0){
                cur_node = heap_pop(&heap);
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
            // uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
            return res;
        }
        if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
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
                    for(int i = 0; i<cur_node->sensor_path.num_sensors; i++){
                        next_node->sensor_path.sensors[i] = cur_node->sensor_path.sensors[i];
                        next_node->sensor_path.dists[i] = cur_node->sensor_path.dists[i];
                    }

                    for(int i = 0; i<cur_node->num_switches; i++){
                        next_node->switches[i].switch_num=cur_node->switches[i].switch_num;
                        next_node->switches[i].dir=cur_node->switches[i].dir;
                    } 
                    next_node->switches[cur_node->num_switches].switch_num = cur_track_node->num;
                    next_node->switches[cur_node->num_switches].dir = dirs[i];
                    next_node->num_switches = cur_node->num_switches + 1;
                    if(cur_track_node->num == 156 && i==1){
                        next_node->switches[next_node->num_switches].switch_num = 155;
                        next_node->switches[next_node->num_switches].dir = 'S';
                        next_node->num_switches += 1;
                    }else if(cur_track_node->num == 154 && i==1){
                        next_node->switches[next_node->num_switches].switch_num = 153;
                        next_node->switches[next_node->num_switches].dir = 'S';
                        next_node->num_switches += 1;
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

// int get_start_sensor(uint8_t src, uint8_t num_skip, char switch_states[], track_node track[], uint16_t *dist, SensorPath *sensor_path){
//     track_node *cur_track_node;
//     int start_sensor = src;
//     *dist = 0;
//     uint8_t num_sensors = 0;

//     int branch_index;
//     uint8_t dir = 0;
//     while(1){
//         cur_track_node = track + start_sensor;
//         if(cur_track_node->type==NODE_SENSOR && start_sensor!=src){
//             sensor_path->sensors[num_sensors] = start_sensor;
//             sensor_path->dists[num_sensors] = *dist;
//             num_sensors += 1;
//             if(num_sensors==num_skip){
//                 // uart_printf(CONSOLE, "\0337\033[50;1H\033[Kget start node %d\0338", start_sensor);
//                 return start_sensor;
//             }
//             *dist = *dist + cur_track_node->edge[0].dist;
//             start_sensor = cur_track_node->edge[0].dest - track;
//         }else if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
//             *dist = *dist + cur_track_node->edge[0].dist;
//             start_sensor = cur_track_node->edge[0].dest - track;
//         }else if(cur_track_node->type == NODE_BRANCH){
//             branch_index = cur_track_node->num-1;
//             if(branch_index>=152) branch_index -= 134;
//             if(switch_states[branch_index]=='S') dir = 0;
//             else if(switch_states[branch_index]=='C')dir = 1;
//             else{
//                 // uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown switch v %u %u %s %u\0338", cur_track_node->num, switch_states[branch_index], switch_states, switch_states[19]);
//                 return -3;
//             } 

//             *dist = *dist + cur_track_node->edge[dir].dist;
//             start_sensor = cur_track_node->edge[dir].dest - track;
//         }else if(cur_track_node->type == NODE_EXIT){
//             return -1;
//         }else{
//             return -2;
//         }
//     }
//     return -2;
// }

int get_next_sensor(uint8_t src, char switch_states[], track_node track[], int *switch_err_sensor, int *next_switch){
    track_node *cur_track_node;
    int cur_sensor = src;

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
        }else if(cur_track_node->type == NODE_EXIT){
            return -1;
        }else{
            return -2;
        }
        cur_track_node = track + cur_sensor;
    }
    return cur_sensor;
}

void path_finding(){

    RegisterAs("pathfind\0");

    int cout = WhoIs("cout\0");
    // int mio = WhoIs("mio\0");
    int switch_tid = WhoIs("switch\0");
    // int clock = WhoIs("clock\0");
    int train_server_tid = WhoIs("trainserver\0");

    // generate track data
    track_node track[TRACK_MAX];
    // size_t n = TRACK_MAX; 
    // for (char* it = (char*)track; n > 0; --n) *it++ = 0;
    // memset(track, 0, sizeof(track_node));

    // uart_printf(CONSOLE, "\033[30;1H\033[Ktracknode size %u", sizeof(track_node));
    init_tracka(track);

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
            path = dijkstra(pm.arg1, pm.dest, track, TRACK_MAX, &nextFreeHeapNode);
            if(path==NULL){
                Puts(cout, 0, "\0337\033[18;1H\033[KDidn't find a route\0338");
                continue;
            }
            // for(int i = 0; i<path->sensor_path.num_sensors; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u %u\0338", 40+i, path->sensor_path.sensors[i], path->sensor_path.dists[i]);
            // }
            // uart_printf(CONSOLE, "\0337\033[19;1H\033[Kswitch changes, %d\0338", path->num_switches);
            // for(int i = 0; i<path->num_switches; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[Kswitch, %d %u\0338", 20+i, path->switches[i].switch_num, path->switches[i].dir);
            // }

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
            start_sensor = get_next_sensor(cur_pos, switch_states, track, NULL, NULL);

            if(start_sensor<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get start node\0338");
                continue;
            }
            path = dijkstra(start_sensor, pm.dest, track, TRACK_MAX, &nextFreeHeapNode);
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
            memcpy(tsm.data, path, sizeof(NavPath));

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
            nsi.next_sensor_switch_err = -3;
            nsi.next_sensor = get_next_sensor(cur_pos, switch_states, track, &(nsi.next_sensor_switch_err), NULL);
            nsi.next_next_sensor = get_next_sensor(nsi.next_sensor, switch_states, track, NULL, &(nsi.switch_after_next_sensor));
            // uart_printf(CONSOLE, "\0337\033[35;1H\033[Knext sensor %u %d\0338", cur_pos, res);
            // uart_printf(CONSOLE, "\0337\033[20;1H\033[KNext sensor: %u %u\0338", skipped_sensors.sensors[0], skipped_sensors.sensors[1]);
            Reply(tid, &nsi, sizeof(NewSensorInfo));
        }else{
            Reply(tid, NULL, 0);
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown pathfind command\0338");
            continue;
        }
    }
}
