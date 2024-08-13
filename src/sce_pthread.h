#ifndef __SCE_PTHREADS__
#define __SCE_PTHREADS__

#include <pthread.h>

#include <psp2/kernel/threadmgr.h>

// {"pthread_attr_init", (uintptr_t)&pthread_attr_init_fake},
//     {"pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate},
//     {"pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_fake},
//     {"pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_fake},
//     {"pthread_cond_init", (uintptr_t)&pthread_cond_init_fake},
//     {"pthread_cond_signal", (uintptr_t)&pthread_cond_signal_fake},
//     {"pthread_cond_timedwait", (uintptr_t)&pthread_cond_timedwait_fake},
//     {"pthread_cond_wait", (uintptr_t)&pthread_cond_wait_fake}, {"pthread_create", (uintptr_t)&pthread_create_fake},
//     {"pthread_detach", (uintptr_t)&import_placeholder}, {"pthread_equal", (uintptr_t)&import_placeholder},
//     {"pthread_getspecific", (uintptr_t)&import_placeholder}, {"pthread_join", (uintptr_t)&import_placeholder},
//     {"pthread_key_create", (uintptr_t)&pthread_key_create_fake},
//     {"pthread_key_delete", (uintptr_t)&pthread_key_delete_fake},
//     {"pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake},
//     {"pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake},
//     {"pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake},
//     {"pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock_fake},
//     {"pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake},
//     {"pthread_mutexattr_destroy", (uintptr_t)&import_placeholder},
//     {"pthread_mutexattr_init", (uintptr_t)&import_placeholder},
//     {"pthread_mutexattr_settype", (uintptr_t)&import_placeholder}, {"pthread_once", (uintptr_t)&import_placeholder},
//     {"pthread_self", (uintptr_t)&import_placeholder}, {"pthread_setschedparam", (uintptr_t)&import_placeholder},
//     {"pthread_setspecific", (uintptr_t)&import_placeholder},

int s_pthread_attr_init(pthread_attr_t *attr);
int s_pthread_attr_destroy(pthread_attr_t *attr);

#endif