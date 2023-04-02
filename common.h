/*
 * common.h
 * Functions common to both server and client
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230402
 */

#ifndef HAVE_COMMON_H
#define HAVE_COMMON_H

#include <sys/un.h>

#include <stdlib.h>

/* MAXBUF is the maximum buffer size we can send or receive on the socket. */
#define MAXBUF 0xFFFF

/* SOCKNAME is the default name for the socket, inside the user's home
 * directory .*/
#define SOCKNAME ".memkvd.sock"

/* Op constants */
#define OP_GET 'g'
#define OP_SET 's'
#define OP_DEL 'd'
#define OP_ALL 'l'

/* FREE frees x if it's not NULL. */
#define FREE(x) do { if (NULL != (x)) {free((x)); (x) = NULL;} } while (0);

/* default_socket is the default socket path.  It must be set with 
 * init_default_socket before use. */
extern char *default_socket;

/* init_default_socket initializes default_socket.  It terminates the program
 * on error. */
void init_default_socket(void);

/* get_socket_addr gets the path for the unix socket either from *path or
 * constructs it from the user's home directory and SOCKNAME and sets up sa.
 * On error, the program is terminated.  On return, *path holds the socket
 * path, even if it was NULL. */
void get_socket_addr(struct sockaddr_un *sa, char **addr);

/* unix_socket gets a new unix socket or terminates the program. */
int unix_socket();

/* read_buf read a string from fd.  It first reads a two-byte length, then
 * allocates a buffer and reads the string into the buffer.  -1 is returned
 * on error.  errno should be set.  The caller is responsible for freeing the
 * allocated buffer. */
ssize_t read_buf(int fd, char **buf);

/* send_buf sends the buffer, preceded by the buffer's size.  It returns -1
 * on error. */
int send_buf(int fd, const char *buf, uint16_t buflen);

#endif /* #ifndef HAVE_COMMON_H */
