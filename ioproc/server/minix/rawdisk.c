#include "helios.h"

/**
*** number_rawdisks should return the number of drives or 0 
***
*** size_rawdisk should return the size of the specified drive in sectors
***
*** read_rawdisk and write_rawdisk should do the appropriate disk I/O,
*** returning 1 for success or 0 for failure
***
**/

static int disk0 = 0;

PUBLIC int fn(number_rawdisks, (void)) {
  disk0 = open("/home/helios/0.disk", O_RDWR);
  if (disk0 > 0)
    return 1;
  return 0;
}

PUBLIC word fn(size_rawdisk, (int)) {
  return 65536;
}

PUBLIC word fn(read_rawdisk, (int disk, word first_sec, word no_secs, byte *buff )) {
  size_t ret;
  if (lseek(disk0, (off_t)(first_sec * 512), SEEK_SET) < 0)
    return errno;
  if ((ret = read(disk0, buf, (size_t)(no_secs * 512))) < 0)
    return errno;
  return ret;
}

PUBLIC word fn(write_rawdisk, (int disk, word first_sec, word no_secs, byte *buff )) {
  size_t ret;
  if (lseek(disk0, (off_t)(first_sec * 512), SEEK_SET) < 0)
    return errno;
  if ((ret = write(disk0, buf, (size_t)(no_secs * 512))) < 0)
    return errno;
  return ret;
}
