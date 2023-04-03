/*
 * memkvs.c
 * In-memory Key-Value store
 * By J. Stuart McMurray
 * Created 20230401
 * Last Modified 20230401
 */

#include <sys/socket.h>

#include <err.h>
#include <pthread.h>
#include <readpassphrase.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

/* BUFLEN is the size of the buffers we use for reading values. */
#define BUFLEN 1024

void
usage(void)
{
        fprintf(stderr, "Usage: %s [-h] [-S path] {-gsdl} [key [value]]\n"
"\n"
"Gets, sets, deletes, or lists key/values pairs stored in memkvd.\n"
"\n"
"Flags:\n"
"  -h      - This help\n"
"  -S path - Path to memkvd's socket (default: %s)\n"
"  -g      - Get a key's value\n"
"  -s      - Set a key's value\n"
"  -d      - Delete a key/value pair\n"
"  -l      - List all keys\n",
                        getprogname(), default_socket);
        exit(1);
}

/* get_value gets a value from stdin and puts a pointer to it in value.  On
 * error, the program is terminated.  The value will be NUL-terminated. */
void
get_value(char **value)
{
        size_t bufsiz;

        /* Make sure we have a value and get its size. */
        if (NULL == (*value = calloc(BUFLEN, sizeof(char))))
                err(18, "calloc");
        bufsiz = BUFLEN * sizeof(char); /* calloc prevents overflow. */
        /* Get the data itself. */
        if (NULL == readpassphrase("Value (will not echo):",
                                *value, bufsiz, 0))
                err(6, "readpassphrase");
}

/* to_stdout copies fd to stdout, spawnable as a thread. */
void *
to_stdout(void *fd)
{
        int fdn;
        char buf[BUFLEN];
        ssize_t nr, nw, off;

        fdn = *(int*)fd;

        for (;;) {

                /* Get a chunk from the server. */
                switch (nr = recv(fdn, buf, BUFLEN, 0)) {
                        case 0: /* EOF */
                                return NULL;
                        case -1: /* Error */
                                err(19, "recv");
                }

                /* Write it all to stdout. */
                for (off = 0; off < nr; off += nw)
                        if ((nw = write(STDOUT_FILENO, buf + off, nr - off))
                                        == 0 || nw == -1)
                                err(20, "write");
        }
}

int
main(int argc, char **argv)
{
        struct sockaddr_un sa;
        int s, ch, op;
        char *addr, *key, *value;
        pthread_t tid;

        if (-1 == pledge("getpw stdio tty unix", ""))
                err(2, "pledge");

        /* Work out where we'll might gonnect. */
        init_default_socket();

        /* Work out what we're meant to do. */
        op = 0;
        addr = NULL;
        while ((ch = getopt(argc, argv, "S:gsdlh")) != -1) {
                switch (ch) {
                        case 'S': addr = optarg; break;
                        case OP_GET: /* Get */
                        case OP_SET: /* Set */
                        case OP_DEL: /* Delete */
                        case OP_ALL: /* List */
                                if (0 != op)
                                        errx(9, "cannot use %c and %c together",
                                                        op, ch);
                                op = ch;
                                break;
                        case 'h':
                        default:
                                usage();
                }
        }
        argc -= optind;
        argv += optind;
        if (0 == op)
                errx(11, "Need one of -g, -s, or -d");

        /* Get the key and maybe the value. */
        key = value = NULL;
        if (OP_ALL != op) {
                if (0 == argc)
                        errx(18, "need a key");
                key = argv[0];
        }
        if (OP_SET == op) {
                if (1 == argc)
                        get_value(&value);
                else
                        value = argv[1];
        }

        /* Work out where to connect or listen. */
        get_socket_addr(&sa, &addr);
        s = unix_socket();
        if (-1 == connect(s, (struct sockaddr *)&sa, sizeof(sa)))
                err(16, "connect");

        if (-1 == pledge("stdio", ""))
                err(17, "pledge");

        /* Anything from the server will go straight to the user. */
        if (0 != pthread_create(&tid, NULL, to_stdout, (void *)&s))
                err(21, "pthread_create");

        /* Send the op, key, and value to the server, as appropriate. */
        if (1 != send(s, &op, 1, 0))
                err(23, "send(op)");
        if (NULL != key && -1 == send_buf(s, key, strlen(key)))
                err(24, "send(key)");
        if (NULL != value && -1 == send_buf(s, value, strlen(value)))
                err(25, "send(value)");
        if (-1 == shutdown(s, SHUT_WR))
                err(26, "shutdown");


        /* Wait for the response to finish. */
        if (-1 == pthread_join(tid, NULL))
                err(22, "pthread_join");

        return 0;
}
