/* command.h */
/* Build and run commands */

#ifndef COMMAND_H_
#define COMMAND_H_

// TODO: create something like `command_chain` or `command_and_then`

#include "arena.h"
#include "array.h"
#include "basic.h"
#include "string.h"
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <windows.h>

#include <direct.h>
#include <io.h>
#include <shellapi.h>
#else
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif // _WIN32

#ifdef _WIN32
typedef HANDLE Process;
#define PROCESS_INVALID INVALID_HANDLE_VALUE
#else
typedef int Process;
#define PROCESS_INVALID (-1)
#endif // _WIN32

ARRAY_TYPEDEF(Process, ProcessList);
ARRAY_DECLARE_PREFIX(Process, ProcessList, process_list);

bool process_wait(Process proc);
bool process_list_wait(ProcessList procs);
bool process_list_flush(ProcessList *procs);
i32 processors_available(void);

#ifdef _WIN32
typedef HANDLE FileDescriptor;
#define FILE_DESCRIPTOR_INVALID INVALID_HANDLE_VALUE
#else
typedef int FileDescriptor;
#define FILE_DESCRIPTOR_INVALID (-1)
#endif // _WIN32

FileDescriptor fd_open_for_read(const char *path);
FileDescriptor fd_open_for_write(const char *path);
void fd_close(FileDescriptor fd);

typedef const char *CommandArgument;
ARRAY_TYPEDEF(CommandArgument, Command);
ARRAY_DECLARE_PREFIX(CommandArgument, Command, command);

typedef struct {
    ProcessList *async;
    i32 concurrency;
    const char *stdin_path;
    const char *stdout_path;
    const char *stderr_path;
    bool keep_arguments;
} CommandRunOpt;

StringView command_render(Arena *arena, Command command);
bool command_run_opt(Arena *arena, Command *command, CommandRunOpt opt);
#define COMMAND_RUN(arena, command, ...)                                       \
    command_run_opt(arena, command, (CommandRunOpt){__VA_ARGS__})
#define COMMAND_APPEND(command, ...)                                           \
    command_append(command, (CommandArgument[]){__VA_ARGS__},                  \
                   sizeof((CommandArgument[]){__VA_ARGS__}) /                  \
                       sizeof(CommandArgument))

#ifdef BOOKSTORE_IMPLEMENTATION

#include "./system.h"

ARRAY_DEFINE_PREFIX(Process, ProcessList, process_list)

bool process_wait(Process proc) {
    if (proc == PROCESS_INVALID) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(proc, INFINITE);

    if (result == WAIT_FAILED) {
        log_error("Failed to wait on child process: %s",
                  system__win32_error_message(GetLastError()));
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        log_error("Failed to get process exit code: %s",
                  system__win32_error_message(GetLastError()));
        return false;
    }

    if (exit_status != 0) {
        log_error("Command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            log_error("Failed to wait on command (pid %d): %s", proc,
                      strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                log_error("Command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            log_error("Command process was terminated by signal %d",
                      WTERMSIG(wstatus));
            return false;
        }
    }

    return true;
#endif // _WIN32
}

