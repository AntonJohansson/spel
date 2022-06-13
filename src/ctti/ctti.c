#include <shared/types.h>

/* libc */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#include <third_party/sds.h>

#define CBEGIN "\033["
#define CEND   "m"

#define FG_BLACK      "30"
#define BG_BLACK      "40"
#define FG_RED        "31"
#define BG_RED        "41"
#define FG_GREEN      "32"
#define BG_GREEN      "42"
#define FG_YELLOW     "33"
#define BG_YELLOW     "43"
#define FG_BLUE       "34"
#define BG_BLUE       "44"
#define FG_MAGENTA    "35"
#define BG_MAGENTA    "45"
#define FG_CYAN       "36"
#define BG_CYAN       "46"
#define FG_WHITE      "37"
#define BG_WHITE      "47"
#define RESET         "0"
#define BOLD_ON       "1"
#define UNDERLINE_ON  "4"
#define INVERSE_ON    "7"
#define BOLD_OFF      "21"
#define UNDERLINE_OFF "24"
#define INVERSE_OFF   "27"

static void error(const char *fmt, ...) {
    fprintf(stderr, CBEGIN FG_RED CEND);
    fprintf(stderr, "Error: ");
    fprintf(stderr, CBEGIN RESET CEND);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static void die(const char *fmt, ...) {
    fprintf(stderr, CBEGIN FG_RED ";" BOLD_ON CEND);
    fprintf(stderr, "Fatal: ");
    fprintf(stderr, CBEGIN RESET CEND);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static const u8 *xstrndup(const u8 *src, u8 len) {
    const u8 *ret = strndup(src, len);
    if (!ret) {
        die("strndup failed - %s\n", strerror(errno));
    }
    return ret;
};

#define TYPE_NAME_LEN 64
#define SEEN_TYPES_LEN 256

struct type {
    char name[TYPE_NAME_LEN];
};

struct ctti_output {
    sds mem_type_str;
    sds mem_def_str;

    u8 seen_types_top;
    struct type seen_types[SEEN_TYPES_LEN];

    u8 seen_structs_top;
    struct type seen_structs[SEEN_TYPES_LEN];
};

#include "lexer.c"
#include "parser.c"

int main(int argc, char **argv) {
    if (argc == 1) {
        die("Usage: ctti file.c [...]\n");
    }

    struct ctti_output out = {
        .mem_type_str = sdsempty(),
        .mem_def_str = sdsempty(),
    };

    out.mem_def_str = sdscat(out.mem_def_str,
            "struct ctti_mem_def {\n"
            "   enum ctti_mem_type type;\n"
            "   const u8 *name;\n"
            "   u32 offset;\n"
            "};\n\n"
            );

    for (int i = 1; i < argc; ++i) {
        const u8 *filepath = argv[1];

        FILE *fd = fopen(filepath, "r");
        if (!fd) {
            die("(fopen) %s\n", strerror(errno));
        }

        if(fseek(fd, 0, SEEK_END) == -1) {
            die("(fseek) %s\n", strerror(errno));
        }

        i64 size = ftell(fd);
        if (size == -1) {
            die("(ftell) %s\n", strerror(errno));
        }

        if (fseek(fd, 0, SEEK_SET) == -1) {
            die("(fseek) %s\n", strerror(errno));
        }

        u8 buf[size+1];
        buf[size] = 0;

        if (fread(buf, 1, size, fd) < size) {
            die("(fread) failed to read entire file!\n");
        }

        parse(filepath, buf, size, &out);

        fclose(fd);
    }

    { /* output seen types */
        out.mem_type_str = sdscat(out.mem_type_str,
                "enum ctti_mem_type {\n"
                );

        for (u8 i = 0; i < out.seen_types_top; ++i) {
            out.mem_type_str = sdscatprintf(out.mem_type_str,
                    "    TYPE_%s,\n", out.seen_types[i].name);
        }
        out.mem_type_str = sdscat(out.mem_type_str, "};\n");
    }

    { /* output macro to generate all switch cases */
        out.mem_type_str = sdscat(out.mem_type_str,
                "\n#define CTTI_GEN_MEM_DEF_SWITCH(func, data)"
                );

        for (u8 i = 0; i < out.seen_structs_top; ++i) {
            out.mem_type_str = sdscat(out.mem_type_str, "\\\n");
            out.mem_type_str = sdscatprintf(out.mem_type_str,
                    "    case TYPE_%s: {func(ctti_%s, ARRLEN(ctti_%s), data);} break;",
                    out.seen_types[i].name, out.seen_types[i].name, out.seen_types[i].name);
        }


        out.mem_type_str = sdscat(out.mem_type_str, "\n");
    }

    puts("#pragma once\n"
         "\n"
         "#include <stddef.h>\n");
    puts(out.mem_type_str);
    puts(out.mem_def_str);

    sdsfree(out.mem_type_str);
    sdsfree(out.mem_def_str);

    return 0;
}
