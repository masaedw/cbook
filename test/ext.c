#include <stdio.h>
#include <stdlib.h>

int noargtest() {
  printf("OK\n");
  return 5;
}

void print_int(int a) {
  printf("%d\n", a);
}

int argtest1(int a) {
  printf("%d\n", a);
  return 1;
}

int argtest2(int a, int b) {
  printf("%d %d\n", a, b);
  return 2;
}

int argtest3(int a, int b, int c) {
  printf("%d %d %d\n", a, b, c);
  return 3;
}

int argtest4(int a, int b, int c, int d) {
  printf("%d %d %d %d\n", a, b, c, d);
  return 4;
}

int argtest5(int a, int b, int c, int d, int e) {
  printf("%d %d %d %d %d\n", a, b, c, d, e);
  return 5;
}

int argtest6(int a, int b, int c, int d, int e, int f) {
  printf("%d %d %d %d %d %d\n", a, b, c, d, e, f);
  return 6;
}

int argtest7(int a, int b, int c, int d, int e, int f, int g) {
  printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g);
  return 7;
}

int argtest8(int a, int b, int c, int d, int e, int f, int g, int h) {
  printf("%d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h);
  return 8;
}

void alloc4(int **ary, int a, int b, int c, int d) {
  int *x = *ary = malloc(sizeof(int) * 4);
  x[0] = a;
  x[1] = b;
  x[2] = c;
  x[3] = d;
}

void print2d(char *fmt, int a, int b) {
  printf(fmt, a, b);
  printf("\n");
}

void print(char *s) {
  printf("%s\n", s);
}
