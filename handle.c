/*
 * handle.c
 * Handle memkv clients.
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230402
 */

#include <sys/socket.h>

#include <err.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "tree.h"

/* handle handles a connection from a client. */
void
handle(int c)
{
        char *key;
        ssize_t klen;
        char op;

        key = NULL;

        /* Get the operation. */
        if (-1 == recv(c, &op, 1, MSG_WAITALL)) {
                warn("recv (op)");
                goto out;
        }

        /* Unless we're listing, we'll need a key. */
        if (OP_ALL != op) {
                switch (klen = read_buf(c, &key)) {
                        case -1: /* Error */
                                warn("recv (key)");
                                goto out;
                        case 0: /* EOF */
                                warnx("eof (key)");
                                goto out;
                }
        }

        /* Work out what to do. */
        switch (op) {
                case OP_GET: get(c, key); break;
                case OP_SET: set(c, key); break;
                case OP_DEL: del(c, key); break;
                case OP_ALL: all(c);      break;
                default:
                        dprintf(c, "Unknown operation %c.\n", op);
                        break;
        }

out:
        FREE(key);
        close(c);
}
