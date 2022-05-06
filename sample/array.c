#include <stdio.h>

int main(int argc, char **argv) {
  int a[10];
  printf("%p, %p, %p, %lu\n", a, &a, a + 1, sizeof(a));
}
