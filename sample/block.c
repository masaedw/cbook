int add(int a, int b);

void f(void) {
  int a;
  {
    int b;
    a = 1;
    b = 2;
    add(a, b);
  }
  {
    int c = 5;
    add(a, c);
  }
}
