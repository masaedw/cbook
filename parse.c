#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

/*

program         = func_definition*
func_definition = type ident "(" (type ident ("," type ident)*)? ")" "{" stmt* "}"
type            = "int" "*"*
stmt            = expr ";"
                | type ident ";"
                | "{" stmt* "}"
                | "if" "(" expr ")" stmt ("else" stmt)?
                | "while" "(" expr ")" stmt
                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
                | "return" expr ";"
expr            = assign
assign          = equality ("=" assign)?
equality        = relational ("==" relational | "!=" relational)*
relational      = add ("<" add | "<=" add | ">" add | ">=" add)*
add             = mul ("+" mul | "-" mul)*
mul             = unary ("*" unary | "/" unary)*
unary           = sizeof unary
                | ("+" | "-")? primary
                | "*" unary
                | "&" unary
primary         = num
                | ident ("(" (ident ("," ident)*)? ")")?
                | "(" expr ")"

*/

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// パース結果
Node *code[100];

// 今読んでいる関数定義
Node *fundef;

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok)
{
    for (LVar *var = fundef->locals; var; var = var->next)
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    return NULL;
}

// 変数を追加する。
LVar *new_lvar(Token *tok, Type *type)
{
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = fundef->locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = fundef->locals->offset + 8;
    lvar->type = type;
    fundef->locals = lvar;
    return lvar;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

bool consume_token(TokenKind kind)
{
    if (token->kind == kind)
    {
        token = token->next;
        return true;
    }
    return false;
}

Token *consume_ident()
{
    if (token->kind == TK_IDENT)
    {
        Token *tmp = token;
        token = token->next;
        return tmp;
    }
    return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error_at(token->str, "'%s'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number()
{
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

void expect_token(TokenKind kind, char *name)
{
    if (token->kind != kind)
    {
        error_at(token->str, "'%s'ではありません", name);
    }
    token = token->next;
}

Token *expect_ident()
{
    Token *token = consume_ident();
    if (!token)
    {
        error_at(token->str, "identではありません。");
    }
    return token;
}

bool at_eof()
{
    return token->kind == TK_EOF;
}

bool is_alnum(char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

bool is_token(char *p, char *token)
{
    int len = strlen(token);
    return strncmp(p, token, len) == 0 && !is_alnum(p[len]);
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// user_inputをトークナイズする
void tokenize()
{
    Token head;
    head.next = NULL;
    Token *cur = &head;
    char *p = user_input;

    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        if (strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0 || strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0)
        {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' ||
            *p == '<' || *p == '>' ||
            *p == '=' || *p == ';' || *p == '{' || *p == '}' || *p == ',' ||
            *p == '&')
        {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        if (is_token(p, "return"))
        {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (is_token(p, "if"))
        {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (is_token(p, "else"))
        {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        if (is_token(p, "while"))
        {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        if (is_token(p, "for"))
        {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        if (is_token(p, "int"))
        {
            cur = new_token(TK_RESERVED, cur, p, 3);
            p += 3;
            continue;
        }

        if (is_token(p, "sizeof"))
        {
            cur = new_token(TK_SIZEOF, cur, p, 6);
            p += 6;
            continue;
        }

        if ('a' <= *p && *p <= 'z')
        {
            char *q = p + 1;
            while (is_alnum(*q))
                q++;
            cur = new_token(TK_IDENT, cur, p, q - p);
            p = q;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    token = head.next;
}

Type *new_type_int()
{
    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;
    return type;
}

Type *new_type_ptr_to(Type *type)
{
    Type *ntype = calloc(1, sizeof(Type));
    ntype->ty = PTR;
    ntype->ptr_to = type;
    return ntype;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *type)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    node->type = type;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->type = new_type_int();
    return node;
}

Node *new_node_lvar(Token *tok, Type *type)
{
    LVar *lvar = find_lvar(tok);
    if (!lvar)
    {
        lvar = new_lvar(tok, type);
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->offset = lvar->offset;
    node->type = lvar->type;
    return node;
}

Node *expr();

Node *primary()
{
    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume("("))
    {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok)
    {
        if (consume("("))
        {
            Node *node = calloc(1, sizeof(Node));
            node->kind = ND_CALL;
            node->name = tok->str;
            node->len = tok->len;
            // TODO: 関数の型を探す
            node->type = new_type_int();

            if (consume(")"))
            {
                return node;
            }

            int i = 0;
            while (i < 8)
            {
                node->args[i++] = expr();
                if (!consume(","))
                    break;
            }
            expect(")");
            node->nargs = i;
            return node;
        }

        LVar *lvar = find_lvar(tok);
        if (!lvar)
        {
            error_at(tok->str, "変数が宣言されていません。");
        }

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;
        node->offset = lvar->offset;
        node->type = lvar->type;
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

Node *unary()
{
    if (consume_token(TK_SIZEOF))
    {
        Node *node = unary();
        switch (node->type->ty)
        {
        case INT:
            return new_node_num(4);
        case PTR:
            return new_node_num(8);
        }
    }

    if (consume("+"))
        return primary();

    if (consume("-"))
    {
        Node *node = primary();
        // TODO: primaryが変数の場合、valはないのでおかしくなる a = -a; のようなパターンを検討する
        node->val = -node->val;
        return node;
    }

    if (consume("*"))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_DEREF;
        Token *tok = token;
        node->lhs = unary();
        if (node->lhs->type->ty != PTR)
        {
            error_at(tok->str, "ポインタではありません");
        }
        node->type = node->lhs->type->ptr_to;
        return node;
    }

    if (consume("&"))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_ADDR;
        node->lhs = unary();
        node->type = new_type_ptr_to(node->lhs->type);
        return node;
    }

    return primary();
}

Node *mul()
{
    Node *node = unary();

    for (;;)
    {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary(), node->type);
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary(), node->type);
        else
            return node;
    }
}

Node *add()
{
    Node *node = mul();

    for (;;)
    {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul(), node->type);
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul(), node->type);
        else
            return node;
    }
}

Node *relational()
{
    Node *node = add();

    for (;;)
    {
        if (consume("<"))
            node = new_node(ND_LT, node, add(), new_type_int());
        else if (consume("<="))
            node = new_node(ND_LE, node, add(), new_type_int());
        else if (consume(">"))
        {
            node = new_node(ND_LT, node, add(), new_type_int());
            Node *tmp = node->lhs;
            node->lhs = node->rhs;
            node->rhs = tmp;
        }
        else if (consume(">="))
        {
            node = new_node(ND_LE, node, add(), new_type_int());
            Node *tmp = node->lhs;
            node->lhs = node->rhs;
            node->rhs = tmp;
        }
        else
            return node;
    }
}

Node *equality()
{
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
            node = new_node(ND_EQ, node, relational(), new_type_int());
        else if (consume("!="))
            node = new_node(ND_NE, node, relational(), new_type_int());
        else
            return node;
    }
}

Node *assign()
{
    Node *node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign(), node->type);
    return node;
}

Node *expr()
{
    return assign();
}

Type *consume_type()
{
    if (!consume("int"))
    {
        return NULL;
    }

    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;

    while (consume("*"))
    {
        type = new_type_ptr_to(type);
    }

    return type;
}

Type *type()
{
    Type *ty = consume_type();
    if (!ty)
    {
        error_at(token->str, "typeではありません");
    }
    return ty;
}

Node *stmt()
{
    if (consume_token(TK_RETURN))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        node->rhs = fundef;

        expect(";");
        return node;
    }

    if (consume_token(TK_IF))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->expr0 = expr();
        expect(")");
        node->lhs = stmt();
        if (consume_token(TK_ELSE))
        {
            node->rhs = stmt();
        }
        return node;
    }

    if (consume_token(TK_WHILE))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->expr0 = expr();
        expect(")");
        node->lhs = stmt();
        return node;
    }

    if (consume_token(TK_FOR))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        if (!consume(";"))
        {
            node->expr0 = expr();
            expect(";");
        }
        if (!consume(";"))
        {
            node->expr1 = expr();
            expect(";");
        }
        if (!consume(")"))
        {
            node->rhs = expr();
            expect(")");
        }
        node->lhs = stmt();
        return node;
    }

    if (consume("{"))
    {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        node->body = calloc(100, sizeof(Node *));
        int i = 0;
        while (!consume("}"))
        {
            node->body[i++] = stmt();
        }
        node->body[i] = NULL;
        return node;
    }

    Type *ty = consume_type();
    if (ty)
    {
        LVar *lvar = new_lvar(expect_ident(), ty);
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_VARDEF;
        node->name = lvar->name;
        node->len = lvar->len;
        expect(";");
        return node;
    }

    Node *node = expr();
    expect(";");
    return node;
}

Node *func_definition()
{
    Type *ty = type();
    Token *ident = expect_ident();
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FUNDEF;
    node->name = ident->str;
    node->len = ident->len;
    node->body = calloc(100, sizeof(Node *));
    node->locals = calloc(1, sizeof(LVar *));
    node->type = ty;
    fundef = node;
    expect("(");

    if (!consume(")"))
    {
        int i = 0;
        while (i < 8)
        {
            Type *ty = type();
            node->args[i++] = new_node_lvar(expect_ident(), ty);
            if (!consume(","))
                break;
        }
        expect(")");
        node->nargs = i;
    }

    expect("{");
    int i = 0;
    while (!consume("}"))
    {
        node->body[i++] = stmt();
    }
    node->body[i] = NULL;
    fundef = NULL;
    return node;
}

void program()
{
    int i = 0;
    while (!at_eof())
        code[i++] = func_definition();
    code[i] = NULL;
}
