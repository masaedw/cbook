#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./hypcc "$input" > tmp.s
  cc -o tmp tmp.s ext.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert_fail() {
  input="$1"

  ./hypcc "$input" > tmp.s
  result="$?"

  if [ "$result" != 0 ]; then
    echo "=> failure as expected"
  else
    echo "expected to fail, but succeeded"
    exit 1
  fi
}

assert_fail "main() {}"

assert 0 "int main() { 0; }"
assert 42 "int main() { 42; }"
assert 21 "int main() { 5+20-4; }"
assert 41 "int main() {  12 + 34 - 5 ; }"
assert 47 "int main() { 5+6*7; }"
assert 15 "int main() { 5*(9-6); }"
assert 4 "int main() { (3+5)/2; }"
assert 5 "int main() { return -5 + 10; }"
assert 5 "int main() { return + 5; }"
assert 1 "int main() { 1 == 1; }"
assert 0 "int main() { 1 != 1; }"
assert 0 "int main() { 1 < 1; }"
assert 1 "int main() { 1 < 2; }"
assert 0 "int main() { 1 > 1; }"
assert 1 "int main() { 2 > 1; }"
assert 0 "int main() { 1 <= 0; }"
assert 1 "int main() { 1 <= 1; }"
assert 1 "int main() { 1 <= 2; }"
assert 0 "int main() { 0 >= 1; }"
assert 1 "int main() { 1 >= 1; }"
assert 1 "int main() { 2 >= 1; }"
assert 2 "int main() { int a; a = 1; int b; b = 2; a * b; }"
assert 50 "int main() { int c; c = 5; int x; x = 10; c * x; }"
assert 50 "int main() { int abc; abc = 50; abc; }"
assert 0 "int main() { return 0; }"
assert 50 "int main() { return 50; }"
assert 1 "int main() { 5 * 5; return 1; 5; }"
assert 0 "int main() { if (1) 0; }"
assert 5 "int main() { if(10) 5; }"
assert 1 "int main() { if (0) return 5; 1; }"
assert 1 "int main() { if (1) 1; else 2; }"
assert 2 "int main() { int a; if (0) a = 1; else a = 2; a; }"
assert 1 "int main() { int a; a = 5; a = a - 4; a; }"
assert 1 "int main() { int a; a = 5; if (a == 5) return 1; else 5; }"
assert 1 "int main() { int a; a = 5; if (a == 5) return 1; else 5; }"
assert 1 "int main() { int a; a = 5; if (a == 0) return 0; else a = a - 4; a; }"
assert 0 "int main() { int a; while (0) a = 5; 0; }"
assert 0 "int main() { while (1) return 0; 1; }"
assert 0 "int main() { int a; a = 5; while (a) return 0; 5; }"
assert 1 "int main() { int a; a = 5; while (a) if (a == 0) return 0; else return 1; 5; }"
assert 0 "int main() { int a; a = 5; while (a) if (a == 1) return 0; else a = a - 1; 5; }"
assert 1 "int main() { for (;;) return 1; }"
assert 0 "int main() { int a; for (a = 1; a == 0; a = a - 1) 1; 0; }"
assert 0 "int main() { int a; for (a = 1; a != 0; a = a - 1) 1; 0; }"
assert 5 "int main() { int a; for (a = 5; a != 0; ) if (a == 5) return 5; else a = 1; }"
assert 5 "int main() { int a; for (a = 5; a != 0; ) if (a == 2) return 5; else a = a - 1; }"
assert 5 "int main() { int a; int b; b = 0; for (a = 5; a != 0; a = a - 1) b = b + 1; b; }"
assert 5 "int main() { {1;2;3;4;5;} }"
assert 0 "int main() { if(1) {0;} else {1;} }"
assert 1 "int main() { { return 1; 5;} }"
assert 5 "int main() { for (;;) { return 5; } }"
assert 5 "int main() { while (1) { return 5; } }"
assert 5 "int main() { noargtest(); }"
assert 1 "int main() { argtest1(1); }"
assert 2 "int main() { argtest2(1, 2); }"
assert 3 "int main() { argtest3(1, 2, 3); }"
assert 4 "int main() { argtest4(1, 2, 3, 4); }"
assert 5 "int main() { argtest5(1, 2, 3, 4, 5); }"
assert 6 "int main() { argtest6(1, 2, 3, 4, 5, 6); }"
assert 7 "int main() { argtest7(1, 2, 3, 4, 5, 6, 7); }"
assert 8 "int main() { argtest8(1, 2, 3, 4, 5, 6, 7, 8); }"
# parse error
# assert 9 "int main() { argtest9(1, 2, 3, 4, 5, 6, 7, 8, 9); }"
assert 5 "int f() { return 5; } int main() { return f(); }"
assert 8 "int add(int a, int b) { return a + b; } int sub(int x, int y) { return x - y; } int main() { return sub(add(3, 8), 3); }"
assert 89 "int fib(int n) { if (n == 0) { return 0; } if (n == 1) { return 1; } return fib(n - 2) + fib(n - 1); } int main() { return fib(11); }"
# スタック上の並びはポインタ演算で取れるとは限らない
# assert 3 "int main() { int x; x = 3; int y; y = 5; int *z; z = &y - 8; return *z; }"
assert 0 "int main() { int **a; int *b; int c; c = 0; return c; }"
assert 3 "int main() { int x; int *y; y = &x; *y = 3; return x; }"

