#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
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

echo OK
