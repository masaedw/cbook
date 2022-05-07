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
 *                 | ident ("(" (ident ("," ident)*)? ")")?
 *                 | "(" expr ")"
 *
 */

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// パース結果
Node *code[100];

// 今読んでいる関数定義
Node *current_fundef;

// グローバル変数
GVar *globals;

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
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
  GVar *lvar = calloc(1, sizeof(GVar));
  lvar->next = globals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->type = type;
  globals = lvar;
  return lvar;
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

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  token = head.next;
}

Type *new_type_int() {
  Type *type = calloc(1, sizeof(Type));
  type->ty = INT;
  return type;
}

Type *new_type_char() {
  Type *type = calloc(1, sizeof(Type));
  type->ty = CHAR;
  return type;
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
    error_at(tok->str, "既に定義済みです");
  }
  gvar = new_global(tok, type);

  Node *node = new_node(ND_GVARDEF, NULL, NULL, type);
  node->name = gvar->name;
  node->len = gvar->len;
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
    node->body = calloc(100, sizeof(Node *));
    int i = 0;
    while (!consume("}")) {
      node->body[i++] = stmt();
    }
    node->body[i] = NULL;
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
  while (!consume("}")) {
    node->body[i++] = stmt();
  }
  node->body[i] = NULL;
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
  code[i] = NULL;
}
