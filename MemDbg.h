//
// Created by dell-pc on 2016/12/10.
//

#ifndef PROJECT_MEMDBG_H
#define PROJECT_MEMDBG_H

#include <iostream>
#include <string>
#include <memory>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

using namespace std;

extern "C" {
#include "efence.h"
};

struct MemoryInfo {
    void *userAddress;
    void *startAddress;
    size_t userSize;
    size_t totalSize;
    MemoryInfo *next;
    MemoryInfo *prev;
};

class MemoryInfoGcQueque {
public:
    void initList() {
        if (head.next == NULL) {
            head.next = &tail;
            tail.prev = &head;
        }
    }
    void enque(MemoryInfo *pInfo) {
        initList();
        MemoryInfo *oldNext = head.next;
        head.next = pInfo;
        pInfo->next = oldNext;
        
        pInfo->prev = &head;
        oldNext->prev = pInfo;
    }

    MemoryInfo *deque() {
        initList();
        MemoryInfo *retMemInfo = tail.prev;
        if (retMemInfo == &head) {
            return NULL;
        }

        MemoryInfo *pPrevMemInfo = retMemInfo->prev;
        pPrevMemInfo->next = &tail;
        tail.prev = pPrevMemInfo;
        return retMemInfo;
    }
private:
    MemoryInfo head;
    MemoryInfo tail;
};

extern MemoryInfoGcQueque g_memoryInfoGcQueque;

MemoryInfo *newMemoryInfo() {
    static MemoryInfo *MemoryInfoBuffer;
    static int MemoryInfoCount = 10240;
    static int MemoryInfoBufferIndex = 0;

    if (MemoryInfoBufferIndex == 0) {
        //try to use gc que
        MemoryInfo *memoryInfo = g_memoryInfoGcQueque.deque();
        if (memoryInfo != NULL) {
            return memoryInfo;
        } else {
            MemoryInfoBuffer = (MemoryInfo *) Page_Create(sizeof(MemoryInfo) * MemoryInfoCount);
        }
    }

    MemoryInfo *address = &MemoryInfoBuffer[MemoryInfoBufferIndex++];

    if (MemoryInfoBufferIndex >= MemoryInfoCount) {
        MemoryInfoBufferIndex = 0;
    }
    return address;
}

#define HASH_MAX_KEY 350899

struct MemInfoKV {
    MemoryInfo memoryInfo;
};

class HashMap {
public:
    unsigned long getKey(void *userAddress) {
        return (unsigned long) userAddress % HASH_MAX_KEY;
    }

    MemoryInfo *get(void *pUserAddress) {
        unsigned long key = this->getKey(pUserAddress);
        MemoryInfo *m = &memInfoKV[key].memoryInfo;
        MemoryInfo *mp = m->next;
        while (mp != NULL) {
            if (mp->userAddress == pUserAddress) {
                return mp;
            }
            mp = mp->next;
        }
        return NULL;
    }

    int insert(MemoryInfo *pInfo) {
        unsigned long key = this->getKey(pInfo->userAddress);
        MemInfoKV &memInfokv = memInfoKV[key];
        MemoryInfo *oldNext = memInfokv.memoryInfo.next;
        memInfokv.memoryInfo.next = pInfo;
        pInfo->next = oldNext;
        return 0;
    }

    int deleteKey(MemoryInfo *pInfo) {
        unsigned long key = this->getKey(pInfo->userAddress);
        MemoryInfo *m = &memInfoKV[key].memoryInfo;
        MemoryInfo *preMp = m;
        MemoryInfo *mp = m->next;
        while (mp != NULL) {
            if (mp->userAddress == pInfo->userAddress) {
                preMp->next = mp->next;
                return 0;
            }
            preMp = mp;
            mp = mp->next;
        }
        EF_Print("delet key no found key address:0x%a\n", pInfo->userAddress);
        return -1;
    }

private:
    MemInfoKV memInfoKV[HASH_MAX_KEY];
};

extern HashMap g_MemoryInfoHashMap;

