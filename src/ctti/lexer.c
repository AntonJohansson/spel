struct location {
    const u8 *at;
    const u8 *file;
    u64 line;
    u8 len;
};

struct lexer {
    struct location loc;
};

enum token_type {
    UNKNOWN = 0,
    COMMA,
    SEMICOLON,
    /* Parens */
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    /* Reserved */
    INTROSPECT,
    STRUCT,
    ENUM,
    UNION,
    /* Literals */
    STRING,
    IDENTIFIER,
    /* EOF, don't move */
    END_OF_FILE,
};

struct token {
    enum token_type type;
    struct location loc;
};

#define TOKEN(t, l) \
    (struct token) { .type = t, .loc = l }

void print_location(struct location *loc, const u8 *fmt, ...) {
    printf(CBEGIN FG_CYAN CEND);
    u64 size = printf("  %s:%u | ", loc->file, loc->line);
    printf(CBEGIN RESET CEND);

    const u8 *begin = loc->at;
    while (*begin && *begin != '\n') {
        begin--;
    }
    if (begin < loc->at) {
        begin++;
    }

    const u8 *end = begin;
    while (*(end) && *(end) != '\n') {
        putchar(*end);
        end++;
    }
    if (end > begin) {
        end--;
    }

    putchar('\n');

    for (u8 i = 0; i < size + (loc->at - begin); ++i) {
        putchar(' ');
    }

    printf(CBEGIN FG_CYAN CEND);

    putchar('^');

    if (loc->len > 3) {
        for (u8 i = 0; i < loc->len-2; ++i) {
            putchar('-');
        }
        putchar('^');
    }

    putchar(' ');
    putchar(' ');

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf(CBEGIN RESET CEND);
}

static inline bool match_str(struct lexer *lex, const u8 *str) {
    u8 len = strlen(str);
    for (u8 i = 0; i < len; ++i) {
        if (lex->loc.at[i] != str[i]) {
            return false;
        }
    }

    return true;
}

static void consume_until(struct lexer *lex, const u8 *str) {
    while (*(lex->loc.at) && !match_str(lex, str)) {
        lex->loc.at++;
    }

    if (!*(lex->loc.at)) {
        die("(consume_until) runaway %s\n", str);
    }
}

static void next_token(struct lexer *lex, struct token *tok) {
    /* Consume whitespace and newlines */
    while (*lex->loc.at && (isspace(*lex->loc.at) || *lex->loc.at == '\n' || *lex->loc.at == '/')) {
        if (*lex->loc.at == '/') {
            if (*(lex->loc.at+1) == '/') {
                consume_until(lex, "\n");
            } else if (*(lex->loc.at+1) == '*') {
                consume_until(lex, "*/");
            }
        }

        if (*lex->loc.at == '\n') {
            lex->loc.line++;
        }

        lex->loc.at++;
    }

    /* 1 char tokens */

    switch (*lex->loc.at) {
        case ',': {
            *tok = TOKEN(COMMA, lex->loc);
            lex->loc.at++;
            return;
        }
        case ';': {
            *tok = TOKEN(SEMICOLON, lex->loc);
            lex->loc.at++;
            return;
        }
        case '(': {
            *tok = TOKEN(LPAREN, lex->loc);
            lex->loc.at++;
            return;
        }
        case ')': {
            *tok = TOKEN(RPAREN, lex->loc);
            lex->loc.at++;
            return;
        }
        case '[': {
            *tok = TOKEN(LBRACKET, lex->loc);
            lex->loc.at++;
            return;
        }
        case ']': {
            *tok = TOKEN(RBRACKET, lex->loc);
            lex->loc.at++;
            return;
        }
        case '{': {
            *tok = TOKEN(LBRACE, lex->loc);
            lex->loc.at++;
            return;
        }
        case '}': {
            *tok = TOKEN(RBRACE, lex->loc);
            lex->loc.at++;
            return;
        }
    }

    /* Reserved */
    if (strncmp(lex->loc.at, "introspect", 10) == 0) {
        *tok = TOKEN(INTROSPECT, lex->loc);
        tok->loc.len = 10;
        lex->loc.at += tok->loc.len;
        return;
    }else if (strncmp(lex->loc.at, "struct", 6) == 0) {
        *tok = TOKEN(STRUCT, lex->loc);
        tok->loc.len = 6;
        lex->loc.at += tok->loc.len;
        return;
    } else if (strncmp(lex->loc.at, "enum", 4) == 0) {
        *tok = TOKEN(ENUM, lex->loc);
        tok->loc.len = 4;
        lex->loc.at += tok->loc.len;
        return;
    } else if (strncmp(lex->loc.at, "union", 5) == 0) {
        *tok = TOKEN(UNION, lex->loc);
        tok->loc.len = 5;
        lex->loc.at += tok->loc.len;
        return;
    }

    /* String */
    if (*lex->loc.at == '\"') {
        lex->loc.at++; /* skip " */
        struct location begin = lex->loc;
        while (*lex->loc.at && *lex->loc.at != '\"') {
            if (*lex->loc.at == '\\') {
                lex->loc.at++;
            }
            lex->loc.at++;
        }
        lex->loc.at++; /* skip " */
        *tok = TOKEN(STRING, begin);
        tok->loc.len = lex->loc.at - begin.at;
        return;
    }

    /* Identifier */
    if (isalpha(*lex->loc.at) || *lex->loc.at == '_') {
        struct location begin = lex->loc;
        while (isalpha(*lex->loc.at) || isdigit(*lex->loc.at) || *lex->loc.at == '_') {
            lex->loc.at++;
        }
        *tok = TOKEN(IDENTIFIER, begin);
        tok->loc.len = lex->loc.at - begin.at;
        return;
    }

    if (!*lex->loc.at) {
        *tok = TOKEN(END_OF_FILE, lex->loc);
        return;
    }

    *tok = TOKEN(UNKNOWN, lex->loc);
    lex->loc.at++;
}
