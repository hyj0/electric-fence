//
// Created by hyj on 2019-02-19.
//

#ifndef PROJECT_SPINLOC_H
typedef unsigned spinlock;

void spin_lock(spinlock *lock);
void spin_unlock(spinlock *lock);

#define PROJECT_SPINLOC_H

#endif //PROJECT_SPINLOC_H
