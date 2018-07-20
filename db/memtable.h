// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_MEMTABLE_H_

#include <string>
#include "leveldb/db.h"
#include "db/dbformat.h"
#include "db/skiplist.h"
#include "util/arena.h"
#include "util/BloomFilter.h"

#include <string>
#include <unordered_set>

namespace leveldb {

class InternalKeyComparator;
class Mutex;
class MemTableIterator;

class MemTable {
public:

	// replacement of a minimal set of functions:
	void* operator new(std::size_t sz);
	void operator delete(void* ptr);
	void *operator new[](std::size_t sz);

	// MemTables are reference counted.  The initial reference count
	// is zero and the caller must call Ref() at least once.
	explicit MemTable(const InternalKeyComparator& comparator);
	explicit MemTable(const InternalKeyComparator& cmp, ArenaNVM&  arena, bool recovery);

	// Increase reference count.
	void Ref() {
		++refs_;
		//numkeys_ = 0;
	}

	//Increase the number of memtable keys
	void IncrKeys() { ++numkeys_; }
	//Decrease the number of memtable keys
	void DecrKeys() {
		if(numkeys_)
		  numkeys_ = numkeys_-1;
	}
	//Get keys
    unsigned int GetNumKeys(){
        return numkeys_;
    }


	// Drop reference count.  Delete if no more references exist.
	void Unref() {
		--refs_;
		assert(refs_ >= 0);
		if (refs_ <= 0) {
			delete this;
		}
	}

	// Returns an estimate of the number of bytes of data in use by this
	// data structure. It is safe to call when MemTable is being modified.
	size_t ApproximateMemoryUsage();

	// Return an iterator that yields the contents of the memtable.
	//
	// The caller must ensure that the underlying MemTable remains live
	// while the returned iterator is live.  The keys returned by this
	// iterator are internal keys encoded by AppendInternalKey in the
	// db/format.{h,cc} module.
	Iterator* NewIterator();

	// Add an entry into memtable that maps key to value at the
	// specified sequence number and with the specified type.
	// Typically value will be empty if type==kTypeDeletion.
	void Add(SequenceNumber seq, ValueType type,
			const Slice& key,
			const Slice& value);

	//NoveLSM:TODO: To purge
	//void AddSpecial(const Slice& key, const Slice& value, char *keybuf);

	// If memtable contains a value for key, store it in *value and return true.
	// If memtable contains a deletion for key, store a NotFound() error
	// in *status and return true.
	// Else, return false.
	bool Get(const LookupKey& key, std::string* value, Status* s);

	void SetMemTableHead(void *ptr);

	void* GeTableoffset();

	uint64_t logfile_number;


	//NoveLSM: Making them public for easier debugging
	//TODO: Revert back to private mode
	Arena arena_;

	//NoveLSM Swap/Alternate between nvm and DRAM arena
	bool isNVMMemtable;

       BloomFilter bloom_;
       std::unordered_set<std::string> predict_set;
       void AddPredictIndex(std::unordered_set<std::string> *set, const uint8_t*);
       int  CheckPredictIndex(std::unordered_set<std::string> *set, const uint8_t*);
       void ClearPredictIndex(std::unordered_set<std::string> *set);

private:
	~MemTable();  // Private since only Unref() should be used to delete it

	struct KeyComparator {
		const InternalKeyComparator comparator;
		explicit KeyComparator(const InternalKeyComparator& c) : comparator(c) { }
		int operator()(const char* a, const char* b) const;
	};
	friend class MemTableIterator;
	friend class MemTableBackwardIterator;

	typedef SkipList<const char*, KeyComparator> Table;

	KeyComparator comparator_;
	int refs_;

	//NoveLSM: Num memtable enteries
	unsigned int numkeys_;

	//NoveLSM: Making them public for easier debugging
	//TODO: Revert back to private mode
	//Arena arena_;
	Table table_;

	// No copying allowed
	MemTable(const MemTable&);
	void operator=(const MemTable&);
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_MEMTABLE_H_
