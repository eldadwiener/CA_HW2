#include <cmath>
#include "cache.h"

// TODO: MLCache must make sure size - setBits > 0
cache::cache(uint32_t size, uint32_t setBits, uint32_t offsetBits, policy pol, level lev) :
    _size(size), _setBits(setBits), _offsetBits(offsetBits), _pol(pol), _level(lev), _sets((1 << setBits), cacheSet((1 << offsetBits), (1 << (size - setBits)))) {};

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
    try
    {
        _sets[set].read(tag, offset);
    }
    catch (found) // add finding cache name to exception
    {
        throw found(_level);
    }
}

void cache::write(uint32_t addr)
{
    uint32_t tag = getTag(addr), set = getSet(addr), offset = getOffset(addr);
    try
    {
        _sets[set].write(tag, offset);
    }
    catch (found) // add finding cache name to exception
    {
        throw found(_level);
    }
}

void cache::insert(uint32_t addr)
{
    uint32_t tag = getTag(addr), set = getSet(addr); 
    try
    {
        _sets[set].insert(tag);
    }
    catch (found) // add finding cache name to exception
    {
        throw found(_level);
    }
}

cacheSet::cacheSet(uint32_t blockSize, uint32_t waySize): _ways(), _blockSize(blockSize), _waySize(waySize){}

// throws if tag does not exist
void cacheSet::read(uint32_t tag, uint32_t offset) {//Cache fix the tag
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
        return; // not found
	}
	cacheBlock temp = *itr;
	_ways.erase(itr);
	_ways.push_back(temp);
	throw found();
}

// throws if tag does not exist
void cacheSet::write(uint32_t tag, uint32_t offset) {
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
        return; // not found
	}
	cacheBlock temp = *itr;
	_ways.erase(itr);
	temp.dirty = true;
	_ways.push_back(temp);
	throw found();
}

// throws evicted block if any
void cacheSet::insert(uint32_t tag) {
	if (_ways.size() < _waySize) {
		_ways.push_back(cacheBlock(tag));
		return;
	}
	cacheBlock temp = _ways.front();
	_ways.pop_front;
	_ways.push_back(cacheBlock(tag));
	throw temp; // temp is evicted, throw it
}

list<cacheBlock>::iterator cacheSet::find(uint32_t tag) {
	list<cacheBlock>::iterator itr = _ways.begin();
	while (itr != _ways.end())
	{
		if (itr->tag == tag){
			return itr;
		}
		itr++;
	}
	return itr;
}

//victim c'tor
victim::victim(uint32_t blockSize) : _blockSize(blockSize), _blocks() {}
// throws block or NOTFOUND exception
void victim::get(uint32_t tag) {
	list<cacheBlock>::iterator itr = _blocks.begin();
	while (itr != _blocks.end())
	{
		if (itr->tag == tag) {
			cacheBlock temp = *itr;
			_blocks.erase(itr);
			throw found(VICTIM);
		}
		itr++;
	}
	return; // not found
}

// throws evicted block if any TODO: is this needed?
void victim::insert(uint32_t tag) {
    if (_blocks.size() < VICTIMSIZE) {
        _blocks.push_back(cacheBlock(tag));
        return;
    }
    _blocks.pop_front;
    _blocks.push_back(cacheBlock(tag));
    //TODO maybe need to update somthing
}

MLCache::MLCache(uint32_t MemCyc, uint32_t BSize, uint32_t L1Size, uint32_t L2Size, uint32_t L1Assoc, uint32_t L2Assoc,
    uint32_t L1Cyc, uint32_t L2Cyc, uint32_t WrAlloc, uint32_t VicCache) :
    _MemCyc(MemCyc), _BSize(BSize), _L1Size(L1Size), _L2Size(L2Size), _L1Assoc(L1Assoc), _L2Assoc(L2Assoc),
    _L1Cyc(L1Cyc), _L2Cyc(L2Cyc), _WrAlloc(WrAlloc), _VicCache(VicCache),
    _L1Misses(0), _L2Misses(0), _totalTime(0), _L1Accesses(0), _L2Accesses(0),
    _L1(L1Size, L1Assoc, BSize, static_cast<policy>(WrAlloc),L1), _L2(L2Size, L2Assoc, BSize, static_cast<policy>(WrAlloc), L2), _vict(BSize) {}


void MLCache::read(uint32_t addr)
{
    // check if exists in L1 cache.
    try
    {
        _totalTime += _L1Cyc; // add access time
        ++_L1Accesses;
        _L1.read(addr); // try reading addr from L1, will throw exception if missing
        // if we are here, addr was not found yet, check L2
        _totalTime += _L2Cyc; // add access time
        ++_L2Accesses;
        _L2.read(addr); // try reading addr from L2

        // not found in L2, check victim if exists
        if (_VicCache)
        {
            _totalTime += VICTIMTIME;
            _vict.get(addr);
        }
        // not in any cache, get from mem
        _totalTime += _MemCyc;
        // TODO: do we need more time for inserting new entry?
        throw found(MEMORY);
        
    }
    catch (const found& fn)
    {
        switch (fn.location)
        {
        case L1: // nothing to do
            break;
        case L2:
            // TODO: need to insert to L1
            break;
        default: // found in MEMORY or VICTIM
            // TODO: need to insert to L1 and L2
            break;
        }
    }
}

void MLCache::write(uint32_t addr)
{
    // try caches until write success is thrown
    try
    {
        _totalTime += _L1Cyc; // add access time
        ++_L1Accesses;
        _L1.write(addr); // try writing to addr in L1, will throw exception if succeeded 
        // if we are here, addr was not in L1, try L2
        _totalTime += _L2Cyc; 
        ++_L2Accesses;
        _L2.write(addr); // try reading addr from L2

        // not in L2, check victim if exists
        if (_VicCache)
        {
            // TODO: how to handle Victim cache writes 
            _totalTime += VICTIMTIME;
        }
        // not in any cache, will require mem access
        _totalTime += _MemCyc;
        // TODO: do we need more time for inserting new entry?
        throw found(MEMORY);
    }

    catch (const found& fn)
    {
        if (_WrAlloc)
        {
            switch (fn.location)
            {
            case L1: // nothing to do
                break;
            case L2:
                // TODO: bring data to L1 
                break;
            default: // found in MEMORY or VICTIM
                // TODO: need to insert to L1 and L2
                break;
            }
        }
    }
}
