#include <stdlib.h>
#include <stdio.h>
#include "9cc.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // トークナイズしてパースする
    locals = calloc(1, sizeof(LVar));
    user_input = argv[1];
    tokenize();
    program();

    // アセンブリの前半部分を出力
    printf("  .section\t__TEXT,__text,regular,pure_instructions\n");
    printf("  .build_version macos, 12, 0\tsdk_version 12, 3\n");
    printf("  .global\t_main\n");
    printf("  .p2align\t2\n");
    printf("_main:\n");

    // プロローグ
    // 変数分の領域を16バイト境界で確保する
    int offset = locals->offset + 8;
    if (offset % 16 != 0)
    {
        offset += 8;
    }
    printf("  stp LR, FP, [SP, #-16]!\n");
    printf("  sub FP, SP, #%d\n", offset);
    printf("  sub SP, SP, #%d\n", offset);

    // 先頭の式から順にコード生成
    for (int i = 0; code[i]; i++)
    {
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っている
        // はずなので、スタックが溢れないようにポップしておく
        printf("  ldr X0, [SP], #16\n");
    }

    // エピローグ
    // 最後の式の結果がX0に残っているのでそれが返り値になる
    printf("  mov SP, FP\n");
    printf("  add SP, SP, #%d\n", offset);
    printf("  ldp LR, FP, [SP], #16\n");
    printf("  ret\n");
    return 0;
}
