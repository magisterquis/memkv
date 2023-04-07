/*
 * memkvsd.c
 * Server side of memkvs
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230407
 */

#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "handle.h"

int   lfd;           /* Listining socket. */
char *path;          /* Socket path. */
int   to_catch[] = { /* Signals to catch and remove the socket. */
        SIGHUP, SIGINT, SIGTERM, SIGUSR1, SIGUSR2, SIGHUP
};

/* usage prints a usage statement and exits. */
void
usage(void)
{
        fprintf(stderr, "Usage: %s [-dhr] [-S path]\n"
"\n"
"Gets, sets, deletes, or lists key/values pairs stored in memkvd.\n"
"\n"
"Flags:\n"
"  -h      - This help\n"
"  -S path - Path to the unixsocket (default: %s)\n"
"  -d      - Debug mode: stay in the foreground while running\n"
"  -r      - Remove the unix socket if it exists\n",
                        getprogname(), default_socket);
        exit(1);
}

/* unlink_sock unlinks the unix socket s. */
void
unlink_sock(void)
{
        if (-1 == unlink(path))
                err(11, "unlink(%s)", path);
}

/* sighandler handles signals.  It unlinks the listening socket then terminates
 * the program. */
void
sighandler(int sig)
{
        unlink_sock();
        errx(12, "caught SIG%s", sys_signame[sig]);
}

int
main(int argc, char **argv)
{
        int dflag, rflag, ch, c, i;
        struct sockaddr_un sa;
        socklen_t len;


        if (-1 == pledge("cpath getpw proc stdio unix unveil", ""))
                err(31, "pledge");

        /* Work out where we'll might gonnect. */
        init_default_socket();

        dflag = rflag = 0;
        path = NULL;
        while ((ch = getopt(argc, argv, "drS:h")) != -1) {
                switch (ch) {
                        case 'd':
                                dflag = 1;
                                break;
                        case 'r':
                                rflag = 1;
                                break;
                        case 'S':
                                path = optarg;
                                break;
                        case 'h':
                        default:
                                usage();
                }
        }
        argc -= optind;
        argv += optind;

        /* Work out the socket. */
        get_socket_addr(&sa, &path);
        if (-1 == unveil(path, "c"))
                err(14, "unveil");
        if (-1 == unveil(NULL, NULL))
                err(15, "unveil");

        /* If we're just removing an old socket, do that and we're done. */
        if (rflag) {
                if (-1 == pledge("cpath stdio", ""))
                        err(28, "pledge");
                /* Try to remove it. */
                if (-1 == unlink(path)) {
                        /* If it wsan't actually there, life's good. */
                        if (ENOENT == errno) {
                                printf("%s does not exist\n", path);
                                return 0;
                        }
                        err(27, "unlink(%s)", path);
                }
                printf("Removed %s\n", path);
                return 0;
        }


        lfd = unix_socket();
        if (-1 == bind(lfd, (struct sockaddr *)&sa, sizeof(sa)))
                err(6, "bind");
        if (-1 == listen(lfd, 128))
                err(7, "listen");

        if (-1 == pledge("cpath proc stdio unix", ""))
                err(30, "pledge");

        /* Catch signals so we can remove the socket before dying. */
        for (i = 1; i < (int)(sizeof(to_catch)/sizeof(to_catch[0])); ++i) {
                if (SIG_ERR == signal(to_catch[i], sighandler))
                        err(13, "signal (%d)", i);
        }

        /* Background ourselves, if we're meant to. */
        if (!dflag)
                if (-1 == daemon(1, 0)) {
                        unlink_sock();
                        err(8, "daemon");
                }

        if (-1 == pledge("cpath proc stdio unix", ""))
                err(29, "pledge");

        printf("Ready\n");

        /* Accept clients and handle requests. */
        for (;;) {
                /* Pop a client and handle if all went well. */
                len = sizeof(sa);
                if (-1 != (c = accept(lfd, (struct sockaddr *)&sa, &len))) {
                        handle(c);
                        continue;
                }
                /* EMFILE and ENFILE are handleable. */
                switch (errno) {
                        case EMFILE:
                        case ENFILE:
                                sleep(1);
                                continue;
                        default:
                                unlink_sock();
                                err(9, "accept");
                }
        }
}
