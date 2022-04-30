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

char *size_prefix(Node *node)
{
    switch (node->type->ty)
    {
    case INT:
        return "w";
    case PTR:
    case ARRAY:
        return "x";
    }
}

int get_size(Type *type)
{
    switch (type->ty)
    {
    case INT:
        return 4;
    case PTR:
        return 8;
    case ARRAY:
        return get_size(type->ptr_to) * type->array_size;
    }
}

void gen_lval(Node *node)
{
    switch (node->kind)
    {
    case ND_DEREF:
        gen(node->lhs);
        return;
    case ND_LVAR:
        // fpからoffsetを引いた値をスタックにプッシュする
        printf("  add x0, fp, #-%d\n", node->offset);
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_GVAR:
        printf("  adrp x0, _%.*s@PAGE\n", node->len, node->name);
        printf("  add x0, x0, _%.*s@PAGEOFF\n", node->len, node->name);
        printf("  str x0, [sp, #-16]!\n");
        return;
    default:
        error("代入の左辺値が変数ないしderefではありません");
    }
}

size_t local_variables_offset(Node *fundef)
{
    size_t offset = fundef->locals->offset;
    size_t rem = offset % 16;
    if (rem != 0)
    {
        offset += 16 - rem;
    }
    return offset;
}

void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("  mov x0, #%d\n", node->val);
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_LVAR:
        if (node->type->ty == ARRAY)
        {
            gen_lval(node);
            return;
        }
        printf("  ldur %s0, [fp, #-%d]\n", size_prefix(node), node->offset);
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_GVAR:
        if (node->type->ty == ARRAY)
        {
            gen_lval(node);
            return;
        }
        printf("  adrp x0, _%.*s@PAGE\n", node->len, node->name);
        printf("  ldr x0, [x0, _%.*s@PAGEOFF]\n", node->len, node->name);
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  ldr x0, [sp], #16\n"); // rhs
        printf("  ldr x1, [sp], #16\n"); // lhs
        printf("  stur %s0, [x1, #0]\n", size_prefix(node->lhs));
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("  ldr x0, [sp], #16\n");
        // エピローグ
        printf("  mov sp, fp\n");
        printf("  ldp fp, lr, [sp], #16\n");
        printf("  ret\n");
        return;
    case ND_IF:
    {
        // 最後に評価した値をスタックトップに残す
        int label = new_label();
        printf("  ; start if expr %03d\n", label);
        gen(node->expr0);
        printf("  ; end if expr %03d\n", label);
        printf("  ldur x0, [sp, #0]\n");
        if (!node->rhs)
        {
            printf("  cbz x0, LEND_%03d\n", label);
            printf("  ldr x0, [sp], #16\n");
            printf("  ; start then %03d\n", label);
            gen(node->lhs);
            printf("LEND_%03d:\n", label);
            printf("  ; end then %03d\n", label);
        }
        else
        {
            printf("  cbz x0, LELSE_%03d\n", label);
            printf("  ldr x0, [sp], #16\n");
            printf("  ; start then %03d\n", label);
            gen(node->lhs);
            printf("  ; end then %03d\n", label);
            printf("  b LEND_%03d\n", label);
            printf("LELSE_%03d:\n", label);
            printf("  ldr x0, [sp], #16\n");
            printf("  ; start else %03d\n", label);
            gen(node->rhs);
            printf("  ; end else %03d\n", label);
            printf("LEND_%03d:\n", label);
        }
        return;
    }
    case ND_WHILE:
    {
        // while (expr0) lhs

        // 条件式の結果をスタックトップに残す
        int label = new_label();
        printf("LBEGIN_%03d:\n", label);
        printf("  ; start while expr %03d\n", label);
        gen(node->expr0);
        printf("  ; end while expr %03d\n", label);
        printf("  ldur x0, [sp, #0]\n");
        printf("  cbz x0, LEND_%03d\n", label);
        printf("  ldr x0, [sp], #16\n");
        printf("  ; start while body %03d\n", label);
        gen(node->lhs);
        printf("  ldr x0, [sp], #16\n");
        printf("  ; end while body %03d\n", label);
        printf("  b LBEGIN_%03d\n", label);
        printf("LEND_%03d:\n", label);
        return;
    }
    case ND_FOR:
    {
        // for (expr0; expr1; rhs) lhs

        // 条件式 expr1がある場合、条件式の結果をスタックトップに残す
        // ない場合、returnで抜けるはずなので考えない
        int label = new_label();
        if (node->expr0)
        {
            printf("  ; start for expr0 %03d\n", label);
            gen(node->expr0);
            printf("  ; end for expr0 %03d\n", label);
            printf("  ldr x0, [sp], #16\n");
        }
        printf("LBEGIN_%03d:\n", label);
        if (node->expr1)
        {
            printf("  ; start for expr1 %03d\n", label);
            gen(node->expr1);
            printf("  ; end for expr1 %03d\n", label);
            printf("  ldur x0, [sp, #0]\n");
            printf("  cbz x0, LEND_%03d\n", label);
            printf("  ldr x0, [sp], #16\n");
        }
        printf("  ; start for lhs %03d\n", label);
        gen(node->lhs);
        printf("  ldr x0, [sp], #16\n");
        printf("  ; end for lhs %03d\n", label);
        if (node->rhs)
        {
            printf("  ; start for rhs %03d\n", label);
            gen(node->rhs);
            printf("  ldr x0, [sp], #16\n");
            printf("  ; end for rhs %03d\n", label);
        }
        printf("  b LBEGIN_%03d\n", label);
        printf("LEND_%03d:\n", label);
        return;
    }
    case ND_BLOCK:
        // 最後に評価した値をスタックトップに残す。
        // bodyが空の場合は0をプッシュしておく。
        printf("  str xzr, [sp, #-16]!\n");
        for (int i = 0; node->body[i]; i++)
        {
            printf("  ldr x0, [sp], #16\n");
            gen(node->body[i]);
        }
        return;
    case ND_CALL:
        for (int i = 0; i < node->nargs; i++)
        {
            gen(node->args[i]);
        }
        for (int i = 0; i < node->nargs; i++)
        {
            printf("  ldur X%d, [sp, #%d]\n", i, 16 * (node->nargs - i - 1));
        }
        printf("  add sp, sp, #%d\n", 16 * node->nargs);
        printf("  bl _%.*s\n", node->len, node->name);
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_FUNDEF:
    {
        printf("  .global  _main  ; -- start function %.*s\n", node->len, node->name);
        printf("  .p2align  2\n");
        printf("_%.*s:\n", node->len, node->name);

        // プロローグ
        // 変数分の領域を16バイト境界で確保する
        size_t offset = local_variables_offset(node);
        printf("  sub sp, sp, #%lu\n", offset + 16);
        printf("  stp fp, lr, [sp, #%lu]\n", offset);
        printf("  add fp, sp, #%lu\n", offset);
        for (int i = 0; i < node->nargs; i++)
        {
            printf("  stur %s%d, [fp, #-%d]\n", size_prefix(node->args[i]), i, node->args[i]->offset);
        }

        // 先頭の式から順にコード生成
        for (int i = 0; node->body[i]; i++)
        {
            gen(node->body[i]);

            // 式の評価結果としてスタックに一つの値が残っている
            // はずなので、スタックが溢れないようにポップしておく
            printf("  ldr x0, [sp], #16\n");
        }

        // エピローグ
        // 最後の式の結果がx0に残っているのでそれが返り値になる
        printf("  mov sp, fp\n");
        printf("  ldp fp, lr, [sp], #16\n");
        printf("  ret\n");
        printf("; -- end function %.*s\n", node->len, node->name);
        return;
    }
    case ND_ADDR:
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        printf("  ldr x0, [sp], #16\n");
        printf("  ldur x0, [x0, #0]\n");
        printf("  str x0, [sp, #-16]!\n");
        return;
    case ND_VARDEF:
        // dummy push
        printf("  str xzr, [sp, #-16]!\n");
        return;
    case ND_GVARDEF:
        printf("  .global _%.*s\n", node->len, node->name);
        printf("  .p2align 2\n");
        printf("_%.*s:\n", node->len, node->name);

        if (node->type->ty != ARRAY)
        {
            printf("  .long 0\n");
            return;
        }

        for (int i = 0; i < node->type->array_size; i++)
        {
            printf("  .long 0\n");
        }
        return;
    default: // 警告抑制
        break;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  ldr %s0, [sp], #16\n", size_prefix(node->rhs));
    printf("  ldr %s1, [sp], #16\n", size_prefix(node->lhs));

    // x1 op x0

    switch (node->kind)
    {
    case ND_ADD:
        if (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)
        {
            int size = get_size(node->lhs->type->ptr_to);
            printf("  mov x2, #%d\n", size);
            printf("  mul x0, x0, x2\n");
        }
        printf("  add x0, x1, x0\n");
        break;
    case ND_SUB:
        if (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)
        {
            int size = get_size(node->lhs->type->ptr_to);
            printf("  mov x2, #%d\n", size);
            printf("  mul x0, x0, x2\n");
        }
        printf("  sub x0, x1, x0\n");
        break;
    case ND_MUL:
        printf("  mul x0, x1, x0\n");
        break;
    case ND_DIV:
        printf("  sdiv x0, x1, x0\n");
        break;
    case ND_LT:
        printf("  cmp x1, x0\n");
        printf("  cset x0, lo\n");
        break;
    case ND_LE:
        printf("  cmp x1, x0\n");
        printf("  cset x0, ls\n");
        break;
    case ND_EQ:
        printf("  cmp x1, x0\n");
        printf("  cset x0, eq\n");
        break;
    case ND_NE:
        printf("  cmp x1, x0\n");
        printf("  cset x0, ne\n");
        break;
    default: // 警告抑制
        break;
    }

    printf("  str x0, [sp, #-16]!\n");
}
