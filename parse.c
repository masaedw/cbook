#include "hypcc.h"

/*
 *
 * program         = global*
 * global          = type_prefix ident "(" (type ident ("," type ident)*)? ")" "{" stmt* "}"
 *                 | type_prefix ident ("[" num "]")? ";"
 * stmt            = expr ";"
 *                 | type_prefix ident type_suffix? ";"
 *                 | "{" stmt* "}"
 *                 | "if" "(" expr ")" stmt ("else" stmt)?
 *                 | "while" "(" expr ")" stmt
 *                 | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *                 | "return" expr ";"
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
 *                 | "(" expr ")"
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

// 今読んでいる関数定義
Node *current_fundef;

// グローバル変数
GVar *globals;

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void error_at(char *loc, char *fmt, ...) {
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

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  for (LVar *var = current_fundef->locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// 変数を追加する。
LVar *new_lvar(Token *tok, Type *type) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = current_fundef->locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->offset = current_fundef->locals->offset + type_size(type);
  lvar->type = type;
  current_fundef->locals = lvar;
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

char *new_unique_str_name(void) {
  static int id;
  return format("l_.str.%d", id++);
}

Type *new_type_array_to(Type *type, size_t size);
Type *new_type_char(void);
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
  gvar->type = new_type_array_to(new_type_char(), len);
  gvar->is_string_literal = true;
  gvar->str_val = val;
  gvar->token = tok;
  globals = gvar;
  return gvar;
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

    if (is_token(p, "return")) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (is_token(p, "if")) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (is_token(p, "else")) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (is_token(p, "while")) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (is_token(p, "for")) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_token(p, "char")) {
      cur = new_token(TK_RESERVED, cur, p, 4);
      p += 4;
      continue;
    }

    if (is_token(p, "int")) {
      cur = new_token(TK_RESERVED, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_token(p, "sizeof")) {
      cur = new_token(TK_SIZEOF, cur, p, 6);
      p += 6;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
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

static Type *g_type_int = &(Type){.ty = INT};

Type *new_type_int() {
  return g_type_int;
}

static Type *g_type_char = &(Type){.ty = CHAR};

Type *new_type_char(void) {
  return g_type_char;
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
  node->type = new_type_int();
  return node;
}

Node *new_node_deref(Node *lhs, Token *tok) {
  if (lhs->type->ty != PTR && lhs->type->ty != ARRAY) {
    error_at(tok->str, "ポインタまたは配列ではありません");
  }

  return new_node(ND_DEREF, lhs, NULL, lhs->type->ptr_to);
}

Node *new_node_lvar(Token *tok, Type *type) {
  LVar *lvar = find_lvar(tok);
  if (!lvar) {
    lvar = new_lvar(tok, type);
  }

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;
  node->offset = lvar->offset;
  node->type = lvar->type;
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

Node *expr();

Node *primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
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
      node->type = new_type_int();

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
  if (consume_token(TK_SIZEOF)) {
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
      node = new_node(ND_LT, node, add(), new_type_int());
    else if (consume("<="))
      node = new_node(ND_LE, node, add(), new_type_int());
    else if (consume(">")) {
      node = new_node(ND_LT, node, add(), new_type_int());
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    } else if (consume(">=")) {
      node = new_node(ND_LE, node, add(), new_type_int());
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
      node = new_node(ND_EQ, node, relational(), new_type_int());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational(), new_type_int());
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

Type *consume_prim_type() {
  if (consume("char")) {
    return new_type_char();
  }
  if (consume("int")) {
    return new_type_int();
  }
  return NULL;
}

Type *consume_type_prefix() {
  Type *type = consume_prim_type();
  if (!type) {
    return NULL;
  }

  while (consume("*")) {
    type = new_type_ptr_to(type);
  }

  return type;
}

Type *type_prefix() {
  Type *ty = consume_type_prefix();
  if (!ty) {
    error_at(token->str, "typeではありません");
  }
  return ty;
}

Node *stmt() {
  if (consume_token(TK_RETURN)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    node->rhs = current_fundef;

    expect(";");
    return node;
  }

  if (consume_token(TK_IF)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->expr0 = expr();
    expect(")");
    node->lhs = stmt();
    if (consume_token(TK_ELSE)) {
      node->rhs = stmt();
    }
    return node;
  }

  if (consume_token(TK_WHILE)) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->expr0 = expr();
    expect(")");
    node->lhs = stmt();
    return node;
  }

  if (consume_token(TK_FOR)) {
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
    node->lhs = stmt();
    return node;
  }

  if (consume("{")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    Node *last = NULL;
    int i = 0;
    if (!consume("}")) {
      last = node->body = stmt();
    }
    while (!consume("}")) {
      last = last->next = stmt();
    }
    return node;
  }

  Type *ty = consume_type_prefix();
  if (ty) {
    LVar *lvar = new_lvar(expect_ident(), consume_type_suffix(ty));
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

Node *fundef(Type *ty, Token *ident) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNDEF;
  node->name = ident->str;
  node->len = ident->len;
  node->body = calloc(100, sizeof(Node *));
  node->locals = calloc(1, sizeof(LVar *));
  node->type = ty;
  current_fundef = node;

  if (!consume(")")) {
    int i = 0;
    while (i < 8) {
      Type *ty = type_prefix();
      node->args[i++] = new_node_lvar(expect_ident(), consume_type_suffix(ty));
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
  current_fundef = NULL;
  return node;
}

Node *global() {
  Type *ty = type_prefix();
  Token *ident = expect_ident();

  if (consume("(")) {
    return fundef(ty, ident);
  }

  if (consume("[")) {
    int n = expect_number();
    expect("]");
    expect(";");
    return new_node_global(ident, new_type_array_to(ty, n));
  }
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
