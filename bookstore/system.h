/* system.h */
/* System utilities on Windows and POSIX */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "./arena.h"
#include "./basic.h"
#include "./string.h"
#include "array.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <direct.h>
#include <io.h>
#include <shellapi.h>
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif // _WIN32

// This is the typical maximum path length (without NUL character) for Linux,
// though it may be longer and could possibly vary file-to-file; for Windows,
// it's typically 260 bytes but this could be modified by Registry settings.
//
// I think it's acceptable not to support file paths longer than this; if you
// need longer paths for some reason, feel free to hack around with this, and
// possibly submit a pull request!
#define SYSTEM_PATH_MAX 4096

typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_OTHER,
} FileType;

typedef enum {
    WALK_CONT,
    WALK_SKIP,
    WALK_STOP,
} WalkAction;

typedef struct {
    Arena *arena;
    void *user_data;
    const char *path;
    WalkAction *action;
    u32 level;
    FileType type;
    bool first;
} WalkEntry;

typedef struct {
    void *user_data;
    bool depth_first;
} WalkDirectoryOpt;

typedef const char *FilePath;
ARRAY_TYPEDEF(FilePath, FilePaths);
ARRAY_DECLARE_PREFIX(FilePath, FilePaths, file_paths);
#define FILE_PATHS_EMPTY (FilePaths){0}
#define FILE_PATHS_APPEND(paths, ...)                                          \
    file_paths_append(paths, (FilePath[]){__VA_ARGS__},                        \
                      sizeof((FilePath[]){__VA_ARGS__}) / sizeof(FilePath))

typedef bool (*WalkVisitCallback)(WalkEntry entry);

StringView dirname_and_basename(StringView *basename);
StringView get_basename(StringView path);
StringView get_dirname(StringView path);
const char *get_current_directory(Arena *arena);
FileType get_file_type(const char *path);
bool delete_file(const char *path);
bool write_file(const char *path, StringView contents);
i64 get_file_position(FILE *f);
StringView read_entire_file(Arena *arena, const char *path);
bool copy_file(Arena *arena, const char *src, const char *dest);
bool rename_file(const char *path, const char *new_path);
bool make_directory(const char *path, bool fail_if_exists);
bool make_directory_recursively(Arena *arena, const char *path);
bool walk_directory_opt(Arena *arena, const char *root, WalkVisitCallback visit,
                        WalkDirectoryOpt opt);
#define WALK_DIRECTORY(arena, root, cb, ...)                                   \
    walk_directory_opt(arena, root, cb, (WalkDirectoryOpt){__VA_ARGS__})
bool list_directory(Arena *arena, const char *path, FilePaths *out);
bool copy_directory_recursively(Arena *arena, const char *src,
                                const char *dest);
bool delete_directory_recursively(Arena *arena, const char *path);

#ifdef BOOKSTORE_IMPLEMENTATION

ARRAY_DEFINE_PREFIX(FilePath, FilePaths, file_paths)

#ifdef _WIN32

// Base on https://stackoverflow.com/a/75644008
// > .NET Core uses `4096 * sizeof(WCHAR)` buffer on stack for `FormatMessageW`
// call. And... that's it.
// >
// >
// https://github.com/dotnet/runtime/blob/3b63eb1346f1ddbc921374a5108d025662fb5ffd/src/coreclr/utilcode/posterror.cpp#L264-L265
#ifndef WIN32_ERR_MSG_SIZE
#define WIN32_ERR_MSG_SIZE (4 * 1024)
#endif // WIN32_ERR_MSG_SIZE

char *system__win32_error_message(DWORD) {
    persist char win_32_err_msg[WIN32_ERR_MSG_SIZE] = {0};
    DWORD err_msg_size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
        LANG_USER_DEFAULT, win_32_err_msg, WIN32_ERR_MSG_SIZE, NULL);

    if (err_msg_size == 0) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win_32_err_msg, "Could not get error message for 0x%lX",
                        err) > 0) {
                return (char *)&win_32_err_msg;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win_32_err_msg, "Invalid Windows Error code (0x%lX)",
                        err) > 0) {
                return (char *)&win_32_err_msg;
            } else {
                return NULL;
            }
        }
    }

    while (err_msg_size > 1 && isspace(win_32_err_msg[err_msg_size - 1])) {
        win_32_err_msg[--err_msg_size] = '\0';
    }

    return win_32_err_msg;
}
#endif // _WIN32

#ifdef _WIN32
#define SYSTEM_PATH_DELIMITER '\\'
#else
#define SYSTEM_PATH_DELIMITER '/'
#endif // _WIN32

StringView dirname_and_basename(StringView *basename) {
#ifdef _WIN32
    return sv_cut_delimiter_end(basename, '\\');
#else
    return sv_cut_delimiter_end(basename, '/');
#endif // _WIN32
}

