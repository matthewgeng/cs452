#include "syscall.h"
#include "rpi.h"
#include "traincontrol.h"
#include "nameserver.h"
#include "track_data.h"
#include "heap.h"

int heap_node_cmp(HeapNode *n1, HeapNode *n2){
    return n1->dist > n2->dist;
}

int get_switches_setup(PathMessage *pm, uint8_t *switches_setup, track_node *track, int track_len, HeapNode *heap_nodes){

    //TODO: start pathfinding maybe 2/3 sensors after so the train doesn't go off course
    /* 
    design decisions:
    using O(ElogV) dijkstra
    storing switches instead of nodes to save space

    */

    Heap heap;
    HeapNode *heap_node_ptrs[200];
    heap_init(&heap, heap_node_ptrs, 150, heap_node_cmp);

    for(int i = 0; i<track_len; i++){
        heap_nodes[i].dist = 100000;
        heap_nodes[i].num_switches = 0;
    }

    heap_nodes[pm->src].dist = 0;
    heap_push(&heap, heap_nodes + pm->src);
    
    HeapNode *cur_node;
    track_node *cur_track_node;
    while(heap.length!=0){
        cur_node = heap_pop(&heap);
        cur_track_node = track[cur_node->]
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

    int cout = WhoIs("cout");
    int mio = WhoIs("mio");

    // generate track data
    track_node track[144];
    int track_len = init_tracka(track);

    PathMessage pm;
    int tid;
    int intended_send_len;
    uint8_t switches_setup[21];
    int num_switch_changes;
    int switch_num;

    HeapNode heap_nodes[track_len];
    for(int i = 0; i<track_len; i++){
        heap_nodes[i].node_index = i;
    }

    for(;;){
        intended_send_len = Receive(&tid, &pm, sizeof(pm));
        if(intended_send_len!=sizeof(pm)){
            uart_printf(CONSOLE, "path finding received unknown object");
        }
        Reply(tid, NULL, 0);

        num_switch_changes = get_switches_setup(&pm, switches_setup, track, track_len, heap_nodes);

        change_switches(cout, mio, num_switch_changes, switches_setup);
    }
}