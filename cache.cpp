#include <math.h>
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

cacheSet::cacheSet(uint32_t blockSize, uint32_t waySize): _ways(), _blockSize(blockSize), _waySize(waySize){}

// throws if tag does not exist
void cacheSet::read(uint32_t tag, uint32_t offset) {//Cache fix the tag
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
		throw notfound;
	}
	cacheBlock temp = *itr;
	_ways.erase(itr);
	_ways.push_back(temp);
	throw temp;
}

// throws if tag does not exist
void cacheSet::write(uint32_t tag, uint32_t offset) {
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
		throw notfound;
	}
	cacheBlock temp = *itr;
	_ways.erase(itr);
	temp.dirty = true;
	_ways.push_back(temp);
	throw temp;
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
	throw temp;
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
			throw temp;
		}
		itr++;
	}
	throw notfound;
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
