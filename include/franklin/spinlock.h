#ifndef _SPIN_
#define _SPIN_ 1


typedef unsigned int lock;
unsigned int spinlock;

void acquire(lock*);
void release(lock*);
void init_lock(lock*);
int trylock(lock*);

#endif
