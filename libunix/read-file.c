#include "string.h"
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s.
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
//
void *read_file(unsigned *size, const char *name) {
  // How:
  //    - use stat() to get the size of the file.
  //    - round up to a multiple of 4.
  //    - allocate a buffer
  //    - zero pads to a multiple of 4.
  //    - read entire file into buffer (read_exact())
  //    - fclose() the file descriptor
  //    - make sure any padding bytes have zeros.
  //    - return it.
  printf("The file is named %s and the size is %d \n", name, *size);
  struct stat buf[10000];
  char *result = NULL;
  if (stat(name, buf) != 0) {
    panic("Error reading file with errno %s \n", strerror(errno));
    return result;
  }
  off_t bufSize = buf->st_size;
  *size = bufSize;
  unsigned toAdd = bufSize % 4;
  bufSize = (4 - toAdd) + bufSize;
  result = malloc(bufSize);
  if (result == NULL) {
    panic("error mallocing data");
  };
  FILE *file = fopen(name, "r");
  char temp[2];
  if (read(file->_file, result, bufSize) < *size) {
    panic("issue with reading!");
  }
  // char* res;
  // int i = 0;
  // while ((res = fgets(temp, sizeof(temp), file))) {
  //     int j = 0;
  //     while (1) {
  //         result[i] = temp[j];
  //         if (temp[j] == '\n' || temp[j] == EOF || j == sizeof(temp) - 1) {
  //             break;
  //         }
  //         //printf("\n temp[i] is %hhd and j is %d \n", result[i], j);
  //         i += 1;
  //         j += 1;
  //     }
  // }
  // if (!feof(file)) {
  //     panic("issue with reading file \n");
  // }
  // result[i] = '\0';
  // printf("result is %s \n", result);
  fclose(file);
  return result;
}
