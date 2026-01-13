#ifndef BUILD_H_
#define BUILD_H_

#include "./arena.h"
#include "./command.h"
#include "./flag.h"
#include "./string.h"
#include "./system.h"

#ifdef BOOKSTORE_IMPLEMENTATION

#ifndef COMMAND_CC
#if _WIN32
#if defined(__GNUC__)
#define COMMAND_CC(command) command_push(command, "cc")
#elif defined(__clang__)
#define COMMAND_CC(command) command_push(command, "clang")
#elif defined(_MSC_VER)
#define COMMAND_CC(command) command_push(command, "cl.exe")
#elif defined(__TINYC__)
#define COMMAND_CC(command) command_push(command, "tcc")
#endif // defined(__GNUC__)
#else
#define COMMAND_CC(command) command_push(command, "cc")
#endif // _WIN32
#endif // COMMAND_CC

#ifndef COMMAND_CC_FLAGS
#if defined(_MSC_VER) && !defined(__clang__)
#define COMMAND_CC_FLAGS(command)                                              \
    COMMAND_APPEND(command, "/W4", "/nologo", "/D_CRT_SECURE_NO_WARNINGS")
#else
#define COMMAND_CC_FLAGS(command) COMMAND_APPEND(command, "-Wall", "-Wextra")
#endif // defined(_MSC_VER) && !defined(__clang__)
#endif // COMMAND_CC_FLAGS

#ifndef COMMAND_CC_DEBUG_INFO
#if defined(_MSC_VER) && !defined(__clang__)
// TODO: Debug info for MSVC
#define COMMAND_CC_DEBUG_INFO(command) TODO("Debug info for MSVC")
#else
#define COMMAND_CC_DEBUG_INFO(command) COMMAND_APPEND(command, "-g")
#endif // defined(_MSC_VER) && !defined(__clang__)
#endif // COMMAND_CC_DEBUG_INFO

#ifndef COMMAND_CC_OPTIMIZE
#if defined(_MSC_VER) && !defined(__clang__)
// TODO: Optimize for MSVC
#define COMMAND_CC_OPTIMIZE(command) TODO("Optimize for MSVC")
#else
#define COMMAND_CC_OPTIMIZE(command) COMMAND_APPEND(command, "-03")
#endif // defined(_MSC_VER) && !defined(__clang__)
#endif // COMMAND_CC_OPTIMIZE

#ifndef COMMAND_CC_ADDRESS_SANITIZE
#if defined(_MSC_VER) && !defined(__clang__)
// TODO: address sanitize for MSVC
#define COMMAND_CC_ADDRESS_SANITIZE(command) TODO("address sanitize for MSVC")
#else
#define COMMAND_CC_ADDRESS_SANITIZE(command)                                   \
    COMMAND_APPEND(command, "-fsanitize=address")
#endif // defined(_MSC_VER) && !defined(__clang__)
#endif // COMMAND_CC_ADDRESS_SANITIZE

#ifndef COMMAND_CC_OUTPUT
#if defined(_MSC_VER) && !defined(__clang__)
#define COMMAND_CC_OUTPUT(arena, command, output_path)                         \
    COMMAND_APPEND(command, arena_sprintf(arena, "/Fe:%s", (output_path)),     \
                   arena_sprintf(arena, "/Fo:%s", (output_path)))
#else
#define COMMAND_CC_OUTPUT(arena, command, output_path)                         \
    COMMAND_APPEND(command, "-o", (output_path))
#endif
#endif // COMMAND_CC_OUTPUT

#ifndef COMMAND_CC_INPUTS
#define COMMAND_CC_INPUTS(command, ...) COMMAND_APPEND(command, __VA_ARGS__)
#endif // COMMAND_CC_INPUTS

typedef struct {
    const char *dir;
} CommandCompileFlagsTxtOpt;
bool command_compile_flags_txt_opt(Arena *arena, Command *command,
                                   CommandCompileFlagsTxtOpt opt);
#define COMMAND_COMPILE_FLAGS_TXT(arena, command, ...)                         \
    command_compile_flags_txt_opt(arena, command,                              \
                                  (CommandCompileFlagsTxtOpt){__VA_ARGS__})

i8 build_needs_rebuild(const char *output_path, FilePaths input_paths);
void build__self_rebuild(Arena *arena, int argc, const char **argv,
                         const char *self_path, FilePaths dependencies);
#define SELF_REBUILD(arena, argc, argv)                                        \
    build__self_rebuild(arena, argc, argv, __FILE__, FILE_PATHS_EMPTY)
