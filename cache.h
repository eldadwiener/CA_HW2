#ifndef _MLCACHE_
#define _MLCACHE_

#include <list>
#include <vector>
#include <iostream>
#include <exception>
#include <stdint.h>

using namespace std;

#define VICTIMSIZE 4
#define VICTIMTIME 1
enum policy{NOWRITEALLOC,WRITEALLOC};
enum level{L1,L2,VICTIM,MEMORY};

typedef struct stats {
    double L1MissRate;
    double L2MissRate;
    double avgAccTime;
}stats;

class EvictedBlock :public exception
{
    public:
        EvictedBlock(uint32_t _addr, bool _dirty = false) : addr(_addr), dirty (_dirty) {};
        uint32_t addr;
		bool dirty;
};

class found :public exception
{
    public:
        found(bool _dirty = false) : dirty(_dirty) {};
        found(level loc, bool _dirty) : location(loc), dirty(_dirty) {};
        level location;
		bool dirty;
};
// TODO: can a block be of size 2? if yes, we need 2 blocks per read/write
typedef struct cacheBlock{
    cacheBlock(uint32_t _tag, bool _dirty = false): tag(_tag), dirty(_dirty) {}
    uint32_t tag;
    bool dirty;
}cacheBlock;

// The set holds a block for each way that exists in the cache
class cacheSet{
    public:
        cacheSet(uint32_t blockSize,uint32_t waySize);
        void read(uint32_t tag, uint32_t offset); // throws if tag does not exist
        void write(uint32_t tag, uint32_t offset); // throws if tag does not exist
        void insert(uint32_t tag, bool dirty = false); // throws evicted block if any
        bool evict(uint32_t tag); // evict addr if in set, otherwise ignore call, support dirty 
		void set_dirty(uint32_t tag, bool dirty);//set line to dirty
    private:
        list<cacheBlock>::iterator find(uint32_t tag);

        list<cacheBlock> _ways;
        uint32_t _blockSize, _waySize;
};

// will hold all the sets and manage them
class cache{
    public:
        cache(uint32_t size, uint32_t setBits, uint32_t offsetBits, uint32_t assoc, policy pol, level lev);
        void read(uint32_t addr); // throws if tag does not exist
        void write(uint32_t addr); // throws if tag does not exist
        void insert(uint32_t addr, bool dirty = false); // throws evicted block if any
        bool evict(uint32_t addr); // evict addr if in cache, otherwise ignore call
		void set_dirty(uint32_t addr, bool dirty);//set line to dirty
    private:
        uint32_t getTag(uint32_t addr);
        uint32_t getSet(uint32_t addr);
        uint32_t getOffset(uint32_t addr);

        uint32_t _size, _setBits, _offsetBits;
        policy _pol;
        level _level;
		vector<cacheSet> _sets;
};


class victim {
    public:
        victim(uint32_t blockSize);
        void get(uint32_t tag, bool write_n_a = false); // throws block or NOTFOUND exception
        void insert(uint32_t tag, bool dirty = false); // throws evicted block if any TODO: is this needed?

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
        void copyToCaches(uint32_t addr, level fromLevel, bool dirty);
    	uint32_t _MemCyc, _BSize, _L1Size, _L2Size, _L1Assoc, _L2Assoc,
					_L1Cyc, _L2Cyc, _WrAlloc, _VicCache,
                    _L1Misses, _L2Misses, _totalTime, _L1Accesses, _L2Accesses;
        cache _L1, _L2;
        victim _vict;
};

#endif