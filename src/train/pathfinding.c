#include "syscall.h"
#include "rpi.h"
#include "traincontrol.h"
#include "nameserver.h"
#include "track_data.h"
#include "heap.h"
#include "pathfinding.h"
#include "util.h"

HeapNode *hns_init(HeapNode hns[], size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            hns[i].next = hns+i+1;
        }else{
            hns[i].next = NULL;
        }
        hns[i].num_switches = 0;
        hns[i].num_sensors = 0;
    }
    return hns;
}
HeapNode *getNextFreeHeapNode(HeapNode **nextFreeHeapNode){
    if(*nextFreeHeapNode==NULL){
        return NULL;
    }
    HeapNode *hn = *nextFreeHeapNode;
    *nextFreeHeapNode = (*nextFreeHeapNode)->next;
    return hn;
}
void reclaimHeapNode(HeapNode **nextFreeHeapNode, HeapNode *hn){
    hn->num_switches = 0;
    hn->num_sensors = 0;
    hn->next = *nextFreeHeapNode;
    *nextFreeHeapNode = hn;
}

int heap_node_cmp(HeapNode *n1, HeapNode *n2) {
    if (n1->dist < n2->dist) {
        return -1;
    } else if (n1->dist > n2->dist) {
        return 1;
    }
    return 0;
}

HeapNode *dijkstra(uint8_t src, uint8_t dest, track_node *track, int track_len, HeapNode **nextFreeHeapNode){

    //TODO: start pathfinding maybe 2/3 sensors after so the train doesn't go off course
    /* 
    design decisions:
    using O(ElogV) dijkstra
    storing switches instead of nodes to save space
    storing switches in order or switch to make sure they all switch in time
    */

    Heap heap;
    HeapNode *heap_node_ptrs[200];
    heap_init(&heap, heap_node_ptrs, 150, heap_node_cmp);

    uint32_t min_dist[track_len];
    for(int i = 0; i<track_len; i++){
        min_dist[i] = 2000000000;
    }
    min_dist[src] = 0;

    int new_dist;
    int new_dest;
    int cur_dist;
    HeapNode *cur_node, *next_node;
    track_node *cur_track_node;

    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = src;
    cur_node->dist = 0;
    cur_node->num_switches = 0;
    cur_node->sensors[0] = src;
    cur_node->num_sensors = 1;
    heap_push(&heap, cur_node);

    uart_printf(CONSOLE, "\0337\033[17;1H\033[Kswitches setup %u %u\0338", src, dest);
    // for(;;){}

    HeapNode *res; 
    char dirs[2] = {'S', 'C'};

    int tmp_count = 0;

    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[Knext node, %d %d\0338", 20+tmp_count, cur_node->node_index, cur_node->dist);
        // tmp_count += 1;
        cur_dist = cur_node->dist;
        if(cur_node->node_index == dest){
            // for(;;){}
            res = cur_node;
            while(heap.length!=0){
                cur_node = heap_pop(&heap);
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
            uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
            return res;
        }
        cur_track_node = track + cur_node->node_index;
        if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
            new_dist = cur_dist + cur_track_node->edge[0].dist;
            new_dest = cur_track_node->edge[0].dest - track;
            if(min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                cur_node->dist = new_dist;
                cur_node->node_index = new_dest;
                cur_node->sensors[cur_node->num_sensors] = new_dest;
                cur_node->num_sensors += 1;

                heap_push(&heap, cur_node);
            }else{
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
        }else if(cur_track_node->type == NODE_BRANCH){
            HeapNode **hns[2];
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
                    next_node->num_sensors = cur_node->num_sensors + 1;
                    for(int i = 0; i<cur_node->num_sensors; i++) next_node->sensors[i]=cur_node->sensors[i];
                    next_node->sensors[cur_node->num_sensors] = new_dest;

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

void pathfind_change_switches(int switch_tid, int num_switch_changes, SwitchChange switches_setup[]){
    uart_printf(CONSOLE, "\0337\033[19;1H\033[Kswitch changes, %d\0338", num_switch_changes);
    for(int i = 0; i<num_switch_changes; i++){
        uart_printf(CONSOLE, "\0337\033[%u;1H\033[Kswitch, %d %u\0338", 20+i, switches_setup[i].switch_num, switches_setup[i].dir);
    }
    change_switches_cmd(switch_tid, switches_setup, num_switch_changes);
}

int get_start_node(uint8_t src, uint16_t buffer_dist, char switch_states[], track_node track[]){
    int total_dist = 0;
    int cur_node = src;
    int branch_index;
    uint8_t dir = 0;
    track_node *cur_track_node;
    while(total_dist<buffer_dist){
        cur_track_node = track + cur_node;
        if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
            total_dist += cur_track_node->edge[0].dist;
            cur_node = cur_track_node->edge[0].dest - track;
        }else if(cur_track_node->type == NODE_BRANCH){
            branch_index = cur_track_node->num-1;
            if(branch_index>=153) branch_index -= 134;
            if(switch_states[branch_index]=='S') dir = 0;
            else if(switch_states[branch_index]=='C')dir = 1;
            else return -2;

            total_dist += cur_track_node->edge[dir].dist;
            cur_node = cur_track_node->edge[dir].dest - track;
        }else if(cur_track_node->type == NODE_EXIT){
            return -1;
        }else{
            return -2;
        }
    }
    return cur_node;
}

void pathfind(int src, int dest, int switch_tid, track_node track[], HeapNode **nextFreeHeapNode){

    HeapNode *setup = dijkstra(src, dest, track, TRACK_MAX, nextFreeHeapNode);
    if(setup==NULL){
        uart_printf(CONSOLE, "\0337\033[18;1H\033[KDidn't find a route\0338");
    }

    for(int i = 0; i<setup->num_sensors; i++){
        uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u\0338", 40+i, setup->sensors[i]);
    }
    pathfind_change_switches(switch_tid, setup->num_switches, setup->switches);
    reclaimHeapNode(nextFreeHeapNode, setup);
}

void path_finding(){

    RegisterAs("pathfind\0");

    // int cout = WhoIs("cout\0");
    // int mio = WhoIs("mio\0");
    int switch_tid = WhoIs("switch\0");

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
    int num_switch_changes;
    int switch_num;

    char switch_states[22];
    int res;
    uint8_t cur_sensor;
    uint16_t buffer_dist;
    int start_node;

    HeapNode heap_nodes[150];
    HeapNode *nextFreeHeapNode = heap_nodes;
    nextFreeHeapNode = hns_init(heap_nodes, 150);

    for(;;){
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        Reply(tid, NULL, 0);
        if(intended_send_len!=sizeof(pm)){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpath finding received unknown object\0338");
            continue;
        }

        if(pm.type=='P'){
            pathfind(pm.arg1, pm.dest, switch_tid, track, &nextFreeHeapNode);
        }else if(pm.type=='T'){
            // TODO: need to get the current location somehow later
            cur_sensor = 10; //CHANGE!
            buffer_dist = 500; //CHANGE!
            res = get_switches_setup(switch_tid, switch_states);
            if(res<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get switches setup\0338");
                continue;
            }
            start_node = get_start_node(cur_sensor, buffer_dist, switch_states, track);
            if(start_node<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get start node\0338");
                continue;
            }
            pathfind(start_node, pm.dest, switch_tid, track, &nextFreeHeapNode);
        }else{
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown pathfind command\0338");
            continue;
        }
    }
}