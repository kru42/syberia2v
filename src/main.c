#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <setjmp.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <pthread.h>
#include <dlfcn.h>
#include <dirent.h>
#include <math.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include <kubridge.h>
#include <pthread.h>

#include "main.h"
#include "common/debugScreen.h"
#include "so_util.h"
#include "dialog.h"
#include "jni.h"
#include "util.h"
#include "log.h"

#define LOAD_ADDRESS 0x98000000
#define printf psvDebugScreenPrintf

so_module syb2_mod = {0};

int vgl_inited = 0;

void* __wrap_memcpy(void* dest, const void* src, size_t n)
{
    return sceClibMemcpy(dest, src, n);
}

void* __wrap_memmove(void* dest, const void* src, size_t n)
{
    return sceClibMemmove(dest, src, n);
}

void* __wrap_memset(void* s, int c, size_t n)
{
    return sceClibMemset(s, c, n);
}

void import_placeholder()
{
    log_info("import placeholder called.\n");
    fatal_error("import placeholder called.\n");
}

void* dlopen_hook(const char* filename, int flags)
{
    log_info("dlopen called: %s\n", filename);
    return (void*)0xBADC0DE;
}

const char* gl_ret0[] = {
    "glDeleteRenderbuffers",
    "glDiscardFramebufferEXT",
    "glFramebufferRenderbuffer",
    "glGenRenderbuffers",
    "glBindRenderbuffer",
    "glHint",
    "glLightf",
    "glMaterialx",
    "glNormalPointer",
    "glPixelStorei",
    "glRenderbufferStorage",
    "glShadeModel",
};
static size_t gl_numret = sizeof(gl_ret0) / sizeof(*gl_ret0);

static so_default_dynlib gl_hook[] = {
    {"glCompileShader", (uintptr_t)&ret0},
    // {"glShaderSource", (uintptr_t)&glShaderSourceHook},
    // {"glTexParameterf", (uintptr_t)&glTexParameterfHook},
    // {"glTexParameteri", (uintptr_t)&glTexParameteriHook},
    // {"glBindFramebuffer", (uintptr_t)&glBindFramebufferHook},
    // {"glReadPixels", (uintptr_t)&glReadPixelsHook},
};
static size_t gl_numhook = sizeof(gl_hook) / sizeof(*gl_hook);

void* dlsym_hook(void* handle, const char* symbol)
{
    log_info("dlsym called: %s\n", symbol);

    for (size_t i = 0; i < gl_numret; ++i)
    {
        if (!strcmp(symbol, gl_ret0[i]))
        {
            return ret0;
        }
    }
    for (size_t i = 0; i < gl_numhook; ++i)
    {
        if (!strcmp(symbol, gl_hook[i].symbol))
        {
            return (void*)gl_hook[i].func;
        }
    }

    return vglGetProcAddress(symbol);
}

int fstat_hook(int fd, struct stat* buf)
{
    log_info("fstat called: %d\tfile size: %d\n", fd, buf->st_size);
    struct stat st;
    int         res = fstat(fd, &st);
    if (res == 0)
        *(uint64_t*)(buf + 0x30) = st.st_size;

    return res;
}

int __android_log_print(int prio, const char* tag, const char* fmt, ...)
{
    va_list     list;
    static char string[0x8000];

    log_info("---> %s | %s\n", __func__);

    va_start(list, fmt);
    vsprintf(string, fmt, list);
    va_end(list);

    debugPrintf("[LOG] %s: %s\n", tag, string);

    return 0;
}

int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list list)
{
    log_info("---> %s | %s\n", __func__);

    static char string[0x8000];
    vsprintf(string, fmt, list);
    va_end(list);

    debugPrintf("[LOGV] %s: %s\n", tag, string);

    return 0;
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    return vglMalloc(length);
}

int munmap(void* addr, size_t length)
{
    vglFree(addr);
    return 0;
}

int pthread_mutex_init_fake(pthread_mutex_t** uid, const pthread_mutexattr_t* mutexattr)
{
    static int count = 0;
    log_info("---> %s, %d\n", __func__, count++);

    pthread_mutex_t* m = vglCalloc(1, sizeof(pthread_mutex_t));
    if (!m)
        return -1;

    const int recursive = (mutexattr && *(const int*)mutexattr == 1);
    *m                  = recursive ? PTHREAD_RECURSIVE_MUTEX_INITIALIZER : PTHREAD_MUTEX_INITIALIZER;

    int ret = pthread_mutex_init(m, mutexattr);
    if (ret < 0)
    {
        free(m);
        return -1;
    }

    *uid = m;

    return 0;
}

