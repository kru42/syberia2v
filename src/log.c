#include "log.h"
#include <vitasdk.h>

static FILE* log_file = NULL;

static void init_log_file(void)
{
    if (log_file == NULL)
    {
        log_file = fopen(LOG_FILE_PATH, "a");
        if (log_file == NULL)
        {
            perror("Failed to open log file");
        }
    }
}

void log_message(const char* format, ...)
{
    if (log_file == NULL)
    {
        init_log_file();
    }

    if (log_file != NULL)
    {
        va_list args;
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

void close_log_file(void)
{
    if (log_file != NULL)
    {
        fclose(log_file);
        log_file = NULL;
    }
}
