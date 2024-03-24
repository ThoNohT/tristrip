#define NOH_IMPLEMENTATION
#include "src/noh.h"
#define NOH_BLD_IMPLEMENTATION
#include "noh_bld.h"

#define DEBUG_TOOL "gf2"
#define COMPILER_TOOL "clang"

bool build_tristrip() {
    bool result = true;
    Noh_Arena arena = noh_arena_init(10 KB);

    Noh_Cmd cmd = {0};
    Noh_File_Paths input_paths = {0};
    noh_da_append(&input_paths, "./src/noh.h");
    noh_da_append(&input_paths, "./src/main.c");
    noh_da_append(&input_paths, "./lib/libraylib.a");

    int needs_rebuild = noh_output_is_older("./build/tristrip", input_paths.elems, input_paths.count);
    if (needs_rebuild < 0) noh_return_defer(false);
    if (needs_rebuild == 0) {
        noh_log(NOH_INFO, "tristrip is up to date.");
        noh_return_defer(true);
    }

    noh_cmd_append(&cmd, COMPILER_TOOL);

    // c-flags
    noh_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    noh_cmd_append(&cmd,  "-I./include/raylib-5.0");

    // Output
    noh_cmd_append(&cmd, "-o", "./build/tristrip");

    // Source
    noh_cmd_append(&cmd, "./src/main.c");

    // Linker
    noh_cmd_append(&cmd, "-L./lib", "-l:libraylib.a");
    noh_cmd_append(&cmd, "-lm");
    // noh_cmd_append(&cmd, "-static");

    if (!noh_cmd_run_sync(cmd)) noh_return_defer(false);

defer:
    noh_cmd_free(&cmd);
    noh_da_free(&input_paths);
    noh_arena_free(&arena);
    return result;
}


void print_usage(char *program) {
    noh_log(NOH_INFO, "Usage: %s <command>", program);
    noh_log(NOH_INFO, "Available commands:");
    noh_log(NOH_INFO, "- build: build tristrip (default).");
    noh_log(NOH_INFO, "- run: build and run tristrip.");
    noh_log(NOH_INFO, "- test: build and debug tristrip using the defined debug tool.");
    noh_log(NOH_INFO, "- clean: clean all build artifacts.");
}

int main(int argc, char **argv) {
    noh_rebuild_if_needed(argc, argv);
    char *program = noh_shift_args(&argc, &argv);

    // Determine command.
    char *command = NULL;
    if (argc == 0) {
        command = "build";
    } else {
        command = noh_shift_args(&argc, &argv);
    }

    // Ensure build directory exists.
    if (!noh_mkdir_if_needed("./build")) return 1;

    if (strcmp(command, "build") == 0) {
        // Only build.
        if (!build_tristrip()) return 1;

    } else if (strcmp(command, "run") == 0) {
        // Build and run.
        if (!build_tristrip()) return 1;

        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, "./build/tristrip");
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else if (strcmp(command, "test") == 0) {
        // Build and debug.
        if (!build_tristrip()) return 1;

        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, DEBUG_TOOL, "./build/tristrip");
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else if (strcmp(command, "clean") == 0) {
        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, "rm", "-rf", "./build/");
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else {
        print_usage(program);
        noh_log(NOH_ERROR, "Invalid command: '%s'", command);
        return 1;

    }
} 
