// engler, cs140e: your code to find the tty-usb device on your laptop.
#include "dirent.h"
#include "libunix.h"
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
    "ttyUSB",       // linux
    "ttyACM",       // linux
    "cu.SLAB_USB",  // mac os
    "cu.usbserial", // mac os
                    // if your system uses another name, add it.
    0};

static int filter(const struct dirent *d) {
  // scan through the prefixes, returning 1 when you find a match.
  // 0 if there is no match.
  for (int i = 0; i < 4; i++) {
    if (strncmp(d->d_name, ttyusb_prefixes[i], strlen(ttyusb_prefixes[i])) ==
        0) {
      return 1;
    }
  }
  return 0;
}

static int (*FUNCTION)(const struct dirent **,
                       const struct dirent **) = alphasort;

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
  printf("finding ttyusb\n");
  struct dirent **namelist;
  int n = -1;
  n = scandir("/dev", &namelist, filter, FUNCTION);
  if (n > 1) {
    panic("There are too many usb options");
  } else if (n < 0) {
    panic("issue scanning directory with error %s\n", strerror(errno));
  }
  char *name = malloc(100);
  strcat(name, "/dev/");
  strcat(name, (*namelist)->d_name);
  return name;
}

static int recentCompare(const struct dirent **d1, const struct dirent **d2) {
  struct stat buf1[10000];
  __uint32_t timeOne = 0;
  __uint32_t timeTwo = 0;
  char *name1 = malloc(100);
  strcat(name1, "/dev/");
  strcat(name1, (*d1)->d_name);
  if (stat(name1, buf1) != 0) {
    panic("Error reading file");
  }
  long time1 = buf1->st_mtime;
  struct stat buf2[10000];
  char *name2 = malloc(100);
  strcat(name2, "/dev/");
  strcat(name2, (*d1)->d_name);
  if (stat(name2, buf2) != 0) {
    panic("Error reading file");
  }
  long time2 = buf1->st_mtime;
  return time1 < time2;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time
// returned by stat.
char *find_ttyusb_last(void) {
  printf("finding ttyusb LAST\n");
  FUNCTION = recentCompare;
  return find_ttyusb();
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) { unimplemented(); }
