#include "syscall.h"
#include "rpi.h"
#include "traincontrol.h"
#include "nameserver.h"
#include "track_data.h"
#include "heap.h"

HeapNode *hns_init(HeapNode* hns, size_t size){
    for(size_t i = 0; i<size; i++){
        if(i<size-1){
            hns[i].next = hns+i+1;
        }else{
            hns[i].next = NULL;
        }
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
    hn->next = *nextFreeHeapNode;
    *nextFreeHeapNode = hn;
}


int heap_node_cmp(HeapNode *n1, HeapNode *n2){
    return n1->dist > n2->dist;
}

HeapNode *get_switches_setup(PathMessage *pm, uint8_t *switches_setup, track_node *track, int track_len, HeapNode *nextFreeHeapNode){

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

    uint16_t min_dist[track_len];
    for(int i = 0; i<track_len; i++){
        min_dist[i] = 10000;
    }
    min_dist[pm->src] = 0;

    int new_dist;
    int new_dest;
    HeapNode *cur_node;
    track_node *cur_track_node;

    cur_node = getNextFreeHeapNode(&nextFreeHeapNode);
    cur_node->node_index = pm->src;
    cur_node->dist = 0;
    cur_node->num_switches = 0;
    heap_push(&heap, cur_node);

    HeapNode *res; 
    
    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        if(cur_node->node_index == pm->dest){
            res = cur_node;
            while(heap.length!=0){
                cur_node = heap_pop(&heap);
                reclaimHeapNode(&nextFreeHeapNode, cur_node);
            }
            return res;
        }
        cur_track_node = track + cur_node->node_index;
        if(cur_track_node->type == NODE_SENSOR || cur_track_node->type == NODE_MERGE){
            new_dist = cur_node->dist + cur_track_node->edge[0].dist;
            new_dest = cur_track_node->edge[0].dest;
            if(min_dist[new_dest] > new_dist){
                min_dist[new_dest] = new_dist;
                cur_node->dist += cur_track_node->edge[0].dist;
                cur_node->node_index = cur_track_node->edge[0].dest;
                heap_push(&heap, cur_node);
            }else{
                reclaimHeapNode(&nextFreeHeapNode, cur_node);
            }
        }else if(cur_track_node->type == NODE_BRANCH){
            reclaimHeapNode(&nextFreeHeapNode, cur_node);
            HeapNode **hns[2];
            hns[0] = getNextFreeHeapNode(&nextFreeHeapNode);
            hns[1] = getNextFreeHeapNode(&nextFreeHeapNode);
            for(int i = 0; i<2; i++){
                new_dist = cur_node->dist + cur_track_node->edge[i].dist;
                new_dest = cur_track_node->edge[i].dest;
                cur_node = hns[i];
                if(min_dist[new_dest] > new_dist){
                    min_dist[new_dest] = new_dist;
                    cur_node->dist += cur_track_node->edge[i].dist;
                    cur_node->node_index = cur_track_node->edge[i].dest;
                    cur_node->switches[cur_node->num_switches] = cur_track_node->num*(i+1);
                    cur_node->num_switches += 1;
                    heap_push(&heap, cur_node);
                }else{
                    reclaimHeapNode(&nextFreeHeapNode, cur_node);
                }
            }
        }else if(cur_track_node->type == NODE_EXIT){
            reclaimHeapNode(&nextFreeHeapNode, cur_node);
        }else{
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpathfinding unexpected node type: %d\0338", cur_node->node_index);
            reclaimHeapNode(&nextFreeHeapNode, cur_node);
        }
    }
}

void change_switches(int cout, int mio, int num_switch_changes, uint8_t *switches_setup){
    int switch_num;
    for(int i = 0; i<num_switch_changes; i++){
        switch_num = switches_setup[i];
        if(switch_num<=20){
            if(switch_num>16){
                switch_num += 136;
            }
            sw(cout, mio, switch_num, 'S');
        }else{
            switch_num /= 2;
            if(switch_num>32){
                switch_num += 136;
            }
            sw(cout, mio, switch_num, 'C');
        }
    }
}

void path_finding(){

    RegisterAs("pathfind\0");

    int cout = WhoIs("cout\0");
    int mio = WhoIs("mio\0");

    // generate track data
    track_node track[144];
    int track_len = init_tracka(track);

    PathMessage pm;
    int tid;
    int intended_send_len;
    uint8_t switches_setup[21];
    int num_switch_changes;
    int switch_num;

    HeapNode heap_nodes[200];
    HeapNode *nextFreeHeapNode = hns_init(heap_nodes, 200);

    for(;;){
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        Reply(tid, NULL, 0);
        if(intended_send_len!=sizeof(pm)){
            uart_printf(CONSOLE, "\0337\033[30;1H\033[Kpath finding received unknown object\0338");
        }

        HeapNode *setup = get_switches_setup(&pm, switches_setup, track, track_len, nextFreeHeapNode);

        change_switches(cout, mio, setup->num_switches, setup->switches);

        reclaimHeapNode(&nextFreeHeapNode, setup);
    }
}