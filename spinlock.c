//
// Created by hyj on 2019-02-19.
//

#include "spinlock.h"

#define __EBUSY 1

/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */
#define cpu_relax() asm volatile("pause\n": : :"memory")

static inline unsigned xchg_32(void *ptr, unsigned x)
{
    asm volatile("xchgl %0,%1"
    :"=r" ((unsigned) x)
    :"m" (*(volatile unsigned *)ptr), "0" (x)
    :"memory");

    return x;
}

void spin_lock(spinlock *lock)
{
    while (1)
    {
        if (!xchg_32(lock, __EBUSY)) return;

        while (*lock) cpu_relax();
    }
}

void spin_unlock(spinlock *lock)
{
    barrier();
    *lock = 0;
}
