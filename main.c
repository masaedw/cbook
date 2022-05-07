#include "hypcc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // トークナイズしてパースする
  user_input = argv[1];
  tokenize();
  program();

  // アセンブリの前半部分を出力
  printf("  .section  __TEXT,__text,regular,pure_instructions\n");
  printf("  .build_version macos, 12, 0  sdk_version 12, 3\n");

  // 先頭の関数定義から順にコード生成
  for (int i = 0; code[i]; i++) {
    if (code[i]->kind == ND_FUNDEF)
      gen(code[i]);
  }

  // グローバル変数のセクション開始
  printf("  .section  __DATA,__data\n");

  // グローバル変数の定義
  for (int i = 0; code[i]; i++) {
    if (code[i]->kind == ND_GVARDEF)
      gen(code[i]);
  }

  return 0;
}
