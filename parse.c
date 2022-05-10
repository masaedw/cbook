#include "hypcc.h"

/*
 *
 * program         = global*
 * global          = type_prefix ident "(" (type ident ("," type ident)*)? ")" "{" stmt* "}"
 *                 | type_prefix ident ("[" num "]")? ";" // prototype decl
 *                 | "typedef" type_prefix ident ("[" num "]")? ";"
 * stmt            = expr ";"
 *                 | type_prefix ident type_suffix? ";"
 *                 | compound_stmt
 *                 | "if" "(" expr ")" stmt ("else" stmt)?
 *                 | "while" "(" expr ")" stmt
 *                 | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *                 | "return" expr ";"
 * compound_stmt   = "{" stmt* "}"
 * prim_type       = "int" | "char"
 * type_prefix     = prim_type "*"*
 * type_suffix     = "[" num "]"
 * expr            = assign
 * assign          = equality ("=" assign)?
 * equality        = relational ("==" relational | "!=" relational)*
 * relational      = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add             = mul ("+" mul | "-" mul)*
 * mul             = unary ("*" unary | "/" unary)*
 * unary           = sizeof unary
 *                 | ("+" | "-")? indexing
 *                 | "*" unary
 *                 | "&" unary
 * indexing        = primary ("[" expr "]")
 * primary         = num
 *                 | string-literal
 *                 | ident ("(" (ident ("," ident)*)? ")")?
 *                 | "(" (compound_stmt | expr) ")"
 *
 */

// 現在着目しているトークン
Token *token;

// 入力ファイル名
char *filename;

// 入力プログラム
char *user_input;

// パース結果
Node *code[100];

// 今の変数のスコープ
Node *current_scope;
// 今の関数定義
Node *current_fundef;
// 今のループ
static Node *current_loop;

// グローバル変数
GVar *globals;

// typedef
TypeDef *tdefs;

static Type *type_int = &(Type){.ty = INT};
static Type *type_char = &(Type){.ty = CHAR};

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
noreturn void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  // locが含まれている行の開始地点と終了地点を取得
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で指し示して、エラーメッセージを表示
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

LVar *find_lvar_in_scope(Node *scope, Token *tok) {
  for (LVar *var = scope->locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  for (Node *scope = current_scope; scope; scope = scope->up) {
    LVar *var = find_lvar_in_scope(scope, tok);
    if (var)
      return var;
  }
  return NULL;
}

// 変数を追加する。
LVar *new_lvar(Token *tok, Type *type) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = current_scope->locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  current_fundef->offset = lvar->offset = current_fundef->offset + type_size(type);
  lvar->type = type;
  current_scope->locals = lvar;
  return lvar;
}