// int pthread_mutex_init_fake(SceKernelLwMutexWork** work)
// {
//     log_info("---> %s\n", __func__);

//     *work = (SceKernelLwMutexWork*)vglMemalign(8, sizeof(SceKernelLwMutexWork));
//     if (sceKernelCreateLwMutex(*work, "mutex", 0, 0, NULL) < 0)
//     {
//         return -1;
//     }

//     return 0;
// }

int pthread_attr_init_fake(pthread_attr_t** attr)
{
    log_info("---> %s\n", __func__);

    *attr = vglCalloc(1, sizeof(pthread_attr_t));
    return 0;
}

int pthread_getspecific_fake(pthread_key_t key)
{
    log_info("---> %s\n", __func__);

    return (int)pthread_getspecific(key);
}

int pthread_setspecific_fake(pthread_key_t key, const void* value)
{
    log_info("---> %s\n", __func__);

    return pthread_setspecific(key, value);
}

int pthread_join_fake(pthread_t thread, void** value_ptr)
{
    log_info("---> %s\n", __func__);

    return pthread_join(thread, value_ptr);
}

int pthread_key_create_fake(pthread_key_t* key, void (*destructor)(void*))
{
    log_info("---> %s\n", __func__);

    return pthread_key_create(key, destructor);
}

int pthread_key_delete_fake(pthread_key_t key)
{
    log_info("---> %s\n", __func__);

    return pthread_key_delete(key);
}

int pthread_mutexattr_init_fake(pthread_mutexattr_t* attr)
{
    log_info("---> %s\n", __func__);
    return pthread_mutexattr_init(attr);
}

int pthread_mutexattr_destroy_fake(pthread_mutexattr_t* attr)
{
    log_info("---> %s\n", __func__);
    return pthread_mutexattr_destroy(attr);
}

int pthread_mutexattr_settype_fake(pthread_mutexattr_t* attr, int type)
{
    log_info("---> %s\n", __func__);
    return pthread_mutexattr_settype(attr, type);
}

int pthread_setschedparam_fake(pthread_t thread, int policy, const struct sched_param* param)
{
    log_info("---> %s\n", __func__);
    return pthread_setschedparam(thread, policy, param);
}

int pthread_attr_setdetachstate_fake(pthread_attr_t** attr, int detachstate)
{
    log_info("---> %s\n", __func__);
    if (!*attr)
    {
        if (pthread_attr_init_fake(attr) < 0)
            return -1;
    }
    return pthread_attr_setdetachstate(*attr, detachstate);
}

int pthread_mutex_destroy_fake(pthread_mutex_t** uid)
{
    log_info("---> %s\n", __func__);
    if (uid && *uid && (uintptr_t)*uid > 0x8000)
    {
        pthread_mutex_destroy(*uid);
        vglFree(*uid);
        *uid = NULL;
    }
    return 0;
}

// int pthread_mutex_destroy_fake(SceKernelLwMutexWork** work)
// {
//     log_info("---> %s\n", __func__);
//     if (sceKernelDeleteLwMutex(*work) < 0)
//     {
//         return -1;
//     }

//     vglFree(*work);
//     return 0;
// }

// int pthread_mutex_lock_fake(SceKernelLwMutexWork** work)
// {
//     log_info("---> %s\n", __func__);
//     if (!*work)
//     {
//         log_info("mutex not initialized. initializing...\n");
//         pthread_mutex_init_fake(work);
//     }
//     if (sceKernelLockLwMutex(*work, 1, NULL) < 0)
//     {
//         log_info("mutex lock failed.\n");
//         return -1;
//     }
//     return 0;
// }

// int pthread_mutex_unlock_fake(SceKernelLwMutexWork** work)
// {
//     log_info("---> %s\n", __func__);
//     if (sceKernelUnlockLwMutex(*work, 1) < 0)
//         return -1;
//     return 0;
// }

