int f();

int x() {
  int a = 1;
  int b = f();
  return a + b;
}

int y() {
  f();
  return 0;
}

int z() {
  return f();
}
