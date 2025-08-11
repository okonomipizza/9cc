#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token kinds
typedef enum {
    TK_RESERVED,    // Symbol
    TK_NUM,         // Integer
    TK_EOF,         // End of inputs
} TokenKind;

typedef struct Token Token;

// Token type
struct Token {
    TokenKind kind; // Token kind
    Token *next;    // Next input token
    int val;        // If kind is TK_NUM, this will be the num
    char *str;
};

// Token currently focused
Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// If next token is expected, read a Token
// and return true. Other => false
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    token = token->next;
    return true;
}

// If next token is expected, read a Token
// Else => report errror
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error("Not a '%c'", op);
    token = token->next;
}

// If next token is number, read a Token and return the num
// Others => report error
int expect_number() {
    if (token->kind != TK_NUM)
        error("Not a number");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// Create new Token and append it to cur
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur -> next = tok;
    return tok;
}

// Tokenise input string p and return that
Token *tokenise(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("Could not tokenisse");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid args count\n");
        return 1;
    }

    // Tokenise
    token = tokenise(argv[1]);

    // Output the first half of the assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // The expression must start with a number, so we
    // check this and print the first mov instruction
    printf("  mov rax, %d\n", expect_number());

    // Consume the token sequence `+ <number>` or `- <number>`
    // while outputting assembly code
    while (!at_eof()) {
        if (consume('+')) {
            printf("  add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf("  sub rax, %d\n", expect_number());
    }

    printf("  ret\n");
    return 0;
}
