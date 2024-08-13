// sce_pthreads.c
// pthreads emulation for PSVita

#include <psp2/kernel/threadmgr.h>
#include "sce_pthread.h"

int s_pthread_attr_init(pthread_attr_t *attr) {
    return 0;
}