int pthread_mutex_lock_fake(pthread_mutex_t** uid)
{
    log_info("---> %s\n", __func__);
    int ret = 0;
    if (!*uid)
    {
        log_info("mutex not initialized. initializing...\n");
        ret = pthread_mutex_init_fake(uid, NULL);
        log_info("mutex initialized");
    }
    else if ((uintptr_t)*uid == 0x4000)
    {
        log_info("recursive mutex\n");
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    else if ((uintptr_t)*uid == 0x8000)
    {
        log_info("errorcheck mutex\n");
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    if (ret < 0)
    {
        log_info("!!!!!!!!!!!!!! mutex init failed.\n");
        return ret;
    }

    return pthread_mutex_lock(*uid);
}

int pthread_mutex_unlock_fake(pthread_mutex_t** uid)
{
    log_info("---> %s | uid: 0x%04x addr: %08x\n", __func__, *uid, uid);
    int ret = 0;
    if (!*uid)
    {
        log_info("mutex not initialized. initializing...\n");
        ret = pthread_mutex_init_fake(uid, NULL);
    }
    else if ((uintptr_t)*uid == 0x4000)
    {
        log_info("recursive mutex\n");
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    else if ((uintptr_t)*uid == 0x8000)
    {
        log_info("errorcheck mutex\n");
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    if (ret < 0)
        return ret;

    log_info("unlocking mutex %08x\n", *uid);
    return pthread_mutex_unlock(*uid);
}

int pthread_cond_init_fake(pthread_cond_t** cnd, const int* condattr)
{
    log_info("---> %s\n", __func__);
    pthread_cond_t* c = vglCalloc(1, sizeof(pthread_cond_t));
    if (!c)
        return -1;

    *c = PTHREAD_COND_INITIALIZER;

    int ret = pthread_cond_init(c, NULL);
    if (ret < 0)
    {
        free(c);
        return -1;
    }

    *cnd = c;

    return 0;
}

int pthread_cond_broadcast_fake(pthread_cond_t** cnd)
{
    log_info("---> %s\n", __func__);
    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_broadcast(*cnd);
}

int pthread_cond_signal_fake(pthread_cond_t** cnd)
{
    log_info("---> %s\n", __func__);
    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_signal(*cnd);
}

int pthread_cond_destroy_fake(pthread_cond_t** cnd)
{
    log_info("---> %s\n", __func__);
    if (cnd && *cnd)
    {
        pthread_cond_destroy(*cnd);
        free(*cnd);
        *cnd = NULL;
    }
    return 0;
}

int pthread_cond_wait_fake(pthread_cond_t** cnd, pthread_mutex_t** mtx)
{
    log_info("---> %s\n", __func__);
    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_wait(*cnd, *mtx);
}

int pthread_cond_timedwait_fake(pthread_cond_t** cnd, pthread_mutex_t** mtx, const struct timespec* t)
{
    log_info("---> %s\n", __func__);
    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_timedwait(*cnd, *mtx, t);
}

int pthread_create_fake(pthread_t* thread, const void* unused, void* entry, void* arg)
{
    log_info("---> %s\n", __func__);
    return pthread_create(thread, NULL, entry, arg);
}

int pthread_once_fake(volatile int* once_control, void (*init_routine)(void))
{
    log_info("---> %s\n", __func__);
    if (!once_control || !init_routine)
        return -1;
    if (__sync_lock_test_and_set(once_control, 1) == 0)
        (*init_routine)();
    return 0;
}

FILE* fopen_hook(const char* filename, const char* mode)
{
    log_info("fopen called: %s\n", filename);
    fatal_error("die");
    // char* s = strstr(filename, "ux0:");
    // if (s)
    //     filename = s + 1;
    // else
    // {
    //     s = strstr(filename, "ux0:");
    //     if (!s)
    //     {
    //         char patched_fname[256];
    //         // sprintf(patched_fname, "%s%s", data_path_root, )
    //     }
    // }
}

void* malloc_fake(size_t size)
{
    void* result = vglMalloc(size);
    return result;
}

EGLBoolean eglInitialize_fake(EGLDisplay dpy, EGLint* major, EGLint* minor)
{
    log_info("eglInitialize called.\n");
    int (*eglInitialize)(EGLDisplay display, EGLint* major, EGLint* minor) = vglGetProcAddress("eglInitialize");
    return eglInitialize(dpy, major, minor);
}

void __assert2(const char* file, int line, const char* func, const char* expr)
{
    log_info("assert failed: %s:%d %s: %s\n", file, line, func, expr);
    // fatal_error("assert failed: %s:%d %s: %s\n", file, line, func, expr);
}

wchar_t* wmemcpy_p(wchar_t* dest, const wchar_t* src, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        dest[i] = src[i];
    }
    return dest;
}

wchar_t* wmemmove_p(wchar_t* dest, const wchar_t* src, size_t n)
{
    log_info("[][][] call me 3 times uwu pls\n");

    if (dest < src)
    {
        // Non-overlapping or src is before dest
        for (size_t i = 0; i < n; ++i)
        {
            dest[i] = src[i];
        }
    }
    else
    {
        // Overlapping regions, copy from end to start
        for (size_t i = n; i > 0; --i)
        {
            dest[i - 1] = src[i - 1];
        }
    }
    return dest;
}

wchar_t* wmemset_p(wchar_t* dest, wchar_t ch, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        dest[i] = ch;
    }
    return dest;
}

size_t wcslen_p(const wchar_t* s)
{
    size_t length = 0;
    while (*s++)
    {
        length++;
    }
    return length;
}

wchar_t* wcsncpy_p(wchar_t* dest, const wchar_t* src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != L'\0'; ++i)
    {
        dest[i] = src[i];
    }
    // Pad with null characters if src is shorter than n
    for (; i < n; ++i)
    {
        dest[i] = L'\0';
    }
    return dest;
}

int wcscmp_p(const wchar_t* s1, const wchar_t* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (int)(*s1 - *s2);
}

extern void* __aeabi_atexit;
extern void* __cxa_atexit;
extern void* __cxa_finalize;
// extern void* __gnu_Unwind_Find_exidx; // idk
extern void* __srget; // idk
extern void* __stack_chk_fail;

static so_default_dynlib dynlib_functions[] = {{"__aeabi_atexit", (uintptr_t)&__aeabi_atexit},
                                               {"__android_log_print", (uintptr_t)&__android_log_print},
                                               {"__android_log_vprint", (uintptr_t)&__android_log_vprint},
                                               {"__assert2", (uintptr_t)&__assert2}, // idk
                                               {"__cxa_atexit", (uintptr_t)&__cxa_atexit},
                                               {"__cxa_finalize", (uintptr_t)&__cxa_finalize},
                                               {"__errno", (uintptr_t)&__errno},
                                               {"__gnu_Unwind_Find_exidx", (uintptr_t)&import_placeholder}, // idk
                                               {"__srget", (uintptr_t)&__srget},
                                               {"__stack_chk_fail", (uintptr_t)&import_placeholder},
                                               {"abort", (uintptr_t)&abort},
                                               {"access", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_delete", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_fromAssetManager", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_getCountry", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_getDensity", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_getLanguage", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_getOrientation", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_new", (uintptr_t)&import_placeholder},
                                               {"acos", (uintptr_t)&acos},
                                               {"acosf", (uintptr_t)&acosf},
                                               {"AInputEvent_getType", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_attachLooper", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_detachLooper", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_finishEvent", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_getEvent", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_hasEvents", (uintptr_t)&import_placeholder},
                                               {"AInputQueue_preDispatchEvent", (uintptr_t)&import_placeholder},
                                               {"AKeyEvent_getAction", (uintptr_t)&import_placeholder},
                                               {"AKeyEvent_getKeyCode", (uintptr_t)&import_placeholder},
                                               {"ALooper_addFd", (uintptr_t)&import_placeholder},
                                               {"ALooper_pollAll", (uintptr_t)&import_placeholder},
                                               {"ALooper_prepare", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getAction", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getAxisValue", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getPointerCount", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getPointerId", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getX", (uintptr_t)&import_placeholder},
                                               {"AMotionEvent_getY", (uintptr_t)&import_placeholder},
                                               {"ANativeActivity_finish", (uintptr_t)&import_placeholder},
                                               {"ANativeWindow_setBuffersGeometry", (uintptr_t)&import_placeholder},
                                               {"ASensorEventQueue_disableSensor", (uintptr_t)&import_placeholder},
                                               {"ASensorEventQueue_enableSensor", (uintptr_t)&import_placeholder},
                                               {"ASensorEventQueue_getEvents", (uintptr_t)&import_placeholder},
                                               {"ASensorEventQueue_setEventRate", (uintptr_t)&import_placeholder},
                                               {"ASensorManager_createEventQueue", (uintptr_t)&import_placeholder},
                                               {"ASensorManager_getDefaultSensor", (uintptr_t)&import_placeholder},
                                               {"ASensorManager_getInstance", (uintptr_t)&import_placeholder},
                                               {"asin", (uintptr_t)&asin},
                                               {"asinf", (uintptr_t)&asinf},
                                               {"atan", (uintptr_t)&atan},
                                               {"atan2", (uintptr_t)&atan2},
                                               {"atan2f", (uintptr_t)&atan2f},
                                               {"atanf", (uintptr_t)&atanf},
                                               {"atoi", (uintptr_t)&atoi},
                                               {"atol", (uintptr_t)&atol},
                                               {"calloc", (uintptr_t)&vglCalloc},
                                               {"ceil", (uintptr_t)&ceil},
                                               {"ceilf", (uintptr_t)&ceilf},
                                               {"chdir", (uintptr_t)&import_placeholder},
                                               {"clock", (uintptr_t)&import_placeholder},
                                               {"close", (uintptr_t)&import_placeholder},
                                               {"closedir", (uintptr_t)&closedir},
                                               {"cos", (uintptr_t)&cos},
                                               {"cosf", (uintptr_t)&cosf},
                                               {"cosh", (uintptr_t)&cosh},
                                               {"difftime", (uintptr_t)&import_placeholder},
                                               {"dlclose", (uintptr_t)&import_placeholder},
                                               {"dlopen", (uintptr_t)&dlopen_hook},
                                               {"dlsym", (uintptr_t)&dlsym_hook},
                                               {"eglChooseConfig", (uintptr_t)&import_placeholder},
                                               {"eglCreateContext", (uintptr_t)&import_placeholder},
                                               {"eglCreateWindowSurface", (uintptr_t)&import_placeholder},
                                               {"eglDestroyContext", (uintptr_t)&import_placeholder},
                                               {"eglDestroySurface", (uintptr_t)&import_placeholder},
                                               {"eglGetConfigAttrib", (uintptr_t)&import_placeholder},
                                               {"eglGetDisplay", (uintptr_t)&eglGetDisplay},
                                               {"eglGetError", (uintptr_t)&eglGetError},
                                               {"eglGetProcAddress", (uintptr_t)&eglGetProcAddress},
                                               {"eglInitialize", (uintptr_t)&eglInitialize_fake},
                                               {"eglMakeCurrent", (uintptr_t)&import_placeholder},
                                               {"eglQuerySurface", (uintptr_t)&import_placeholder},
                                               {"eglSwapBuffers", (uintptr_t)&eglSwapBuffers},
                                               {"eglTerminate", (uintptr_t)&import_placeholder},
                                               {"exit", (uintptr_t)&exit},
                                               {"exp", (uintptr_t)&exp},
                                               {"fclose", (uintptr_t)&import_placeholder}, // TODO
                                               {"fcntl", (uintptr_t)&import_placeholder},
                                               {"feof", (uintptr_t)&feof},
                                               {"ferror", (uintptr_t)&ferror},
                                               {"fflush", (uintptr_t)&fflush},
                                               {"fgetpos", (uintptr_t)&import_placeholder},
                                               {"fgets", (uintptr_t)&import_placeholder},
                                               {"floor", (uintptr_t)&floor},
                                               {"floorf", (uintptr_t)&floorf},
                                               {"fmod", (uintptr_t)&fmod},
                                               {"fmodf", (uintptr_t)&fmodf},
                                               {"fopen", (uintptr_t)&fopen_hook},
                                               {"fprintf", (uintptr_t)&import_placeholder}, // TODO
                                               {"fputc", (uintptr_t)&import_placeholder},
                                               {"fputs", (uintptr_t)&import_placeholder},
                                               {"fread", (uintptr_t)&import_placeholder}, // TODO
                                               {"free", (uintptr_t)&vglFree},
                                               {"freopen", (uintptr_t)&import_placeholder}, // TODO
                                               {"frexp", (uintptr_t)&import_placeholder},
                                               {"fscanf", (uintptr_t)&import_placeholder},
                                               {"fseek", (uintptr_t)&import_placeholder},
                                               {"fsetpos", (uintptr_t)&import_placeholder},
                                               {"fstat", (uintptr_t)&fstat_hook},
                                               {"ftell", (uintptr_t)&import_placeholder},
                                               {"fwrite", (uintptr_t)&import_placeholder}, // TODO
                                               {"getc", (uintptr_t)&import_placeholder},
                                               {"getcwd", (uintptr_t)&import_placeholder},
                                               {"getenv", (uintptr_t)&import_placeholder},
                                               {"gettimeofday", (uintptr_t)&gettimeofday},
                                               {"glAlphaFuncx", (uintptr_t)&import_placeholder},
                                               {"glBindTexture", (uintptr_t)&import_placeholder},
                                               {"glBlendFunc", (uintptr_t)&import_placeholder},
                                               {"glClear", (uintptr_t)&import_placeholder},
                                               {"glClearColorx", (uintptr_t)&import_placeholder},
                                               {"glClearDepthx", (uintptr_t)&import_placeholder},
                                               {"glClearStencil", (uintptr_t)&import_placeholder},
                                               {"glColor4x", (uintptr_t)&import_placeholder},
                                               {"glColorMask", (uintptr_t)&import_placeholder},
                                               {"glColorPointer", (uintptr_t)&import_placeholder},
                                               {"glCompressedTexImage2D", (uintptr_t)&import_placeholder},
                                               {"glCompressedTexSubImage2D", (uintptr_t)&import_placeholder},
                                               {"glCopyTexSubImage2D", (uintptr_t)&import_placeholder},
                                               {"glCullFace", (uintptr_t)&import_placeholder},
                                               {"glDeleteTextures", (uintptr_t)&import_placeholder},
                                               {"glDepthFunc", (uintptr_t)&import_placeholder},
                                               {"glDepthMask", (uintptr_t)&import_placeholder},
                                               {"glDisable", (uintptr_t)&import_placeholder},
                                               {"glDisableClientState", (uintptr_t)&import_placeholder},
                                               {"glDrawArrays", (uintptr_t)&import_placeholder},
                                               {"glDrawElements", (uintptr_t)&import_placeholder},
                                               {"glEnable", (uintptr_t)&import_placeholder},
                                               {"glEnableClientState", (uintptr_t)&import_placeholder},
                                               {"glGenTextures", (uintptr_t)&import_placeholder},
                                               {"glGetIntegerv", (uintptr_t)&import_placeholder},
                                               {"glGetString", (uintptr_t)&import_placeholder},
                                               {"glHint", (uintptr_t)&import_placeholder},
                                               {"glLightModelxv", (uintptr_t)&import_placeholder},
                                               {"glLightx", (uintptr_t)&import_placeholder},
                                               {"glLightxv", (uintptr_t)&import_placeholder},
                                               {"glLoadMatrixf", (uintptr_t)&import_placeholder},
                                               {"glLoadMatrixx", (uintptr_t)&import_placeholder},
                                               {"glMaterialx", (uintptr_t)&import_placeholder},
                                               {"glMaterialxv", (uintptr_t)&import_placeholder},
                                               {"glMatrixMode", (uintptr_t)&import_placeholder},
                                               {"glNormalPointer", (uintptr_t)&import_placeholder},
                                               {"glPixelStorei", (uintptr_t)&import_placeholder},
                                               {"glPopMatrix", (uintptr_t)&import_placeholder},
                                               {"glPushMatrix", (uintptr_t)&import_placeholder},
                                               {"glReadPixels", (uintptr_t)&import_placeholder},
                                               {"glScissor", (uintptr_t)&import_placeholder},
                                               {"glShadeModel", (uintptr_t)&import_placeholder},
                                               {"glTexCoordPointer", (uintptr_t)&import_placeholder},
                                               {"glTexEnvx", (uintptr_t)&import_placeholder},
                                               {"glTexEnvxv", (uintptr_t)&import_placeholder},
                                               {"glTexImage2D", (uintptr_t)&import_placeholder},
                                               {"glTexParameteri", (uintptr_t)&import_placeholder},
                                               {"glTexSubImage2D", (uintptr_t)&import_placeholder},
                                               {"glVertexPointer", (uintptr_t)&import_placeholder},
                                               {"glViewport", (uintptr_t)&import_placeholder},
                                               {"gmtime", (uintptr_t)&import_placeholder}, // todo
                                               {"isalpha", (uintptr_t)&isalpha},
                                               {"iscntrl", (uintptr_t)&iscntrl},
                                               {"islower", (uintptr_t)&islower},
                                               {"isprint", (uintptr_t)&isprint},
                                               {"ispunct", (uintptr_t)&ispunct},
                                               {"isspace", (uintptr_t)&isspace},
                                               {"isupper", (uintptr_t)&isupper},
                                               {"iswalpha", (uintptr_t)&iswalpha},
                                               {"iswcntrl", (uintptr_t)&iswcntrl},
                                               {"iswdigit", (uintptr_t)&iswdigit},
                                               {"iswlower", (uintptr_t)&iswlower},
                                               {"iswprint", (uintptr_t)&iswprint},
                                               {"iswpunct", (uintptr_t)&iswpunct},
                                               {"iswspace", (uintptr_t)&iswspace},
                                               {"iswupper", (uintptr_t)&iswupper},
                                               {"iswxdigit", (uintptr_t)&iswxdigit},
                                               {"isxdigit", (uintptr_t)&isxdigit},
                                               {"ldexp", (uintptr_t)&ldexp},
                                               {"localtime", (uintptr_t)&import_placeholder}, // TODO
                                               {"log", (uintptr_t)&log},
                                               {"log10", (uintptr_t)&log10},
                                               {"log10f", (uintptr_t)&log10f},
                                               {"longjmp", (uintptr_t)&import_placeholder}, // todo
                                               {"lrand48", (uintptr_t)&lrand48},
                                               {"lseek", (uintptr_t)&import_placeholder}, // todo
                                               {"lstat", (uintptr_t)&import_placeholder}, // todo
                                               {"malloc", (uintptr_t)&malloc_fake},
                                               {"memchr", (uintptr_t)&sceClibMemchr},
                                               {"memcmp", (uintptr_t)&sceClibMemcmp},
                                               {"memcpy", (uintptr_t)&sceClibMemcpy},
                                               {"memmove", (uintptr_t)&sceClibMemmove},
                                               {"memset", (uintptr_t)&sceClibMemset},
                                               {"mkdir", (uintptr_t)&import_placeholder},  // TODO
                                               {"mktemp", (uintptr_t)&import_placeholder}, //
                                               {"mktime", (uintptr_t)&import_placeholder}, //
                                               {"mmap", (uintptr_t)&mmap},                 // implemented
                                               {"modf", (uintptr_t)&modf},
                                               {"munmap", (uintptr_t)&munmap},              // implemented
                                               {"open", (uintptr_t)&import_placeholder},    // TODO
                                               {"opendir", (uintptr_t)&import_placeholder}, // TODO
                                               {"pipe", (uintptr_t)&import_placeholder},    // todo
                                               {"pow", (uintptr_t)&pow},
                                               {"printf", (uintptr_t)&import_placeholder}, // TODO: log this stuff
                                               {"pthread_attr_init", (uintptr_t)&pthread_attr_init_fake},
                                               {"pthread_attr_setdetachstate", (uintptr_t)&import_placeholder},
                                               {"pthread_cond_broadcast", (uintptr_t)&import_placeholder},
                                               {"pthread_cond_destroy", (uintptr_t)&import_placeholder},
                                               {"pthread_cond_init", (uintptr_t)&import_placeholder},
                                               {"pthread_cond_wait", (uintptr_t)&import_placeholder},
                                               {"pthread_create", (uintptr_t)&import_placeholder},
                                               {"pthread_getspecific", (uintptr_t)&import_placeholder},
                                               {"pthread_join", (uintptr_t)&import_placeholder},
                                               {"pthread_key_create", (uintptr_t)&pthread_key_create_fake},
                                               {"pthread_key_delete", (uintptr_t)&import_placeholder},
                                               {"pthread_mutex_destroy", (uintptr_t)&import_placeholder},
                                               {"pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake},
                                               {"pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake},
                                               {"pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake},
                                               {"pthread_mutexattr_destroy", (uintptr_t)&import_placeholder},
                                               {"pthread_mutexattr_init", (uintptr_t)&import_placeholder},
                                               {"pthread_mutexattr_settype", (uintptr_t)&import_placeholder},
                                               {"pthread_setschedparam", (uintptr_t)&import_placeholder},
                                               {"pthread_setspecific", (uintptr_t)&import_placeholder},
                                               {"putc", (uintptr_t)&putc},
                                               {"puts", (uintptr_t)&puts},
                                               {"qsort", (uintptr_t)&qsort},
                                               {"raise", (uintptr_t)&raise},
                                               {"read", (uintptr_t)&import_placeholder},
                                               {"readdir", (uintptr_t)&import_placeholder},
                                               {"realloc", (uintptr_t)&vglRealloc},
                                               {"remove", (uintptr_t)&import_placeholder}, // todo
                                               {"rename", (uintptr_t)&import_placeholder}, // todo
                                               {"rint", (uintptr_t)&rint},
                                               {"rmdir", (uintptr_t)&import_placeholder}, // TODO
                                               {"setjmp", (uintptr_t)&import_placeholder},
                                               {"setlocale", (uintptr_t)&import_placeholder}, // TODO
                                               {"setvbuf", (uintptr_t)&import_placeholder},
                                               {"sin", (uintptr_t)&sin},
                                               {"sinf", (uintptr_t)&sinf},
                                               {"sinh", (uintptr_t)&sinh},
                                               {"slCreateEngine", (uintptr_t)&import_placeholder}, // TODO
                                               {"snprintf", (uintptr_t)&sceClibSnprintf},
                                               {"sprintf", (uintptr_t)&sprintf},
                                               {"sqrt", (uintptr_t)&sqrt},
                                               {"sqrtf", (uintptr_t)&sqrtf},
                                               {"srand48", (uintptr_t)&srand48},
                                               {"sscanf", (uintptr_t)&sscanf},
                                               {"strcat", (uintptr_t)&strcat},
                                               {"strchr", (uintptr_t)&sceClibStrchr},
                                               {"strcmp", (uintptr_t)&sceClibStrcmp},
                                               {"strcoll", (uintptr_t)&strcoll},
                                               {"strcpy", (uintptr_t)&strcpy},
                                               {"strerror", (uintptr_t)&strerror},
                                               {"strftime", (uintptr_t)&import_placeholder}, // TODO
                                               {"strlen", (uintptr_t)&strlen},
                                               {"strncat", (uintptr_t)&strncat},
                                               {"strncmp", (uintptr_t)&strncmp},
                                               {"strncpy", (uintptr_t)&strncpy},
                                               {"strpbrk", (uintptr_t)&strpbrk},
                                               {"strrchr", (uintptr_t)&strrchr},
                                               {"strspn", (uintptr_t)&strspn},
                                               {"strstr", (uintptr_t)&strstr},
                                               {"strtod", (uintptr_t)&strtod},
                                               {"sysconf", (uintptr_t)&import_placeholder},
                                               {"system", (uintptr_t)&import_placeholder},
                                               {"tan", (uintptr_t)&tan},
                                               {"tanf", (uintptr_t)&tanf},
                                               {"tanh", (uintptr_t)&tanh},
                                               {"time", (uintptr_t)&time},
                                               {"tmpfile", (uintptr_t)&import_placeholder},
                                               {"tmpnam", (uintptr_t)&import_placeholder},
                                               {"tolower", (uintptr_t)&tolower},
                                               {"toupper", (uintptr_t)&toupper},
                                               {"towlower", (uintptr_t)&towlower},
                                               {"towupper", (uintptr_t)&towupper},
                                               {"ungetc", (uintptr_t)&import_placeholder},
                                               {"unlink", (uintptr_t)&import_placeholder},
                                               {"usleep", (uintptr_t)&sceKernelDelayThread},
                                               {"vprintf", (uintptr_t)&vprintf},
                                               {"vsnprintf", (uintptr_t)&sceClibVsnprintf},
                                               {"vsprintf", (uintptr_t)&vsprintf},
                                               {"vsscanf", (uintptr_t)&vsscanf},
                                               {"wcscmp", (uintptr_t)&wcscmp_p},
                                               {"wcslen", (uintptr_t)&wcslen_p},
                                               {"wcsncpy", (uintptr_t)&wcsncpy_p},
                                               {"wmemcpy", (uintptr_t)&wmemcpy_p}, // don't really need to hook these
                                               {"wmemmove", (uintptr_t)&wmemmove_p},
                                               {"wmemset", (uintptr_t)&wmemset_p},
                                               {"write", (uintptr_t)&import_placeholder}};

static int check_kubridge()
{
    int search_unk[2];
    return _vshKernelSearchModuleByName("kubridge", search_unk) >= 0;
}

extern void* __cxa_guard_acquire;
extern void* __cxa_guard_release;

int main(int argc, char* argv[])
{
    psvDebugScreenInit();
    printf("======== syberia2v ========\n");
    printf("initializing...\n");

    log_info("======== syberia2v ========\n");

    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    if (check_kubridge() < 0)
        fatal_error("kubridge is not loaded.\ninstall it and reboot.");

    printf("loading libsyberia2.so...\n");
    if (so_file_load(&syb2_mod, DATA_PATH "/libsyberia2.so", LOAD_ADDRESS) < 0)
        fatal_error("failed to load libsyberia2.so.");

    printf("relocating libsyberia2.so...\n");
    so_relocate(&syb2_mod);

    printf("resolving libsyberia2.so imports...\n");
    so_resolve(&syb2_mod, dynlib_functions, sizeof(dynlib_functions), 0);

    log_info("libsyberia2.so loaded and initialized.\n");

    // hook_addr(so_symbol(&syb2_mod, "_ZN7TeMutex4lockEv"), (uintptr_t)&import_placeholder);
    // log_info("hooked TeMutex::lock\n");

    printf("flushing and initializing .so modules...\n");
    so_flush_caches(&syb2_mod);
    so_initialize(&syb2_mod);

    // jni_load();

    // printf("hooking game...\n");
    // patch_game();

    // printf("loading fake jni env...\n");
    // jni_load();

    // uintptr_t slCreateEngine_sub = so_symbol(&syb2_mod, "slCreateEngine");
    // printf("slCreateEngine at %p\n", (void *)slCreateEngine_sub);

    // printf("calling entry point...\n");

    // int (*hl2_LauncherMain)(void *unk1, void *unk2) = (void *)so_symbol(&hl2_mod, "LauncherMain");
    // printf("resolved LauncherMain at %p\n", hl2_LauncherMain);

    // hl2_LauncherMain(NULL, NULL);

    printf("all done!! waiting 10 secs before exiting...\n");

    sceKernelDelayThread(10 * 1000000);
    return 0;
}
