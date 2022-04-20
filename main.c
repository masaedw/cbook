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
    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    printf("\t.section\t__TEXT,__text,regular,pure_instructions\n");
    printf("\t.build_version macos, 12, 0\tsdk_version 12, 3\n");
    printf("\t.global\t_main\n");
    printf("\t.p2align\t2\n");
    printf("_main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをW0にロードして関数からの返り値とする
    printf("\tldr w0, [SP], #16\n");
    printf("\tret\n");
    return 0;
}
