#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
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
#include <pthread.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include <kubridge.h>

#include "main.h"
#include "common/debugScreen.h"
#include "so_util.h"
#include "dialog.h"
#include "jni.h"
#include "util.h"
#include "log.h"

#define PROGRAM_NAME    "syberia2v"
#define PROGRAM_VERSION "0.1"

#define LOAD_ADDRESS 0x98000000

so_module syb2_mod = {0};

int vgl_inited = 0;

void* __wrap_memcpy(void* dest, const void* src, size_t n)
{
    // log_info("wrap memcpy called.");
    return sceClibMemcpy(dest, src, n);
}

void* __wrap_memmove(void* dest, const void* src, size_t n)
{
    // log_info("wrap memmove called.");
    return sceClibMemmove(dest, src, n);
}

void* __wrap_memset(void* s, int c, size_t n)
{
    // log_info("wrap memset called.");
    return sceClibMemset(s, c, n);
}

void import_placeholder()
{
    log_info("import placeholder called from (?)libsyberia2.%08x.",
             (uintptr_t)__builtin_return_address(0) - LOAD_ADDRESS);
    fatal_error("import placeholder called.");
}

void* dlopen_hook(const char* filename, int flags)
{
    log_info("dlopen called: %s", filename);
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
    log_info("dlsym called: %s", symbol);

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
    log_info("fstat called: %d\tfile size: %d", fd, buf->st_size);
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

    log_debug("---> %s | %s", __func__);

    va_start(list, fmt);
    vsprintf(string, fmt, list);
    va_end(list);

    log_debug("[LOG] %s: %s", tag, string);

    return 0;
}

