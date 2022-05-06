#include <stdio.h>

extern long add(long a, long b);

long f64() {
  long a = 1;
  long b = 2;
  return add((long)&a, (long)&b);
}

long f32() {
  int a = 1;
  int b = 2;
  return add((long)&a, (long)&b);
}
