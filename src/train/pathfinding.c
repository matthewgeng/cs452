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
        hns[i].num_segments = 0;
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
    hn->num_segments = 0;
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

int get_switch_dir(track_node *cur_track_node, char switch_states[]){
    int dir;
    int branch_index = cur_track_node->num-1;
    if(branch_index>=152) branch_index -= 134;
    if(switch_states[branch_index]=='S') dir = 0;
    else if(switch_states[branch_index]=='C')dir = 1;
    else{
        // uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown switch v %u %u %s %u\0338", cur_track_node->num, switch_states[branch_index], switch_states, switch_states[19]);
        return -3;
    } 
    return dir;
}

int get_next_sensor(uint8_t src, char switch_states[], track_node track[], int *switch_err_sensor, int *next_switch, uint16_t *dist){
    if(src<0){
        return -2;
    }
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

void copy_node(NavPath *next_node, NavPath *cur_node){

    next_node->sensor_path.num_sensors = cur_node->sensor_path.num_sensors;
    memcpy(&(next_node->sensor_path.initial_scs), &(cur_node->sensor_path.initial_scs), sizeof(SwitchChange)*2);
    memcpy(&(next_node->sensor_path.sensors), &(cur_node->sensor_path.sensors), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
    memcpy(&(next_node->sensor_path.dists), &(cur_node->sensor_path.dists), sizeof(uint16_t)*cur_node->sensor_path.num_sensors);
    memcpy(&(next_node->sensor_path.does_reverse), &(cur_node->sensor_path.does_reverse), sizeof(uint8_t)*cur_node->sensor_path.num_sensors);
    memcpy(&(next_node->sensor_path.scs[0]), &(cur_node->sensor_path.scs[0]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
    memcpy(&(next_node->sensor_path.scs[1]), &(cur_node->sensor_path.scs[1]), sizeof(SwitchChange)*cur_node->sensor_path.num_sensors);
    next_node->num_segments = cur_node->num_segments;
    memcpy(&(next_node->used_segments), &(cur_node->used_segments), sizeof(uint16_t)*cur_node->num_segments);
}

uint8_t try_add_used_segment(uint8_t train_on_segment[TRACK_MAX][2], int train_id, NavPath *cur_node, track_node *track, uint8_t node, uint8_t edge){
    cur_node->used_segments[cur_node->num_segments] = node*2+edge;
    uint8_t this_reverse = (track+node)->reverse - track;
    uint8_t next_reverse = (track+node)->edge[edge].dest->reverse - track;
    uint8_t reverse_edge = 0;
    if(((track+next_reverse)->edge[1].dest-track)==this_reverse){
        reverse_edge = 1;
    }
    cur_node->used_segments[cur_node->num_segments+1] = next_reverse*2+reverse_edge;
    cur_node->num_segments += 2;
    if(train_on_segment[node][edge]!=0 && train_on_segment[node][edge]!=train_id){
        return 0;
    }else if(train_on_segment[next_reverse][reverse_edge]!=0 && train_on_segment[next_reverse][reverse_edge]!=train_id){
        return 0;
    }
    return 1;
}

NavPath *dijkstra(uint8_t src, uint8_t dest, int train_id, track_node *track, int track_len, char switch_states[], uint8_t train_on_segment[TRACK_MAX][2], NavPath **nextFreeHeapNode){

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

    uint8_t segment_available;

    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = src;
    cur_node->dist = 0;
    cur_node->sensor_path.num_sensors = 0;
    cur_node->sensor_path.initial_scs[0].switch_num = 255;
    cur_node->sensor_path.initial_scs[1].switch_num = 255;
    cur_node->num_segments = 0;
    heap_push(&heap, cur_node);

    cur_track_node = track + src;
    cur_node = getNextFreeHeapNode(nextFreeHeapNode);
    cur_node->node_index = cur_track_node->reverse - track;
    cur_node->dist = reverse_cost;
    cur_node->sensor_path.num_sensors = 0;
    add_to_sensor_path(cur_node, src, 0, 1);
    cur_node->sensor_path.initial_scs[0].switch_num = 255;
    cur_node->sensor_path.initial_scs[1].switch_num = 255;
    cur_node->num_segments = 0;
    segment_available = try_add_used_segment(train_on_segment, train_id, cur_node, track, src, 0);
    if(segment_available==1){
        heap_push(&heap, cur_node);
    }

    // uart_printf(CONSOLE, "\0337\033[35;1H\033[Kreverse: %u\0338", cur_node->node_index);
    // for(;;){}

    NavPath *res; 
    char dirs[2] = {'S', 'C'};

    int tmp_count = 0;

    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        cur_dist = cur_node->dist;

        // uart_printf(CONSOLE, "\0337\033[%u;1H\033[Knext node, %d, num seg: %u, num_seg[0]: %u\0338", 15+tmp_count, cur_node->node_index, cur_node->num_segments, cur_node->used_segments[0]);
        // tmp_count += 1;
        
        cur_track_node = track + cur_node->node_index;
        if(cur_track_node->type == NODE_SENSOR){
            add_to_sensor_path(cur_node, cur_node->node_index, cur_node->dist, 0);
        }

        if(cur_node->node_index == dest){
            // for(;;){}
            res = cur_node;
            // get_speeds_from_reverse(res);
            while(heap.length!=0){
                cur_node = heap_pop(&heap);
                reclaimHeapNode(nextFreeHeapNode, cur_node);
            }
            // uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
            return res;
        // }else if((cur_track_node->reverse-track) == dest){
        //     add_used_segment(cur_node, track, cur_node->node_index, 0);
        //     cur_node->sensor_path.does_reverse[cur_node->sensor_path.num_sensors-1] = 1;
        //     cur_node->node_index = dest;
        //     cur_node->dist += reverse_cost;
        //     heap_push(&heap, cur_node);
        //     continue;
        }

        if(cur_track_node->type == NODE_SENSOR){
            next_node = getNextFreeHeapNode(nextFreeHeapNode);
            copy_node(next_node, cur_node);
            next_node->sensor_path.does_reverse[next_node->sensor_path.num_sensors-1] = 1;
            new_dist = cur_dist + reverse_cost;
            new_dest = cur_track_node->reverse - track;

            segment_available = try_add_used_segment(train_on_segment, train_id, next_node, track, cur_node->node_index, 0);
            if(segment_available==1 && min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                // add_used_segment(next_node, track, cur_node->node_index, 0);
                next_node->dist = new_dist;
                next_node->node_index = new_dest;
                heap_push(&heap, next_node);
            }else{
                reclaimHeapNode(nextFreeHeapNode, next_node);
            }
        }
        
        if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
            new_dist = cur_dist + cur_track_node->edge[0].dist;
            new_dest = cur_track_node->edge[0].dest - track;
            segment_available = try_add_used_segment(train_on_segment, train_id, cur_node, track, cur_node->node_index, 0);
            if(segment_available==1 && min_dist[new_dest] > new_dist){
            // if(train_on_segment[src][0]==0 && min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                // add_used_segment(cur_node, track, cur_node->node_index, 0);
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
                segment_available = try_add_used_segment(train_on_segment, train_id, next_node, track, cur_node->node_index, i);
                if(segment_available==1 && min_dist[new_dest] > new_dist){
                // if(train_on_segment[src][0]==0 && min_dist[new_dest] > new_dist){
                    min_dist[new_dest] = new_dist;
                    copy_node(next_node, cur_node);
                    // add_used_segment(next_node, track, cur_node->node_index, i);
                    next_node->dist = new_dist;
                    next_node->node_index = new_dest;

                    SwitchChange *sc_to_change;
                    if(cur_node->sensor_path.num_sensors==1){
                        sc_to_change = next_node->sensor_path.initial_scs;
                    }else{
                        sc_to_change = next_node->sensor_path.scs[0] + (next_node->sensor_path.num_sensors-2);
                        if(sc_to_change->switch_num!=255){
                            sc_to_change = next_node->sensor_path.scs[1] + (next_node->sensor_path.num_sensors-2);
                        }
                    }
                    sc_to_change->switch_num = cur_track_node->num;
                    sc_to_change->dir = dirs[i];

                    sc_to_change = next_node->sensor_path.scs[1] + (next_node->sensor_path.num_sensors-1);;
                    if(cur_track_node->num == 156 && i==1){
                        sc_to_change->switch_num = 155;
                        sc_to_change->dir = 'S';
                    }else if(cur_track_node->num == 154 && i==1){
                        sc_to_change->switch_num = 153;
                        sc_to_change->dir = 'S';
                    }
                    heap_push(&heap, next_node);
                }else{
                    reclaimHeapNode(nextFreeHeapNode, next_node);
                }
            }
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        }else if(cur_track_node->type == NODE_EXIT){
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        // }else if(cur_track_node->type == NODE_MERGE){
        //     NavPath *next_node;
        //     next_node = getNextFreeHeapNode(nextFreeHeapNode);
        //     copy_node(next_node, cur_node);

        //     // TODO: maybe don't need to go to next sensor to reverse if close
        //     uint8_t prev_sensor = cur_node->sensor_path.sensors[cur_node->sensor_path.num_sensors-1];
        //     uint16_t dist_bt_sensors;
        //     uint8_t next_sensor = get_next_sensor(prev_sensor, switch_states, track, NULL, NULL, &dist_bt_sensors);
        //     add_used_segment(next_node, track, cur_node->node_index, 0);
        //     if(next_sensor==dest){
        //         add_to_sensor_path(next_node, next_sensor, next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors, 0);
                
        //         res = next_node;
        //         get_speeds_from_reverse(res);
        //         while(heap.length!=0){
        //             cur_node = heap_pop(&heap);
        //             reclaimHeapNode(nextFreeHeapNode, cur_node);
        //         }
        //         // uart_printf(CONSOLE, "\0337\033[18;1H\033[Kswitches setup done %u %u %d\0338", src, dest, res->num_switches);
        //         return res;
        //     }

        //     add_to_sensor_path(next_node, next_sensor, next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+dist_bt_sensors, 1);

        //     add_used_segment(next_node, track, next_sensor, 0);
        //     new_dest = (track+next_sensor)->reverse - track;
        //     new_dist = next_node->sensor_path.dists[next_node->sensor_path.num_sensors-1]+reverse_cost;

        //     if(min_dist[new_dest] > new_dist){
        //         min_dist[new_dest] = new_dist;
        //         next_node->dist = new_dist;
        //         next_node->node_index = new_dest;
        //         heap_push(&heap, next_node);
        //     }else{
        //         reclaimHeapNode(nextFreeHeapNode, next_node);
        //     }

        //     new_dist = cur_dist + cur_track_node->edge[0].dist;
        //     new_dest = cur_track_node->edge[0].dest - track;
        //     if(min_dist[new_dest] > new_dist){
        //         min_dist[new_dest] = new_dist;
        //         add_used_segment(cur_node, track, cur_node->node_index, 0);
        //         cur_node->dist = new_dist;
        //         cur_node->node_index = new_dest;
        //         heap_push(&heap, cur_node);
        //     }else{
        //         reclaimHeapNode(nextFreeHeapNode, cur_node);
        //     }
        }else{
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpathfinding unexpected node type: %d\0338", cur_node->node_index);
            reclaimHeapNode(nextFreeHeapNode, cur_node);
        }
    }
    return NULL;
}

uint8_t reverse_node(track_node *track, track_node *cur_track_node){
    return cur_track_node->reverse - track;
}

uint8_t segments_reserved(track_node *track, uint8_t train_on_segment[TRACK_MAX][2], char switch_states[], int train_id, int node){
    // if(node<0){
    //     return 0;
    // }
    int cout = WhoIs("cout\0");
    int cur_node = node;
    int dir;
    track_node *cur_track_node = track + cur_node;
    while(cur_node==node || cur_track_node->type!=NODE_SENSOR){
        if(cur_track_node->type==NODE_BRANCH){
            int dir;
            int branch_index = cur_track_node->num-1;
            if(branch_index>=152) branch_index -= 134;
            if(switch_states[branch_index]=='S') dir = 0;
            else if(switch_states[branch_index]=='C')dir = 1;
            else{
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind segments_reserved switch dir failed, %d, %d, %d\0338", cur_node, branch_index, switch_states[branch_index]);

                // new_printf(cout, 0, "\0337\033[31;1H\033[Kswitches: %s\0338", switch_states);
                dir = 0; // weird err case
            } 
            // dir = get_switch_dir(cur_track_node, switch_states);
            // if(dir==-3){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind segments_reserved switch dir failed, %d\0338", cur_node);
            //     return 1; // err case assume taken
            // }
            if(train_on_segment[cur_node][dir]!=0 && train_on_segment[cur_node][dir]!=train_id){
                return train_on_segment[cur_node][dir];
            }
            cur_node = cur_track_node->edge[dir].dest - track;
        }else if(cur_track_node->type==NODE_EXIT){
            if(train_on_segment[cur_node][0]!=0 && train_on_segment[cur_node][0]!=train_id){
                return train_on_segment[cur_node][0];
            }
            return 0;
        }else{
            if(train_on_segment[cur_node][0]!=0 && train_on_segment[cur_node][0]!=train_id){
                return train_on_segment[cur_node][0];
            }
            cur_node = cur_track_node->edge[0].dest - track;
        }
        cur_track_node = track+cur_node;
    }
    return 0;
}

void unreserve_segment(uint8_t train_on_segment[TRACK_MAX][2], int train_id, int src, int cur_node, int reverse, int forward_dir, int backward_dir){
    if(train_on_segment[cur_node][forward_dir]==train_id){
        train_on_segment[cur_node][forward_dir] = 0;
    }else if(train_on_segment[cur_node][forward_dir]!=0){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind unreserve_segments unexpected reservation %d %d %d\0338", cur_node, forward_dir, train_on_segment[cur_node][forward_dir]);
    }
    if(cur_node!=src){
        if(train_on_segment[reverse][backward_dir]==train_id){
            train_on_segment[reverse][backward_dir] = 0;
        }else if(train_on_segment[reverse][backward_dir]!=0){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind unreserve_segments unexpected reservation %d %d %d\0338", reverse, backward_dir, train_on_segment[reverse][backward_dir]);
        }
    }
}

void unreserve_segments(track_node *track, uint8_t train_on_segment[TRACK_MAX][2], char switch_states[], int train_id, int src){
    int prev_node = -1;
    int cur_node = src;
    int forward_dir, backward_dir;
    int reverse;
    track_node *cur_track_node = track + cur_node;
    while(cur_node==src || cur_track_node->type!=NODE_SENSOR){
        reverse = reverse_node(track, cur_track_node);
        forward_dir = 0;
        if(cur_track_node->type==NODE_BRANCH){
            forward_dir = get_switch_dir(cur_track_node, switch_states);
            if(forward_dir==-3){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind unreserve_segments switch dir failed\0338");
                return; // err case
            }
            unreserve_segment(train_on_segment, train_id, src, cur_node, reverse, forward_dir, 0);
        }else if(cur_track_node->type==NODE_EXIT){
            unreserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, 0);
            break;
        }else if(cur_track_node->type==NODE_SENSOR){
            unreserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, 0);
        }else if(cur_track_node->type==NODE_MERGE){
            backward_dir = 0;
            if((track+reverse)->edge[1].dest-track == prev_node){
                backward_dir = 1;
            }
            unreserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, backward_dir);
        }
        prev_node = cur_node;
        cur_node = cur_track_node->edge[forward_dir].dest - track;
        cur_track_node = track+cur_node;
    }
    reverse = reverse_node(track, cur_track_node);
    if(train_on_segment[reverse][0]==train_id){
        train_on_segment[reverse][0] = 0;
    }else if(train_on_segment[reverse][0]!=0){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind unreserve_segments unexpected reservation %d %d %d\0338", reverse, 0, train_on_segment[reverse][0]);
    }

    return 0;
}


