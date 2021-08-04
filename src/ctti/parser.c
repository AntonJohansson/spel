#define TOKEN_BUF_LEN 16

struct token_buffer {
    struct token toks[TOKEN_BUF_LEN];
    u8 begin;
    u8 end;
    u8 len;
};

void push_token(struct token_buffer *buf, struct token *tok) {
    if (buf->len >= TOKEN_BUF_LEN) {
        die("Ran out of token buffer space!\n");
    }

    buf->toks[buf->end] = *tok;
    buf->end = (buf->end + 1) % TOKEN_BUF_LEN;
    buf->len++;
}

void pop_token(struct token_buffer *buf, struct token *tok) {
    if (buf->len == 0) {
        die("Cannot pop empty buffer!\n");
    }

    if (tok) {
        *tok = buf->toks[buf->begin];
    }

    buf->begin = (buf->begin + 1) % TOKEN_BUF_LEN;
    buf->len--;
}

struct parser {
    struct lexer lex;
    struct token_buffer buf;
    struct ctti_output *out;
    struct token *tok_struct;
};

static void fetch_tokens(struct parser *p, u8 amount) {
    if (amount + p->buf.len > TOKEN_BUF_LEN) {
        die("Requested more tokens than available space in buffer\n");
    }

    struct token tok;
    for (u8 i = 0; i < amount; ++i) {
        next_token(&p->lex, &tok);
        if (tok.type == END_OF_FILE) {
            break;
        }
        push_token(&p->buf, &tok);
    }
}

static void fetch_tokens_if_needed(struct parser *p, u8 amount) {
    if (p->buf.len < amount) {
        fetch_tokens(p, amount - p->buf.len);
    }
}

static void assert_tokens_in_buffer(struct token_buffer *buf, u8 len) {
    if (len > buf->len) {
        die("Not enough tokens in buffer, requested %u, have %u!\n", len, buf->len);
    }
}

static void expect(struct token_buffer *buf, enum token_type tok_type, struct token *tok) {
    assert_tokens_in_buffer(buf, 1);

    struct token tmp;
    pop_token(buf, &tmp);
    if (tmp.type != tok_type) {
        error("Token mismatch!\n");
        print_location(&buf->toks[buf->begin].loc, "Expected: %u, got %u\n", tok_type, tmp.type);
        die("Cannot recover!\n");
    }

    if (tok) {
        *tok = tmp;
    }
}

static bool match_seq(struct token_buffer *buf, enum token_type *tok_types, u8 len) {
    assert_tokens_in_buffer(buf, len);

    for (u8 i = 0; i < len; ++i) {
        if (buf->toks[(buf->begin + i) % TOKEN_BUF_LEN].type != tok_types[i]) {
            return false;
        }
    }

    return true;
}

static bool match_either(struct token_buffer *buf, enum token_type *tok_types, u8 len) {
    assert_tokens_in_buffer(buf, 1);

    for (u8 i = 0; i < len; ++i) {
        if (buf->toks[(buf->begin + i) % TOKEN_BUF_LEN].type == tok_types[i]) {
            return true;
        }
    }

    return false;
}

static bool match(struct token_buffer *buf, enum token_type tok) {
    assert_tokens_in_buffer(buf, 1);

    if (buf->toks[buf->begin].type == tok) {
        return true;
    }

    return false;
}

