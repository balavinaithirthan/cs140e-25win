// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define error(args...)                                                         \
  do {                                                                         \
    fprintf(stderr, ##args);                                                   \
    exit(1);                                                                   \
  } while (0)

// a not very interesting compile: throw the input into a
// temporary file and then call gcc on the result.

void print_debug_string(const char *str) {
  while (*str) {
    switch (*str) {
    case '\n':
      printf("\\n"); // Makes newlines visible
      break;
    case '\t':
      printf("\\t"); // Makes tabs visible
      break;
    default:
      putchar(*str); // Print other characters normally
    }
    str++;
  }
  printf("\n");
}

static void compile(char *program, char *outname) {
  FILE *fp = fopen("./temp-out.c", "w");
  assert(fp);
  FILE *output = fp;

  static char login_sig[] = "int login(char *user) {";
  static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";
  static char first_line[] =
      "static void compile(char *program, char *outname) {";
  static char second_line[] = "  FILE *fp = fopen(\"./temp-out.c\", \"w\");";
  static char third_line[] = "  assert(fp);";
  static char compile_attack[] =
      "printf(\"%s: could have run your attack here!!\\n\", __FUNCTION__);";
  char loginNextLineGet = 0;
  char compilerNextLineGet = 0;
  char *line = strtok(program, "\n");
  while (line != NULL) {
    if (loginNextLineGet == 1) {
      fprintf(output, "%s", login_attack);
      loginNextLineGet = 0;
    }
    if (compilerNextLineGet == 3) {
      // print_debug_string(line);
      fprintf(output, "%s", compile_attack);
      compilerNextLineGet = 0;
      line = strtok(NULL, "\n");
      continue;
    }
    fprintf(output, "%s\n", line);
    if (strcmp(line, login_sig) == 0) {
      loginNextLineGet = 1;
    }
    if (strcmp(line, first_line) == 0 && compilerNextLineGet == 0) {
      compilerNextLineGet += 1;
    } else if (strcmp(line, second_line) == 0 && compilerNextLineGet == 1) {
      compilerNextLineGet += 1;
    } else if (strcmp(line, third_line) == 0 && compilerNextLineGet == 2) {
      compilerNextLineGet += 1;
    }
    line = strtok(NULL, "\n"); // Continue to tokenize the string
  }

  fclose(fp);
  char debugLine[100];
  FILE *debug = fopen("./temp-out.c", "r");
  assert(debug);
  while (fgets(debugLine, sizeof(debugLine), debug) != NULL) {
    printf("%s", debugLine);
  }
  fclose(debug);

  /************************************************************
   * don't modify the rest.
   */

  // gross, call gcc.
  char buf[1024];

  sprintf(buf, "gcc ./temp-out.c -o %s -Wno-nullability-completeness",
          outname); // how this works
  if (system(buf) != 0)
    error("system failed\n");
}

#define N 8 * 1024 * 1024
static char buf[N + 1];

int main(int argc, char *argv[]) {
  if (argc != 4)
    error("expected 4 arguments have %d\n", argc);
  if (strcmp(argv[2], "-o") != 0)
    error("expected -o as second argument, have <%s>\n", argv[2]);

  // read in the entire file.
  int fd;
  if ((fd = open(argv[1], O_RDONLY)) < 0)
    error("file <%s> does not exist\n", argv[1]);

  int n;
  if ((n = read(fd, buf, N)) < 1)
    error("invalid read of file <%s>\n", argv[1]);
  if (n == N)
    error("input file too large\n");

  // WHY DOES BUF NULL TERMINATED??

  // "compile" it.
  compile(buf, argv[3]);
  return 0;
}