void reserve_segment(uint8_t train_on_segment[TRACK_MAX][2], int train_id, int src, int cur_node, int reverse, int forward_dir, int backward_dir){
    if(train_on_segment[cur_node][forward_dir]==0){
        train_on_segment[cur_node][forward_dir] = train_id;
    }else if(train_on_segment[cur_node][forward_dir]!=train_id){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind reserve_segments unexpected reservation %d\0338",train_on_segment[cur_node][forward_dir]);
    }
    if(cur_node!=src){
        if(train_on_segment[reverse][backward_dir]==0){
            train_on_segment[reverse][backward_dir] = train_id;
        }else if(train_on_segment[reverse][backward_dir]!=train_id){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind reserve_segments unexpected reservation %d\0338",train_on_segment[reverse][backward_dir]);
        }
    }
}

void reserve_segments(track_node *track, uint8_t train_on_segment[TRACK_MAX][2], char switch_states[], int train_id, int src){
    int prev_node = -1;
    int cur_node = src;
    int forward_dir, backward_dir;
    int reverse;
    track_node *cur_track_node = track + cur_node;
    while(cur_node==src || cur_track_node->type!=NODE_SENSOR){
        reverse = reverse_node(track, cur_track_node);
        forward_dir = 0;
        if(cur_track_node->type==NODE_BRANCH){
            forward_dir = get_switch_dir(cur_track_node, switch_states);
            if(forward_dir==-3){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind reserve_segments switch dir failed\0338");
                return; // err case
            }
            reserve_segment(train_on_segment, train_id, src, cur_node, reverse, forward_dir, 0);
        }else if(cur_track_node->type==NODE_EXIT){
            reserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, 0);
            break;
        }else if(cur_track_node->type==NODE_SENSOR){
            reserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, 0);
        }else if(cur_track_node->type==NODE_MERGE){
            backward_dir = 0;
            if((track+reverse)->edge[1].dest-track == prev_node){
                backward_dir = 1;
            }
            reserve_segment(train_on_segment, train_id, src, cur_node, reverse, 0, backward_dir);
        }
        prev_node = cur_node;
        cur_node = cur_track_node->edge[forward_dir].dest - track;
        cur_track_node = track+cur_node;
    }
    reverse = reverse_node(track, cur_track_node);
    if(train_on_segment[reverse][0]==0){
        train_on_segment[reverse][0] = train_id;
    }else if(train_on_segment[reverse][0]!=train_id){
        uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind reserve_segments unexpected reservation %d\0338", train_on_segment[reverse][0]);
    }

    return 0;
}

