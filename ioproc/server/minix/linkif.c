/*
 * linklib low level functions for MINIX
 */

#define Linklib_Module

#include "../helios.h"

#include <sys/types.h>
#include <sys/ioctl.h>

#define FAIL 1
#define SUCCESS 0

#define link_fd (link_table[current_link].fildes)

static int current_timeout = -1;

void minix_init_link() {

  number_of_links = 1;
  strcpy(link_table[0].link_name, "/dev/link0");
}

int minix_open_link(int idx) {

  link_table[idx].fildes = open(link_table[idx].link_name, O_RDWR);
  if (link_table[idx].fildes eq -1)
    return 0;

#if 0
  if (get_config("no_dma") ne (char *) NULL)
    ioctl(link_table[idx].fildes, B004NODMA);
#endif

  link_table[idx].flags |= Link_flags_not_selectable;

  return link_table[idx].fildes;
}

void minix_free_link(int idx) {

  if (link_table[idx].fildes ne -1)
    close(link_table[idx].fildes);

  link_table[idx].fildes = -1;
}

void minix_reset_processor() {

  if (ioctl(link_fd, B004RESET) eq -1)
    ServerDebug("Warning: failed to reset processor");
}

void minix_analyse_processor() {

  if (ioctl(link_fd, B004ANALYSE) eq -1)
    ServerDebug("Warning: failed to set processor to analyse mode");
}

WORD minix_byte_from_link(UBYTE *data) {

  /* timeout often defaults to a fifth of a second */
  if (current_timeout != 2) {
    current_timeout = 2;
    ioctl(link_fd, B004SETTIMEOUT, &current_timeout);
  }

  if (read(link_fd, data, 1) != 1)
    return FAIL;
  
  return SUCCESS;
}

WORD minix_byte_to_link(int data) {

  /* timeout often defaults to two seconds */
  if (current_timeout != 20) {
    current_timeout = 20;
    ioctl(link_fd, B004SETTIMEOUT, &current_timeout);
  }

  if (write(link_fd, &data, 1) != 1)
    return FAIL;
  
  return SUCCESS;
}

WORD minix_send_block(int count, BYTE *data, int timeout) {
  int sent, ret;
  BYTE *ptr;

  /* timeout often defaults to five seconds */
  timeout = 50;
  if (timeout != current_timeout) {
    ioctl(link_fd, B004SETTIMEOUT, &timeout);
    current_timeout = timeout;
  }

  sent = 0;
  ptr = data;
  while (sent < count) {
    ret = write(link_fd, ptr, MIN((count - sent), 4000));
    if (ret <= 0)
      break;
    sent += ret;
    ptr += ret;
  }
  
  return (count - sent);
}

WORD minix_fetch_block(int count, BYTE *data, int timeout) {
  int fetched, ret;
  BYTE *ptr;
  
  /* timeout often defaults to five seconds */
  timeout = 50;
  if (timeout != current_timeout) {
    ioctl(link_fd, B004SETTIMEOUT, &timeout);
    current_timeout = timeout;
  }

  fetched = 0;
  ptr = data;
  while (fetched < count) {
    ret = read(link_fd, ptr, MIN((count - fetched), 4000));
    if (ret <= 0)
      break;
    fetched += ret;
    ptr += ret;
  }

  return (count - fetched);
}

WORD minix_rdrdy() {

  if (ioctl(link_fd, B004READABLE))
    return TRUE;

  return FALSE;
}

WORD minix_wrrdy() {

  if (ioctl(link_fd, B004WRITEABLE))
    return TRUE;

  return FALSE;
}

/* eof */
