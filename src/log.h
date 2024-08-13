#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdarg.h>

#define LOG_FILE_PATH "ux0:data/syberia2v/syb2.log"

// TODO: how do i clang-format this?
void log_info(const char* format, ...);
void log_err(const char* format, ...);
void log_warn(const char* format, ...);
void log_debug(const char* format, ...);
void log_fatal(const char* format, ...);

// void log_winfo(const wchar_t* format, ...);
// [...]

#endif