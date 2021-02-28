#include "capstone_bluetooth.h"
#include "emic.h"

int bt_parse_msg(const char *buffer, const int length, const int emic) {
    int ret = 0;
    for(int i = 0; i < length; ++i) {
        if(buffer[i] == '\n') {
            int rv = emic_text_to_speech(emic, buffer + ret, i - ret);
            if(rv < 0) {
                return rv;
            }
            ret += i;
        }
    }
    return ret + 1;
}
