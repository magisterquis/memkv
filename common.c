/*
 * common.c
 * Functions common to both server and client
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230407
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "common.h"

/* default_socket is the default socket path.  It must be set with 
 * init_default_socket before use. */
char *default_socket;

/* init_default_socket initializes default_socket.  It terminates the program
 * on error. */
void
init_default_socket(void)
{
        struct passwd *pw;

        if (NULL == (pw = getpwuid(getuid())))
                err(3, "getpwuid");
        if (-1 == asprintf(&default_socket, "%s/%s", pw->pw_dir, SOCKNAME))
                err(4, "asprintf");
}


/* get_socket_addr gets the path for the unix socket either from *path or
 * constructs it from the user's home directory and SOCKNAME and sets up sa.
 * On error, the program is terminated.  On return, *path holds the socket
 * path, even if it was NULL. */
void
get_socket_addr(struct sockaddr_un *sa, char **addr)
{
        explicit_bzero(sa, sizeof(sa));

        /* Work out where to connect or listen. */
        if (NULL == *addr) {
                *addr = default_socket;
        }
        if (sizeof(sa->sun_path)-1 < strlen(*addr))
                errx(5, "socket address %s too long (should be <= %lu", *addr,
                                sizeof(sa->sun_path)-1);
        strcpy(sa->sun_path, *addr);

        /* Fill in the rest of the address. */
        sa->sun_len = sizeof(sa);
        sa->sun_family = AF_UNIX;
}

/* unix_socket gets a new unix socket or terminates the program. */
int
unix_socket()
{
        int s;
        if (-1 == (s = socket(AF_UNIX, SOCK_STREAM, 0)))
                err(2, "socket");
        return s;
}

/* read_buf read a string from fd.  It first reads a two-byte length, then
 * allocates a buffer and reads the string into the buffer.  -1 is returned
 * on error, 0 on EOF.  errno should be set.  If *buf isn't NULL, The caller is
 * responsible for freeing the allocated buffer.  The buffer will be
 * NUL-terminated. */
ssize_t
read_buf(int fd, char **buf)
{
        uint16_t sbuf;
        ssize_t slen;
        ssize_t nr;

        /* Work out how long the buffer is. */
        if (sizeof(sbuf) != (nr = recv(fd, &sbuf, sizeof(sbuf), MSG_WAITALL)))
                return -1 == nr ? nr : 0;
        slen = sbuf+1;

        /* Get the contents. */
        if (NULL == (*buf = calloc(slen, 1)))
                return -1;
        if (sbuf != (nr = recv(fd, *buf, sbuf, MSG_WAITALL))) {
                FREE(*buf);
                return -1 == nr ? nr : 0;
        }
        return slen;
}

/* send_buf sends the buffer, preceded by the buffer's size.  It returns -1
 * on error. */
int
send_buf(int fd, const char *buf, uint16_t buflen)
{
        /* Send the length. */
        if (-1 == send(fd, (const char *)&buflen, 2, 0))
                return -1;
        /* Send the buffer itself. */
        if (-1 == send(fd, buf, buflen, 0))
                return -1;

        return 0;
}
