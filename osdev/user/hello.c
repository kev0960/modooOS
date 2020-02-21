int main() {
  int j = 2;
  int i = 3;
  while (1) {
    j++;
    i = i % j;
    j = j * i;
    if (j == 0) j = 2;
  }
  return 0;
}
