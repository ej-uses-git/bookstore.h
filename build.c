#define BOOKSTORE_IMPLEMENTATION

#include "./bookstore/build.h"
#include "./bookstore/arena.h"
#include "./bookstore/basic.h"
#include "./bookstore/command.h"
#include "./bookstore/flag.h"
#include "./bookstore/string.h"
#include "./bookstore/system.h"

#define BIN_DIR         "bin"
#define BOOKSTORE_DIR   "bookstore"
#define TEST_OUTPUT_DIR BIN_DIR SYSTEM_PATH_DELIMITER_STRING "test"
#define TEST_INPUT_DIR  "test"

bool build_tests(Arena *arena, FilePaths tests, FilePaths dependencies);
bool run_tests(Arena *arena, FilePaths tests);

void usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " [options] <commands..>\n"
            "\n"

            "Commands:\n"
            PAD_NAME"clean\n"
            PAD_DESCRIPTION"Clean up the build output directory.\n"
            "\n"
            PAD_NAME"build\n"
            PAD_DESCRIPTION"Build all outputs.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()));
    FLAG_PRINT_OPTIONS(stream);
}

int main(int argc, const char **argv) {
    Arena *arena = arena_new(MiB(1));

    FLAG_INIT(arena);

    bool *help =
        FLAG_BOOL("-help", .alias = "h",
                  .description = "Print this help information and exit.");
    bool *debug = FLAG_BOOL("-debug", .alias = "d",
                            .description = "Print debug information.");

    if (!FLAG_PARSE_MAIN(arena, argc, argv, .parse_all = true)) {
        usage(stderr);
        FLAG_PRINT_ERROR(stderr);
        return 1;
    }

    if (*debug) {
        min_log_level = LOG_DEBUG;
    }

    FilePaths dependencies = file_paths_new(arena, 256);
    list_directory(arena, BOOKSTORE_DIR, &dependencies);

    Lifetime lt = lifetime_begin(arena);
    SELF_REBUILD_DEPENDENCIES(lt.arena, argc, argv, dependencies);
    lifetime_end(lt);

    if (*help) {
        usage(stdout);
        return 0;
    }

    Args args = FLAG_REST_ARGS();

    if (!args.count) {
        usage(stderr);
        fprintf(stderr, "ERROR: missing commands\n");
        return 1;
    }

    if (args_index_of(args, sv_from_cstr("clean")) >= 0) {
        if (!delete_directory_recursively(arena, BIN_DIR)) return 1;
    }

    FilePaths tests = file_paths_new(arena, 64);
    list_directory(arena, TEST_INPUT_DIR, &tests);

    if (args_index_of(args, sv_from_cstr("build")) >= 0) {
        if (!build_tests(arena, tests, dependencies)) return 1;
    }
    if (args_index_of(args, sv_from_cstr("test")) >= 0) {
        if (!run_tests(arena, tests)) return 1;
    }

    return 0;
}

