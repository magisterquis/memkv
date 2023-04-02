/*
 * tree.h
 * Store k/v pairs in a tree.
 * By J. Stuart McMurray
 * Created 20230402
 * Last Modified 20230402
 */

#ifndef HAVE_TREE_H
#define HAVE_TREE_H

void get(int c, char *key);
void set(int c, char *key);
void del(int c, char *key);
void all(int c);

#endif /* #ifdef HAVE_TREE_H */

