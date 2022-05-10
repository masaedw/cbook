typedef int x;

int f() {
  //  sizeof(x);
  x x[10];
  x[1] = 1;
  return sizeof(x);
}
