#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdarg.h>

#define LOG_FILE_PATH "ux0:data/syberia2v/syb2.log"

void log_info(const char* format, ...);

#endif