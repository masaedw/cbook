#include <stdio.h>

int main(int argc, char **argv)
{
    int a;
    int b;
    printf("%p %p %ld\n", &a - 1, &b, (long)&a - (long)&b);
}
