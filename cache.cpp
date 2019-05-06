#include <cmath>
#include <assert.h>
#include "cache.h"
#include <iostream>//DEBUG

using namespace std;//DEBUG

int d_count = 1; //DEBUG


// TODO: MLCache must make sure size - setBits > 0
cache::cache(uint32_t size, uint32_t setBits, uint32_t offsetBits,uint32_t assoc, policy pol, level lev) :
    _size(size), _setBits(setBits), _offsetBits(offsetBits), _pol(pol), _level(lev), _sets((1 << setBits), cacheSet((1 << offsetBits), (1 << assoc))) {};

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

void cache::set_dirty(uint32_t addr) {
	uint32_t tag = getTag(addr), set = getSet(addr);
	_sets[set].set_dirty(tag);
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

void cache::insert(uint32_t addr, bool dirty)
{
    uint32_t tag = getTag(addr), set = getSet(addr); 
    try
    {
        _sets[set].insert(tag, dirty);
    }
    catch (const cacheBlock& cb) // construct full addr and rethrow
    {
        uint32_t evAddr = (cb.tag << (_setBits + _offsetBits)) + (set << _offsetBits); //with offset 0
        throw EvictedBlock(evAddr, cb.dirty);
    }
}

bool cache::evict(uint32_t addr)
{
    uint32_t tag = getTag(addr), set = getSet(addr); 
    return _sets[set].evict(tag);
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
void cacheSet::insert(uint32_t tag, bool dirty) {
	if (_ways.size() < _waySize) {
		_ways.push_back(cacheBlock(tag, dirty));
		return;
	}
	cacheBlock temp = _ways.front();
	_ways.pop_front();
	_ways.push_back(cacheBlock(tag, dirty));
	throw temp; // temp is evicted, throw it
}

bool cacheSet::evict(uint32_t tag)
{
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
        return false; // not found
	}
	bool currstate = itr->dirty;
    _ways.erase(itr);
	return currstate; // TODO: if dirty is relevant, need to check and writeback
}

//set line to dirty
void cacheSet::set_dirty(uint32_t tag) {
	list<cacheBlock>::iterator itr = find(tag);
	if (itr != _ways.end())
	{
		itr->dirty = true; // not found
	}
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
void victim::get(uint32_t addr, bool write_n_a) {
	uint32_t tag = addr >> _blockSize;
	list<cacheBlock>::iterator itr = _blocks.begin();
	while (itr != _blocks.end())
	{
		if (itr->tag == tag) {
			bool dirty = itr->dirty;
			if (write_n_a) { // if it's write command with w_n_a state
				itr->dirty = true;
                dirty = true;
			}else{
				_blocks.erase(itr);
			}
			throw found(VICTIM, dirty); //the data is in the victim cache and it's status is bool-dirty 
		}
		itr++;
	}
	return; // not found
}

// throws evicted block if any TODO: is this needed?
void victim::insert(uint32_t addr, bool dirty) {
	uint32_t tag = addr >> _blockSize;
    if (_blocks.size() < VICTIMSIZE) {
        _blocks.push_back(cacheBlock(tag, dirty));
        return;
    }
    _blocks.pop_front();
    _blocks.push_back(cacheBlock(tag, dirty)); //TODO maybe need to update somthing
}

MLCache::MLCache(uint32_t MemCyc, uint32_t BSize, uint32_t L1Size, uint32_t L2Size, uint32_t L1Assoc, uint32_t L2Assoc,
    uint32_t L1Cyc, uint32_t L2Cyc, uint32_t WrAlloc, uint32_t VicCache) :
    _MemCyc(MemCyc), _BSize(BSize), _L1Size(L1Size), _L2Size(L2Size), _L1Assoc(L1Assoc), _L2Assoc(L2Assoc),
    _L1Cyc(L1Cyc), _L2Cyc(L2Cyc), _WrAlloc(WrAlloc), _VicCache(VicCache),
    _L1Misses(0), _L2Misses(0), _totalTime(0), _L1Accesses(0), _L2Accesses(0),
    _L1(L1Size, L1Size - BSize - L1Assoc - 1, BSize, L1Assoc, static_cast<policy>(WrAlloc),L1), _L2(L2Size, L2Size - BSize - L2Assoc - 1 , BSize, L2Assoc,static_cast<policy>(WrAlloc), L2), _vict(BSize) {}


void MLCache::read(uint32_t addr)
{
	cout << "trnsaction num: " << d_count << "op is read" <<endl; //DEBUG
    // check if exists in L1 cache.
    try
    {
        _totalTime += _L1Cyc; // add access time
        ++_L1Accesses;
        _L1.read(addr); // try reading addr from L1, will throw exception if missing
		cout << "l1 miss ";//DEBUG
		// if we are here, addr was not found yet, check L2
        ++_L1Misses;
        _totalTime += _L2Cyc; // add access time
        ++_L2Accesses;
        _L2.read(addr); // try reading addr from L2
		cout << "l2 miss ";//DEBUG       
		// not found in L2, check victim if exists
        ++_L2Misses;
        if (_VicCache)
        {
            _totalTime += VICTIMTIME;
            _vict.get(addr);
			cout << "vic miss";//DEBUG
        }
        // not in any cache, get from mem
        _totalTime += _MemCyc;
        // TODO: do we need more time for inserting new entry?
        throw found(MEMORY);
    }
    catch (const found& fn)
    {
        copyToCaches(addr, fn.location, fn.dirty);
    }
	d_count++;
	cout << endl;
}

void MLCache::write(uint32_t addr)
{
	cout << "trnsaction num: " << d_count << "op is write" << endl; //DEBUG
    // try caches until write success is thrown
    try
    {
        _totalTime += _L1Cyc; // add access time
        ++_L1Accesses;
        _L1.write(addr); // try writing to addr in L1, will throw exception if succeeded 
        // if we are here, addr was not in L1, try L2
        ++_L1Misses;
        _totalTime += _L2Cyc; 
        ++_L2Accesses;
		cout << "l1 miss ";//DEBUG
        _L2.write(addr); // try reading addr from L2
        // not in L2, check victim if exists
        ++_L2Misses;
		cout << "l2 miss ";//DEBUG
        if (_VicCache)
        {
            _totalTime += VICTIMTIME;
			_vict.get(addr, true); // TODO: how to handle Victim cache writes
			cout << "vic miss ";//DEBUG
        }
        // not in any cache, will require mem access
        _totalTime += _MemCyc;
        // TODO: do we need more time for inserting new entry?
        throw found(MEMORY);
    }
    catch (const found& fn)
    {
        if (_WrAlloc) {
            cout << "write alloc, update tabels" << endl; // TODO DEBUG
            copyToCaches(addr, fn.location, fn.dirty);
        }
    }
    ++d_count; // TODO DEBUG
    cout << endl; // TODO DEBUG
}

void MLCache::copyToCaches(uint32_t addr, level fromLevel, bool dirty)
{
    if(fromLevel == L1) return; // nothing to copy
    if (fromLevel != L2) // need to copy into L2 too
    {
        try
        {
            _L2.insert(addr); // only L1 will keep dirty flag
        }
        catch (const EvictedBlock& eb) // there was an eviction in L2, evict from L1
        {
			//snooping
			bool dirty = _L1.evict(eb.addr);
			// send the block to VicCache
			if (_VicCache)
				_vict.insert(eb.addr, dirty || eb.dirty);
        }
    }
    // insert to L1
    try
    {
        _L1.insert(addr, dirty);
    }
    catch (const EvictedBlock& eb)
    {
		if (eb.dirty == true) {//need to writeback to L2
            try
            {
                _L2.write(eb.addr);
                throw exception("Did not find addr in L2 after WB from L1"); // Should not happen!!
            }
            catch (found) {} // since we wrote back from L1, should always find in L2
		}
    }
}


stats MLCache::getStats()
{
    stats st;
    st.L1MissRate = ((double)_L1Misses) / _L1Accesses;
    st.L2MissRate = ((double)_L2Misses) / _L2Accesses;
    st.avgAccTime = ((double)_totalTime) / _L1Accesses;
    return st;
}