static void parse_decl(struct parser *p) {
    fetch_tokens_if_needed(p, 1);
    struct token tok_type, tok_id;
    pop_token(&p->buf, &tok_type);

    switch (tok_type.type) {
        case STRUCT:
        case ENUM:
        case UNION: {
            fetch_tokens_if_needed(p, 3);
            expect(&p->buf, IDENTIFIER, &tok_type);
            expect(&p->buf, IDENTIFIER, &tok_id);
            expect(&p->buf, SEMICOLON, NULL);

            u32 len = (tok_type.loc.len < TYPE_NAME_LEN) ? tok_type.loc.len : TYPE_NAME_LEN;
            bool found = false;
            for (u8 i = 0; i < p->out->seen_types_top; ++i) {
                if (strncmp(p->out->seen_types[i].name, tok_type.loc.at, len) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                strncpy(p->out->seen_types[p->out->seen_types_top].name, tok_type.loc.at, len);
                p->out->seen_types[p->out->seen_types_top].name[len] = 0;
                p->out->seen_types_top++;
            }

            break;
        }
        case IDENTIFIER: {
            fetch_tokens_if_needed(p, 2);
            expect(&p->buf, IDENTIFIER, &tok_id);
            expect(&p->buf, SEMICOLON, NULL);

            u32 len = (tok_type.loc.len < TYPE_NAME_LEN) ? tok_type.loc.len : TYPE_NAME_LEN;
            bool found = false;
            for (u8 i = 0; i < p->out->seen_types_top; ++i) {
                if (strncmp(p->out->seen_types[i].name, tok_type.loc.at, len) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                strncpy(p->out->seen_types[p->out->seen_types_top].name, tok_type.loc.at, len);
                p->out->seen_types[p->out->seen_types_top].name[len] = 0;
                p->out->seen_types_top++;
            }

            break;
        }
        default: {
            die("Failed to parse decl\n");
        }
    };


    p->out->mem_def_str = sdscatprintf(p->out->mem_def_str,
            "    {TYPE_%.*s,", (int) tok_type.loc.len, tok_type.loc.at);
    p->out->mem_def_str = sdscatprintf(p->out->mem_def_str,
            " \"%.*s\",", (int) tok_id.loc.len, tok_id.loc.at);
    p->out->mem_def_str = sdscatprintf(p->out->mem_def_str,
            " offsetof(struct %.*s, %.*s)},\n",
            (int) p->tok_struct->loc.len, p->tok_struct->loc.at,
            (int) tok_id.loc.len, tok_id.loc.at);
}

static void parse_struct(struct parser *p) {
    fetch_tokens_if_needed(p, 2);
    struct token tok_id;
    expect(&p->buf, IDENTIFIER, &tok_id);
    expect(&p->buf, LBRACE, NULL);

    p->out->mem_def_str = sdscatprintf(p->out->mem_def_str,
            "static ctti_mem_def ctti_%.*s[] = {\n",
            (int)tok_id.loc.len, tok_id.loc.at);

    { /* Add to seen types */
        u32 len = (tok_id.loc.len < TYPE_NAME_LEN) ? tok_id.loc.len : TYPE_NAME_LEN;
        bool found = false;
        for (u8 i = 0; i < p->out->seen_types_top; ++i) {
            if (strncmp(p->out->seen_types[i].name, tok_id.loc.at, len) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            strncpy(p->out->seen_types[p->out->seen_types_top].name, tok_id.loc.at, len);
            p->out->seen_types[p->out->seen_types_top].name[len] = 0;
            p->out->seen_types_top++;
        }
    }

    { /* Add to seen structs */
        u32 len = (tok_id.loc.len < TYPE_NAME_LEN) ? tok_id.loc.len : TYPE_NAME_LEN;
        bool found = false;
        for (u8 i = 0; i < p->out->seen_structs_top; ++i) {
            if (strncmp(p->out->seen_structs[i].name, tok_id.loc.at, len) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            strncpy(p->out->seen_structs[p->out->seen_structs_top].name, tok_id.loc.at, len);
            p->out->seen_structs[p->out->seen_structs_top].name[len] = 0;
            p->out->seen_structs_top++;
        }
    }

    p->tok_struct = &tok_id;

    fetch_tokens_if_needed(p, 1);
    while (!match(&p->buf, RBRACE)) {
        parse_decl(p);
        fetch_tokens_if_needed(p, 1);
    }
    p->tok_struct = NULL;
    fetch_tokens_if_needed(p, 2);
    expect(&p->buf, RBRACE, NULL);
    expect(&p->buf, SEMICOLON, NULL);
    p->out->mem_def_str = sdscat(p->out->mem_def_str, "};\n");
}

static void parse_enum(struct parser *p) {
}

static void parse_union(struct parser *p) {
}

static void parse_statement(struct parser *p) {
    fetch_tokens_if_needed(p, 1);
    struct token tok;
    pop_token(&p->buf, &tok);

    if (tok.type == STRUCT){
        parse_struct(p);
    } else if (tok.type == ENUM) {
        parse_union(p);
    } else if (tok.type == UNION) {
        parse_enum(p);
    } else {
        error("Cannot parse token!\n");
        print_location(&p->buf.toks[p->buf.begin].loc, "token type: %u\n", tok.type);
        die("Cannot recover!\n");
    }
}

static void parse(const u8 *filepath, const u8 *buf, const u64 size, struct ctti_output *output) {
    struct parser p = {
        .lex = {
            .loc = {
                .at   = buf,
                .file = filepath,
                .line = 1,
                .len  = 1,
            },
        },
        .buf = {0},
        .out = output,
    };

    struct token tok = {0};
    while (tok.type != END_OF_FILE) {
        next_token(&p.lex, &tok);
        if (tok.type == INTROSPECT) {
            fetch_tokens_if_needed(&p, 3);
            expect(&p.buf, LPAREN, NULL);
            expect(&p.buf, RPAREN, NULL);
            expect(&p.buf, SEMICOLON, NULL);

            parse_statement(&p);
        }
    }

}