// グローバル変数を名前で検索する。見つからなかった場合はNULLを返す。
GVar *find_global(Token *tok) {
  for (GVar *var = globals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// グローバル変数を追加する。
GVar *new_global(Token *tok, Type *type) {
  GVar *gvar = calloc(1, sizeof(GVar));
  gvar->next = globals;
  gvar->name = tok->str;
  gvar->len = tok->len;
  gvar->type = type;
  gvar->token = tok;
  globals = gvar;
  return gvar;
}

bool is_variable(void) {
  if (token->kind != TK_IDENT) {
    return false;
  }

  return find_lvar(token) || find_global(token);
}

// typedefを名前で検索する。見つからなかった場合はNULLを返す。
TypeDef *find_typedef(Token *tok) {
  for (TypeDef *def = tdefs; def; def = def->next)
    if (def->len == tok->len && !memcmp(tok->str, def->name, def->len))
      return def;
  return NULL;
}

// typedefを追加する。
TypeDef *new_typedef(Token *tok, Type *type) {
  TypeDef *tdef = calloc(1, sizeof(TypeDef));
  tdef->next = tdefs;
  tdef->name = tok->str;
  tdef->len = tok->len;
  tdef->type = type;
  tdef->token = tok;
  tdefs = tdef;
  return tdef;
}

char *new_unique_str_name(void) {
  static int id;
  return format("l_.str.%d", id++);
}

Type *new_type_array_to(Type *type, size_t size);
Node *new_node_gvar(GVar *gvar);

// 文字列リテラルをグローバル変数として追加する
GVar *new_string_literal(Token *tok) {
  GVar *gvar = calloc(1, sizeof(GVar));

  size_t len = tok->len + 1;
  char *val = calloc(len, 1);
  memcpy(val, tok->str, len - 1);
  val[len] = 0;

  gvar->next = globals;
  gvar->name = new_unique_str_name();
  gvar->len = strlen(gvar->name) + 1;
  gvar->type = new_type_array_to(type_char, len);
  gvar->is_string_literal = true;
  gvar->str_val = val;
  gvar->token = tok;
  globals = gvar;
  return gvar;
}

bool equal(char *str) {
  return strlen(str) == token->len && memcmp(token->str, str, token->len) == 0;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

bool consume_token(TokenKind kind) {
  if (token->kind == kind) {
    token = token->next;
    return true;
  }
  return false;
}

Token *consume_ident() {
  if (token->kind == TK_IDENT) {
    Token *tmp = token;
    token = token->next;
    return tmp;
  }
  return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

void expect_token(TokenKind kind, char *name) {
  if (token->kind != kind) {
    error_at(token->str, "'%s'ではありません", name);
  }
  token = token->next;
}

Token *expect_ident() {
  Token *token = consume_ident();
  if (!token) {
    error_at(token->str, "identではありません。");
  }
  return token;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

bool is_alnum(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_');
}

bool is_token(char *p, char *token) {
  int len = strlen(token);
  return strncmp(p, token, len) == 0 && !is_alnum(p[len]);
}

bool is_keyword(char *p, char **token) {
  static char *keywords[] = {"return",   "if",   "else", "while",  "for",    "break",
                             "continue", "char", "int",  "sizeof", "typedef"};
  for (int i = 0; i < sizeof(keywords) / sizeof(char *); i++) {
    if (is_token(p, keywords[i])) {
      *token = keywords[i];
      return true;
    }
  }
  return false;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// user_inputをトークナイズする
void tokenize() {
  Token head;
  head.next = NULL;
  Token *cur = &head;
  char *p = user_input;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 行コメントをスキップ
    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // ブロックコメントをスキップ
    if (strncmp(p, "/*", 2) == 0) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "コメントが閉じられていません");
      p = q + 2;
      continue;
    }

    if (strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0 || strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;{},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    char *kw;
    if (is_keyword(p, &kw)) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    if (isalpha(*p)) {
      char *q = p + 1;
      while (is_alnum(*q))
        q++;
      cur = new_token(TK_IDENT, cur, p, q - p);
      p = q;
      continue;
    }

    if ('"' == *p) {
      char *q = p + 1;
      while (*q != '"')
        q++;
      // リテラルのデータとしてはダブルクオートは入らない
      cur = new_token(TK_STRING_LITERAL, cur, p + 1, q - p - 1);
      p = q + 1;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  token = head.next;
}

Type *new_type_ptr_to(Type *type) {
  Type *ntype = calloc(1, sizeof(Type));
  ntype->ty = PTR;
  ntype->ptr_to = type;
  return ntype;
}

Type *new_type_array_to(Type *type, size_t size) {
  Type *ntype = calloc(1, sizeof(Type));
  ntype->ty = ARRAY;
  ntype->ptr_to = type;
  ntype->array_size = size;
  return ntype;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *type) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  node->type = type;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = type_int;
  return node;
}

Node *new_node_deref(Node *lhs, Token *tok) {
  if (lhs->type->ty != PTR && lhs->type->ty != ARRAY) {
    error_at(tok->str, "ポインタまたは配列ではありません");
  }

  return new_node(ND_DEREF, lhs, NULL, lhs->type->ptr_to);
}

Node *new_node_vardef(Token *tok, Type *type) {
  if (find_lvar_in_scope(current_scope, tok)) {
    error_at(tok->str, "定義済みです");
  }
  LVar *lvar = new_lvar(tok, type);
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_VARDEF;
  node->offset = lvar->offset;
  node->type = lvar->type;
  node->name = lvar->name;
  node->len = lvar->len;
  return node;
}

Node *new_node_global(Token *tok, Type *type) {
  GVar *gvar = find_global(tok);
  if (gvar) {
    error_at(tok->str, "定義済みです");
  }
  return new_node_gvar(new_global(tok, type));
}

Node *new_node_gvar(GVar *gvar) {
  Node *node = new_node(ND_GVARDEF, NULL, NULL, gvar->type);
  node->name = gvar->name;
  node->len = gvar->len;
  node->gvar = gvar;
  return node;
}

Node *new_node_typedef(Token *tok, Type *type) {
  TypeDef *tdef = find_typedef(tok);
  if (tdef) {
    error_at(tok->str, "定義済みです");
  }
  tdef = new_typedef(tok, type);
  Node *node = new_node(ND_TYPEDEF, NULL, NULL, tdef->type);
  node->name = tdef->token->str;
  node->len = tdef->token->len;
  node->tdef = tdef;
  return node;
}

Node *expr();
Node *stmt();
Node *compound_stmt();

Node *primary() {
  if (consume("(")) {
    Node *node;
    if (consume("{"))
      node = compound_stmt();
    else
      node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    if (consume("(")) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_CALL;
      node->name = tok->str;
      node->len = tok->len;
      // TODO: 関数の型を探す
      node->type = type_int;

      if (consume(")")) {
        return node;
      }

      int i = 0;
      while (i < 8) {
        node->args[i++] = expr();
        if (!consume(","))
          break;
      }
      expect(")");
      node->nargs = i;
      return node;
    }

    LVar *lvar = find_lvar(tok);
    if (lvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_LVAR;
      node->offset = lvar->offset;
      node->type = lvar->type;
      return node;
    }
    GVar *gvar = find_global(tok);
    if (gvar) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_GVAR;
      node->type = gvar->type;
      node->name = gvar->name;
      node->len = gvar->len;
      return node;
    }

    error_at(tok->str, "変数が宣言されていません。");
  }

  if (token->kind == TK_STRING_LITERAL) {
    GVar *gvar = new_string_literal(token);
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_GVAR;
    node->type = gvar->type;
    node->name = gvar->name;
    node->len = gvar->len;
    token = token->next;
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

Node *indexing() {
  Node *node = primary();
  if (consume("[")) {
    node = new_node_deref(new_node(ND_ADD, node, expr(), node->type), token);
    expect("]");
  }
  return node;
}

Node *unary() {
  if (consume("sizeof")) {
    Node *node = unary();
    return new_node_num(type_size(node->type));
  }

  if (consume("+"))
    return indexing();

  if (consume("-")) {
    Node *node = indexing();
    // TODO: indexingが変数の場合、valはないのでおかしくなる a = -a;
    // のようなパターンを検討する
    node->val = -node->val;
    return node;
  }

  if (consume("*")) {
    return new_node_deref(unary(), token);
  }

  if (consume("&")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_ADDR;
    node->lhs = unary();
    node->type = new_type_ptr_to(node->lhs->type);
    return node;
  }

  return indexing();
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary(), node->type);
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary(), node->type);
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul(), node->type);
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul(), node->type);
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add(), type_int);
    else if (consume("<="))
      node = new_node(ND_LE, node, add(), type_int);
    else if (consume(">")) {
      node = new_node(ND_LT, node, add(), type_int);
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    } else if (consume(">=")) {
      node = new_node(ND_LE, node, add(), type_int);
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    } else
      return node;
  }
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational(), type_int);
    else if (consume("!="))
      node = new_node(ND_NE, node, relational(), type_int);
    else
      return node;
  }
}

Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign(), node->type);
  return node;
}

