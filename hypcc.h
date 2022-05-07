#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_RETURN,   // return
  TK_IF,       // if
  TK_ELSE,     // else
  TK_WHILE,    // while
  TK_FOR,      // for
  TK_SIZEOF,   // sizeof
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,     // +
  ND_SUB,     // -
  ND_MUL,     // *
  ND_DIV,     // /
  ND_LT,      // <
  ND_LE,      // <=
  ND_EQ,      // ==
  ND_NE,      // !=
  ND_ASSIGN,  // =
  ND_NUM,     // 整数
  ND_LVAR,    // 変数
  ND_GVAR,    // グローバル変数
  ND_RETURN,  // return
  ND_IF,      // if
  ND_WHILE,   // while
  ND_FOR,     // for
  ND_BLOCK,   // block
  ND_CALL,    // function call
  ND_FUNDEF,  // function definition
  ND_ADDR,    // &
  ND_DEREF,   // *
  ND_VARDEF,  // variable definition
  ND_GVARDEF, // global variables
} NodeKind;

typedef struct Type Type;

// 型の型
struct Type {
  enum { CHAR, INT, PTR, ARRAY } ty;
  struct Type *ptr_to;
  size_t array_size;
};

// 型ごとのサイズ
int type_size(Type *type);

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // FPからのオフセット
  Type *type; // 型
};

typedef struct Node Node;

// 抽象構文木のノードの型
// if (expr0) lhs else rhs
// while (expr0) lhs
// for (expr0; expr1; rhs) lhs
struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺
  Node *rhs;     // 右辺
  int val;       // kindがND_NUMの場合のみ使う
  int offset;    // kindがND_LVARの場合のみ使う
  Node *expr0;   // if/while/forの場合に使う
  Node *expr1;   // forの場合のみ使う
  Node **body;   // blockと関数定義の場合に使う
  char *name;    // 関数の名前
  int len;       // 名前の長さ
  Node *args[8]; // 関数の引数
  int nargs;     // 引数の個数
  LVar *locals;  // ローカル変数
  Type *type;    // 値の型
};

// パース結果
extern Node *code[100];

// user_inputをトークナイズする
void tokenize();

void gen(Node *node);

// tokenをパースして、codeに保存する
void program();