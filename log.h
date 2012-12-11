/*
Copyright (C) 2012 Serval Project Inc.
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __SERVALD_LOG_H
#define __SERVALD_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>

#define LOG_LEVEL_SILENT    (-1)
#define LOG_LEVEL_DEBUG     (0)
#define LOG_LEVEL_INFO      (1)
#define LOG_LEVEL_WARN      (2)
#define LOG_LEVEL_ERROR     (3)
#define LOG_LEVEL_FATAL     (4)

/*
 * Every log message identifies the location in the source code at which the
 * message was produced.  This location is represented by a struct __sourceloc,
 * which is passed by value to the logMessage() function and its ilk.
 *
 * A struct __sourceloc value is generated by the __HERE__ macro, which uses
 * the cpp(1) built-in macros __FILE__, __LINE__ and __FUNCTION__ to generate
 * its elements.  The __NOWHERE__ macro creates a struct __sourceloc with NULL
 * and zero fields.  If you pass __NOWHERE__ to logMessage(), it will omit
 * location information from the log line.
 *
 * Sometimes, a function wants to log a message as though its caller were the
 * origin of the message.  This is typical of "primitive" type functions that
 * are used in many places throughout the code, and whose internal workings are
 * generally well-debugged and of little interest for ongoing development.  In
 * this case, the code pattern is to declare the underscore-prefixed function
 * as taking a struct __sourceloc argument, and a macro that invokes the
 * function, passing the __HERE__ macro for that argument:
 *
 *    int _primitive(struct __sourceloc __whence, int arg1, const char *arg2);
 *
 *    #define primitive(arg1, arg2)  _primitive(__HERE__, (arg1), (arg2))
 *
 * Within the _primitive() function, the standard logging macros defined below
 * (WHYF(), WARNF(), INFOF(), DEBUGF() etc.) will use the __whence argument
 * instead of __HERE__ when logging their message.  This is achieved using a
 * dirty trick: in the function *definition*, the __sourceloc argument MUST be
 * named '__whence'.  The trick is that there is a global variable called
 * '__whence' which always contains the value of __NOWHERE__.  If that variable
 * is lexically obscured by a local variable or parameter called '__whence',
 * then the DEBUG macros will use __whence, otherwise they will use __HERE__.
 * This logic is encapsulated in the __WHENCE__ macro, to make it available to
 * for other purposes.  For example, a better definition of the primitive()
 * macro above would be:
 *
 *    #define primitive(arg1, arg2)  _primitive(__WHENCE__, (arg1), (arg2))
 *
 * Then, if it were invoked from within another primitive-type function, it
 * would log messages with the __sourceloc of that primitive's caller, which is
 * probably the most useful for diagnosis.
 *
 * @author Andrew Bettison <andrew@servalproject.com>
 */

struct __sourceloc {
    const char *file;
    unsigned int line;
    const char *function;
};

extern const struct __sourceloc __whence; // see above

void set_logging(FILE *f);
FILE *open_logging();
void close_logging();
void logArgv(int level, struct __sourceloc whence, const char *label, int argc, const char *const *argv);
void logString(int level, struct __sourceloc whence, const char *str);
void logMessage(int level, struct __sourceloc whence, const char *fmt, ...);
void vlogMessage(int level, struct __sourceloc whence, const char *fmt, va_list);
int logDump(int level, struct __sourceloc whence, char *name, const unsigned char *addr, size_t len);
ssize_t get_self_executable_path(char *buf, size_t len);
int log_backtrace(struct __sourceloc whence);
struct strbuf;
void set_log_implementation(void (*log_function)(int level, struct strbuf *buf));

#define __HERE__            ((struct __sourceloc){ .file = __FILE__, .line = __LINE__, .function = __FUNCTION__ })
#define __NOWHERE__         ((struct __sourceloc){ .file = NULL, .line = 0, .function = NULL })

#define __WHENCE__          (__whence.file ? __whence : __HERE__)

#define LOGF(L,F,...)       logMessage(L, __WHENCE__, F, ##__VA_ARGS__)
#define LOGF_perror(L,F,...) logMessage_perror(L, __WHENCE__, F, ##__VA_ARGS__)
#define LOG_perror(L,X)     LOGF_perror(L, "%s", (X))

#define logMessage_perror(L,whence,F,...) (logMessage(L, whence, F ": %s [errno=%d]", ##__VA_ARGS__, strerror(errno), errno))

#define FATALF(F,...)       do { LOGF(LOG_LEVEL_FATAL, F, ##__VA_ARGS__); abort(); exit(-1); } while (1)
#define FATAL(X)            FATALF("%s", (X))
#define FATALF_perror(F,...) FATALF(F ": %s [errno=%d]", ##__VA_ARGS__, strerror(errno), errno)
#define FATAL_perror(X)     FATALF_perror("%s", (X))

#define WHYF(F,...)         (LOGF(LOG_LEVEL_ERROR, F, ##__VA_ARGS__), -1)
#define WHY(X)              WHYF("%s", (X))
#define WHYFNULL(F,...)     (LOGF(LOG_LEVEL_ERROR, F, ##__VA_ARGS__), NULL)
#define WHYNULL(X)          (WHYFNULL("%s", (X)))
#define WHYF_perror(F,...)  (LOGF_perror(LOG_LEVEL_ERROR, F, ##__VA_ARGS__), -1)
#define WHY_perror(X)       WHYF_perror("%s", (X))

#define WARNF(F,...)        LOGF(LOG_LEVEL_WARN, F, ##__VA_ARGS__)
#define WARN(X)             WARNF("%s", (X))
#define WARNF_perror(F,...) LOGF_perror(LOG_LEVEL_WARN, F, ##__VA_ARGS__)
#define WARN_perror(X)      WARNF_perror("%s", (X))
#define WHY_argv(X,ARGC,ARGV) logArgv(LOG_LEVEL_ERROR, __WHENCE__, (X), (ARGC), (ARGV))

#define INFOF(F,...)        LOGF(LOG_LEVEL_INFO, F, ##__VA_ARGS__)
#define INFO(X)             INFOF("%s", (X))

#define DEBUGF(F,...)       LOGF(LOG_LEVEL_DEBUG, F, ##__VA_ARGS__)
#define DEBUG(X)            DEBUGF("%s", (X))
#define DEBUGF_perror(F,...) LOGF_perror(LOG_LEVEL_DEBUG, F, ##__VA_ARGS__)
#define DEBUG_perror(X)     DEBUGF_perror("%s", (X))
#define D                   DEBUG("D")
#define DEBUG_argv(X,ARGC,ARGV) logArgv(LOG_LEVEL_DEBUG, __WHENCE__, (X), (ARGC), (ARGV))

#define dump(X,A,N)         logDump(LOG_LEVEL_DEBUG, __WHENCE__, (X), (const unsigned char *)(A), (size_t)(N))

#define BACKTRACE           log_backtrace(__WHENCE__)

#endif // __SERVALD_LOG_H
