#include <stdio.h>
#include "9cc.h"

void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("\tmov X0, #%d\n", node->val);
        printf("\tstr X0, [SP, #-16]!\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("\tldr X0, [SP], #16\n"); // rhs
    printf("\tldr X1, [SP], #16\n"); // lhs

    // X1 op X0

    switch (node->kind)
    {
    case ND_ADD:
        printf("\tadd X0, X1, X0\n");
        break;
    case ND_SUB:
        printf("\tsub X0, X1, X0\n");
        break;
    case ND_MUL:
        printf("\tmul X0, X1, X0\n");
        break;
    case ND_DIV:
        printf("\tsdiv X0, X1, X0\n");
        break;
    case ND_LT:
        printf("\tcmp X1, X0\n");
        printf("\tmov X1, #1\n");
        printf("\tmov X2, #0\n");
        printf("\tcsel X0, X1, X2, LO\n");
        break;
    case ND_LE:
        printf("\tcmp X1, X0\n");
        printf("\tmov X1, #1\n");
        printf("\tmov X2, #0\n");
        printf("\tcsel X0, X1, X2, LS\n");
        break;
    case ND_EQ:
        printf("\tcmp X1, X0\n");
        printf("\tmov X1, #1\n");
        printf("\tmov X2, #0\n");
        printf("\tcsel X0, X1, X2, EQ\n");
        break;
    case ND_NE:
        printf("\tcmp X1, X0\n");
        printf("\tmov X1, #1\n");
        printf("\tmov X2, #0\n");
        printf("\tcsel X0, X1, X2, NE\n");
        break;
    case ND_NUM:; // 警告抑制
    }

    printf("\tstr X0, [SP, #-16]!\n");
}
