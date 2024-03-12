#include "trainconstants.h"


int train_terminal_speed(uint32_t train_num, uint32_t train_speed) {
    switch (train_num) {
        case 2:
            return train2_terminal_speed(train_speed);
        case 47:
            return train47_terminal_speed(train_speed);
        default:
            // unsupported train
            return -1;          
    }
}

int train2_terminal_speed(uint32_t train_speed) {
    switch (train_speed) {
        case 0:
            return 0;
        case 4:
            return 85;
        case 8:
            return 191;
        case 12:
            return 299;
        case 14:
            return 315;
        default:
            // unsupported speed
            return -1;          
            break;
    }
}

int train47_terminal_speed(uint32_t train_speed) {
    switch (train_speed) {
        case 0:
            return 0;
        case 4:
            return 89;
        case 8:
            return 192;
        case 12:
            return 286;
        case 14:
            return 291;
        default:
            // unsupported speed
            return -1;          
            break;
    }
}
