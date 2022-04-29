#include <stdio.h>

int main(int argc, char **argv)
{
    int a[10];
    printf("%p, %p, %lu\n", a, a + 1, sizeof(a));
}
