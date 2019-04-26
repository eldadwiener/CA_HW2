#ifndef _MLCACHE_
#define _MLCACHE_

#include <list>
#include <vector>
#include <iostream>
#include <exception>

using namespace std;

#define VICTIMSIZE 4
#define VICTIMTIME 1
enum policy{NOWRITEALLOC,WRITEALLOC};

typedef struct stats {
    double L1Missrate;
    double L2Missrate;
    double avgAccTime;
}stats;

class notfound :public exception {} ;

// TODO: can a block be of size 2? if yes, we need 2 blocks per read/write
typedef struct cacheBlock{
    cacheBlock(uint32_t _tag, bool _dirty): tag(_tag), dirty(_dirty) {}
    uint32_t tag;
    bool dirty;
}cacheBlock;

// The set holds a block for each way that exists in the cache
class cacheSet{
    public:
        cacheSet(uint32_t _blockSize,uint32_t _waySize);
        void read(uint32_t tag, uint32_t offset); // throws if tag does not exist
        void write(uint32_t tag, uint32_t offset); // throws if tag does not exist
        void insert(uint32_t tag); // throws evicted block if any
        
    private:
        list<cacheBlock>::iterator find(uint32_t tag);
        list<cacheBlock> _ways;
        uint32_t _blockSize,_waySize;
};

// will hold all the sets and manage them
class cache{
    public:
        cache(uint32_t size, uint32_t numSets, uint32_t blockSize, policy pol);
        void read(uint32_t addr); // throws if tag does not exist
        void write(uint32_t addr); // throws if tag does not exist
        void insert(uint32_t addr); // throws evicted block if any

    private:
        vector<cacheSet> _sets;
        uint32_t _size, _numSets, _blockSize;
        policy _pol;
};


class victim {
    public:
        victim(uint32_t blockSize);
        void get(uint32_t tag); // throws block or NOTFOUND exception
        void insert(uint32_t tag); // throws evicted block if any TODO: is this needed?

    private: 
        uint32_t _blockSize;
        list<cacheBlock> _blocks;
};

class MLCache {
    public:
        MLCache(uint32_t MemCyc,uint32_t BSize,uint32_t L1Size,uint32_t L2Size,uint32_t L1Assoc,uint32_t L2Assoc,
            uint32_t L1Cyc,uint32_t L2Cyc,uint32_t WrAlloc,uint32_t VicCache);
        void read(uint32_t addr);
        void write(uint32_t addr);
        stats getStats();

    private:
    	uint32_t _MemCyc, _BSize, _L1Size, _L2Size, _L1Assoc, _L2Assoc,
					_L1Cyc, _L2Cyc, _WrAlloc, _VicCache,
                    _L1Misses, _L2Misses, _totalTime, _numAccesses;
        cache _L1, _L2;
        victim _vict;
};

#endif