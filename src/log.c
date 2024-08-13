#include <vitasdk.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>

#include "log.h"
#include "dialog.h"
#include "common/debugScreen.h"

#define LOG_PREFIX_FMT "[syb2:%s]: "

#define LOG_INFO_NAME  "info"
#define LOG_ERR_NAME   "err"
#define LOG_WARN_NAME  "warn"
#define LOG_DEBUG_NAME "debug"
#define LOG_FATAL_NAME "fatal"

typedef enum
{
    LOG_INFO,
    LOG_ERR,
    LOG_WARN,
    LOG_DEBUG,
    LOG_FATAL,
} log_level_t;

typedef struct
{
    log_level_t level;
    const char* name;
} log_level_map_t;

// TODO: clang-format this properly
static log_level_map_t log_level_map[] = {
    {LOG_INFO, LOG_INFO_NAME},   {LOG_ERR, LOG_ERR_NAME},     {LOG_WARN, LOG_WARN_NAME},
    {LOG_DEBUG, LOG_DEBUG_NAME}, {LOG_FATAL, LOG_FATAL_NAME},
};

static SceUID log_fd    = 0;
static SceUID log_mutex = 0;

const char* get_log_level_name(log_level_t level)
{
    for (int i = 0; i < sizeof(log_level_map) / sizeof(log_level_map_t); i++)
    {
        if (log_level_map[i].level == level)
        {
            return log_level_map[i].name;
        }
    }

    return "UNK";
}

//
// crashes on error
// TODO: don't crash, just log the error and ignore any further log calls
//
void log_init()
{
    if (log_fd == 0)
    {
        log_mutex = sceKernelCreateMutex("log_mutex", 0, 0, NULL);
        psvDebugScreenInit();

        log_fd = sceIoOpen(LOG_FILE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (log_fd < 0)
        {
            fatal_error("FATAL: failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }
}

void log_terminate()
{
    if (log_fd != 0)
    {
        sceIoClose(log_fd);
        log_fd = 0;

        psvDebugScreenFinish();
    }
}

static void log_print(log_level_t level, const char* log_str)
{
    sceKernelLockMutex(log_mutex, 1, NULL);

    if (log_fd == 0)
    {
        log_init();

        // also init psv debug screen
        psvDebugScreenInit();
    }

    if (log_fd != 0)
    {
        // char time_buffer[64];
        char log_buffer_with_prefix[1024 + 64 + 64];

        // // get current time
        // time_t     now = time(NULL);
        // struct tm* t   = localtime(&now);
        // strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", t);

        const char* level_name = get_log_level_name(level);
        sprintf(log_buffer_with_prefix, LOG_PREFIX_FMT, level_name);

        // append the log message to the prefix
        strcat(log_buffer_with_prefix, log_str);
        strcat(log_buffer_with_prefix, "\n");

        // write to log file
        sceIoWrite(log_fd, log_buffer_with_prefix, strlen(log_buffer_with_prefix));
        sceIoSyncByFd(log_fd, 0);

        // write to screen
        psvDebugScreenPrintf(log_buffer_with_prefix);
    }

    sceKernelUnlockMutex(log_mutex, 1);
}

void log_info(const char* format, ...)
{
    char log_str[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(log_str, sizeof(log_str), format, args);
    va_end(args);

    log_print(LOG_INFO, log_str);
}

void log_err(const char* format, ...)
{
    char log_str[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(log_str, sizeof(log_str), format, args);
    va_end(args);

    log_print(LOG_INFO, log_str);
}

void log_warn(const char* format, ...)
{
    char log_str[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(log_str, sizeof(log_str), format, args);
    va_end(args);

    log_print(LOG_INFO, log_str);
}

void log_debug(const char* format, ...)
{
    char log_str[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(log_str, sizeof(log_str), format, args);
    va_end(args);

    log_print(LOG_INFO, log_str);
}

void log_fatal(const char* format, ...)
{
    char log_str[4096];

    va_list args;
    va_start(args, format);
    vsnprintf(log_str, sizeof(log_str), format, args);
    va_end(args);

    log_print(LOG_INFO, log_str);
}
