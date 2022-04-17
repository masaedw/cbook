#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    char *p = argv[1];

    printf("\t.section\t__TEXT,__text,regular,pure_instructions\n");
    printf("\t.build_version macos, 12, 0\tsdk_version 12, 3\n");
    printf("\t.global\t_main\n");
    printf("\t.p2align\t2\n");
    printf("_main:\n");
    printf("\tmov w0, #%ld\n", strtol(p, &p, 10));

    while (*p)
    {
        if (*p == '+')
        {
            p++;
            printf("\tadd w0, w0, #%ld\n", strtol(p, &p, 10));
            continue;
        }

        if (*p == '-')
        {
            p++;
            printf("\tadd w0, w0, #-%ld\n", strtol(p, &p, 10));
            continue;
        }

        fprintf(stderr, "予期しない文字です: '%c'\n", *p);
        return 1;
    }

    printf("\tret\n");
    return 0;
}
