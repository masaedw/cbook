#include <stdio.h>
#include "9cc.h"

void gen(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("\tmov w0, #%d\n", node->val);
        printf("\tstr w0, [SP, #-16]!\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("\tldr w0, [SP], #16\n"); // rhs
    printf("\tldr w1, [SP], #16\n"); // lhs

    // w1 op w0

    switch (node->kind)
    {
    case ND_ADD:
        printf("\tadd w0, w1, w0\n");
        break;
    case ND_SUB:
        printf("\tsub w0, w1, w0\n");
        break;
    case ND_MUL:
        printf("\tmul w0, w1, w0\n");
        break;
    case ND_DIV:
        printf("\tsdiv w0, w1, w0\n");
        break;
    case ND_LT:
        printf("\tcmp w1, w0\n");
        printf("\tmov w1, #1\n");
        printf("\tmov w2, #0\n");
        printf("\tcsel w0, w1, w2, LO\n");
        break;
    case ND_LE:
        printf("\tcmp w1, w0\n");
        printf("\tmov w1, #1\n");
        printf("\tmov w2, #0\n");
        printf("\tcsel w0, w1, w2, LS\n");
        break;
    case ND_EQ:
        printf("\tcmp w1, w0\n");
        printf("\tmov w1, #1\n");
        printf("\tmov w2, #0\n");
        printf("\tcsel w0, w1, w2, EQ\n");
        break;
    case ND_NE:
        printf("\tcmp w1, w0\n");
        printf("\tmov w1, #1\n");
        printf("\tmov w2, #0\n");
        printf("\tcsel w0, w1, w2, NE\n");
        break;
    case ND_NUM:; // 警告抑制
    }

    printf("\tstr w0, [SP, #-16]!\n");
}
