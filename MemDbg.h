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
    int status;//1--Ok, 0--has been free
};

MemoryInfo *newMemoryInfo() {
    //todo: need optimize space
    void *address = Page_Create(sizeof(MemoryInfo));
    return (MemoryInfo *) address;
}

MemoryInfo g_memoryInfoHeadInstance;
MemoryInfo *g_memoryInfoHead;

void init_g_memoryInfoHead() {
    g_memoryInfoHead = &g_memoryInfoHeadInstance;
}

class AllocManager {
public:
    static AllocManager &getInstance() {
        static AllocManager allocManager;
        return (AllocManager &) allocManager;
    }
    MemoryInfo * getMemoryInfo(void *userAddress) {
        init_g_memoryInfoHead();
        //todo:just a list, need optimize
        MemoryInfo *mp = g_memoryInfoHead->next;
        while (mp != NULL) {
            if (mp->userAddress == userAddress && mp->status == 1) {
                return mp;
            }
            mp = mp->next;
        }
        return mp;
    }
    void deleteMemoryInfo(MemoryInfo *memoryInfo) {
        memoryInfo->status = 0;
    }

    void addMemoryInfo(MemoryInfo *memoryInfo) {
        init_g_memoryInfoHead();
        MemoryInfo *g_next = g_memoryInfoHead->next;
        g_memoryInfoHead->next = memoryInfo;
        memoryInfo->next = g_next;
        memoryInfo->status = 1;
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
                EF_Print("delete Memory no found address:0x%a\n", address);
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