internal i8 process__wait_async(Process proc, i32 ms) {
#ifdef _WIN32
    DWORD result = WaitForSingleObject(proc, ms);

    if (result == WAIT_TIMEOUT) {
        return 0;
    }

    if (result == WAIT_FAILED) {
        log_error("Failed to wait on child process: %s",
                  system__win32_error_message(GetLastError()));
        return -1;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        log_error("Failed to get process exit code: %s",
                  system__win32_error_message(GetLastError()));
        return -1;
    }

    if (exit_status != 0) {
        log_error("Command exited with exit code %lu", exit_status);
        return -1;
    }

    CloseHandle(proc);

    return 1;
#else
    i64 ns = ms * 1000000;
    struct timespec duration = {
        .tv_sec = ns / 1000000000,
        .tv_nsec = ns % 1000000000,
    };

    int wstatus = 0;
    pid_t pid = waitpid(proc, &wstatus, WNOHANG);
    if (pid < 0) {
        log_error("Failed to wait on command (pid %d): %s", proc,
                  strerror(errno));
        return -1;
    }

    if (pid == 0) {
        nanosleep(&duration, NULL);
        return 0;
    }

    if (WIFEXITED(wstatus)) {
        int exit_status = WEXITSTATUS(wstatus);
        if (exit_status != 0) {
            log_error("Command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        log_error("Command process was terminated by signal %d",
                  WTERMSIG(wstatus));
        return -1;
    }

    nanosleep(&duration, NULL);
    return 0;
#endif // _WIN32
}

bool process_list_wait(ProcessList procs) {
    bool success = true;
    for (i32 i = 0; i < procs.count; i++)
        success = process_wait(procs.items[i]) && success;
    return success;
}

bool process_list_flush(ProcessList *procs) {
    bool success = process_list_wait(*procs);
    procs->count = 0;
    return success;
}

i32 processors_available(void) {
#ifdef _WIN32
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    return siSysInfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

FileDescriptor fd_open_for_read(const char *path) {
#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    FileDescriptor result =
        CreateFile(path, GENERIC_READ, 0, &saAttr, OPEN_EXISTING,
                   FILE_ATTRIBUTE_READONLY, NULL);

    if (result == INVALID_HANDLE_VALUE) {
        log_error("Failed to open '%s' for reading: %s", path,
                  system__win32_error_message(GetLastError()));
        return FILE_DESCRIPTOR_INVALID;
    }

    return result;
#else
    FileDescriptor result = open(path, O_RDONLY);
    if (result < 0) {
        log_error("Failed to open '%s' for reading: %s", path, strerror(errno));
        return FILE_DESCRIPTOR_INVALID;
    }
    return result;
#endif // _WIN32
}

FileDescriptor fd_open_for_write(const char *path) {
#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    FileDescriptor result = CreateFile(path,          // name of the write
                                       GENERIC_WRITE, // open for writing
                                       0,             // do not share
                                       &saAttr,       // default security
                                       CREATE_ALWAYS, // create always
                                       FILE_ATTRIBUTE_NORMAL, // normal file
                                       NULL // no attribute template
    );

    if (result == INVALID_HANDLE_VALUE) {
        log_error("Failed to open '%s' for writing: %s", path,
                  system__win32_error_message(GetLastError()));
        return FILE_DESCRIPTOR_INVALID;
    }

    return result;
#else
    FileDescriptor result = open(path, O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (result < 0) {
        log_error("Failed to open '%s' for writing: %s", path, strerror(errno));
        return FILE_DESCRIPTOR_INVALID;
    }
    return result;
#endif // _WIN32
}

void fd_close(FileDescriptor fd) {
#ifdef _WIN32
    CloseHandle(fd);
#else
    close(fd);
#endif // _WIN32
}

ARRAY_DEFINE_PREFIX(CommandArgument, Command, command)

#ifdef _WIN32
internal void command__win32_quote(Command command, StringBuilder *quoted) {
    for (i32 i = 0; i < command.count; ++i) {
        const char *arg = command.items[i];
        if (arg == NULL) break;
        i32 len = strlen(arg);
        if (i > 0) sb_push(quoted, ' ');
        if (len != 0 && NULL == strpbrk(arg, " \t\n\v\"")) {
            // no need to quote
            sb_append(quoted, arg, len);
        } else {
            // we need to escape:
            // 1. Double quotes in the original argument
            // 2. Consequent backslashes before a double quote
            size_t backslashes = 0;
            sb_push(quoted, '\"');
            for (i32 j = 0; j < len; ++j) {
                char x = arg[j];
                if (x == '\\') {
                    backslashes += 1;
                } else {
                    if (x == '\"') {
                        // Escape backslashes (if any) and the double quote
                        for (size_t k = 0; k < 1 + backslashes; ++k) {
                            sb_push(quoted, '\\');
                        }
                    }
                    backslashes = 0;
                }
                sb_push(quoted, x);
            }
            // escape backslashes (if any)
            for (size_t k = 0; k < backslashes; ++k) {
                sb_push(quoted, '\\');
            }
            sb_push(quoted, '\"');
        }
    }
}
#endif // _WIN32

StringView command_render(Arena *arena, Command command) {
    i32 capacity = 0;
    i32 arg_count = command.count;
    StringView *svs = arena_alloc(arena, sizeof(StringView) * arg_count);
    for (i32 i = 0; i < arg_count; i++) {
        StringView sv = sv_from_cstr(command.items[i]);
        svs[i] = sv;
        // Pretend that every argument needs to be quoted;
        // this overshoots the capacity but probably not by that much
        capacity += sv.count + 2 + (i != 0);
    }

    StringBuilder sb = sb_new(arena, capacity);
    for (i32 i = 0; i < arg_count; i++) {
        if (i > 0) sb_push(&sb, ' ');

        StringView sv = svs[i];

        if (sv_index_of(sv, ' ') < 0) {
            sb_append_sv(&sb, sv);
        } else {
            sb_push(&sb, '\'');
            sb_append_sv(&sb, sv);
            sb_push(&sb, '\'');
        }
    }

    return sb_to_sv(sb);
}

internal Process command__start_process(Arena *arena, Command command,
                                        FileDescriptor *in, FileDescriptor *out,
                                        FileDescriptor *err) {
    ASSERT(command.count > 0, "cannot run empty command");

    // Don't want to allocate memory for log if no need
    if (min_log_level <= LOG_DEBUG) {
        Lifetime lt = lifetime_begin(arena);

        StringView rendered = command_render(lt.arena, command);
        log_debug("CMD: " SV_FMT, SV_ARG(rendered));

        lifetime_end(lt);
    }

#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = err ? *err : GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = out ? *out : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = in ? *in : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    Lifetime lt = lifetime_begin(arena);
    i32 capacity = 0;
    for (i32 i = 0; i < command.count; i++) {
        if (i) capacity++;
        StringView arg = sv_from_cstr(command.items[i]);
        i32 slashes = 0;
        bool quotes = false;
        for (i32 j = 0; j < arg.count; j++) {
            if (arg.data[j] == '\\') {
                slashes++;
            } else if (isspace(arg.data[j])) {
                quotes = true;
            }
        }
        capacity += arg.count + slashes + (quotes * 2);
    }
    StringBuilder quoted = sb_new(lt.arena, capacity);
    command__win32_quote(command, &quoted);
    sb_push_null(&quoted);
    BOOL bSuccess = CreateProcessA(NULL, quoted.items, NULL, NULL, TRUE, 0,
                                   NULL, NULL, &siStartInfo, &piProcInfo);
    lifetime_end(lt);

    if (!bSuccess) {
        log_error("Failed create child process for %s: %s", command.items[0],
                  system__win32_error_message(GetLastError()));
        return PROCESS_INVALID;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        log_error("Failed to fork child process: %s", strerror(errno));
        return PROCESS_INVALID;
    }

    if (cpid == 0) {
        if (in) {
            if (dup2(*in, STDIN_FILENO) < 0) {
                log_error("Failed to setup STDIN for child process: %s",
                          strerror(errno));
                exit(1);
            }
        }

        if (out) {
            if (dup2(*out, STDOUT_FILENO) < 0) {
                log_error("Failed to setup STDOUT for child process: %s",
                          strerror(errno));
                exit(1);
            }
        }

        if (err) {
            if (dup2(*err, STDERR_FILENO) < 0) {
                log_error("Failed to setup STDERR for child process: %s",
                          strerror(errno));
                exit(1);
            }
        }

        Command with_null = command_new(arena, command.count + 1);
        command_append_other(&with_null, command);
        command_push(&with_null, NULL);

        if (execvp(command.items[0], (char *const *)with_null.items) < 0) {
            log_error("Failed to exec child process '%s': %s", command.items[0],
                      strerror(errno));
            exit(1);
        }

        UNREACHABLE("execvp called; should never get here!");
    }

    return cpid;
#endif // _WIN32
}

bool command_run_opt(Arena *arena, Command *command, CommandRunOpt opt) {
    DEFER_SETUP(bool, true);

    FileDescriptor in = FILE_DESCRIPTOR_INVALID;
    FileDescriptor out = FILE_DESCRIPTOR_INVALID;
    FileDescriptor err = FILE_DESCRIPTOR_INVALID;
    FileDescriptor *opt_in = NULL;
    FileDescriptor *opt_out = NULL;
    FileDescriptor *opt_err = NULL;

    i32 max_processes =
        opt.concurrency > 0 ? opt.concurrency : processors_available() + 1;

    if (opt.async && max_processes > 0) {
        while (opt.async->count >= max_processes) {
            for (i32 i = 0; i < opt.async->count; i++) {
                int ret = process__wait_async(opt.async->items[i], 1);
                if (ret < 0) DEFER_RETURN(false);
                if (ret) {
                    process_list_remove_swapback(opt.async, i);
                    break;
                }
            }
        }
    }

    if (opt.stdin_path) {
        in = fd_open_for_read(opt.stdin_path);
        if (in == FILE_DESCRIPTOR_INVALID) DEFER_RETURN(false);
        opt_in = &in;
    }
    if (opt.stdout_path) {
        out = fd_open_for_write(opt.stdout_path);
        if (out == FILE_DESCRIPTOR_INVALID) DEFER_RETURN(false);
        opt_out = &out;
    }
    if (opt.stderr_path) {
        err = fd_open_for_write(opt.stderr_path);
        if (err == FILE_DESCRIPTOR_INVALID) DEFER_RETURN(false);
        opt_err = &err;
    }
    Process proc =
        command__start_process(arena, *command, opt_in, opt_out, opt_err);

    if (opt.async) {
        if (proc == PROCESS_INVALID) DEFER_RETURN(false);
        process_list_push(opt.async, proc);
    } else {
        if (!process_wait(proc)) DEFER_RETURN(false);
    }

    DEFER_LABEL({
        if (opt_in) fd_close(*opt_in);
        if (opt_out) fd_close(*opt_out);
        if (opt_err) fd_close(*opt_err);
        if (!opt.keep_arguments) command->count = 0;
    });
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // COMMAND_H_
