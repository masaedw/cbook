enum AA { A, B, C };

enum AA f() {
  enum { X, Y, Z, A };
  typedef int X;
  return A;
}