#define SELF_REBUILD_DEPENDENCIES(arena, argc, argv, paths)                    \
    build__self_rebuild(arena, argc, argv, __FILE__, paths)

#endif // BOOKSTORE_IMPLEMENTATION

bool command_compile_flags_txt_opt(Arena *arena, Command *command,
                                   CommandCompileFlagsTxtOpt opt) {
    const char *filepath = "compile_flags.txt";
    if (opt.dir) {
        StringView dir = sv_from_cstr(opt.dir);
        StringView sv = sv_from_cstr(filepath);
        StringBuilder sb = sb_new(arena, sv.count + dir.count + 1);
        sb_append_sv(&sb, dir);
        sb_push(&sb, SYSTEM_PATH_DELIMITER);
        sb_append_sv(&sb, sv);
        sb_push_null(&sb);
        filepath = sb.items;
    }

    StringView sv = read_entire_file(arena, filepath);
    if (sv.count < 0) return false;

    while (sv.count) {
        StringView line = sv_cut_delimiter(&sv, '\n');
        // NOTE: we are mangling the underlying data here, but `sv` is discarded
        // after this function so it doesn't matter. This is instead of copying
        // over the entirety of the line with a NUL-character at the end, which
        // would mean allocating twice as much data as necessary.
        char *newline = (char *)(&line.data[line.count]);
        *newline = '\0';
        command_push(command, line.data);
    }

    return true;
}

i8 build_needs_rebuild(const char *output_path, FilePaths input_paths) {
#ifdef _WIN32
    BOOL bSuccess;

    HANDLE output_path_fd =
        CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        log_error("Failed to open '%s': %s", output_path,
                  system__win32_error_message(GetLastError()));
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        log_error("Failed to get time of '%s': %s", output_path,
                  system__win32_error_message(GetLastError()));
        return -1;
    }

    for (size_t i = 0; i < input_paths.count; ++i) {
        const char *input_path = input_paths.items[i];
        HANDLE input_path_fd =
            CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for
            // building in the first place
            log_error("Failed to open file '%s': %s", input_path,
                      system__win32_error_message(GetLastError()));
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            log_error("Failed to get time of '%s': %s", input_path,
                      system__win32_error_message(GetLastError()));
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's
        // 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) {
            errno = 0;
            return 1;
        }
        log_error("Failed to stat '%s': %s", output_path, strerror(errno));
        return -1;
    }
    time_t output_path_time = statbuf.st_mtime;
    for (i32 i = 0; i < input_paths.count; ++i) {
        FilePath input_path = input_paths.items[i];
        if (stat(input_path, &statbuf) < 0) {
            log_info("HERE");
            // NOTE: non-existing input is an error cause it is needed for
            // building in the first place
            log_error("Failed to stat '%s': %s", input_path, strerror(errno));
            return -1;
        }
        time_t input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's
        // 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }

    return 0;

#endif // _WIN32
}

// This implementation idea is stolen from https://github.com/tsoding/nob.h,
// where it was stolen from https://github.com/zhiayang/nabs
void build__self_rebuild(Arena *arena, int argc, const char **argv,
                         const char *self_path, FilePaths dependencies) {
    Lifetime lt = lifetime_begin(arena);

    Args args = args_from_main(lt.arena, argc, argv);
    StringView binary_path = args_shift(&args);

#ifdef _WIN32
    if (!sv_ends_with(binary_path, sv_from_cstr(".exe"))) {
        binary_path = sv_printf(arena, SV_FMT ".exe", SV_ARG(binary_path));
    }
#endif // _WIN32

    FilePaths paths = file_paths_new(lt.arena, dependencies.count + 1);
    file_paths_push(&paths, self_path);
    file_paths_append_other(&paths, dependencies);

    i8 needs_rebuild =
        build_needs_rebuild(sv_to_cstr(lt.arena, binary_path), paths);
    if (needs_rebuild < 0) exit(1);
    if (!needs_rebuild) {
        lifetime_end(lt);
        return;
    }

    const char *output = sv_to_cstr(lt.arena, binary_path);

    Command command = command_new(lt.arena, MAX(argc, 31) + 1);

    COMMAND_CC(&command);
    COMMAND_CC_DEBUG_INFO(&command);
    COMMAND_CC_OUTPUT(arena, &command, output);
    COMMAND_CC_INPUTS(&command, self_path);
    if (!COMMAND_RUN(arena, &command)) {
        exit(1);
    }

    COMMAND_APPEND(&command, output);
    command_append(&command, argv + 1, argc - 1);
    if (!COMMAND_RUN(arena, &command)) {
        exit(1);
    }

    exit(0);
}

#endif // BUILD_H_