# step 19
assert 8 "int main() { int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; print_int(*q); q = p + 3; return *q; }"
# intは32ビット分だけ保存する
assert 4 "int main() { int *p; alloc4(&p, 1, 2, 4, 8); *(p + 1) = 2147483647; return *(p + 2); }"

# step 20
assert 4 "int main() { return sizeof(5); }"
assert 8 "int main() { int *a; return sizeof(a); }"
assert 8 "int main() { int a; return sizeof(&a); }"
assert 4 "int main() { int a; return sizeof(*&a); }"
assert 4 "int main() { return sizeof(5 + 3); }"
assert 4 "int main() { return sizeof(main()); }"

# step 21
assert 40 "int main() { int a[10]; return sizeof a; }"
assert 1 "int main() { int a[10]; *a = 1; return *a; }"
assert 3 "int main() { int a[10]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }"
assert 11 "int main() { int a[10]; *a = 1; *(a + 1) = 2; *(a + 2) = 3; *(a + 3) = 4; *(a + 4) = 5; *(a + 5) = 6; *(a + 6) = 7; *(a + 7) = 8; *(a + 8) = 9; *(a + 9) = 10; int *p; p = a; return *p + *(p + 9); }"

# step 22
assert 3 "int main() { int a[10]; a[0] = 1; a[1] = 2; int *p; p = a; return *p + p[1]; }"
assert 11 "int main() { int a[10]; a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5; a[5] = 6; a[6] = 7; a[7] = 8; a[8] = 9; a[9] = 10; int *p; p = a; return p[0] + p[9]; }"

# step 23
assert 1 "int a; int b[10]; int *c[5]; int main() { return 1; }"
assert 4 "int a; int b[10]; int *c[5]; int main() { a = 4; return a; }"
assert 9 "int a; int b[10]; int *c[5]; int main() { c[0] = b; a = 5; *c[0] = 4; return b[0] + a; }"

# step 24
assert 1 "int main() { char a; return sizeof(a); }"
assert 10 "int main() { char a[10]; return sizeof(a); }"
assert 3 "int main() { char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; }"
assert 4 "int a; int b[10]; int *c[5]; int main() { a = 4; return a; }"
assert 4 "char a; char b[10]; char *c[5]; int main() { a = 4; return a; }"

# step 25
assert 1 "int main() { char *a; a = \"1234\"; return a[2] == 51; }"

echo OK
