#ifndef LOCKLIB_H
#define LOCKLIB_H

int lock_init(char *name);
int lock_release(int id);
int lock_get(int id);
int lock_wait(int id);

#endif
