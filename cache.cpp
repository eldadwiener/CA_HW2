#include "cache.h"

cacheSet::cacheSet(uint32_t blockSize, uint32_t waySize): _ways(), _blockSize(blockSize), _waySize(waySize){}

// throws if tag does not exist
void cacheSet::read(uint32_t tag, uint32_t offset) {//Cache fix the tag
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
		throw notfound;
	}
	throw itr;
}

// throws if tag does not exist
void cacheSet::write(uint32_t tag, uint32_t offset) {
	list<cacheBlock>::iterator itr = find(tag);
	if (itr == _ways.end())
	{
		throw notfound;
	}
	itr->dirty = true;
	throw itr;
}

// throws evicted block if any
void cacheSet::insert(uint32_t tag) {

	if (_ways.size() < _waySize) {
		_ways.push_back(cacheBlock(tag));
		_waySize++;
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


