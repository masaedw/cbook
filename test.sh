#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
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

assert 0 "main() { 0; }"
assert 42 "main() { 42; }"
assert 21 "main() { 5+20-4; }"
assert 41 "main() {  12 + 34 - 5 ; }"
assert 47 "main() { 5+6*7; }"
assert 15 "main() { 5*(9-6); }"
assert 4 "main() { (3+5)/2; }"
assert 1 "main() { 1 == 1; }"
assert 0 "main() { 1 != 1; }"
assert 0 "main() { 1 < 1; }"
assert 1 "main() { 1 < 2; }"
assert 0 "main() { 1 > 1; }"
assert 1 "main() { 2 > 1; }"
assert 0 "main() { 1 <= 0; }"
assert 1 "main() { 1 <= 1; }"
assert 1 "main() { 1 <= 2; }"
assert 0 "main() { 0 >= 1; }"
assert 1 "main() { 1 >= 1; }"
assert 1 "main() { 2 >= 1; }"
assert 2 "main() { a = 1; b = 2; a * b; }"
assert 50 "main() { c = 5; x = 10; c * x; }"
assert 50 "main() { abc = 50; abc; }"
assert 0 "main() { return 0; }"
assert 50 "main() { return 50; }"
assert 1 "main() { 5 * 5; return 1; 5; }"
assert 0 "main() { if (1) 0; }"
assert 5 "main() { if(10) 5; }"
assert 1 "main() { if (0) return 5; 1; }"
assert 1 "main() { if (1) 1; else 2; }"
assert 2 "main() { if (0) a = 1; else a = 2; a; }"
assert 1 "main() { a = 5; a = a - 4; a; }"
assert 1 "main() { a = 5; if (a == 5) return 1; else 5; }"
assert 1 "main() { a = 5; if (a == 0) return 0; else a = a - 4; a; }"
assert 0 "main() { while (0) a = 5; 0; }"
assert 0 "main() { while (1) return 0; 1; }"
assert 0 "main() { a = 5; while (a) return 0; 5; }"
assert 1 "main() { a = 5; while (a) if (a == 0) return 0; else return 1; 5; }"
assert 0 "main() { a = 5; while (a) if (a == 1) return 0; else a = a - 1; 5; }"
assert 1 "main() { for (;;) return 1; }"
assert 0 "main() { for (a = 1; a == 0; a = a - 1) 1; 0; }"
assert 0 "main() { for (a = 1; a != 0; a = a - 1) 1; 0; }"
assert 5 "main() { for (a = 5; a != 0; ) if (a == 5) return 5; else a = 1; }"
assert 5 "main() { for (a = 5; a != 0; ) if (a == 2) return 5; else a = a - 1; }"
assert 5 "main() { b = 0; for (a = 5; a != 0; a = a - 1) b = b + 1; b; }"
assert 5 "main() { {1;2;3;4;5;} }"
assert 0 "main() { if(1) {0;} else {1;} }"
assert 1 "main() { { return 1; 5;} }"
assert 5 "main() { for (;;) { return 5; } }"
assert 5 "main() { while (1) { return 5; } }"
assert 5 "main() { noargtest(); }"
assert 1 "main() { argtest1(1); }"
assert 2 "main() { argtest2(1, 2); }"
assert 3 "main() { argtest3(1, 2, 3); }"
assert 4 "main() { argtest4(1, 2, 3, 4); }"
assert 5 "main() { argtest5(1, 2, 3, 4, 5); }"
assert 6 "main() { argtest6(1, 2, 3, 4, 5, 6); }"
assert 7 "main() { argtest7(1, 2, 3, 4, 5, 6, 7); }"
assert 8 "main() { argtest8(1, 2, 3, 4, 5, 6, 7, 8); }"
# parse error
# assert 9 "main() { argtest9(1, 2, 3, 4, 5, 6, 7, 8, 9); }"
assert 5 "f() { return 5; } main() { return f(); }"
assert 8 "add(a, b) { return a + b; } sub(x, y) { return x - y; } main() { return sub(add(3, 8), 3); }"
assert 89 "fib(n) { if (n == 0) { return 0; } if (n == 1) { return 1; } return fib(n - 2) + fib(n - 1); } main() { return fib(11); }"

echo OK
