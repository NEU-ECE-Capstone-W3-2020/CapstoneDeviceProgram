#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "capstone.h"
#include "nano.h"
#include "emic.h"

// These are global because we need lasting pointers to them
static TMR_ReadListenerBlock rlb; // structure describing callback
static TMR_ReadPlan read_plan; // read plan
static uint8_t ant_list[] = {1}; // antenna list

static size_t tag_to_message(uint8_t tag, char* buffer, const size_t length) {
    const char *str = NULL;
    size_t len = 0;

    switch(tag){
        case END_OF_TAG:
            break;
        case WOMENS_BATHROOM:
            str = "Women's Bathroom ";
            break;
        case MENS_BATHROOM:
            str = "Men's Bathroom ";
            break;
        case UNISEX_BATHROOM:
            str = "Unisex Bathroom ";
            break;
        case CLASSROOM:
            str = "Classroom ";
            break;
        case OFFICE:
            str = "Office ";
            break;
        case FLOOR:
            str = "Floor ";
            break;
        case ELEVATOR:
            str = "Elevator ";
            break;
        case STAIRCASE:
            str = "STAIRCASE ";
            break;
        case EXIT:
            str = "Exit ";
            break;
        case 0x01:
            str = "One ";
            break;
        case 0x02:
            str = "Two ";
            break;
        case 0x03:
            str = "Three ";
            break;
        case 0x04:
            str = "Four ";
            break;
        case 0x05:
            str = "Five ";
            break;
        default:
            break;
    }

    if(str){
        len = strlen(str);
        if(len <= length) {
            memcpy(buffer, str, len);
        } else {
            return -1;
        }
    }
    return len;
}

// Read callback
static void read_callback(TMR_Reader *_reader, const TMR_TagReadData *t, void *cookie) {
    UNUSED(_reader);
    DPRINTF("read_callback");

    char msg[NANO_BUFFER_SIZE];
    size_t msg_idx = 0;
    if(t->data.len < EPC_OFFSET) {
        printf("Not enough data read!\n");
        return;
    }

    if(t->data.list[EPC_OFFSET] != VERSION_TAG) {
        printf("Wrong Version\n");
        return;
    }

    int i = EPC_OFFSET + 1;
    while(i < t->data.len){
        int len = tag_to_message(t->data.list[i++], msg + msg_idx, NANO_BUFFER_SIZE - msg_idx);

        if(len == 0) break;
        else if(len < 0) return;

        msg_idx += len;
    }
    if(msg_idx + 1 >= NANO_BUFFER_SIZE) {
        printf("RFID Message too long!\n");
        return;
    }
    msg[msg_idx-1] = '.';
    msg[msg_idx++] = '\0';
    emic_text_to_speech(*((int *) cookie), msg, msg_idx);
}

int nano_init(Nano *nano, const int emic) {
    int *emic_fd = NULL;
    int16_t power_max, power_min, power;
    power = ANTENNA_POWER;
    uint32_t baudrate = BAUDRATE;
    TMR_Region region = TMR_REGION_NA;
    TMR_String model;

    TMR_Status status;

    // Create nano
    // TODO: path
    status = TMR_create(nano, "tmr:///dev/ttyUSB0");
    if(status != TMR_SUCCESS) {
        printf("Error initializing nano\n");
        return -1;
    }

    // Connect to device
    status = TMR_connect(nano);
    if(status != TMR_SUCCESS) {
        printf("Error connecting to nano: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    // Check model
    status = TMR_paramGet(nano, TMR_PARAM_VERSION_MODEL, &model);
    if(status != TMR_SUCCESS) {
        printf("Failed to get model: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }
    if(strcmp(model.value, "M6e Nano") != 0) {
        printf("This is reporting its model as %s, not as a M6e Nano\n", model.value);
        nano_cleanup(nano);
        return -1;
    }

    // Set Baudrate
    status = TMR_paramSet(nano, TMR_PARAM_BAUDRATE, &baudrate);
    if(status != TMR_SUCCESS) {
        printf("Failed to set baudrate: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    // Set up read plan
    status = TMR_RP_init_simple(&read_plan, 1, ant_list, TMR_TAG_PROTOCOL_GEN2, 1);
    if(status != TMR_SUCCESS) {
        printf("Failed creating readplan\n");
        nano_cleanup(nano);
        return -1;
    }

    // Set nano to use read plan
    status = TMR_paramSet(nano, TMR_PARAM_READ_PLAN, &read_plan);
    if(status != TMR_SUCCESS) {
        printf("Failed to set readplan: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    // Set region
    status = TMR_paramSet(nano, TMR_PARAM_REGION_ID, &region);
    if(status != TMR_SUCCESS) {
        printf("Error setting region: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    // Get antenna power
    status = TMR_paramGet(nano, TMR_PARAM_RADIO_POWERMAX, &power_max);
    if(status != TMR_SUCCESS) {
        printf("Error getting maximum power: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }
    status = TMR_paramGet(nano, TMR_PARAM_RADIO_POWERMIN, &power_min);
    if(status != TMR_SUCCESS) {
        printf("Error getting minimum power: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    // Ensure antenna is within allowed range
    if(power > power_max){
        printf("Power outside of possible range, setting to max!\n");
        power = power_max; 
    } else if(power < power_min) {
        printf("Power outside of possible range, setting to min!\n");
        power = power_min;
    }

    // Set antenna power
    status = TMR_paramSet(nano, TMR_PARAM_RADIO_READPOWER, &power);
    if(status != TMR_SUCCESS) {
        printf("Error setting read power: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    status = TMR_paramSet(nano, TMR_PARAM_RADIO_WRITEPOWER, &power);
    if(status != TMR_SUCCESS) {
        printf("Error setting write power: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }


    // Set callback
    rlb.listener = &read_callback;
    emic_fd = malloc(sizeof(int));
    *emic_fd = emic;
    rlb.cookie = emic_fd;

    status = TMR_addReadListener(nano, &rlb);
    if(status != TMR_SUCCESS) {
        printf("Error setting read listener callback: %s\n", TMR_strerror(status));
        nano_cleanup(nano);
        return -1;
    }

    return 0;
}

int nano_cleanup(Nano* nano) {
    TMR_Status status;

    status = TMR_destroy(nano);
    if(status != TMR_SUCCESS) {
        printf("Error closing reader: %s\n", TMR_strerror(status));
        return -1;
    }
    free(rlb.cookie);
    rlb.cookie = NULL;
    return 0;
}


int nano_start_reading(Nano *nano) {
    TMR_Status status;

    status = TMR_startReading(nano);
    if(status != TMR_SUCCESS) {
        printf("Failure to start reading: %s\n", TMR_strerror(status));
        return -1;
    }
    return 0;
}

int nano_stop_reading(Nano *nano){
    TMR_Status status;

    status = TMR_stopReading(nano);
    if(status != TMR_SUCCESS) {
        printf("Failure to stop reading: %s\n", TMR_strerror(status));
        return -1;
    }
    return 0;
}
