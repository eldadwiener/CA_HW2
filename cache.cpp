#include <math>
#include "cache.h"






// TODO: MLCache must make sure size/numsets is a valid integer (>0)
cache::cache(uint32_t size, uint32_t numSets, uint32_t blockSize, policy pol) :
    _size(size), _setBits(log2(numSets)), _offsetBits(log2(blockSize)), _pol(pol), _sets(numSets,cacheSet(blockSize,size/numSets)) {};

uint32_t cache::getSet(uint32_t addr)
{
    return (addr >> _offsetBits) & ((1 << _setBits) - 1);
}

uint32_t cache::getTag(uint32_t addr)
{
    return addr >> (_offsetBits + _setBits);
}

uint32_t cache::getOffset(uint32_t addr)
{
    return addr & ((1 << _offsetBits) - 1);
}

void cache::read(uint32_t addr)
{
    uint32_t tag = getTag(addr), set = getSet(addr), offset = getOffset(addr);
    _sets[set].read(tag, offset);
}