Node *expr() {
  return assign();
}

Type *consume_type_suffix(Type *type) {
  if (!consume("[")) {
    return type;
  }
  int n = expect_number();
  expect("]");
  return new_type_array_to(type, n);
}

typedef struct Pair Pair;

struct Pair {
  void *key;
  void *value;
};

static Pair *types[] = {
    &(Pair){"char", &type_char},
    &(Pair){"int", &type_int},
};

Type *consume_type_name(void) {
  for (int i = 0; i < sizeof(types) / sizeof(Pair *); i++) {
    if (equal(types[i]->key)) {
      token = token->next;
      return *(Type **)types[i]->value;
    }
  }
  TypeDef *ty = find_typedef(token);
  if (ty) {
    token = token->next;
    return ty->type;
  }
  return NULL;
}

Type *expect_type_name(void) {
  Type *type = consume_type_name();
  if (!type) {
    error_at(token->str, "型名ではありません");
  }
  return type;
}

Type *expect_prim_type(void) {
  for (int i = 0; i < sizeof(types) / sizeof(Pair *); i++) {
    if (equal(types[i]->key)) {
      token = token->next;
      return *(Type **)types[i]->value;
    }
  }
  error_at(token->str, "型名ではありません");
}

Type *expect_type_prefix(Type *type) {
  while (consume("*")) {
    type = new_type_ptr_to(type);
  }
  return type;
}

