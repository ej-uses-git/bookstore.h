#define BOOKSTORE_IMPLEMENTATION

#include "../bookstore/system.h"
#include "../bookstore/flag.h"

global FlagContext *command_ctx;
global bool help;
global bool debug;

void set_help_and_debug(FlagContext *ctx) {
    FLAG_BOOL("-help", .alias = "h",
              .description = "Print this help information and exit.",
              .ctx = ctx, .var = &help);
    FLAG_BOOL("-debug", .alias = "d", .description = "Print debug information.",
              .ctx = ctx, .var = &debug);
}

bool help_and_debug(void (*usage_cb)(FILE *stream)) {
    if (debug) {
        min_log_level = LOG_DEBUG;
    }

    if (help) {
        usage_cb(stdout);
        return true;
    }

    return false;
}

void copy_file_usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " " SV_FMT " <options>\n"
            "\n"
            "Copy a file.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()),
            SV_ARG(FLAG_GET_PROGRAM_NAME(.ctx = command_ctx)));
    FLAG_PRINT_OPTIONS(stream, .ctx = command_ctx);
}

bool copy_file_command(Arena *arena, Args args) {
    command_ctx = FLAG_NEW_CONTEXT(arena, .flag_capacity = 4);

    StringView *src = FLAG_STRING(
        "-source", .alias = "i",
        .description = "The source file.\n" PAD_DESCRIPTION "Required.",
        .ctx = command_ctx);
    StringView *dest = FLAG_STRING(
        "-destination", .alias = "o",
        .description = "The destination file.\n" PAD_DESCRIPTION "Required.",
        .ctx = command_ctx);
    set_help_and_debug(command_ctx);

    if (!FLAG_PARSE(arena, args, .ctx = command_ctx)) {
        copy_file_usage(stderr);
        FLAG_PRINT_ERROR(stderr, .ctx = command_ctx);
        return false;
    }

    if (help_and_debug(copy_file_usage)) {
        return true;
    }

    if (!src->count) {
        copy_file_usage(stderr);
        fprintf(stderr, "ERROR: Missing required option '-" SV_FMT "'\n",
                SV_ARG(FLAG_NAME(src, .ctx = command_ctx)));
        return false;
    }

    if (!dest->count) {
        copy_file_usage(stderr);
        fprintf(stderr, "ERROR: Missing required option '-" SV_FMT "'\n",
                SV_ARG(FLAG_NAME(dest, .ctx = command_ctx)));
        return false;
    }

    return copy_file(arena, sv_to_cstr(arena, *src), sv_to_cstr(arena, *dest));
}

void delete_file_usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " " SV_FMT " [options] <file>\n"
            "\n"
            "Delete a file.\n"
            "\n"

            "Arguments:\n"
            PAD_NAME"file\n"
            PAD_DESCRIPTION"The file to delete.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()),
            SV_ARG(FLAG_GET_PROGRAM_NAME(.ctx = command_ctx)));
    FLAG_PRINT_OPTIONS(stream, .ctx = command_ctx);
}

bool delete_file_command(Arena *arena, Args args) {
    command_ctx = FLAG_NEW_CONTEXT(arena, .flag_capacity = 2);

    set_help_and_debug(command_ctx);

    if (!FLAG_PARSE(arena, args, .ctx = command_ctx)) {
        delete_file_usage(stderr);
        FLAG_PRINT_ERROR(stderr, .ctx = command_ctx);
        return false;
    }

    if (help_and_debug(delete_file_usage)) {
        return true;
    }

    args = FLAG_REST_ARGS(.ctx = command_ctx);
    if (!args.count) {
        delete_file_usage(stderr);
        fprintf(stderr, "ERROR: Missing required argument <file>\n");
        return false;
    }

    return delete_file(sv_to_cstr(arena, args_get(args, 0)));
}

void list_directory_usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " " SV_FMT " [options]\n"
            "\n"
            "List contents of a directory.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()),
            SV_ARG(FLAG_GET_PROGRAM_NAME(.ctx = command_ctx)));
    FLAG_PRINT_OPTIONS(stream, .ctx = command_ctx);
}

bool list_directory_visit(WalkEntry entry) {
    if (entry.level == 1) {
        if (!entry.first) printf(" ");
        printf("%s", entry.path);
    } else if (entry.level > 1) {
        *entry.action = WALK_SKIP;
    }
    return true;
}

bool list_directory_command(Arena *arena, Args args) {
    command_ctx = FLAG_NEW_CONTEXT(arena);

    StringView *directory = FLAG_STRING(
        "-directory", ._default = sv_from_cstr("."), .ctx = command_ctx,
        .description = "The directory to list the contents of.");
    set_help_and_debug(command_ctx);

    if (!FLAG_PARSE(arena, args, .ctx = command_ctx)) {
        list_directory_usage(stderr);
        FLAG_PRINT_ERROR(stderr, .ctx = command_ctx);
        return false;
    }

    if (help_and_debug(list_directory_usage)) {
        return true;
    }

    Lifetime lt = lifetime_begin(arena);
    bool result = WALK_DIRECTORY(lt.arena, sv_to_cstr(arena, *directory),
                                 list_directory_visit);
    lifetime_end(lt);
    printf("\n");
    return result;
}

void copy_directory_recursively_usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " " SV_FMT " <options>\n"
            "\n"
            "Copy a directory recursively.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()),
            SV_ARG(FLAG_GET_PROGRAM_NAME(.ctx = command_ctx)));
    FLAG_PRINT_OPTIONS(stream, .ctx = command_ctx);
}