StringView get_basename(StringView path) {
    dirname_and_basename(&path);
    return path;
}

StringView get_dirname(StringView path) {
    return dirname_and_basename(&path);
}

const char *get_current_directory(Arena *arena) {
#ifdef _WIN32
    DWORD nBufferLength = GetCurrentDirectory(0, NULL);
    if (nBufferLength == 0) {
        log_error("Failed to get current directory: %s",
                  system__win32_error_message(GetLastError()));
        return NULL;
    }

    char *buffer = (char *)arena_alloc(nBufferLength);
    if (GetCurrentDirectory(nBufferLength, buffer) == 0) {
        log_error("Failed to get current directory: %s",
                  system__win32_error_message(GetLastError()));
        return NULL;
    }

    return buffer;
#else
    char *buffer = (char *)arena_alloc(arena, SYSTEM_PATH_MAX);
    if (getcwd(buffer, SYSTEM_PATH_MAX) == NULL) {
        log_error("Failed to get current directory: %s", strerror(errno));
        return NULL;
    }

    return buffer;
#endif // _WIN32
}

FileType get_file_type(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        log_error("Failed to get attributes of '%s': %s", path,
                  system__win32_error_message(GetLastError()));
        return FILE_TYPE_INVALID;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) return FILE_TYPE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return FILE_TYPE_REGULAR;
#else
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        log_error("Failed to stat '%s': %s", path, strerror(errno));
        return -1;
    }

    if (S_ISREG(statbuf.st_mode)) return FILE_TYPE_REGULAR;
    if (S_ISDIR(statbuf.st_mode)) return FILE_TYPE_DIRECTORY;
    if (S_ISLNK(statbuf.st_mode)) return FILE_TYPE_SYMLINK;
    return FILE_TYPE_OTHER;
#endif // _WIN32
}

bool delete_file(const char *path) {
#ifdef _WIN32
    if (!DeleteFileA(path)) {
        log_error("Failed to delete file '%s': %s", path,
                  system__win32_error_message(GetLastError()));
        return false;
    }
#else
    if (remove(path) < 0) {
        log_error("Failed to delete file '%s': %s", path, strerror(errno));
        return false;
    }
#endif // _WIN32
    log_debug("Deleted '%s'", path);
    return true;
}

bool write_file(const char *path, StringView contents) {
    DEFER_SETUP(bool, true);

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        log_error("Failed to open '%s' for writing: %s", path, strerror(errno));
        DEFER_RETURN(false);
    }

    size_t n = fwrite(contents.data, 1, contents.count, f);
    if ((i32)n < contents.count) {
        log_error("Failed to write into '%s': %s", path, strerror(errno));
        DEFER_RETURN(false);
    }

    DEFER_LABEL({
        if (f) fclose(f);
    });
}

i64 get_file_position(FILE *f) {
#ifdef _WIN32
    return _ftelli64(f);
#else
    return ftell(f);
#endif // _WIN32
}

StringView read_entire_file(Arena *arena, const char *path) {
    DEFER_SETUP(StringView, SV_INVALID);

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        log_error("Failed to open '%s' for reading: %s", path, strerror(errno));
        DEFER_RETURN(SV_INVALID);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        log_error("Failed to read '%s': %s", path, strerror(errno));
        DEFER_RETURN(SV_INVALID);
    }

    i64 file_size = get_file_position(f);

    if (file_size < 0) {
        log_error("Failed to read '%s': %s", path, strerror(errno));
        DEFER_RETURN(SV_INVALID);
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        log_error("Failed to read '%s': %s", path, strerror(errno));
        DEFER_RETURN(SV_INVALID);
    }

    StringBuilder sb = sb_new(arena, file_size);
    fread(sb.items, file_size, 1, f);
    if (ferror(f)) {
        log_error("Failed to read '%s': %s", path, strerror(errno));
        DEFER_RETURN(SV_INVALID);
    }
    sb.count = file_size;

    DEFER_RETURN(sb_to_sv(sb));

    DEFER_LABEL({
        if (f) fclose(f);
    });
}

