// int puts(char *s);
// int print2d(char *fmt, int a, int b);

int expect(int expected, int actual) {
  if (expected != actual) {
    print2d("%d expected, but got %d", expected, actual);
    exit(1);
  } else {
    print2d("%d => %d", expected, actual);
  }
}

int while_test0() {
  while (1)
    return 0;
  return 1;
}

int while_test1() {
  int a;
  a = 5;
  while (a)
    return 0;
  return 5;
}

int while_test2() {
  int a;
  a = 5;
  while (a)
    if (a == 0)
      return 0;
    else
      return 1;
  return 5;
}

int while_test3() {
  int a;
  a = 5;
  while (a)
    if (a == 1)
      return 0;
    else
      a = a - 1;
  return 5;
}

int for_test0() {
  for (;;)
    return 1;
}

int for_test1() {
  int a;
  for (a = 1; a == 0; a = a - 1)
    1;
  return 0;
}

int for_test2() {
  int a;
  for (a = 1; a != 0; a = a - 1)
    1;
  return 0;
}

int for_test3() {
  int a;
  int b;
  b = 0;
  for (a = 5; a != 0; a = a - 1)
    b = b + 1;
  return b;
}

int for_test4() {
  int a;
  for (a = 5; a != 0;)
    if (a == 5)
      return 5;
    else
      a = 1;
}

int for_test5() {
  int a;
  for (a = 5; a != 0;)
    if (a == 2)
      return 5;
    else
      a = a - 1;
}

int add(int a, int b) {
  return a + b;
}

int sub(int x, int y) {
  return x - y;
}

int fib(int n) {
  if (n == 0) {
    return 0;
  }
  if (n == 1) {
    return 1;
  }
  return fib(n - 2) + fib(n - 1);
}

int g_a;
int g_b[10];
int *g_c[5];
char g_d[10];
char *g_e[5];
char g_f;

int main() {
  // clang-format off

  expect(0, 0);
  expect(42, 42);
  expect(21, 5 + 20 - 4);
  expect(41, 12 + 34 - 5);
  expect(47, 5 + 6 * 7);
  expect(15, 5 * (9 - 6));
  expect(4, (3 + 5) / 2);
  expect(1, 1 == 1);
  expect(0, 1 != 1);
  expect(0, 1 < 1);
  expect(1, 1 < 2);
  expect(0, 1 > 1);
  expect(1, 2 > 1);
  expect(0, 1 <= 0);
  expect(1, 1 <= 1);
  expect(1, 1 <= 2);
  expect(0, 0 >= 1);
  expect(1, 1 >= 1);
  expect(1, 2 >= 1);


  expect(2, ({ int a; a = 1; int b; b = 2; a * b; }));
  expect(50, ({ int a; a = 5; int b; b = 10; a * b; }));
  expect(2, ({ int a; if (0) a = 1; else a = 2; a; }));
  expect(1, ({ int a; a = 5; a = a - 4; a; }));
  expect(1, ({ int a; a = 5; if (a == 0) return 0; else a = a - 4; a; }));
  expect(0, ({ int a; a = 0; while (0) a = 5; a; }));
  expect(5, ({ int x; int *p; p = &x; x = 5; *p; }));
  expect(0, while_test0());
  expect(0, while_test1());
  expect(1, while_test2());
  expect(0, while_test3());
  expect(1, for_test0());
  expect(0, for_test1());
  expect(0, for_test2());
  expect(5, for_test3());
  expect(5, for_test4());
  expect(5, for_test5());

  expect(5, noargtest());
  expect(1, argtest1(1));
  expect(2, argtest2(1, 2));
  expect(3, argtest3(1, 2, 3));
  expect(4, argtest4(1, 2, 3, 4));
  expect(5, argtest5(1, 2, 3, 4, 5));
  expect(6, argtest6(1, 2, 3, 4, 5, 6));
  expect(7, argtest7(1, 2, 3, 4, 5, 6, 7));
  expect(8, argtest8(1, 2, 3, 4, 5, 6, 7, 8));

  expect(8, sub(add(3, 8), 3));
  expect(89, fib(11));

  expect(0, ({ int **a; int *b; int c; c = 0; c; }));
  expect(3, ({ int a; int *b; b = &a; *b = 3; a; }));

  // step 19
  expect(8, ({ int *a; alloc4(&a, 1, 2, 4, 8); int *b; b = a + 2; print_int(*b); b = a + 3; *b; }));
  // intは32ビット分だけ保存する
  expect(4, ({ int *a; alloc4(&a, 1, 2, 4, 8); *(a + 1) = 2147483647; *(a + 2); }));

  // step 20
  expect(4, sizeof(5));
  expect(8, ({ int *a; sizeof(a); }));
  expect(8, ({ int a; sizeof(&a); }));
  expect(4, ({ int a; sizeof(*&a); }));
  expect(4, sizeof(5 + 3));
  expect(4, sizeof(main()));

  // step 21
  expect(40,({ int a[10]; sizeof a;}));
  expect(1, ({ int a[10]; *a = 1; *a;}));
  expect(3, ({ int a[10]; *a = 1; *(a + 1) = 2; int *p; p = a; *p + *(p + 1);}));

  // step 22
  expect(3, ({ int a[10]; a[0] = 1; a[1] = 2; int *p; p = a; *p + p[1]; }));
  expect(11, ({ int a[10]; a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5; a[5] = 6; a[6] = 7; a[7] = 8; a[8] = 9; a[9] = 10; int *p; p = a; p[0] + p[9]; }));

  // step 23
  expect(4, ({ g_a = 4; g_a; }));
  expect(9, ({ g_c[0] = g_b; g_a = 5; *g_c[0] = 4; g_b[0] + g_a; }));

  // step 24

  expect(1, ({ char x; x = -1; expect(-1, x); char a; sizeof(a); }));
  expect(10, ({ char a[10]; sizeof(a);}));
  expect(3, ({ char a[3]; a[0] = -1; a[1] = 2; int b; b = 4; a[0] + b; }));
  expect(6, ({ g_d[0] = 1; g_d[1] = 2; g_d[2] = 3; g_d[0] + g_d[1] + g_d[2]; }));

  // step25
  expect(51, ({ char *a; a = "1234"; a[2]; }));

  // clang-format on

  puts("OK");

  return 0;
}
