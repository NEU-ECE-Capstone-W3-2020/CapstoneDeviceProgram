#ifndef CONCURRENT_EMIC
#define CONCURRENT_EMIC

#include <pthread.h>

int init_emic(const char *sysfspath);
int emic_text_to_speech(int emic, const char *buffer, const size_t length);


#endif
