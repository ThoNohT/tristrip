// An extension of noh.h that includes helpers for creating build scripts.
// It is hand written, but inspired heavily on nob.h (see the copyright
// notice below). The license is therefore maintained as is.
//
// Copyright 2024 ThoNohT <e.c.p.bataille@gmail.com>
// Copyright 2023 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Ensure that noh.h is available.
#ifndef NOH_IMPLEMENTATION
#error "Please include noh.h with implementation!"
#else

#ifndef NOH_BLD_H_
#define NOH_BLD_H_


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define _WINUSER_
    #define _WINGDI_
    #define _IMM_
    #define _WINCON_
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/stat.h>
#endif // _WIN32

///////////////////////// Processes /////////////////////////

// Process identifiers.
#ifdef _WIN32
    typedef HANDLE Noh_Pid;
    #define NOH_INVALID_PROC INVALID_HANDLE_VALUE
#else
    typedef pid_t Noh_Pid;
    #define NOH_INVALID_PROC (-1)
#endif // _WIN32

// A collection of processes.
typedef struct {
    Noh_Pid *elems;
    size_t count;
    size_t capacity;
} Noh_Procs;

// Waits for a single process.
bool noh_proc_wait(Noh_Pid pid);

// Waits for a collection of processes.
bool noh_procs_wait(Noh_Procs procs);

// Frees the collection pocesses.
#define noh_procs_free(procs) noh_da_free(procs);

// Resets the collection of processes.
#define noh_procs_reset(procs) noh_da_reset(procs);

///////////////////////// Commands /////////////////////////

// Defines a command that can be run.
typedef struct { 
     const char **elems;
     size_t count;
     size_t capacity;
 } Noh_Cmd;

// Appends one or more strings to a command.
#define noh_cmd_append(cmd, ...) \
noh_da_append_multiple(          \
    cmd,                         \
    ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*)))

// Frees a command, freeing the memory used for its elements and setting the count and capacity to 0.
#define noh_cmd_free(cmd) noh_da_free(cmd)

// Resets a command, setting the count to 0.
#define noh_cmd_reset(cmd) noh_da_reset(cmd)

// Runs a command asynchronously and returns the process id.
Noh_Pid noh_cmd_run_async(Noh_Cmd cmd);

// Runs a command synchronously.
bool noh_cmd_run_sync(Noh_Cmd cmd);

// Renders a textual representation of the command into the provided string.
void noh_cmd_render(Noh_Cmd cmd, Noh_String *string);

///////////////////////// Building /////////////////////////

#ifdef _WIN32
bool noh_bld_exit_on_rebuild_fail = true;
#else
bool noh_bld_exit_on_rebuild_fail = false;
#endif // _WIN32


// Call this macro at the start of a build script. It will check if the source file is newer than the executable
// that is running, and if so, rebuild the source and call the new executable.
#define noh_rebuild_if_needed(argc, argv)                                               \
    do {                                                                                \
        const char *source_path = __FILE__;                                             \
        noh_assert(argc >= 1);                                                          \
        const char *binary_path = argv[0];                                              \
                                                                                        \
        int rebuild_needed = noh_output_is_older(binary_path, (char**)&source_path, 1); \
        if (rebuild_needed < 0) exit(1);                                                \
        if (rebuild_needed) {                                                           \
            if (noh_bld_exit_on_rebuild_fail) {                                         \
                noh_log(NOH_ERROR, "Build script needs to be recompiled.");             \
                exit(27);                                                               \
            }                                                                           \
            Noh_String backup_path = {0};                                               \
            noh_string_append_cstr(&backup_path, binary_path);                          \
            noh_string_append_cstr(&backup_path, ".old");                               \
            noh_string_append_null(&backup_path);                                       \
            if (!noh_rename(binary_path, backup_path.elems)) exit(1);                   \
                                                                                        \
            Noh_Cmd rebuild = {0};                                                      \
            noh_cmd_append(&rebuild, "cc", "-o", binary_path, source_path);             \
            bool rebuild_succeeded = noh_cmd_run_sync(rebuild);                         \
            noh_cmd_free(&rebuild);                                                     \
            if (!rebuild_succeeded) {                                                   \
                noh_rename(backup_path.elems, binary_path);                             \
                exit(1);                                                                \
            }                                                                           \
                                                                                        \
            noh_remove(backup_path.elems);                                              \
                                                                                        \
            Noh_Cmd actual_run = {0};                                                   \
            noh_da_append_multiple(&actual_run, argv, argc);                            \
            if (!noh_cmd_run_sync(actual_run)) exit(1);                                 \
            exit(0);                                                                    \
        }                                                                               \
    } while(0)

// Indicates whether the file at the output is older than any of the files at the input paths.
// 1 means it is older, 0 means it is not older, -1 means the check failed.
int noh_output_is_older(const char *output_path, char **input_paths, size_t input_paths_count);

#endif // NOH_BLD_H

#ifdef NOH_BLD_IMPLEMENTATION

///////////////////////// Processes /////////////////////////