bool copy_file(Arena *arena, const char *src, const char *dest) {
#ifdef _WIN32
    if (!CopyFile(src, dest, false)) {
        log_error("Failed copy '%s' to '%s': %s", src, dest,
                  system__win32_error_message(GetLastError()));
        return false;
    }
    log_debug("Copied '%s' to '%s'", src, dest);
    return true;
#else
    DEFER_SETUP(bool, false);

    i32 src_fd = -1, dest_fd = -1;
    Lifetime lt = lifetime_begin(arena);

    src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        log_error("Failed to open '%s' for reading: %s", src, strerror(errno));
        DEFER_RETURN(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        log_error("Failed to stat '%s': %s", src, strerror(errno));
        DEFER_RETURN(false);
    }

    dest_fd = open(dest, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dest_fd < 0) {
        log_error("Failed to open '%s' for writing: %s", dest, strerror(errno));
        DEFER_RETURN(false);
    }

    size_t buf_size = KiB(32);
    void *buf = arena_alloc(lt.arena, buf_size);
    while (true) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) {
            log_error("Failed to read from '%s': %s", src, strerror(errno));
            DEFER_RETURN(false);
        }
        char *buf_temp = buf;
        while (n > 0) {
            ssize_t m = write(dest_fd, buf_temp, n);
            if (m < 0) {
                log_error("Failed to write to '%s': %s", dest, strerror(errno));
                DEFER_RETURN(false);
            }
            n -= m;
            buf_temp += m;
        }
    }

    log_debug("Copied '%s' to '%s'", src, dest);

    DEFER_LABEL({
        if (src_fd >= 0) close(src_fd);
        if (dest_fd >= 0) close(dest_fd);
        lifetime_end(lt);
    });
#endif // _WIN32
}

bool rename_file(const char *path, const char *new_path) {
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        log_error("Failed to rename '%s' to '%s': %s", path, new_path,
                  system__win32_error_message(GetLastError()));
        return false;
    }
#else
    if (rename(path, new_path) < 0) {
        log_error("Failed to rename '%s' to '%s': %s", path, new_path,
                  strerror(errno));
        return false;
    }
#endif // _WIN32
    log_debug("Renamed '%s' to '%s'", path, new_path);
    return true;
}

bool make_directory(const char *path, bool fail_if_exists) {
#ifdef _WIN32
    int result = _mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif // _WIN32

    if (result < 0) {
        if (errno == EEXIST && !fail_if_exists) {
            log_debug("Directory '%s' already exists", path);
            return true;
        }
        log_error("Failed to create directory '%s': %s", path, strerror(errno));
        return false;
    }

    log_debug("Created directory '%s'", path);
    return true;
}

bool make_directory_recursively(Arena *arena, const char *path) {
    DEFER_SETUP(bool, true);

    Lifetime lt = lifetime_begin(arena);

    StringView sv = sv_from_cstr(path);
    const char *cur = NULL;

    while (sv.count) {
        StringView basename = sv_cut_delimiter(&sv, SYSTEM_PATH_DELIMITER);

        if (cur) {
            cur = arena_sprintf(lt.arena, "%s/" SV_FMT, cur, SV_ARG(basename));
        } else {
            cur = sv_to_cstr(lt.arena, basename);
        }

        if (!make_directory(cur, false)) DEFER_RETURN(false);
    }

    DEFER_LABEL({ lifetime_end(lt); });
}

// NOTE: `path` here has a NUL character at the end of it, so `path->items` can
// be used as a C-string.
bool system__walk_directory_opt_impl(Arena *arena, StringBuilder *path,
                                     WalkVisitCallback visit,
                                     WalkDirectoryOpt opt, u32 level,
                                     bool *stop, bool first_on_level) {
    DEFER_SETUP(bool, true);

#ifdef _WIN32
    TODO("WIN32");
#else
    DIR *dir = NULL;
#endif // _WIN32

    FileType type = get_file_type(path->items);
    if (type < 0) DEFER_RETURN(false);

    WalkAction action = WALK_CONT;
    WalkEntry entry = {
        .arena = arena,
        .user_data = opt.user_data,
        .path = path->items,
        .type = type,
        .level = level,
        .action = &action,
        .first = first_on_level,
    };

    if (!opt.depth_first) {
        if (!visit(entry)) DEFER_RETURN(false);

        switch (action) {
        case WALK_CONT: break;
        case WALK_STOP: *stop = true; DEFER_RETURN(true);
        case WALK_SKIP: DEFER_RETURN(true);
        }
    }

    if (type != FILE_TYPE_DIRECTORY) {
        if (opt.depth_first && !visit(entry)) DEFER_RETURN(false);
        DEFER_RETURN(true);
    }

#ifdef _WIN32
    TODO("WIN32");
#else
    dir = opendir(path->items);
    if (dir == NULL) {
        log_error("Failed to open directory '%s': %s", path->items,
                  strerror(errno));
        DEFER_RETURN(false);
    }
#endif // _WIN32

    // Mark before the NUL character
    i32 mark = path->count - 1;

#ifdef _WIN32
    TODO("WIN32");
#else
    errno = 0;
    struct dirent *ent = readdir(dir);
    bool first = true;
    while (ent != NULL) {
        const char *name = ent->d_name;
        i32 name_length = strlen(name);
        bool is_default = (name_length == 1 && name[0] == '.') ||
            (name_length == 2 && name[0] == '.' && name[1] == '.');
        if (!is_default) {
            // Go back to the end of the directory's path
            path->count = mark;
            if (sb_get(*path, -1) != SYSTEM_PATH_DELIMITER)
                sb_push(path, SYSTEM_PATH_DELIMITER);
            sb_append(path, name, name_length);
            sb_push_null(path);
            if (!system__walk_directory_opt_impl(arena, path, visit, opt,
                                                 level + 1, stop, first)) {
                DEFER_RETURN(false);
            }
            if (*stop) DEFER_RETURN(true);
            first = false;
        }
        ent = readdir(dir);
    }
    path->count = mark;
    sb_push_null(path);

    if (errno != 0) {
        log_error("Failed to read directory '%s': %s", path->items,
                  strerror(errno));
        DEFER_RETURN(false);
    }
#endif // _WIN32

    if (opt.depth_first) {
        if (!visit(entry)) DEFER_RETURN(false);

        if (action == WALK_STOP) *stop = true;
    }

#ifdef _WIN32
    DEFER_LABEL({
        // TODO: clean up on Windows
        TODO("WIN32");
    });
#else
    DEFER_LABEL({
        if (dir) closedir(dir);
    });
#endif // _WIN32
}

