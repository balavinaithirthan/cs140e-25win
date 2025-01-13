// convert the contents of stdin to their ASCII values (e.g.,
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>
#include <string.h>

int main(void) {
  // maybe dynamic
  char buffer[1000000];
  char result[1000000];
  int j = 0;
  printf("char prog[] = {\n");
  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    int i = 0;
    while (buffer[i] != '\0') {
      printf("\t%d,%c", buffer[i], (j + 1) % 8 == 0 ? '\n' : ' ');
      result[j] = buffer[i];
      i += 1;
      j += 1;
    }
  }
  printf("0 };\n");

  result[j] = '\0'; // null terminates
  printf("%s", result);
  return 0;
}