bool noh_proc_wait(Noh_Pid pid)
{
    if (pid == NOH_INVALID_PROC) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(pid, INFINITE);

    if (result == WAIT_FAILED) {
        noh_log(NOH_ERROR, "Could not wait for command: %lu", GetLastError());
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(pid, &exit_status)) {
        noh_log(NOH_ERROR, "Could not get command exit code: %lu", GetLastError());
        return false;
    }

    if (exit_status != 0) {
        noh_log(NOH_ERROR, "Command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(pid);
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(pid, &wstatus, 0) < 0) {
            noh_log(NOH_ERROR, "Could not wait for command (pid %d): %s", pid, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                noh_log(NOH_ERROR, "Command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            noh_log(NOH_ERROR, "Command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }

#endif // _WIN32
    return true;
}

bool noh_procs_wait(Noh_Procs procs) {
    bool success = true;
    for (size_t i = 0; i < procs.count; i++) {
        success = noh_proc_wait(procs.elems[i]) && success;
    }

    return success;
}

///////////////////////// Commands /////////////////////////

// Adds a c string to a string, surrounding it with single quotes if it contains any spaces.
void noh_quote_if_needed(const char *value, Noh_String *string) {
    if (!strchr(value, ' ')) {
        noh_string_append_cstr(string, value);
    } else {
        noh_da_append(string, '\'');
        noh_string_append_cstr(string, value);
        noh_da_append(string, '\'');
    }
}

void noh_cmd_render(Noh_Cmd cmd, Noh_String *string) {
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.elems[i];
        if (arg == NULL) break;
        if (i > 0) noh_string_append_cstr(string, " ");
        noh_quote_if_needed(arg, string);
    }
}

Noh_Pid noh_cmd_run_async(Noh_Cmd cmd) {
    if (cmd.count < 1) {
        noh_log(NOH_ERROR, "Cannot run an empty command.");
        return NOH_INVALID_PROC;
    }

    // Log the command.
    Noh_String sb = {0};
    noh_cmd_render(cmd, &sb);
    noh_da_append(&sb, '\0');
    noh_log(NOH_INFO, "CMD: %s", sb.elems);

#ifdef _WIN32
    noh_string_reset(&sb);

    // https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
    STARTUPINFO suInfo;
    ZeroMemory(&suInfo, sizeof(STARTUPINFO));
    suInfo.cb = sizeof(STARTUPINFO);
    suInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    suInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    suInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    suInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION procInfo;
    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

    noh_cmd_render(cmd, &sb);
    noh_string_append_null(&sb);
    bool success = CreateProcessA(NULL, sb.elems, NULL, NULL, true, 0, NULL, NULL, &suInfo, &procInfo);
    noh_string_free(&sb);

    if (!success) {
        noh_log(NOH_ERROR, "Could not create child process: %lu", GetLastError());
        return NOH_INVALID_PROC;
    }

    CloseHandle(procInfo.hThread);
    return procInfo.hProcess;
#else
    noh_string_free(&sb);

    Noh_Pid cpid = fork();
    if (cpid < 0) {
        noh_log(NOH_ERROR, "Could not fork child process: %s", strerror(errno));
        return NOH_INVALID_PROC;
    }

    if (cpid == 0) {
        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        // Create a command that is null terminated.
        Noh_Cmd cmd_null = {0};
        noh_da_append_multiple(&cmd_null, cmd.elems, cmd.count);
        noh_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.elems[0], (char * const*) cmd_null.elems) < 0) {
            noh_log(NOH_ERROR, "Could not execute child process: %s", strerror(errno));
            exit(1);
        }
        noh_assert(0 && "unreachable");
    }

    return cpid;
#endif // _WIN32
}

bool noh_cmd_run_sync(Noh_Cmd cmd) {
    Noh_Pid pid = noh_cmd_run_async(cmd);
    if (pid == NOH_INVALID_PROC) return false;

    return noh_proc_wait(pid);
}

///////////////////////// Building /////////////////////////

int noh_output_is_older(const char *output_path, char **input_paths, size_t input_paths_count) {
#ifdef _WIN32
    bool success;
    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1; // The output path doesn't exist, consider that older.
        noh_log(NOH_ERROR, "Could not open file '%s': %lu", output_path, GetLastError());
        return -1;
    }
    
    FILETIME output_path_time;
    success = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!success) {
        noh_log(NOH_ERROR, "Could not get file time of file '%s': %lu", output_path, GetLastError());
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; i++) {
        HANDLE input_path_fd = CreateFile(input_paths[i], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // Non-existent input path means a source file does not exist, fail.
            noh_log(NOH_ERROR, "Could not open file '%s': %lu", input_paths[i], GetLastError());
            return -1;
        }

        FILETIME input_path_time;
        success = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        if (!success) {
            noh_log(NOH_ERROR, "Could not get file time of file '%s': %lu", input_paths[i], GetLastError());
            return -1;
        }

        // Any newer source file means the output is older.
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        if (errno == ENOENT) return 1; // The output path doesn't exist, consider that older.
        noh_log(NOH_ERROR, "Could not stat '%s': %s", output_path, strerror(errno));
        return -1;
    }

    struct timespec output_time = statbuf.st_mtim;

    for (size_t i = 0; i < input_paths_count; i++) {
        if (stat(input_paths[i], &statbuf) < 0) {
            // Non-existent input path means a source file does not exist, fail.
            noh_log(NOH_ERROR, "Could not stat '%s': %s", input_paths[i], strerror(errno));
            return -1;
        }

        // Any newer source file means the output is older.
        if (noh_diff_timespec_ms(&statbuf.st_mtim, &output_time) > 0) return 1;
    }

    return 0;
#endif // _WIN32
}

#endif // NOH_BLD_IMPLEMENTATION
#endif // NOH_IMPLEMENTATION
