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

#include "main.h"
#include "common/debugScreen.h"
#include "so_util.h"
#include "dialog.h"
#include "jni.h"
#include "util.h"

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
    fatal_error("import placeholder called.\n");
}

int __android_log_print(int prio, const char* tag, const char* fmt, ...)
{
    va_list     list;
    static char string[0x8000];

    va_start(list, fmt);
    vsprintf(string, fmt, list);
    va_end(list);

    debugPrintf("[LOG] %s: %s\n", tag, string);

    return 0;
}

int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list list)
{
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

extern void* __aeabi_atexit;
// extern void* __assert2;
extern void* __cxa_atexit;
extern void* __cxa_finalize;
extern void* __gnu_Unwind_Find_exidx;
// extern void *__srget;
extern void* __stack_chk_fail;

// void patch_game()
// {
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_delete"), AConfiguration_delete);
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_fromAssetManager"), );
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_getCountry"), (uintptr_t)&import_placeholder);
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_getDensity"), (uintptr_t)&import_placeholder);
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_getDensity"), (uintptr_t)&import_placeholder);
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_getDensity"), (uintptr_t)&import_placeholder);
//     hook_addr(so_symbol(&syb2_mod, "AConfiguration_new"), (uintptr_t)&import_placeholder);
// }

static so_default_dynlib dynlib_functions[] = {{"__aeabi_atexit", (uintptr_t)&__aeabi_atexit},
                                               {"__android_log_print", (uintptr_t)&__android_log_print},
                                               {"__android_log_vprint", (uintptr_t)&__android_log_vprint},
                                               {"__assert2", (uintptr_t)&import_placeholder},
                                               {"__cxa_atexit", (uintptr_t)&__cxa_atexit},
                                               {"__cxa_finalize", (uintptr_t)&__cxa_finalize},
                                               {"__errno", (uintptr_t)&__errno},
                                               {"__gnu_Unwind_Find_exidx", (uintptr_t)&import_placeholder}, // idk
                                               {"__srget", (uintptr_t)&import_placeholder},
                                               {"__stack_chk_fail", (uintptr_t)&import_placeholder},
                                               {"abort", (uintptr_t)&abort},
                                               {"access", (uintptr_t)&access},
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
                                               {"calloc", (uintptr_t)&calloc},
                                               {"ceil", (uintptr_t)&ceil},
                                               {"ceilf", (uintptr_t)&ceilf},
                                               {"chdir", (uintptr_t)&chdir},
                                               {"clock", (uintptr_t)&clock},
                                               {"close", (uintptr_t)&close},
                                               {"closedir", (uintptr_t)&closedir},
                                               {"cos", (uintptr_t)&cos},
                                               {"cosf", (uintptr_t)&cosf},
                                               {"cosh", (uintptr_t)&cosh},
                                               {"difftime", (uintptr_t)&difftime},
                                               {"dlclose", (uintptr_t)&dlclose},
                                               {"dlopen", (uintptr_t)&dlopen},
                                               {"dlsym", (uintptr_t)&dlsym},
                                               {"eglChooseConfig", (uintptr_t)&import_placeholder},
                                               {"eglCreateContext", (uintptr_t)&import_placeholder},
                                               {"eglCreateWindowSurface", (uintptr_t)&import_placeholder},
                                               {"eglDestroyContext", (uintptr_t)&import_placeholder},
                                               {"eglDestroySurface", (uintptr_t)&import_placeholder},
                                               {"eglGetConfigAttrib", (uintptr_t)&import_placeholder},
                                               {"eglGetDisplay", (uintptr_t)&import_placeholder},
                                               {"eglGetError", (uintptr_t)&import_placeholder},
                                               {"eglGetProcAddress", (uintptr_t)&import_placeholder},
                                               {"eglInitialize", (uintptr_t)&import_placeholder},
                                               {"eglMakeCurrent", (uintptr_t)&import_placeholder},
                                               {"eglQuerySurface", (uintptr_t)&import_placeholder},
                                               {"eglSwapBuffers", (uintptr_t)&import_placeholder},
                                               {"eglTerminate", (uintptr_t)&import_placeholder},
                                               {"exit", (uintptr_t)&exit},
                                               {"exp", (uintptr_t)&exp},
                                               {"fclose", (uintptr_t)&fclose},
                                               {"fcntl", (uintptr_t)&fcntl},
                                               {"feof", (uintptr_t)&feof},
                                               {"ferror", (uintptr_t)&ferror},
                                               {"fflush", (uintptr_t)&fflush},
                                               {"fgetpos", (uintptr_t)&fgetpos},
                                               {"fgets", (uintptr_t)&fgets},
                                               {"floor", (uintptr_t)&floor},
                                               {"floorf", (uintptr_t)&floorf},
                                               {"fmod", (uintptr_t)&fmod},
                                               {"fmodf", (uintptr_t)&fmodf},
                                               {"fopen", (uintptr_t)&fopen},
                                               {"fprintf", (uintptr_t)&fprintf},
                                               {"fputc", (uintptr_t)&fputc},
                                               {"fputs", (uintptr_t)&fputs},
                                               {"fread", (uintptr_t)&fread},
                                               {"free", (uintptr_t)&free},
                                               {"freopen", (uintptr_t)&freopen},
                                               {"frexp", (uintptr_t)&frexp},
                                               {"fscanf", (uintptr_t)&fscanf},
                                               {"fseek", (uintptr_t)&fseek},
                                               {"fsetpos", (uintptr_t)&fsetpos},
                                               {"fstat", (uintptr_t)&fstat},
                                               {"ftell", (uintptr_t)&ftell},
                                               {"fwrite", (uintptr_t)&fwrite},
                                               {"getc", (uintptr_t)&getc},
                                               {"getcwd", (uintptr_t)&getcwd},
                                               {"getenv", (uintptr_t)&getenv},
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
                                               {"gmtime", (uintptr_t)&gmtime},
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
                                               {"localtime", (uintptr_t)&localtime},
                                               {"log", (uintptr_t)&log},
                                               {"log10", (uintptr_t)&log10},
                                               {"log10f", (uintptr_t)&log10f},
                                               {"longjmp", (uintptr_t)&longjmp},
                                               {"lrand48", (uintptr_t)&lrand48},
                                               {"lseek", (uintptr_t)&lseek},
                                               {"lstat", (uintptr_t)&lstat},
                                               {"malloc", (uintptr_t)&malloc},
                                               {"memchr", (uintptr_t)&memchr},
                                               {"memcmp", (uintptr_t)&memcmp},
                                               {"memcpy", (uintptr_t)&memcpy},
                                               {"memmove", (uintptr_t)&memmove},
                                               {"memset", (uintptr_t)&memset},
                                               {"mkdir", (uintptr_t)&mkdir},
                                               {"mktemp", (uintptr_t)&mktemp},
                                               {"mktime", (uintptr_t)&mktime},
                                               {"mmap", (uintptr_t)&mmap}, // TODO
                                               {"modf", (uintptr_t)&modf},
                                               {"munmap", (uintptr_t)&munmap}, // TODO
                                               {"open", (uintptr_t)&open},
                                               {"opendir", (uintptr_t)&opendir},
                                               {"pipe", (uintptr_t)&pipe},
                                               {"pow", (uintptr_t)&pow},
                                               {"printf", (uintptr_t)&printf},
                                               {"pthread_attr_init", (uintptr_t)&pthread_attr_init},
                                               {"pthread_attr_setdetachstate", (uintptr_t)&pthread_attr_setdetachstate},
                                               {"pthread_cond_broadcast", (uintptr_t)&pthread_cond_broadcast},
                                               {"pthread_cond_destroy", (uintptr_t)&pthread_cond_destroy},
                                               {"pthread_cond_init", (uintptr_t)&pthread_cond_init},
                                               {"pthread_cond_wait", (uintptr_t)&pthread_cond_wait},
                                               {"pthread_create", (uintptr_t)&pthread_create},
                                               {"pthread_getspecific", (uintptr_t)&pthread_getspecific},
                                               {"pthread_join", (uintptr_t)&pthread_join},
                                               {"pthread_key_create", (uintptr_t)&pthread_key_create},
                                               {"pthread_key_delete", (uintptr_t)&pthread_key_delete},
                                               {"pthread_mutex_destroy", (uintptr_t)&pthread_mutex_destroy},
                                               {"pthread_mutex_init", (uintptr_t)&pthread_mutex_init},
                                               {"pthread_mutex_lock", (uintptr_t)&pthread_mutex_lock},
                                               {"pthread_mutex_unlock", (uintptr_t)&pthread_mutex_unlock},
                                               {"pthread_mutexattr_destroy", (uintptr_t)&pthread_mutexattr_destroy},
                                               {"pthread_mutexattr_init", (uintptr_t)&pthread_mutexattr_init},
                                               {"pthread_mutexattr_settype", (uintptr_t)&pthread_mutexattr_settype},
                                               {"pthread_setschedparam", (uintptr_t)&pthread_setschedparam},
                                               {"pthread_setspecific", (uintptr_t)&pthread_setspecific},
                                               {"putc", (uintptr_t)&putc},
                                               {"puts", (uintptr_t)&puts},
                                               {"qsort", (uintptr_t)&qsort},
                                               {"raise", (uintptr_t)&raise},
                                               {"read", (uintptr_t)&read},
                                               {"readdir", (uintptr_t)&readdir},
                                               {"realloc", (uintptr_t)&realloc},
                                               {"remove", (uintptr_t)&remove},
                                               {"rename", (uintptr_t)&rename},
                                               {"rint", (uintptr_t)&rint},
                                               {"rmdir", (uintptr_t)&rmdir},
                                               {"setjmp", (uintptr_t)&setjmp},
                                               {"setlocale", (uintptr_t)&setlocale},
                                               {"setvbuf", (uintptr_t)&setvbuf},
                                               {"sin", (uintptr_t)&sin},
                                               {"sinf", (uintptr_t)&sinf},
                                               {"sinh", (uintptr_t)&sinh},
                                               {"slCreateEngine", (uintptr_t)&import_placeholder}, // TODO
                                               {"snprintf", (uintptr_t)&snprintf},
                                               {"sprintf", (uintptr_t)&sprintf},
                                               {"sqrt", (uintptr_t)&sqrt},
                                               {"sqrtf", (uintptr_t)&sqrtf},
                                               {"srand48", (uintptr_t)&srand48},
                                               {"sscanf", (uintptr_t)&sscanf},
                                               {"strcat", (uintptr_t)&strcat},
                                               {"strchr", (uintptr_t)&strchr},
                                               {"strcmp", (uintptr_t)&strcmp},
                                               {"strcoll", (uintptr_t)&strcoll},
                                               {"strcpy", (uintptr_t)&strcpy},
                                               {"strerror", (uintptr_t)&strerror},
                                               {"strftime", (uintptr_t)&strftime},
                                               {"strlen", (uintptr_t)&strlen},
                                               {"strncat", (uintptr_t)&strncat},
                                               {"strncmp", (uintptr_t)&strncmp},
                                               {"strncpy", (uintptr_t)&strncpy},
                                               {"strpbrk", (uintptr_t)&strpbrk},
                                               {"strrchr", (uintptr_t)&strrchr},
                                               {"strspn", (uintptr_t)&strspn},
                                               {"strstr", (uintptr_t)&strstr},
                                               {"strtod", (uintptr_t)&strtod},
                                               {"sysconf", (uintptr_t)&sysconf},
                                               {"system", (uintptr_t)&system},
                                               {"tan", (uintptr_t)&tan},
                                               {"tanf", (uintptr_t)&tanf},
                                               {"tanh", (uintptr_t)&tanh},
                                               {"time", (uintptr_t)&time},
                                               {"tmpfile", (uintptr_t)&tmpfile},
                                               {"tmpnam", (uintptr_t)&tmpnam},
                                               {"tolower", (uintptr_t)&tolower},
                                               {"toupper", (uintptr_t)&toupper},
                                               {"towlower", (uintptr_t)&towlower},
                                               {"towupper", (uintptr_t)&towupper},
                                               {"ungetc", (uintptr_t)&ungetc},
                                               {"unlink", (uintptr_t)&unlink},
                                               {"usleep", (uintptr_t)&usleep},
                                               {"vprintf", (uintptr_t)&vprintf},
                                               {"vsnprintf", (uintptr_t)&vsnprintf},
                                               {"vsprintf", (uintptr_t)&vsprintf},
                                               {"vsscanf", (uintptr_t)&vsscanf},
                                               {"wcscmp", (uintptr_t)&wcscmp},
                                               {"wcslen", (uintptr_t)&wcslen},
                                               {"wcsncpy", (uintptr_t)&wcsncpy},
                                               {"wmemcpy", (uintptr_t)&wmemcpy},
                                               {"wmemmove", (uintptr_t)&wmemmove},
                                               {"wmemset", (uintptr_t)&wmemset},
                                               {"write", (uintptr_t)&write}};

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

    printf("flushing and initializing jni env\n");
    so_flush_caches(&syb2_mod);
    so_initialize(&syb2_mod);

    jni_load();

    printf("calling entry point...\n");

    // int (*hl2_LauncherMain)(void *unk1, void *unk2) = (void *)so_symbol(&hl2_mod, "LauncherMain");
    // printf("resolved LauncherMain at %p\n", hl2_LauncherMain);

    // hl2_LauncherMain(NULL, NULL);

    printf("all done!! waiting 10 secs before exiting...\n");

    sceKernelDelayThread(10 * 1000000);
    return 0;
}
