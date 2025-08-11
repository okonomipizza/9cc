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

char *user_input;

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

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
        error_at(token->str, "expected '%c'", op);
    token = token->next;
}

// If next token is number, read a Token and return the num
// Others => report error
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "expected a number");
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
Token *tokenise() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strchr("+-*/()", *p)) {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

// AST Nodes
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // int
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // Only use when kind is ND_NUM
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}


Node *expr();
Node *mul();
Node *primary();

Node *expr() {
    Node *node = mul();

    for (;;) {
        if (consume('+'))
            node = new_node(ND_ADD, node, mul());
        else if (consume('-'))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = primary();

    for (;;) {
        if (consume('*'))
            node = new_node(ND_MUL, node, primary());
        else if (consume('/'))
            node = new_node(ND_DIV, node, primary());
        else
            return node;
    }
}

Node *primary() {
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }

    return new_node_num(expect_number());
}

void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rax, rdi\n");
        break;
    }

    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid args count\n");
        return 1;
    }

    // Tokenise and Parse
    user_input = argv[1];
    token = tokenise();
    Node *node = expr();

    // Output the first half of the assembly
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Code generation while traversing the AST
    gen(node);

    // Result of all expresion should be at top of stack
    // Load the result to rax
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