class AllocManager {
public:
    static AllocManager &getInstance() {
        static AllocManager allocManager;
        return (AllocManager &) allocManager;
    }
    MemoryInfo * getMemoryInfo(void *userAddress) {
        MemoryInfo *pMemInfo =  g_MemoryInfoHashMap.get(userAddress);
        return pMemInfo;
    }
    void deleteMemoryInfo(MemoryInfo *memoryInfo) {
        int ret = g_MemoryInfoHashMap.deleteKey(memoryInfo);
        if (ret != 0) {
            EF_Abort("delete err no found MemoryInfo userAddress:%a\n", memoryInfo->userAddress);
        }
        //:record memoryInfo has been delete
        g_memoryInfoGcQueque.enque(memoryInfo);
    }

    void addMemoryInfo(MemoryInfo *memoryInfo) {
        int ret = g_MemoryInfoHashMap.insert(memoryInfo);
    }
};

class MemDbg {
public:
    static MemDbg &getInstance() {
        static MemDbg memDbg;
        return (MemDbg &) memDbg;
    }
    MemDbg() {
        this->allocManager = AllocManager::getInstance();
        this->pageSize = Page_Size();
        if (pageSize <= 0) {
            EF_Abort("pageSize=%d err:%s\n", pageSize, strerror(errno));
        }
    }
    void *createMemory(size_t size) {
        if (pageSize <= 0) {
            //before entry the `main` function, some time also need alloc memory
            pageSize = Page_Size();
            this->allocManager = AllocManager::getInstance();
        }

        /*
         * memory
         * |startAddress|userAddress| one pagesize|
         * 1.set page protect in 'one pagesize' to detect access
         * 2.when free, check startAddress has been change or not
         */

        size_t pageNum = size / this->pageSize + 2;
        size_t totalSize = pageNum * this->pageSize;
        void *startAddress = Page_Create(totalSize);
        void *endAddress = startAddress + totalSize;
        void *userAddress = endAddress - this->pageSize - size;

        //record the memory info for free
        MemoryInfo *memoryInfo = newMemoryInfo();
        memoryInfo->userAddress = userAddress;
        memoryInfo->startAddress = startAddress;
        memoryInfo->userSize = size;
        memoryInfo->totalSize = totalSize;
        allocManager.addMemoryInfo(memoryInfo);

        Page_DenyAccess(endAddress-pageSize, pageSize);
        if (size % this->pageSize == 0) {
            Page_DenyAccess(startAddress, pageSize);
        }
        return userAddress;
    }

    void deleteMemory(void *address) {
        MemoryInfo *memoryInfo = allocManager.getMemoryInfo(address);
        if (memoryInfo == NULL) {
            if (address == 0) {
                //i do not know why address == 0 happy
//                EF_Print("delete Memory no found address:0x%a\n", address);
                return;
            }
            EF_Abort("delete Memory no found address:0x%a\n", address);
        }

        //check memory
        char *p = (char *) memoryInfo->startAddress;
        if (memoryInfo->userSize % this->pageSize != 0) {
            for (int i = 0; i < memoryInfo->totalSize - memoryInfo->userSize - pageSize; ++i) {
                if (p[i] != 0) {
                    EF_Abort("memory has be write !! address:0x%a c=%c\n", p+i, p[i]);
                }
            }
        }

        Page_DenyAccess(memoryInfo->startAddress, memoryInfo->totalSize);
        allocManager.deleteMemoryInfo(memoryInfo);
        //unmap, and it can also be protect
        munmap(memoryInfo->startAddress, memoryInfo->totalSize);
    }

    size_t getAllocSize(void *address) {
        MemoryInfo *memoryInfo = allocManager.getMemoryInfo(address);
        if (memoryInfo == NULL) {
            EF_Abort("no found memoryInfo address:0x%a\n", address);
        }
        return memoryInfo->userSize;
    }

private:
    size_t pageSize;
    AllocManager allocManager;
};

#endif //PROJECT_MEMDBG_H