int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list list)
{
    log_debug("---> %s | %s", __func__);

    static char string[0x8000];
    vsprintf(string, fmt, list);
    va_end(list);

    log_debug("[LOGV] %s: %s", tag, string);

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

int pthread_mutexattr_init_fake(pthread_mutexattr_t** uid)
{
    log_debug("pthread_mutexattr_init called.");

    pthread_mutexattr_t* m = calloc(1, sizeof(pthread_mutexattr_t));
    if (!m)
        return -1;

    int ret = pthread_mutexattr_init(m);
    if (ret < 0)
    {
        free(m);
        return -1;
    }

    *uid = m;

    return 0;
}

int pthread_mutexattr_destroy_fake(pthread_mutexattr_t** m)
{
    log_debug("pthread_mutexattr_destroy called.");

    if (m && *m)
    {
        pthread_mutexattr_destroy(*m);
        free(*m);
        *m = NULL;
    }
    return 0;
}

int pthread_mutexattr_settype_fake(pthread_mutexattr_t** m, int type)
{
    log_debug("pthread_mutexattr_settype called.");

    pthread_mutexattr_settype(*m, type);
    return 0;
}

int pthread_mutex_init_fake(pthread_mutex_t** uid, const pthread_mutexattr_t** mutexattr)
{
    log_debug("pthread_mutex_init called.");

    pthread_mutex_t* m = vglCalloc(1, sizeof(pthread_mutex_t));
    if (!m)
        return -1;

    int ret = pthread_mutex_init(m, mutexattr ? *mutexattr : NULL);
    if (ret < 0)
    {
        free(m);
        return -1;
    }

    *uid = m;

    return 0;
}

int pthread_mutex_destroy_fake(pthread_mutex_t** uid)
{
    log_debug("pthread_mutex_destroy called.");

    if (uid && *uid && (uintptr_t)*uid > 0x8000)
    {
        pthread_mutex_destroy(*uid);
        vglFree(*uid);
        *uid = NULL;
    }
    return 0;
}

int pthread_mutex_lock_fake(pthread_mutex_t** uid)
{
    log_debug("pthread_mutex_lock called with %08x, thread: %08x", *uid, sceKernelGetThreadId());

    int ret = 0;
    if (!*uid)
    {
        ret = pthread_mutex_init_fake(uid, NULL);
    }
    else if ((uintptr_t)*uid == 0x4000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init_fake(&attr);
        pthread_mutexattr_settype_fake(&attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy_fake(&attr);
    }
    else if ((uintptr_t)*uid == 0x8000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init_fake(&attr);
        pthread_mutexattr_settype_fake(&attr, PTHREAD_MUTEX_ERRORCHECK);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy_fake(&attr);
    }
    if (ret < 0)
        return ret;
    return pthread_mutex_lock(*uid);
}

int pthread_mutex_unlock_fake(pthread_mutex_t** uid)
{
    log_debug("pthread_mutex_unlock called with %08x, thread: %08x", *uid, sceKernelGetThreadId());

    int ret = 0;
    if (!*uid)
    {
        ret = pthread_mutex_init_fake(uid, NULL);
    }
    else if ((uintptr_t)*uid == 0x4000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init_fake(&attr);
        pthread_mutexattr_settype_fake(&attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy_fake(&attr);
    }
    else if ((uintptr_t)*uid == 0x8000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init_fake(&attr);
        pthread_mutexattr_settype_fake(&attr, PTHREAD_MUTEX_ERRORCHECK);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy_fake(&attr);
    }
    if (ret < 0)
    {
        return ret;
    }

    return pthread_mutex_unlock(*uid);
}

int pthread_cond_init_fake(pthread_cond_t** cnd, const int* condattr)
{
    log_debug("pthread_cond_init called.");

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
    log_debug("pthread_cond_broadcast called.");

    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_broadcast(*cnd);
}

int pthread_cond_signal_fake(pthread_cond_t** cnd)
{
    log_debug("pthread_cond_signal called.");

    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_signal(*cnd);
}

int pthread_cond_destroy_fake(pthread_cond_t** cnd)
{
    log_debug("pthread_cond_destroy called.");

    if (cnd && *cnd)
    {
        pthread_cond_destroy(*cnd);
        vglFree(*cnd);
        *cnd = NULL;
    }
    return 0;
}

int pthread_cond_wait_fake(pthread_cond_t** cnd, pthread_mutex_t** mtx)
{
    log_debug("pthread_cond_wait called.");

    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_wait(*cnd, *mtx);
}

int pthread_cond_timedwait_fake(pthread_cond_t** cnd, pthread_mutex_t** mtx, const struct timespec* t)
{
    log_debug("pthread_cond_timedwait called.");

    if (!*cnd)
    {
        if (pthread_cond_init_fake(cnd, NULL) < 0)
            return -1;
    }
    return pthread_cond_timedwait(*cnd, *mtx, t);
}

int pthread_attr_init_fake(pthread_attr_t** attr)
{
    log_debug("pthread_attr_init called.");

    if (!*attr)
    {
        *attr = vglCalloc(1, sizeof(pthread_attr_t));
    }

    return pthread_attr_init(*attr);
}

int pthread_create_fake(pthread_t* thread, const pthread_attr_t* attr, void* entry, void* arg)
{
    log_debug("pthread_create called.");
    return pthread_create(thread, NULL, entry, arg);
}

int pthread_mutex_trylock_fake(pthread_mutex_t** uid)
{
    log_debug("pthread_mutex_trylock called.");

    int ret = 0;
    if (!*uid)
    {
        ret = pthread_mutex_init_fake(uid, NULL);
    }
    else if ((uintptr_t)*uid == 0x4000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    else if ((uintptr_t)*uid == 0x8000)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        ret = pthread_mutex_init_fake(uid, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    if (ret < 0)
        return ret;
    return pthread_mutex_trylock(*uid);
}

int pthread_once_fake(volatile int* once_control, void (*init_routine)(void))
{
    log_debug("pthread_once called.");

    if (!once_control || !init_routine)
        return -1;
    if (__sync_lock_test_and_set(once_control, 1) == 0)
        (*init_routine)();
    return 0;
}

typedef struct
{
    pthread_mutex_t keyLock;
    void (*destructor)(void*);
} pthread_key_t_struct;

int pthread_key_create_fake(pthread_key_t* key, void (*destructor)(void*))
{
    log_debug("pthread_key_create called.");

    int                   result = 0;
    pthread_key_t_struct* newkey;

    newkey = (pthread_key_t_struct*)vglCalloc(1, sizeof(*newkey));
    if (newkey == NULL)
    {
        result = ENOMEM;
    }
    else
    {
        // Simulate successful allocation by not performing any real action
        newkey->keyLock    = PTHREAD_MUTEX_INITIALIZER;
        newkey->destructor = destructor;

        // Normally, you would handle errors here, but we'll just simulate success.
    }

    *key = (pthread_key_t)newkey;

    return result;
}

// int pthread_key_create_fake(pthread_key_t* key, void (*destructor)(void*))
// {
//     log_debug("pthread_key_create_fake called.");

//     if (!*key)
//     {
//         key = vglCalloc(1, sizeof(pthread_key_t));
//     }

//     int res = pthread_key_create(key, destructor);
//     log_debug("pthread_key_create: %d", res);

//     return res;
// }

int pthread_key_delete_fake(pthread_key_t* key)
{
    log_debug("pthread_key_delete called.");

    if (key)
    {
        pthread_key_delete(*key);
        vglFree(key);
    }

    return 0;
}

FILE* fopen_hook(const char* filename, const char* mode)
{
    log_info("fopen called: %s", filename);
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
    //         // slog_info(patched_fname, "%s%s", data_path_root, )
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
    log_info("eglInitialize called.");
    int (*eglInitialize)(EGLDisplay display, EGLint* major, EGLint* minor) = vglGetProcAddress("eglInitialize");
    return eglInitialize(dpy, major, minor);
}

void __assert2(const char* file, int line, const char* func, const char* expr)
{
    log_info("assert failed: %s:%d %s: %s", file, line, func, expr);
    fatal_error("assert failed: %s:%d %s: %s", file, line, func, expr);
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

void* sceClibMemclr(void* dst, SceSize len)
{
    return sceClibMemset(dst, 0, len);
}

void exit_hook(int status)
{
    log_info("exit called.");
    fatal_error("exit was called. ec: %d", status);
}

void* AConfiguration_new_fake()
{
    log_info("AConfiguration_new called.");
    return NULL;
}

extern void* __aeabi_atexit;
extern void* __cxa_atexit;
extern void* __cxa_finalize;
// extern void* __gnu_Unwind_Find_exidx; // idk
extern void* __stack_chk_fail;
extern void* __stack_chk_guard;

static so_default_dynlib dynlib_functions[] = {{"__aeabi_memclr", (uintptr_t)&sceClibMemclr},
                                               {"__aeabi_memclr4", (uintptr_t)&sceClibMemclr},
                                               {"__aeabi_memclr8", (uintptr_t)&sceClibMemclr},
                                               {"__aeabi_memcpy", (uintptr_t)&sceClibMemcpy},
                                               {"__aeabi_memcpy4", (uintptr_t)&sceClibMemcpy},
                                               {"__aeabi_memcpy8", (uintptr_t)&sceClibMemcpy},
                                               {"__aeabi_memmove", (uintptr_t)&sceClibMemmove},
                                               {"__aeabi_memmove4", (uintptr_t)&sceClibMemmove},
                                               {"__aeabi_memmove8", (uintptr_t)&sceClibMemmove},
                                               {"__aeabi_memset", (uintptr_t)&sceClibMemset},
                                               {"__aeabi_memset4", (uintptr_t)&sceClibMemset},
                                               {"__aeabi_memset8", (uintptr_t)&sceClibMemset},
                                               {"__android_log_print", (uintptr_t)&__android_log_print},
                                               {"__android_log_vprint", (uintptr_t)&__android_log_vprint},
                                               {"__assert2", (uintptr_t)&__assert2}, // idk
                                               {"__cxa_atexit", (uintptr_t)&__cxa_atexit},
                                               {"__cxa_finalize", (uintptr_t)&__cxa_finalize},
                                               {"__errno", (uintptr_t)&__errno},
                                               {"__gnu_Unwind_Find_exidx", (uintptr_t)&import_placeholder}, // idk
                                               {"__sF", (uintptr_t)&import_placeholder},
                                               {"__stack_chk_fail", (uintptr_t)&__stack_chk_fail},
                                               {"__stack_chk_guard", (uintptr_t)&__stack_chk_guard},
                                               {"abort", (uintptr_t)&abort},
                                               {"accept", (uintptr_t)&import_placeholder},
                                               {"access", (uintptr_t)&import_placeholder},
                                               {"AConfiguration_delete", (uintptr_t)&ret0},
                                               {"AConfiguration_fromAssetManager", (uintptr_t)&ret0},
                                               {"AConfiguration_getCountry", (uintptr_t)&ret0},
                                               {"AConfiguration_getDensity", (uintptr_t)&ret0},
                                               {"AConfiguration_getLanguage", (uintptr_t)&ret0},
                                               {"AConfiguration_getOrientation", (uintptr_t)&ret0},
                                               {"AConfiguration_new", (uintptr_t)&AConfiguration_new_fake},
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
                                               {"ALooper_addFd", (uintptr_t)&ret0},
                                               {"ALooper_pollAll", (uintptr_t)&import_placeholder},
                                               {"ALooper_prepare", (uintptr_t)&ret0},
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
                                               {"acos", (uintptr_t)&acos},
                                               {"acosf", (uintptr_t)&acosf},
                                               {"asin", (uintptr_t)&asin},
                                               {"asinf", (uintptr_t)&asinf},
                                               {"atan", (uintptr_t)&atan},
                                               {"atan2", (uintptr_t)&atan2},
                                               {"atan2f", (uintptr_t)&atan2f},
                                               {"atanf", (uintptr_t)&atanf},
                                               {"atoi", (uintptr_t)&atoi},
                                               {"atol", (uintptr_t)&atol},
                                               {"bind", (uintptr_t)&import_placeholder},
                                               {"btowc", (uintptr_t)&btowc},
                                               {"calloc", (uintptr_t)&vglCalloc},
                                               {"ceil", (uintptr_t)&ceil},
                                               {"ceilf", (uintptr_t)&ceilf},
                                               {"chdir", (uintptr_t)&import_placeholder},
                                               {"clearerr", (uintptr_t)&clearerr},
                                               {"clock", (uintptr_t)&import_placeholder},
                                               {"close", (uintptr_t)&import_placeholder},
                                               {"closedir", (uintptr_t)&import_placeholder},
                                               {"connect", (uintptr_t)&import_placeholder},
                                               {"cos", (uintptr_t)&cos},
                                               {"cosh", (uintptr_t)&cosh},
                                               {"difftime", (uintptr_t)&import_placeholder},
                                               {"dladdr", (uintptr_t)&import_placeholder},
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
                                               {"exit", (uintptr_t)&exit_hook},
                                               {"exp", (uintptr_t)&exp},
                                               {"fclose", (uintptr_t)&ret0}, // TODO
                                               {"fcntl", (uintptr_t)&import_placeholder},
                                               {"fdopen", (uintptr_t)&import_placeholder},
                                               {"feof", (uintptr_t)&ret0},
                                               {"ferror", (uintptr_t)&ret0},
                                               {"fflush", (uintptr_t)&ret0},
                                               {"fgets", (uintptr_t)&import_placeholder},
                                               {"floor", (uintptr_t)&floor},
                                               {"floorf", (uintptr_t)&floorf},
                                               {"fmod", (uintptr_t)&fmod},
                                               {"fmodf", (uintptr_t)&fmodf},
                                               {"fopen", (uintptr_t)&fopen_hook},
                                               {"flog_info", (uintptr_t)&ret0}, // TODO
                                               {"fputc", (uintptr_t)&import_placeholder},
                                               {"fputs", (uintptr_t)&import_placeholder},
                                               {"fread", (uintptr_t)&import_placeholder}, // TODO
                                               {"free", (uintptr_t)&vglFree},
                                               {"freopen", (uintptr_t)&import_placeholder}, // TODO
                                               {"frexp", (uintptr_t)&import_placeholder},
                                               {"fscanf", (uintptr_t)&import_placeholder},
                                               {"fseek", (uintptr_t)&import_placeholder},
                                               {"ftell", (uintptr_t)&import_placeholder},
                                               {"fwrite", (uintptr_t)&import_placeholder}, // TODO
                                               {"getc", (uintptr_t)&import_placeholder},
                                               {"getcwd", (uintptr_t)&import_placeholder},
                                               {"getenv", (uintptr_t)&import_placeholder},
                                               {"gethostbyname", (uintptr_t)&import_placeholder},
                                               {"gethostname", (uintptr_t)&import_placeholder},
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
                                               {"inet_ntoa", (uintptr_t)&import_placeholder},
                                               {"isalnum", (uintptr_t)&isalnum},
                                               {"isalpha", (uintptr_t)&isalpha},
                                               {"iscntrl", (uintptr_t)&iscntrl},
                                               {"isblank", (uintptr_t)&isblank},
                                               {"isgraph", (uintptr_t)&isgraph},
                                               {"islower", (uintptr_t)&islower},
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
                                               {"listen", (uintptr_t)&import_placeholder},    // TODO
                                               {"localtime", (uintptr_t)&import_placeholder}, // TODO
                                               {"log", (uintptr_t)&log},
                                               {"log10", (uintptr_t)&log10},
                                               {"log10f", (uintptr_t)&log10f},
                                               {"longjmp", (uintptr_t)&import_placeholder}, // todo
                                               {"lrand48", (uintptr_t)&lrand48},
                                               {"lstat", (uintptr_t)&import_placeholder}, // todo
                                               {"malloc", (uintptr_t)&malloc_fake},
                                               {"mbrlen", (uintptr_t)&mbrlen},        // TODO: check this
                                               {"memalign", (uintptr_t)&vglMemalign}, // TODO: check this
                                               {"memchr", (uintptr_t)&sceClibMemchr},
                                               {"memcmp", (uintptr_t)&sceClibMemcmp},
                                               {"memcpy", (uintptr_t)&sceClibMemcpy},
                                               {"memset", (uintptr_t)&sceClibMemset},
                                               {"mkdir", (uintptr_t)&import_placeholder},  // TODO
                                               {"mktemp", (uintptr_t)&import_placeholder}, //
                                               {"mktime", (uintptr_t)&import_placeholder}, //
                                               {"modf", (uintptr_t)&modf},
                                               {"nanosleep", (uintptr_t)&import_placeholder}, // TODO
                                               {"open", (uintptr_t)&import_placeholder},      // TODO
                                               {"opendir", (uintptr_t)&import_placeholder},   // TODO
                                               {"pipe", (uintptr_t)&ret0},
                                               {"pow", (uintptr_t)&pow},
                                               {"pthread_attr_init", (uintptr_t)&pthread_attr_init_fake},
                                               {"pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate},
                                               {"pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast_fake},
                                               {"pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy_fake},
                                               {"pthread_cond_init", (uintptr_t)&pthread_cond_init_fake},
                                               {"pthread_cond_signal", (uintptr_t)&pthread_cond_signal_fake},
                                               {"pthread_cond_timedwait", (uintptr_t)&pthread_cond_timedwait_fake},
                                               {"pthread_cond_wait", (uintptr_t)&pthread_cond_wait_fake},
                                               {"pthread_create", (uintptr_t)&pthread_create_fake},
                                               {"pthread_detach", (uintptr_t)&import_placeholder},
                                               {"pthread_equal", (uintptr_t)&import_placeholder},
                                               {"pthread_getspecific", (uintptr_t)&import_placeholder},
                                               {"pthread_join", (uintptr_t)&import_placeholder},
                                               {"pthread_key_create", (uintptr_t)&pthread_key_create_fake},
                                               {"pthread_key_delete", (uintptr_t)&pthread_key_delete_fake},
                                               {"pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy_fake},
                                               {"pthread_mutex_init", (uintptr_t)&pthread_mutex_init_fake},
                                               {"pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock_fake},
                                               {"pthread_mutex_trylock", (uintptr_t)&pthread_mutex_trylock_fake},
                                               {"pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock_fake},
                                               {"pthread_mutexattr_destroy", (uintptr_t)&import_placeholder},
                                               {"pthread_mutexattr_init", (uintptr_t)&import_placeholder},
                                               {"pthread_mutexattr_settype", (uintptr_t)&import_placeholder},
                                               {"pthread_once", (uintptr_t)&import_placeholder},
                                               {"pthread_self", (uintptr_t)&import_placeholder},
                                               {"pthread_setschedparam", (uintptr_t)&import_placeholder},
                                               {"pthread_setspecific", (uintptr_t)&import_placeholder},
                                               {"putc", (uintptr_t)&putc},
                                               {"puts", (uintptr_t)&puts},
                                               {"qsort", (uintptr_t)&qsort},
                                               {"raise", (uintptr_t)&raise},
                                               {"read", (uintptr_t)&import_placeholder},
                                               {"readdir", (uintptr_t)&import_placeholder},
                                               {"realloc", (uintptr_t)&vglRealloc},
                                               {"recvfrom", (uintptr_t)&import_placeholder}, // todo
                                               {"remove", (uintptr_t)&import_placeholder},   // todo
                                               {"rename", (uintptr_t)&import_placeholder},   // todo
                                               {"rint", (uintptr_t)&rint},
                                               {"rintf", (uintptr_t)&rintf},
                                               {"rmdir", (uintptr_t)&import_placeholder}, // TODO
                                               {"sched_yield", (uintptr_t)&import_placeholder},
                                               {"select", (uintptr_t)&import_placeholder},
                                               {"sendto", (uintptr_t)&import_placeholder},
                                               {"setjmp", (uintptr_t)&import_placeholder},
                                               {"setsockopt", (uintptr_t)&import_placeholder},
                                               {"setvbuf", (uintptr_t)&import_placeholder},
                                               {"shutdown", (uintptr_t)&import_placeholder},
                                               {"sin", (uintptr_t)&sin},
                                               {"sincos", (uintptr_t)&sincos},
                                               {"sincosf", (uintptr_t)&sincosf},
                                               {"sinf", (uintptr_t)&sinf},
                                               {"sinh", (uintptr_t)&sinh},
                                               {"slCreateEngine", (uintptr_t)&import_placeholder}, // TODO
                                               {"snprintf", (uintptr_t)&sceClibSnprintf},
                                               {"socket", (uintptr_t)&import_placeholder},
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
                                               {"strerror_r", (uintptr_t)&import_placeholder}, // TODO !!!!
                                               {"strftime", (uintptr_t)&import_placeholder},   // TODO
                                               {"strlen", (uintptr_t)&strlen},
                                               {"strncat", (uintptr_t)&strncat},
                                               {"strncmp", (uintptr_t)&strncmp},
                                               {"strncpy", (uintptr_t)&strncpy},
                                               {"strpbrk", (uintptr_t)&strpbrk},
                                               {"strrchr", (uintptr_t)&strrchr},
                                               {"strspn", (uintptr_t)&strspn},
                                               {"strstr", (uintptr_t)&strstr},
                                               {"strtod", (uintptr_t)&strtod},
                                               {"strtoimax", (uintptr_t)&import_placeholder}, // TODO !!!!
                                               {"strtol", (uintptr_t)&strtol},
                                               {"strtoll", (uintptr_t)&strtoll},
                                               {"strtoul", (uintptr_t)&strtoul},
                                               {"strtoull", (uintptr_t)&strtoull},
                                               {"strtoumax", (uintptr_t)&import_placeholder}, // TODO !!!!
                                               {"strxfrm", (uintptr_t)&strxfrm},
                                               {"syscall", (uintptr_t)&import_placeholder}, // TODO !!!!
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
                                               {"vaslog_info", (uintptr_t)&import_placeholder}, // TODO: implement this
                                               {"vfprintf", (uintptr_t)&vfprintf},              // check these
                                               {"vprintf", (uintptr_t)&vprintf},
                                               {"vsnprintf", (uintptr_t)&sceClibVsnprintf},
                                               {"vsprintf", (uintptr_t)&vsprintf},
                                               {"vsscanf", (uintptr_t)&vsscanf},
                                               {"wcscoll", (uintptr_t)&wcscoll},
                                               {"wcsxfrm", (uintptr_t)&wcsxfrm},
                                               {"wctob", (uintptr_t)&wctob},
                                               {"write", (uintptr_t)&import_placeholder}};

static int check_kubridge()
{
    int search_unk[2];
    return _vshKernelSearchModuleByName("kubridge", search_unk) >= 0;
}

// typedef struct game_activity_callback_table
// {
//     void* onStart;                    // 0x00
//     void* onResume;                   // 0x04
//     void* onSaveInstanceState;        // 0x08
//     void* onPause;                    // 0x0C
//     void* onStop;                     // 0x10
//     void* onDestroy;                  // 0x14
//     void* onWindowFocusChanged;       // 0x18
//     void* onNativeWindowCreated;      // 0x1C
//     void* onNativeWindowResized;      // 0x20
//     void* onNativeWindowRedrawNeeded; // 0x24
//     void* onNativeWindowDestroyed;    // 0x28
//     void* onInputQueueCreated;        // 0x2C
//     void* onInputQueueDestroyed;      // 0x30
//     void* onContentRectChanged;       // 0x34
//     void* onConfigurationChanged;     // 0x38
//     void* onLowMemory;                // 0x3C
// } game_activity_callback_table_t;

void call_me(uint32_t unk1, int* unk2, uint32_t unk3)
{
    log_info("call_me called.");
    fatal_error("call_me called.");
}

//
// eh I should look into ndk but I'm lazy and dysfunctional
//
typedef void(__fastcall* func_t)(uintptr_t, int*, uintptr_t);

typedef struct __attribute__((packed, aligned(1))) func_table
{
    func_t* func_ptrs; // array of function pointers
} func_table_t;

typedef struct __attribute__((packed, aligned(1))) unk_type1
{
    uint32_t      unk1;       // 0x00
    func_table_t* func_table; // 0x04
} unk_type1_t;

typedef struct __attribute__((packed, aligned(1))) game_activity
{
    unk_type1_t* unk1;          // 0x00
    uint8_t      padding[0x18]; // 0x04 - 0x1B
    void*        instance;      // 0x1C
} game_activity_t;              // 148 bytes? inited at 0

extern void* __cxa_guard_acquire;
extern void* __cxa_guard_release;

int main(int argc, char* argv[])
{
    log_info(PROGRAM_NAME " " PROGRAM_VERSION);
    log_info(PROGRAM_NAME " " PROGRAM_VERSION " log initialized");

    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    if (check_kubridge() < 0)
        fatal_error("kubridge is not loaded.\ninstall it and reboot.");

    log_info("loading libsyberia2.so file");
    if (so_file_load(&syb2_mod, DATA_PATH "/libsyberia2.so", LOAD_ADDRESS) < 0)
        fatal_error("failed to load libsyberia2.so.");

    log_info("relocating libsyberia2.so");
    so_relocate(&syb2_mod);

    log_info("resolving libsyberia2.so imports");
    so_resolve(&syb2_mod, dynlib_functions, sizeof(dynlib_functions), 0);

    log_info("flushing and initializing .so modules...");
    so_flush_caches(&syb2_mod);
    so_initialize(&syb2_mod);

    log_info("libsyberia2.so loaded");
    log_info("libsyberia2.so loaded and initialized");

    // log_info("hooking game...");
    // patch_game();

    log_info("loading fake jni env");
    jni_load();

    game_activity_t activity             = {0};
    activity.unk1                        = malloc(sizeof(unk_type1_t));
    activity.unk1->func_table            = malloc(sizeof(func_table_t));
    activity.unk1->func_table->func_ptrs = malloc(16 * sizeof(func_t)); // game has only 16 callbacks?
    // TODO: free the previous allocations on exit
    // func_t* fptrs = activity.unk1->func_table->func_ptrs;
    // fptrs[1]      = (func_t)&call_me;

    int (*ANativeActivity_onCreate)(game_activity_t*, void*, size_t) =
        (void*)so_symbol(&syb2_mod, "ANativeActivity_onCreate");

    log_info("calling ANativeActivity_onCreate at %p...", ANativeActivity_onCreate);
    log_info("calling ANativeActivity_onCreate...");
    ANativeActivity_onCreate(&activity, NULL, 0);

    log_info("all done!! waiting 10 secs before exiting...");

    sceKernelDelayThread(10 * 1000000);
    return 0;
}