void print_reservation(int cout, uint8_t train_on_segment[TRACK_MAX][2]){

    int tmp_c[3] = {0,0,0};
    // new_printf(cout, 0, "\0337\033[%d;%dHTrain %d, terminal speed %d    \0338");
    new_printf(cout, 0, "\0337\033[33;0H\033[K tr: 1 \0338");
    new_printf(cout, 0, "\0337\033[34;0H\033[K tr: 2 \0338");
    new_printf(cout, 0, "\0337\033[35;0H\033[K tr: 54 \0338");
    // Puts(cout, 0, "\033[35;0H\033[K");
    for(int i = 0; i<TRACK_MAX; i++){
        for(int u = 0; u<2; u++){
            if(train_on_segment[i][u]==1){
                new_printf(cout, 0, "\0337\033[33;%dH %u;%u \0338", tmp_c[0]*6+10, i, u);
                tmp_c[0]+=1;
            }else if(train_on_segment[i][u]==2){
                new_printf(cout, 0, "\0337\033[34;%dH %u;%u \0338", tmp_c[1]*6+10, i, u);
                tmp_c[1]+=1;
            }else if(train_on_segment[i][u]==54){
                new_printf(cout, 0, "\0337\033[35;%dH %u;%u \0338", tmp_c[2]*6+10, i, u);
                tmp_c[2]+=1;
            }
        }
    }
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

    uint8_t train_on_segment[TRACK_MAX][2];
    for(int i = 0; i<TRACK_MAX; i++){
        train_on_segment[i][0] = 0;
        train_on_segment[i][1] = 0;
    }

    PathMessage pm;
    int tid;
    int intended_send_len;
    int intended_reply_len;
    int switch_num;

    int res;
    int cur_pos = -2;
    int train_id;
    int do_get_next;
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

    uint8_t train_loc[100];
    memset(train_loc, 255, sizeof(uint8_t)*100);

    uint8_t run_res = 1;

    for(;;){
        // uart_printf(CONSOLE, "\0337\033[55;1H\033[Kpathfinding before receive %d\0338", Time(clock));
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        new_printf(cout, 0, "\033[80;0Hrun_res :%u", run_res); 
        // if(intended_send_len!=sizeof(pm)){
        //     uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpath finding received unknown object\0338");
        //     continue;
        // }
        if(pm.type == PATH_SWITCH_CHANGE){
            Reply(tid, NULL, 0);
            for(int i = 0; i<22; i++) switch_states[i] = pm.switches[i];
        }else if(pm.type==PATH_PF){

            Reply(tid, NULL, 0);
            path = dijkstra(pm.arg1, pm.dest, 0, track, TRACK_MAX, switch_states, train_on_segment, &nextFreeHeapNode);
            if(path==NULL){
                Puts(cout, 0, "\0337\033[16;1H\033[KDidn't find a route\0338");
                continue;
            }

            new_printf(cout, 0, "\033[24;0Hnum segments:%u", path->num_segments);
            // new_printf(cout, 0, "\0337\033[25;0H\033[K");
            for(int i = 0; i<path->num_segments; i++){
                new_printf(cout, 0, "\033[%u;1H%u", 25+i, path->used_segments[i]);
            }

            // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K initial sc: %u %u\0338", 39, path->sensor_path.initial_scs[0].switch_num, path->sensor_path.initial_scs[0].dir);
            // for(int i = 0; i<path->sensor_path.num_sensors; i++){
            //     uart_printf(CONSOLE, "\0337\033[%u;1H\033[K sensor: %u, dist: %u, reverse: %u, switch: %u %u\0338", 40+i, path->sensor_path.sensors[i], path->sensor_path.dists[i], path->sensor_path.does_reverse[i], path->sensor_path.scs[0][i].switch_num, path->sensor_path.scs[0][i].dir);
            // }
            // uart_printf(CONSOLE, "\0337\033[%u;1H\033[K\0338", 40+path->sensor_path.num_sensors);

            // memcpy(tsm.data, &(path->sensor_path), sizeof(SensorPath));
            // intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsg), NULL, 0);
            // if(intended_reply_len!=0){
            //     uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind sent sensor path and received unexpected rplen\0338");
            // }

            reclaimHeapNode(nextFreeHeapNode, path);
        }else if(pm.type==PATH_NAV){
            Reply(tid, NULL, 0);
            run_res = 1;
            
            cur_pos = pm.arg1;
            train_id = pm.arg2;
            do_get_next = pm.arg3;
            if(cur_pos<0 || cur_pos>80){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Knav invalid cur pos\0338");
                continue;
            }
            if(do_get_next==1){
                start_sensor = get_next_sensor(cur_pos, switch_states, track, NULL, NULL, NULL);
            }else{
                start_sensor = cur_pos;
            }

            if(start_sensor<0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[Kfailed to get start node\0338");
                continue;
            }
            path = dijkstra(start_sensor, pm.dest, train_id, track, TRACK_MAX, switch_states, train_on_segment, &nextFreeHeapNode);
            if(path==NULL){
                Puts(cout, 0, "\0337\033[15;1H\033[KDidn't find a route\0338");
                tsm.type = TRAIN_SERVER_NAV_PATH;
                tsm.arg1 = train_id;
                tsm.arg2 = 0;

                intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsg), NULL, 0);
                if(intended_reply_len!=0){
                    uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind sent sensor path and received unexpected rplen\0338");
                }
                continue;
            }
            Puts(cout, 0, "\0337\033[15;1H\033[KTrain navigation ran\0338");

            Puts(cout, 0, "\033[36;0H\033[K");
            uint8_t node, edge;
            for(int i = 0; i<path->num_segments; i++){
                node = path->used_segments[i]/2;
                edge = path->used_segments[i]%2;
                train_on_segment[node][edge] = train_id;
            }
            
            tsm.type = TRAIN_SERVER_NAV_PATH;
            tsm.arg1 = train_id;
            tsm.arg2 = 1;
            memcpy(tsm.data, &(path->sensor_path), sizeof(SensorPath));

            intended_reply_len = Send(train_server_tid, &tsm, sizeof(TrainServerMsg), NULL, 0);
            if(intended_reply_len!=0){
                uart_printf(CONSOLE, "\0337\033[30;1H\033[KPathfind sent sensor path and received unexpected rplen\0338");
            }

            new_printf(cout, 0, "\0337\033[32;0H\033[Knum_segments: %d\0338", path->num_segments);
            // int r;
            // if(train_id==1){
            //     r = 33;
            // }else if(train_id==2){
            //     r = 34;
            // }else if(train_id==54){
            //     r = 35;
            // }
            print_reservation(cout, train_on_segment);

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
            train_id = pm.arg2;

            nsi.next_sensor_switch_err = -3;
            nsi.next_sensor = get_next_sensor(cur_pos, switch_states, track, &(nsi.next_sensor_switch_err), NULL, NULL);
            nsi.next_next_sensor = get_next_sensor(nsi.next_sensor, switch_states, track, NULL, &(nsi.switch_after_next_sensor), NULL);
            nsi.reverse_sensor = (track+cur_pos)->reverse - track;

            // if next next segment is taken, send back reverse command

            // new_printf(cout, 0, "\0337\033[31;1H\033[KPathfind unreserved %d %d\0338", train_loc[train_id], train_id);
            if(nsi.next_sensor == -1 || nsi.next_next_sensor == -1){
                nsi.exit_incoming = 1;
            }else{
                nsi.exit_incoming = 0;
            }
            if(run_res==1){
                nsi.cur_segment_is_reserved = segments_reserved(track, train_on_segment, switch_states, train_id, cur_pos);
                nsi.next_segment_is_reserved = segments_reserved(track, train_on_segment, switch_states, train_id, nsi.next_sensor);
                nsi.next_next_segment_is_reserved = segments_reserved(track, train_on_segment, switch_states, train_id, nsi.next_next_sensor);
                if(nsi.cur_segment_is_reserved==0){
                    reserve_segments(track, train_on_segment, switch_states, train_id, cur_pos);
                }

                if(train_loc[train_id]!=255){
                    unreserve_segments(track, train_on_segment, switch_states, train_id, train_loc[train_id]);
                }
            }

            Reply(tid, &nsi, sizeof(NewSensorInfo));
            train_loc[train_id] = cur_pos;
            
            print_reservation(cout, train_on_segment);
        // }else if(pm.type==PATH_NAV_END){
        //     Reply(tid, NULL, 0);
        //     train_id = pm.arg1;
        //     for(int i = 0; i<TRACK_MAX; i++){
        //         for(int u = 0; u<2; u++){
        //             if(train_on_segment[i][u]==train_id){
        //                 train_on_segment[i][u] = 0;
        //             }
        //         }
        //     }
        }else if(pm.type==PATH_SEGMENT_RESET){
            Reply(tid, NULL, 0);
            train_id = pm.arg1;
            for(int i = 0; i<TRACK_MAX; i++){
                for(int u = 0; u<2; u++){
                    train_on_segment[i][u] = 0;
                }
            }
        }else{
            Reply(tid, NULL, 0);
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kunknown pathfind command\0338");
            continue;
        }
    }
}
