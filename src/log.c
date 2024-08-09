#include "log.h"
#include <vitasdk.h>

static SceUID* log_handle = NULL;

static void init_log_file(void)
{
    if (log_handle == NULL)
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
    if (log_handle == NULL)
    {
        init_log_file();
    }

    if (log_handle != NULL)
    {
        char log_buffer[1024];

        va_list args;
        va_start(args, format);
        vsnprintf(log_buffer, sizeof(log_buffer), format, args);
        va_end(args);

        sceIoWrite(log_handle, log_buffer, strlen(log_buffer));

        sceIoSyncByFd(log_handle, NULL);
    }
}

void close_log_file(void)
{
    if (log_handle != NULL)
    {
        sceIoClose(log_handle);
        log_handle = NULL;
    }
}
