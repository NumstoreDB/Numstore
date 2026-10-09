/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#ifndef NS_LOGGER_H
#define NS_LOGGER_H

#include "core/os/ns_os.h"

#include <stdio.h>

struct console
{
  struct i_file out;
  struct i_file err;
  char          buffer[2048];
  error         e;
};

struct console
standard_console ()
{
  return (struct console){
      .out = i_stdout,
      .err = i_stderr,
  };
};

void console_flush (struct console *c);

//////////////////////// PRINTING
void console_printf (struct console *c, const char *fmt, ...) PRINTF_ATTR (2, 3);

void console_printf_err (struct console *c, const char *fmt, ...) PRINTF_ATTR (2, 3);

//////////////////////// LOGGING
void console_logf (
    struct console *c,
    const char     *prefix,
    const char     *color,
    const char     *fmt,
    ...
) PRINTF_ATTR (4, 5);

void console_logf_err (
    struct console *c,
    const char     *prefix,
    const char     *color,
    const char     *fmt,
    ...
) PRINTF_ATTR (4, 5);

#define i_log_assert(c, ...)    console_logf_err (c, "ASSERT", RED, __VA_ARGS__)
#define i_log_failure(c, ...)   console_logf_err (c, "FAILURE", BOLD_RED, __VA_ARGS__)
#define i_log_passed(c, ...)    console_logf (c, "PASSED", BOLD_GREEN, __VA_ARGS__)
#define i_log_test_case(c, ...) console_logf (c, "CASE", GREEN, __VA_ARGS__)

////// Log Levels
#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4
#define LOG_TRACE 5

////// Colors
#if ANSI_COLORS
#  define RED        "\033[0;31m"
#  define GREEN      "\033[0;32m"
#  define YELLOW     "\033[0;33m"
#  define BLUE       "\033[0;34m"
#  define BOLD_RED   "\033[1;31m"
#  define BOLD_GREEN "\033[1;32m"
#  define BOLD_WHITE "\033[1;37m"
#  define RESET      "\033[0m"
#else
// ANSI escape codes not supported - use empty strings.
#  define RED        ""
#  define GREEN      ""
#  define YELLOW     ""
#  define BLUE       ""
#  define BOLD_RED   ""
#  define BOLD_GREEN ""
#  define BOLD_WHITE ""
#  define RESET      ""
#endif

#define I_LOG_LEVEL LOG_TRACE

#ifdef NLOG
#  define SHOULD_LOG_AT(lvl) 0
#else
#  define SHOULD_LOG_AT(lvl) ((I_LOG_LEVEL) >= (lvl))
#endif

// TRACE
#if SHOULD_LOG_AT(LOG_TRACE)
#  define i_log_trace(c, ...) console_logf (c, "TRACE", BOLD_WHITE, __VA_ARGS__)
#else
#  define i_log_trace(c, ...) ((void)c)
#endif

// DEBUG
#if SHOULD_LOG_AT(LOG_DEBUG)
#  define i_log_debug(c, ...) console_logf (c, "DEBUG", BLUE, __VA_ARGS__)
#else
#  define i_log_debug(c, ...) ((void)c)
#endif

// INFO
#if SHOULD_LOG_AT(LOG_INFO)
#  define i_log_info(c, ...) console_logf (c, "INFO", GREEN, __VA_ARGS__)
#else
#  define i_log_info(c, ...) ((void)c)
#endif

// WARN
#if SHOULD_LOG_AT(LOG_WARN)
#  define i_log_warn(c, ...) console_logf_err (c, "WARN", YELLOW, __VA_ARGS__)
#else
#  define i_log_warn(c, ...) ((void)c)
#endif

// ERROR
#if SHOULD_LOG_AT(LOG_ERROR)
#  define i_log_error(c, ...) console_logf_err (c, "ERROR", RED, __VA_ARGS__)
#else
#  define i_log_error(c, ...) ((void)c)
#endif

// Programatically choose the log level
#define i_log(c, lvl, ...)           \
  do {                               \
    if ((lvl) == LOG_TRACE) {        \
      i_log_trace (c, __VA_ARGS__);  \
    } else if ((lvl) == LOG_DEBUG) { \
      i_log_debug (c, __VA_ARGS__);  \
    } else if ((lvl) == LOG_INFO) {  \
      i_log_info (c, __VA_ARGS__);   \
    } else if ((lvl) == LOG_WARN) {  \
      i_log_warn (c, __VA_ARGS__);   \
    } else if ((lvl) == LOG_ERROR) { \
      i_log_error (c, __VA_ARGS__);  \
    } else if ((lvl) == LOG_NONE) {  \
    } else {                         \
      UNREACHABLE ();                \
    }                                \
  }                                  \
  while (0)

#endif
