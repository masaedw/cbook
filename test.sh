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

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 1 '1 == 1;'
assert 0 '1 != 1;'
assert 0 '1 < 1;'
assert 1 '1 < 2;'
assert 0 '1 > 1;'
assert 1 '2 > 1;'
assert 0 '1 <= 0;'
assert 1 '1 <= 1;'
assert 1 '1 <= 2;'
assert 0 '0 >= 1;'
assert 1 '1 >= 1;'
assert 1 '2 >= 1;'
assert 2 'a = 1; b = 2; a * b;'
assert 50 'c = 5; x = 10; c * x;'
assert 50 'abc = 50; abc;'
assert 0 "return 0;"
assert 50 "return 50;"
assert 1 "5 * 5; return 1; 5;"
assert 0 "if (1) 0;"
assert 5 "if(10) 5;"
assert 1 "if (0) return 5; 1;"
assert 1 "if (1) 1; else 2;"
assert 2 "if (0) a = 1; else a = 2; a;"
assert 1 "a = 5; a = a - 4; a;"
assert 1 "a = 5; if (a == 5) return 1; else 5;"
assert 1 "a = 5; if (a == 0) return 0; else a = a - 4; a;"
assert 0 "while (0) a = 5; 0;"
assert 0 "while (1) return 0; 1;"
assert 0 "a = 5; while (a) return 0; 5;"
assert 1 "a = 5; while (a) if (a == 0) return 0; else return 1; 5;"
assert 0 "a = 5; while (a) if (a == 1) return 0; else a = a - 1; 5;"
assert 1 "for (;;) return 1;"
assert 0 "for (a = 1; a == 0; a = a - 1) 1; 0;"
assert 0 "for (a = 1; a != 0; a = a - 1) 1; 0;"
assert 5 "for (a = 5; a != 0; ) if (a == 5) return 5; else a = 1;"
assert 5 "for (a = 5; a != 0; ) if (a == 2) return 5; else a = a - 1;"
assert 5 "b = 0; for (a = 5; a != 0; a = a - 1) b = b + 1; b;"
assert 5 "{1;2;3;4;5;}"
assert 0 "if(1) {0;} else {1;}"
assert 1 "{ return 1; 5;}"
assert 5 "for (;;) { return 5; }"
assert 5 "while (1) { return 5; }"
assert 5 "noargtest();"
assert 1 "argtest1(1);"
assert 2 "argtest2(1, 2);"
assert 3 "argtest3(1, 2, 3);"
assert 4 "argtest4(1, 2, 3, 4);"
assert 5 "argtest5(1, 2, 3, 4, 5);"
assert 6 "argtest6(1, 2, 3, 4, 5, 6);"
assert 7 "argtest7(1, 2, 3, 4, 5, 6, 7);"
assert 8 "argtest8(1, 2, 3, 4, 5, 6, 7, 8);"
# parse error
# assert 9 "argtest9(1, 2, 3, 4, 5, 6, 7, 8, 9);"

echo OK
