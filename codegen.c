#include <stdio.h>
#include "9cc.h"

void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  mov X0, #%d\n", node->val);
        printf("  str X0, [SP, #-16]!\n");
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
        printf("  mov X1, #1\n");
        printf("  mov X2, #0\n");
        printf("  csel X0, X1, X2, LO\n");
        break;
    case ND_LE:
        printf("  cmp X1, X0\n");
        printf("  mov X1, #1\n");
        printf("  mov X2, #0\n");
        printf("  csel X0, X1, X2, LS\n");
        break;
    case ND_EQ:
        printf("  cmp X1, X0\n");
        printf("  mov X1, #1\n");
        printf("  mov X2, #0\n");
        printf("  csel X0, X1, X2, EQ\n");
        break;
    case ND_NE:
        printf("  cmp X1, X0\n");
        printf("  mov X1, #1\n");
        printf("  mov X2, #0\n");
        printf("  csel X0, X1, X2, NE\n");
        break;
    case ND_NUM:; // 警告抑制
    }

    printf("  str X0, [SP, #-16]!\n");
}