bool walk_directory_opt(Arena *arena, const char *root, WalkVisitCallback visit,
                        WalkDirectoryOpt opt) {
    bool stop = false;

    StringBuilder path = sb_new(arena, SYSTEM_PATH_MAX);
    sb_append_cstr(&path, root);
    sb_push_null(&path);

    bool result = system__walk_directory_opt_impl(arena, &path, visit, opt, 0,
                                                  &stop, true);

    return result;
}

bool system__list_directory_visit(WalkEntry entry) {
    if (entry.level == 1) {
        FilePaths *paths = entry.user_data;
        file_paths_push(paths, arena_clone_cstr(entry.arena, entry.path));
    } else if (entry.level > 1) {
        *entry.action = WALK_STOP;
    }
    return true;
}

bool list_directory(Arena *arena, const char *path, FilePaths *out) {
    return WALK_DIRECTORY(arena, path, system__list_directory_visit,
                          .user_data = out);
}

typedef struct {
    const char *src;
    const char *dest;
} System__CopyDirectoryRecursivelyData;

bool system__copy_directory_recursively_visit(WalkEntry entry) {
    DEFER_SETUP(bool, true);

    System__CopyDirectoryRecursivelyData *data = entry.user_data;

    if (entry.level == 0) {
        return make_directory(data->dest, false);
    }

    StringView shared_path = sv_from_cstr(entry.path);
    if (!sv_strip_prefix(&shared_path, sv_from_cstr(data->src))) return false;

    StringView dest = sv_from_cstr(data->dest);
    bool needs_delimiter = sv_get(dest, -1) != SYSTEM_PATH_DELIMITER;

    Lifetime lt = lifetime_begin(entry.arena);
    StringBuilder path =
        sb_new(lt.arena, dest.count + shared_path.count + needs_delimiter + 1);
    sb_append_sv(&path, dest);
    if (needs_delimiter) sb_push(&path, SYSTEM_PATH_DELIMITER);
    sb_append_sv(&path, shared_path);
    sb_push_null(&path);

    switch (entry.type) {
    case FILE_TYPE_DIRECTORY: DEFER_RETURN(make_directory(path.items, false));
    case FILE_TYPE_REGULAR:
        DEFER_RETURN(copy_file(entry.arena, entry.path, path.items));
    case FILE_TYPE_SYMLINK: TODO("FILE_TYPE_SYMLINK"); DEFER_RETURN(false);
    case FILE_TYPE_OTHER:
        log_error("Unsupported file type for '%s'", entry.path);
        DEFER_RETURN(false);
    }

    DEFER_LABEL({ lifetime_end(lt); });
}

bool copy_directory_recursively(Arena *arena, const char *src,
                                const char *dest) {
    Lifetime lt = lifetime_begin(arena);
    System__CopyDirectoryRecursivelyData user_data = {.dest = dest, .src = src};
    bool result =
        WALK_DIRECTORY(lt.arena, src, system__copy_directory_recursively_visit,
                       .user_data = &user_data);
    lifetime_end(lt);
    return result;
}

bool system__delete_directory_recursively_visit(WalkEntry entry) {
    return delete_file(entry.path);
}

bool delete_directory_recursively(Arena *arena, const char *path) {
    Lifetime lt = lifetime_begin(arena);
    bool result =
        WALK_DIRECTORY(arena, path, system__delete_directory_recursively_visit,
                       .depth_first = true);
    lifetime_end(lt);
    return result;
}

#endif // BOOKSTORE_IMPLEMENTATION

#endif // SYSTEM_H_
