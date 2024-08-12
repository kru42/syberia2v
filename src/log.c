#include "log.h"
#include <vitasdk.h>
#include <wchar.h>
#include <locale.h>

#define LOG_PREFIX "[syb2]: "

static SceUID log_handle = 0;
static SceUID log_mutex  = 0;

void init_log_file(void)
{
    log_mutex = sceKernelCreateMutex("log_mutex", 0, 0, NULL);

    if (log_handle == 0)
    {
        log_handle = sceIoOpen(LOG_FILE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (log_handle < 0)
        {
            printf("===> WARNING: Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }
}

void log_info(const char* format, ...)
{
    if (log_handle == 0)
    {
        init_log_file();
    }

    if (log_handle != 0)
    {
        char log_buffer[1024];
        char log_buffer_with_prefix[1024 + sizeof(LOG_PREFIX)];

        va_list args;
        va_start(args, format);
        vsnprintf(log_buffer, sizeof(log_buffer), format, args);
        va_end(args);

        snprintf(log_buffer_with_prefix, sizeof(log_buffer_with_prefix), "%s%s\n", LOG_PREFIX, log_buffer);

        sceKernelLockMutex(log_mutex, 1, NULL);

        sceIoWrite(log_handle, log_buffer_with_prefix, strlen(log_buffer_with_prefix));
        sceIoSyncByFd(log_handle, 0);

        sceKernelUnlockMutex(log_mutex, 1);
    }
}

void close_log_file(void)
{
    if (log_handle != 0)
    {
        sceIoClose(log_handle);
        log_handle = 0;
    }
}
