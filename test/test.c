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

  int a;
  a = 1;
  int b;
  b = 2;
  expect(2, a * b);

  int c;
  c = 5;
  int x;
  x = 10;
  expect(50, c * x);

  int d;
  if (0)
    d = 1;
  else
    d = 2;
  expect(2, d);

  int e;
  e = 5;
  e = e - 4;
  expect(1, e);

  int f;
  f = 5;
  if (f == 0)
    return 0;
  else
    f = f - 4;
  expect(1, f);

  int g;
  g = 0;
  while (0)
    g = 5;
  expect(0, g);

  expect(5, ({
           int xx;
           int *p;
           p = &xx;
           xx = 5;
           *p;
         }));
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

  expect(0, ({
           int **h;
           int *i;
           int j;
           j = 0;
           j;
         }));
  expect(3, ({
           int k;
           int *l;
           l = &k;
           *l = 3;
           k;
         }));

  // step 19
  expect(8, ({
           int *m;
           alloc4(&m, 1, 2, 4, 8);
           int *n;
           n = m + 2;
           print_int(*n);
           n = m + 3;
           *n;
         }));
  // intは32ビット分だけ保存する
  expect(4, ({
           int *o;
           alloc4(&o, 1, 2, 4, 8);
           *(o + 1) = 2147483647;
           *(o + 2);
         }));

  // step 20
  expect(4, sizeof(5));
  int *a20;
  expect(8, sizeof(a20));
  int b20;
  expect(8, sizeof(&b20));
  expect(4, sizeof(*&b20));
  expect(4, sizeof(5 + 3));
  expect(4, sizeof(main()));

  // step 21
  int a21[10];
  expect(40, sizeof a21);
  *a21 = 1;
  expect(1, *a21);
  *a21 = 1;
  *(a21 + 1) = 2;
  int *p21;
  p21 = a21;
  expect(3, *p21 + *(p21 + 1));

  // step 22
  int a22[10];
  a22[0] = 1;
  a22[1] = 2;
  int *p22;
  p22 = a22;
  expect(3, *p22 + p22[1]);
  int b22[10];
  b22[0] = 1;
  b22[1] = 2;
  b22[2] = 3;
  b22[3] = 4;
  b22[4] = 5;
  b22[5] = 6;
  b22[6] = 7;
  b22[7] = 8;
  b22[8] = 9;
  b22[9] = 10;
  p22 = b22;
  expect(11, p22[0] + p22[9]);

  // step 23
  g_a = 4;
  expect(4, g_a);
  g_c[0] = g_b;
  g_a = 5;
  *g_c[0] = 4;
  expect(9, g_b[0] + g_a);

  // step 24
  char x24;
  x24 = -1;
  expect(-1, x24);
  char a24;
  expect(1, sizeof(a24));
  char b24[10];
  expect(10, sizeof(b24));
  char c24[3];
  c24[0] = -1;
  c24[1] = 2;
  int d24;
  d24 = 4;
  expect(3, c24[0] + d24);
  g_d[0] = 1;
  g_d[1] = 2;
  g_d[2] = 3;
  expect(6, g_d[0] + g_d[1] + g_d[2]);

  // step25
  char *a25;
  a25 = "1234";
  expect(51, a25[2]);

  puts("OK");

  return 0;
}
