#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "9cc.h"

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

int new_label()
{
    static int count = 0;
    return count++;
}

void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("  mov X0, #%d\n", node->val);
        printf("  str X0, [SP, #-16]!\n");
        return;
    case ND_LVAR:
        if (node->kind != ND_LVAR)
            error("代入の左辺値が変数ではありません");
        printf("  ldur X0, [FP, #%d]\n", node->offset);
        printf("  str X0, [SP, #-16]!\n");
        return;
    case ND_ASSIGN:
        if (node->lhs->kind != ND_LVAR)
            error("代入の左辺値が変数ではありません");
        gen(node->rhs);

        printf("  ldur X0, [SP, #0]\n");
        printf("  stur X0, [FP, #%d]\n", node->lhs->offset);
        return;
    case ND_RETURN:
        gen(node->lhs);
        int offset = locals->offset + 8;
        if (offset % 16 != 0)
        {
            offset += 8;
        }
        printf("  ldr X0, [SP], #16\n");
        // エピローグ
        printf("  add SP, SP, #%d\n", offset);
        printf("  ldp LR, FP, [SP], #16\n");
        printf("  ret\n");
        return;
    case ND_IF:
        gen(node->expr0);
        printf("  ldur X0, [SP, #0]\n");
        if (!node->rhs)
        {
            int lend = new_label();
            printf("  cbz X0, LEND_%03d\n", lend);
            printf("  ldr X0, [SP], #16\n");
            gen(node->lhs);
            printf("LEND_%03d:\n", lend);
        }
        else
        {
            int lelse = new_label();
            int lend = new_label();
            printf("  cbz X0, LELSE_%03d\n", lelse);
            printf("  ldr X0, [SP], #16\n");
            gen(node->lhs);
            printf("  b LEND_%03d\n", lend);
            printf("LELSE_%03d:\n", lelse);
            printf("  ldr X0, [SP], #16\n");
            gen(node->rhs);
            printf("LEND_%03d:\n", lend);
        }
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  ldr X0, [SP], #16\n"); // rhs
    printf("  ldr X1, [SP], #16\n"); // lhs

    // X1 op X0

    switch (node->kind)
    {
    case ND_ADD:
        printf("  add X0, X1, X0\n");
        break;
    case ND_SUB:
        printf("  sub X0, X1, X0\n");
        break;
    case ND_MUL:
        printf("  mul X0, X1, X0\n");
        break;
    case ND_DIV:
        printf("  sdiv X0, X1, X0\n");
        break;
    case ND_LT:
        printf("  cmp X1, X0\n");
        printf("  cset X0, LO\n");
        break;
    case ND_LE:
        printf("  cmp X1, X0\n");
        printf("  cset X0, LS\n");
        break;
    case ND_EQ:
        printf("  cmp X1, X0\n");
        printf("  cset X0, EQ\n");
        break;
    case ND_NE:
        printf("  cmp X1, X0\n");
        printf("  cset X0, NE\n");
        break;
    }

    printf("  str X0, [SP, #-16]!\n");
}