bool copy_directory_recursively_command(Arena *arena, Args args) {
    command_ctx = FLAG_NEW_CONTEXT(arena, .flag_capacity = 4);

    StringView *src = FLAG_STRING(
        "-source", .alias = "i",
        .description = "The source directory.\n" PAD_DESCRIPTION "Required.",
        .ctx = command_ctx);
    StringView *dest = FLAG_STRING(
        "-destination", .alias = "o",
        .description =
            "The destination directory.\n" PAD_DESCRIPTION "Required.",
        .ctx = command_ctx);
    set_help_and_debug(command_ctx);

    if (!FLAG_PARSE(arena, args, .ctx = command_ctx)) {
        copy_directory_recursively_usage(stderr);
        FLAG_PRINT_ERROR(stderr, .ctx = command_ctx);
        return false;
    }

    if (help_and_debug(copy_directory_recursively_usage)) {
        return true;
    }

    if (!src->count) {
        copy_directory_recursively_usage(stderr);
        fprintf(stderr, "ERROR: Missing required option '-" SV_FMT "'\n",
                SV_ARG(FLAG_NAME(src, .ctx = command_ctx)));
        return false;
    }

    if (!dest->count) {
        copy_directory_recursively_usage(stderr);
        fprintf(stderr, "ERROR: Missing required option '-" SV_FMT "'\n",
                SV_ARG(FLAG_NAME(dest, .ctx = command_ctx)));
        return false;
    }

    return copy_directory_recursively(arena, sv_to_cstr(arena, *src),
                                      sv_to_cstr(arena, *dest));
}

void delete_directory_recursively_usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " " SV_FMT " [options] <directory>\n"
            "\n"
            "Delete a directory recursively.\n"
            "\n"

            "Arguments:\n"
            PAD_NAME"directory\n"
            PAD_DESCRIPTION"The directory to delete.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()),
            SV_ARG(FLAG_GET_PROGRAM_NAME(.ctx = command_ctx)));
    FLAG_PRINT_OPTIONS(stream, .ctx = command_ctx);
}

bool delete_directory_recursively_command(Arena *arena, Args args) {
    command_ctx = FLAG_NEW_CONTEXT(arena, .flag_capacity = 2);

    set_help_and_debug(command_ctx);

    if (!FLAG_PARSE(arena, args, .ctx = command_ctx)) {
        delete_directory_recursively_usage(stderr);
        FLAG_PRINT_ERROR(stderr, .ctx = command_ctx);
        return false;
    }

    if (help_and_debug(delete_directory_recursively_usage)) {
        return true;
    }

    args = FLAG_REST_ARGS(.ctx = command_ctx);
    if (!args.count) {
        delete_directory_recursively_usage(stderr);
        fprintf(stderr, "ERROR: Missing required argument <directory>\n");
        return false;
    }

    return delete_directory_recursively(arena,
                                        sv_to_cstr(arena, args_get(args, 0)));
}

void usage(FILE *stream) {
    // clang-format off
    fprintf(stream,
            "Usage: " SV_FMT " [options] <command> [..]\n"
            "\n"

            "Commands:\n"
            PAD_NAME"copy_file\n"
            PAD_DESCRIPTION"Copy a file.\n"
            "\n"
            PAD_NAME"delete_file\n"
            PAD_DESCRIPTION"Delete a file.\n"
            "\n"
            PAD_NAME"list_directory\n"
            PAD_DESCRIPTION"List contents of a directory.\n"
	    "\n"
	    PAD_NAME"copy_directory_recursively\n"
            PAD_DESCRIPTION"Copy a directory recursively.\n"
            "\n"

            // clang-format on
            "Options:\n",
            SV_ARG(FLAG_GET_PROGRAM_NAME()));
    FLAG_PRINT_OPTIONS(stream);
}

int main(int argc, const char **argv) {
    Arena *arena = arena_new(MiB(1));
    FLAG_INIT(arena, .flag_capacity = 2);

    set_help_and_debug(NULL);

    if (!FLAG_PARSE_MAIN(arena, argc, argv)) {
        usage(stderr);
        FLAG_PRINT_ERROR(stderr);
        return 1;
    }

    if (help_and_debug(usage)) {
        return 0;
    }

    Args args = FLAG_REST_ARGS();

    if (!args.count) {
        usage(stderr);
        fprintf(stderr, "ERROR: Missing command\n");
        return 1;
    }

    StringView command = args_get(args, 0);
    if (sv_eq_cstr(command, "copy_file")) {
        if (!copy_file_command(arena, args)) return 1;
    } else if (sv_eq_cstr(command, "delete_file")) {
        if (!delete_file_command(arena, args)) return 1;
    } else if (sv_eq_cstr(command, "list_directory")) {
        if (!list_directory_command(arena, args)) return 1;
    } else if (sv_eq_cstr(command, "copy_directory_recursively")) {
        if (!copy_directory_recursively_command(arena, args)) return 1;
    } else if (sv_eq_cstr(command, "delete_directory_recursively")) {
        if (!delete_directory_recursively_command(arena, args)) return 1;
    } else {
        usage(stderr);
        fprintf(stderr, "ERROR: unknown command '" SV_FMT "'", SV_ARG(command));
        return 1;
    }

    return 0;
}
