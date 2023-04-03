/*
 * tree.c
 * Store k/v pairs in a tree.
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230403
 */

/* Most of the below cribbed from OpenBSD's tree(3) manpage, under the
 * following license:
 *
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <sys/tree.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

struct node {
        RB_ENTRY(node) entry;
        char *key;
        char *value;
};

static int
kvcmp(struct node *e1, struct node *e2)
{
        return strcmp(e1->key, e2->key);
}

RB_HEAD(kvtree, node) head = RB_INITIALIZER(&head);
RB_PROTOTYPE(kvtree, node, entry, kvcmp)
RB_GENERATE(kvtree, node, entry, kvcmp)

/* all prints all of the keys to c. */
void
all(int c)
{
        struct node *n;

        RB_FOREACH(n, kvtree, &head) {
                dprintf(c, "%s\n", n->key);
        }
}

/* get prints the value for the key to c. */
void
get(int c, char *key)
{
        struct node kn, *fn;

        /* Look for the key. */
        kn.key = key;
        fn = RB_FIND(kvtree, &head, &kn);
        if (NULL == fn) {
                dprintf(c, "__Key %s not found__\n", key);
                return;
        }
        dprintf(c, "%s\n", fn->value);
}

/* set gets a value from c and sets the key/value pair. */
void
set(int c, char *key)
{
        char *nkey, *value;
        ssize_t vlen;
        struct node *old, *new;

        nkey = value = NULL;
        old = new = NULL;

        /* Get the value. */
        switch (vlen = read_buf(c, &value)) {
                case -1: /* Error */
                        warn("recv (value)");
                        return;
                case 0: /* EOF */
                        warnx("eof (value)");
                        return;
        }

        /* May need to save a copy of the key. */
        if (-1 == asprintf(&nkey, "%s", key)) {
                dprintf(c, "Copying key: %s\n", strerror(errno));
                warn("asprintf");
                goto out;
        }

        /* New pair to insert. */
        if (NULL == (new = calloc(1, sizeof(struct node)))) {
                dprintf(c, "Allocating memory: %s\n", strerror(errno));
                warn("calloc");
                goto out;
        }
        new->key = nkey;
        new->value = value;

        /* Try inserting. */
        if (NULL == (old = RB_INSERT(kvtree, &head, new))) {
                /* Insert success. */
                dprintf(c, "Added %s\n", key);
                printf("Added %s\n", key);
                nkey = NULL; /* Don't free it. */
        } else {
                /* Just an update. */
                FREE(new);
                FREE(old->value);
                old->value = value;
                dprintf(c, "Updated %s\n", key);
                printf("Updated %s\n", key);
        }

out:
        FREE(nkey);
        FREE(value);
}

/* del deletes a key/value pair. */
void
del(int c, char *key)
{
        struct node kn, *fn;

        /* Look for the key. */
        kn.key = key;
        fn = RB_FIND(kvtree, &head, &kn);
        if (NULL == fn) {
                dprintf(c, "__Key %s not found__\n", key);
                return;
        }
        if (fn != RB_REMOVE(kvtree, &head, fn)) {
                dprintf(c, "__Removed something wrong?__\n");
                return;
        }
        FREE(fn->key);
        FREE(fn->value);
        FREE(fn);
        dprintf(c, "Deleted %s\n", key);
        printf("Deleted %s\n", key);
}