Node *compound_stmt() {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_BLOCK;
  node->up = current_scope;
  current_scope = node;
  Node *last = NULL;
  int i = 0;
  if (!consume("}")) {
    last = node->body = stmt();
  }
  while (!consume("}")) {
    last = last->next = stmt();
  }
  current_scope = node->up;
  return node;
}

Node *stmt() {
  if (consume("return")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();

    expect(";");
    return node;
  }

  if (consume("if")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->expr0 = expr();
    expect(")");
    node->lhs = stmt();
    if (consume("else")) {
      node->rhs = stmt();
    }
    return node;
  }

  if (consume("while")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->expr0 = expr();
    expect(")");
    Node *tmp = current_loop;
    current_loop = node;
    node->lhs = stmt();
    current_loop = tmp;
    return node;
  }

  if (consume("for")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect("(");
    if (!consume(";")) {
      node->expr0 = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->expr1 = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->rhs = expr();
      expect(")");
    }
    Node *tmp = current_loop;
    current_loop = node;
    node->lhs = stmt();
    current_loop = tmp;
    return node;
  }

  Token *tok = token;

  if (consume("break")) {
    if (current_loop == NULL)
      error_at(tok->str, "ループの中ではありません");
    expect(";");
    return new_node(ND_BREAK, NULL, NULL, NULL);
  }

  if (consume("continue")) {
    if (current_loop == NULL)
      error_at(tok->str, "ループの中ではありません");
    expect(";");
    return new_node(ND_CONTINUE, NULL, NULL, NULL);
  }

  if (consume("{")) {
    return compound_stmt();
  }

  // 型名と変数名が重複する場合があるので、変数名かどうかを先にチェックする。
  if (is_variable()) {
    Node *node = expr();
    expect(";");
    return node;
  }

  Type *ty = consume_type_name();
  if (ty) {
    ty = expect_type_prefix(ty);
    Token *ident = expect_ident();
    Node *node = new_node_vardef(ident, consume_type_suffix(ty));
    expect(";");
    return node;
  }

  Node *node = expr();
  expect(";");
  return node;
}

Node *fundef(Type *ty, Token *ident) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNDEF;
  node->name = ident->str;
  node->len = ident->len;
  node->locals = calloc(1, sizeof(LVar *));
  node->type = ty;
  current_fundef = current_scope = node;

  if (!consume(")")) {
    int i = 0;
    while (i < 8) {
      Type *ty = expect_type_prefix(expect_type_name());
      node->args[i++] = new_node_vardef(expect_ident(), consume_type_suffix(ty));
      if (!consume(","))
        break;
    }
    expect(")");
    node->nargs = i;
  }

  expect("{");
  int i = 0;
  Node *last = NULL;
  if (!consume("}")) {
    last = node->body = stmt();
  }
  while (!consume("}")) {
    last = last->next = stmt();
  }
  current_fundef = current_scope = NULL;
  return node;
}

Node *global() {
  if (consume("typedef")) {
    Type *ty = expect_type_prefix(expect_type_name());
    Token *ident = expect_ident();
    ty = consume_type_suffix(ty);
    expect(";");
    return new_node_typedef(ident, ty);
  }

  Type *ty = expect_type_prefix(expect_type_name());
  Token *ident = expect_ident();

  if (consume("(")) {
    return fundef(ty, ident);
  }

  ty = consume_type_suffix(ty);
  expect(";");
  return new_node_global(ident, ty);
}

void program() {
  int i = 0;
  while (!at_eof()) {
    code[i++] = global();
  }
  for (GVar *g = globals; g; g = g->next) {
    if (!g->is_string_literal)
      continue;

    code[i++] = new_node_gvar(g);
  }
  code[i] = NULL;
}