bool build_test(Arena *arena, const char *path, FilePaths dependencies,
                ProcessList *procs, i32 concurrency) {
    DEFER_SETUP(bool, true);

    Lifetime lt = lifetime_begin(arena);

    FilePaths all_dependencies =
        file_paths_new(lt.arena, dependencies.count + 1);
    file_paths_push(&all_dependencies, path);
    file_paths_append_other(&all_dependencies, dependencies);

    StringView output_dir = sv_from_cstr(TEST_OUTPUT_DIR);

    StringView basename = get_basename(sv_from_cstr(path));
    sv_strip_suffix(&basename, sv_from_cstr(".c"));

    StringBuilder output =
        sb_new(lt.arena, basename.count + output_dir.count + 2);
    sb_appendf(&output, SV_FMT SYSTEM_PATH_DELIMITER_STRING SV_FMT,
               SV_ARG(output_dir), SV_ARG(basename));
    sb_push_null(&output);

    i8 needs_rebuild = build_needs_rebuild(output.items, all_dependencies);
    if (needs_rebuild < 0) DEFER_RETURN(false);
    if (!needs_rebuild) {
        log_debug("Nothing to do for '%s'", output.items);
        DEFER_RETURN(true);
    }

    Command command = command_new(lt.arena, 32);

#ifdef _WIN32
    COMMAND_CC(&command);

    COMMAND_CC_FLAGS(&command);

    COMMAND_APPEND(&command, "/EP");

    COMMAND_CC_INPUTS(&command, path);

    StringBuilder processed =
        sb_new(lt.arena, basename.count + output_dir.count + 4);
    sb_appendf(&output, SV_FMT SYSTEM_PATH_DELIMITER_STRING SV_FMT ".c",
               SV_ARG(output_dir), SV_ARG(basename));
    sb_push_null(&output);

    if (!COMMAND_RUN(arena, &command, .stdout_path = processed.items))
        DEFER_RETURN(false);

    path = processed.items;

    StringView sv = read_entire_file(lt.arena, processed.items);
    i32 i = 0;
    while (sv.count) {
        i++;
        StringView line = sv_cut_delimiter(&sv, '\n');
        printf("%d " SV_FMT "\n", i, SV_ARG(line));
    }
#endif // _WIN32

    COMMAND_CC(&command);

    COMMAND_CC_FLAGS(&command);

    // TODO: do this only in debug mode?
    COMMAND_CC_DEBUG_INFO(&command);
    COMMAND_CC_ADDRESS_SANITIZE(&command);

    COMMAND_CC_DEFINE(lt.arena, &command, "BOOKSTORE_IMPLEMENTATION");
    COMMAND_CC_OUTPUT(lt.arena, &command, output.items);
    COMMAND_CC_INPUTS(&command, path);

    if (!COMMAND_RUN(lt.arena, &command, .async = procs,
                     .concurrency = concurrency))
        DEFER_RETURN(false);

    DEFER_LABEL({ lifetime_end(lt); });
}

bool build_tests(Arena *arena, FilePaths tests, FilePaths dependencies) {
    if (!make_directory_recursively(arena, TEST_OUTPUT_DIR)) return false;

    i32 concurrency = 64;
    ProcessList procs = process_list_new(arena, concurrency);

    for (i32 i = 0; i < tests.count; i++) {
        if (!build_test(arena, file_paths_get(tests, i), dependencies, &procs,
                        concurrency))
            return false;
    }

    return process_list_wait(procs);
}

bool run_test(Arena *arena, const char *path) {
    DEFER_SETUP(bool, true);

    Lifetime lt = lifetime_begin(arena);

    StringView output_dir = sv_from_cstr(TEST_OUTPUT_DIR);

    StringView basename = get_basename(sv_from_cstr(path));
    sv_strip_suffix(&basename, sv_from_cstr(".c"));

#ifdef _WIN32
    i32 executable_length = basename.count + output_dir.count + 6;
#else
    i32 executable_length = basename.count + output_dir.count + 2;
#endif // _WIN32

    StringBuilder executable = sb_new(lt.arena, executable_length);
    sb_appendf(&executable, SV_FMT SYSTEM_PATH_DELIMITER_STRING SV_FMT,
               SV_ARG(output_dir), SV_ARG(basename));
#ifdef _WIN32
    sb_append_cstr(&executable, ".exe");
#endif // _WIN32
    sb_push_null(&executable);

    Command command = command_new(lt.arena, 1);
    command_push(&command, executable.items);

    if (!COMMAND_RUN(lt.arena, &command)) DEFER_RETURN(false);

    DEFER_LABEL({ lifetime_end(lt); });
}

bool run_tests(Arena *arena, FilePaths tests) {
    for (i32 i = 0; i < tests.count; i++) {
        if (!run_test(arena, file_paths_get(tests, i))) return false;
    }
    return true;
}
