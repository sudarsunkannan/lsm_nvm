#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Thread safety
// -------------
//
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the SkipList will not be destroyed
// while the read is in progress.  Apart from that, reads progress
// without any internal locking or synchronization.
//
// Invariants:
//
// (1) Allocated nodes are never deleted until the SkipList is
// destroyed.  This is trivially guaranteed by the code since we
// never delete any skip list nodes.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the SkipList.
// Only Insert() modifies the list, and it is careful to initialize
// a node and use release-stores to publish the nodes in one or
// more lists.
//
// ... prev vs. next pointer ordering ...

#include <assert.h>
#include <stdlib.h>
#include "port/port.h"
#include "util/arena.h"
#include "util/random.h"
#include "port/cache_flush.h"


namespace leveldb {

class Arena;

template<typename Key, class Comparator>
class SkipList {

private:
    struct Node;

public:
    // Create a new SkipList object that will use "cmp" for comparing keys,
    // and will allocate memory using "*arena".  Objects allocated in the arena
    // must remain allocated for the lifetime of the skiplist object.
    explicit SkipList(Comparator cmp, Arena* arena, bool recovery = false);

    // Insert key into the list.
    // REQUIRES: nothing that compares equal to key is currently in the list.
#ifdef ENABLE_RECOVERY
    void Insert(const Key& key, uint64_t s);
#else
    void Insert(const Key& key);
#endif

    // Returns true iff an entry that compares equal to key is in the list.
    bool Contains(const Key& key) const;

    void SetHead(void *ptr);

    // Iteration over the contents of a skip list
    class Iterator {
    public:
        // Initialize an iterator over the specified list.
        // The returned iterator is not valid.
        explicit Iterator(const SkipList* list);

        // Returns true iff the iterator is positioned at a valid node.
        bool Valid() const;

        // Returns the key at the current position.
        // REQUIRES: Valid()

#ifdef USE_OFFSETS
        const Key& key_offset() const;
#else
        const Key& key() const;
#endif

        // Advances to the next position.
        // REQUIRES: Valid()
        void Next();

        // Advances to the previous position.
        // REQUIRES: Valid()
        void Prev();

        // Advance to the first entry with a key >= target
        void Seek(const Key& target);

        // Position at the first entry in list.
        // Final state of iterator is Valid() iff list is not empty.
        void SeekToFirst();

        // Position at the last entry in list.
        // Final state of iterator is Valid() iff list is not empty.
        void SeekToLast();

        //void SetHead(void *ptr);
        Node* node_;

    private:
        const SkipList* list_;
        // Intentionally copyable
    };

private:
    enum { kMaxHeight = 12 };

    // Immutable after construction
    Comparator const compare_;
    Arena* const arena_;    // Arena used for allocations of nodes

    //TODO: NoveLSM Make them private again
    //void* head_offset_;   // Head offset from map_start
    //Node* head_;

    // Modified only by Insert().  Read racily by readers, but stale
    // values are ok.
    port::AtomicPointer max_height_;   // Height of the entire list

    inline int GetMaxHeight() const {
        return static_cast<int>(
                reinterpret_cast<intptr_t>(max_height_.NoBarrier_Load()));
    }

    // Read/written only by Insert().
    Random rnd_;

    Node* NewNode(const Key& key, int height, bool head_alloc);
    int RandomHeight();
    bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

    // Return true if key is greater than the data stored in "n"
    bool KeyIsAfterNode(const Key& key, Node* n) const;

    // Return the earliest node that comes at or after key.
    // Return NULL if there is no such node.
    //
    // If prev is non-NULL, fills prev[level] with pointer to previous
    // node at "level" for every level in [0..max_height_-1].
    Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

    // Return the latest node with a key < key.
    // Return head_ if there is no such node.
    Node* FindLessThan(const Key& key) const;

    // Return the last node in the list.
    // Return head_ if list is empty.
    Node* FindLast() const;

    // No copying allowed
    SkipList(const SkipList&);
    void operator=(const SkipList&);

public:
    //TODO: NoveLSM Make them private again
    void* head_offset_;   // Head offset from map_start
    Node* head_;
    size_t* alloc_rem;
    uint64_t *sequence;
    int* m_height;
};

// Implementation details follow
template<typename Key, class Comparator>
struct SkipList<Key,Comparator>::Node {
#ifdef USE_OFFSETS
    explicit Node(const Key& k, const Key& mem) : key_offset(reinterpret_cast<const Key>(mem - k)) { }

    Key const key_offset;
#else
    explicit Node(const Key& k) : key(k) { }

    Key const key;
#endif
    // Accessors/mutators for links.  Wrapped in methods so we can
    // add the appropriate barriers as necessary.
    Node* Next(int n) {
        assert(n >= 0);

#if defined(USE_OFFSETS)
        intptr_t offset = reinterpret_cast<intptr_t>( next_[n].Acquire_Load());
        return (offset != 0) ? reinterpret_cast<Node *>((intptr_t)this - offset) : NULL;
#else
        // Use an 'acquire load' so that we observe a fully initialized
        // version of the returned Node.
        return reinterpret_cast<Node*>(next_[n].Acquire_Load());
#endif
    }
    void SetNext(int n, Node* x) {
        assert(n >= 0);
#if defined(USE_OFFSETS)
        (x != NULL) ? next_[n].Release_Store(reinterpret_cast<void*>((intptr_t)this - (intptr_t)x)) : next_[n].Release_Store(reinterpret_cast<void*>(0));
#else
        // Use a 'release store' so that anybody who reads through this
        // pointer observes a fully initialized version of the inserted node.
        next_[n].Release_Store(x);
#endif
    }

    // No-barrier variants that can be safely used in a few locations.
    Node* NoBarrier_Next(int n) {
        assert(n >= 0);
#if defined(USE_OFFSETS)
        intptr_t offset = reinterpret_cast<intptr_t>(next_[n].NoBarrier_Load());
        return (offset != 0) ? reinterpret_cast<Node *>((intptr_t)this - offset) : NULL;
#else
        return reinterpret_cast<Node*>(next_[n].NoBarrier_Load());
#endif
    }
    void NoBarrier_SetNext(int n, Node* x) {
        assert(n >= 0);
#if defined(USE_OFFSETS)
        (x != NULL) ? next_[n].NoBarrier_Store( reinterpret_cast<void*> ((intptr_t)this - (intptr_t)x)) : next_[n].NoBarrier_Store( reinterpret_cast<void*> (0));
#else
        next_[n].NoBarrier_Store(x);
#endif
    }

private:
    // Array of length equal to the node height.  next_[0] is lowest level link.
    port::AtomicPointer next_[1];
};

template<typename Key, class Comparator>
typename SkipList<Key,Comparator>::Node*
SkipList<Key,Comparator>::NewNode(const Key& key, int height, bool head_alloc) {
    char* mem;
    bool return_special = head_alloc && arena_->nvmarena_;
    if(arena_->nvmarena_) {
        ArenaNVM *nvm_arena = (ArenaNVM *)arena_;
        if (head_alloc == true)
            mem = nvm_arena->AllocateAlignedNVM(
                    sizeof(size_t) + sizeof (uint64_t) + sizeof(int) + sizeof(Node) + sizeof(port::AtomicPointer) * (height - 1));
        else
            mem = nvm_arena->AllocateAlignedNVM(
                    sizeof(Node) + sizeof(port::AtomicPointer) * (height - 1));
    }else {
        mem = arena_->AllocateAligned(
                sizeof(Node) + sizeof(port::AtomicPointer) * (height - 1));
    }
#if !defined(USE_OFFSETS)
    return new (mem) Node(key);
#else
#ifdef ENABLE_RECOVERY
    if (return_special) {
        char *offset_mem = mem + sizeof(size_t) + sizeof (uint64_t) + sizeof(int);
        return new (offset_mem) Node(key, mem);
    } else {
        return new (mem) Node(key, mem);
    }
#else
    return new (mem) Node(key, mem);
#endif
#endif
}

template<typename Key, class Comparator>
inline SkipList<Key,Comparator>::Iterator::Iterator(const SkipList* list) {
    list_ = list;
    node_ = NULL;
}

template<typename Key, class Comparator>
inline bool SkipList<Key,Comparator>::Iterator::Valid() const {
    return node_ != NULL;
}

#ifdef USE_OFFSETS
template<typename Key, class Comparator>
inline const Key& SkipList<Key,Comparator>::Iterator::key_offset() const {
#else
    template<typename Key, class Comparator>
    inline const Key& SkipList<Key,Comparator>::Iterator::key() const {
#endif
        assert(Valid());
#if defined(USE_OFFSETS)
        return node_->key_offset;
#else
        return node_->key;
#endif
    }

    template<typename Key, class Comparator>
    inline void SkipList<Key,Comparator>::Iterator::Next() {
        assert(Valid());
        node_ = node_->Next(0);
    }

    template<typename Key, class Comparator>
    inline void SkipList<Key,Comparator>::Iterator::Prev() {
        // Instead of using explicit "prev" links, we just search for the
        // last node that falls before key.
        assert(Valid());
#if defined(USE_OFFSETS)
        node_ = list_->FindLessThan(reinterpret_cast<Key>((intptr_t)node_ - (intptr_t)node_->key_offset));
#else
        node_ = list_->FindLessThan(node_->key);
#endif
        if (node_ == list_->head_) {
            node_ = NULL;
        }
    }

    /*template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::SetHead(void *ptr) {
  //list_->head_= (Node *)ptr;
}*/

    template<typename Key, class Comparator>
    inline void SkipList<Key,Comparator>::Iterator::Seek(const Key& target) {
        node_ = list_->FindGreaterOrEqual(target, NULL);
    }

    template<typename Key, class Comparator>
    inline void SkipList<Key,Comparator>::Iterator::SeekToFirst() {
        node_ = list_->head_->Next(0);
    }

    template<typename Key, class Comparator>
    inline void SkipList<Key,Comparator>::Iterator::SeekToLast() {
        node_ = list_->FindLast();
        if (node_ == list_->head_) {
            node_ = NULL;
        }
    }

    template<typename Key, class Comparator>
    int SkipList<Key,Comparator>::RandomHeight() {
        // Increase height with probability 1 in kBranching
        static const unsigned int kBranching = 4;
        int height = 1;
        while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
            height++;
        }
        assert(height > 0);
        assert(height <= kMaxHeight);
        return height;
    }

    template<typename Key, class Comparator>
    bool SkipList<Key,Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
        // NULL n is considered infinite
#if defined(USE_OFFSETS)
        return (n != NULL) && (compare_(reinterpret_cast<Key>((intptr_t)n - (intptr_t)n->key_offset), key) < 0);
#else
        return (n != NULL) && (compare_(n->key, key) < 0);
#endif
    }

    template<typename Key, class Comparator>
    typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::FindGreaterOrEqual(const Key& key, Node** prev)
    const {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
        while (true) {
            Node* next = x->Next(level);
            if (KeyIsAfterNode(key, next)) {
                // Keep searching in this list
                x = next;
            } else {
                if (prev != NULL) prev[level] = x;
                if (level == 0) {
                    return next;
                } else {
                    // Switch to next list
                    level--;
                }
            }
        }
    }

    template<typename Key, class Comparator>
    typename SkipList<Key,Comparator>::Node*
    SkipList<Key,Comparator>::FindLessThan(const Key& key) const {
        Node* x = head_;
        int level = GetMaxHeight() - 1;
        while (true) {
#if defined(USE_OFFSETS)
            assert(x == head_ || compare_(reinterpret_cast<Key>((intptr_t)x - (intptr_t)x->key_offset), key) < 0);
#else
            assert(x == head_ || compare_(x->key, key) < 0);
#endif
            Node* next = x->Next(level);
#if defined(USE_OFFSETS)
            if (next == NULL || compare_(reinterpret_cast<Key>((intptr_t)next - (intptr_t)next->key_offset), key) >= 0) {
#else
                if (next == NULL || compare_(next->key, key) >= 0) {
#endif
                    if (level == 0) {
                        return x;
                    } else {
                        // Switch to next list
                        level--;
                    }
                } else {
                    x = next;
                }
            }
        }

        template<typename Key, class Comparator>
        typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::FindLast()
        const {
            Node* x = head_;
            int level = GetMaxHeight() - 1;
            while (true) {
                Node* next = x->Next(level);
                if (next == NULL) {
                    if (level == 0) {
                        return x;
                    } else {
                        // Switch to next list
                        level--;
                    }
                } else {
                    x = next;
                }
            }
        }

        template<typename Key, class Comparator>
        SkipList<Key,Comparator>::SkipList(Comparator cmp, Arena* arena, bool recovery)
        : compare_(cmp),
          arena_(arena),
          //head_(NewNode(0 /* any key will do */, kMaxHeight)),
          max_height_(reinterpret_cast<void*>(1)),
          rnd_(0xdeadbeef) {
#ifdef ENABLE_RECOVERY
            if (recovery) {
                ArenaNVM *arena_nvm = (ArenaNVM*) arena;
                head_ = (Node*)((uint8_t*)arena_nvm->getMapStart() + sizeof(size_t) + sizeof(uint64_t) + sizeof(int));
                alloc_rem = (size_t *)arena_nvm->getMapStart();
                sequence = (uint64_t *)((uint8_t*)arena_nvm->getMapStart() + sizeof(size_t));
                m_height = (int *)((uint8_t*)arena_nvm->getMapStart() + sizeof(size_t) + sizeof(uint64_t));
                max_height_.NoBarrier_Store(reinterpret_cast<void*>(*m_height));
            }
            else
#endif
#ifdef ENABLE_RECOVERY
                head_ = NewNode(0, kMaxHeight, true);
#else
            head_ = NewNode(0, kMaxHeight, false);
#endif

#ifdef ENABLE_RECOVERY
            if (!recovery && arena->nvmarena_) {
                ArenaNVM *arena_nvm = (ArenaNVM*) arena;
                alloc_rem = (size_t *)arena_nvm->getMapStart();
                *alloc_rem = arena_->getAllocRem();
                flush_cache(alloc_rem, CACHE_LINE_SIZE);

                sequence = (uint64_t *)((uint8_t*)arena_->getMapStart() + sizeof(size_t));
                *sequence = 0;
                flush_cache(sequence, CACHE_LINE_SIZE);

                m_height = (int *)((uint8_t*)arena_->getMapStart() + sizeof(size_t) + sizeof(uint64_t));
                *m_height = GetMaxHeight();
		flush_cache(m_height, CACHE_LINE_SIZE);
            }
#endif

            //NoveLSM: We find the offset from the starting address
            head_offset_ = (reinterpret_cast<void*>(arena_->CalculateOffset(static_cast<void*>(head_))));
            //head_offset_ = (size_t)(arena_->CalculateOffset(static_cast<void*>(head_)));

            if (!recovery) {
                for (int i = 0; i < kMaxHeight; i++) {
                    head_->SetNext(i, NULL);
                }
            }
        }

#ifdef ENABLE_RECOVERY
        template<typename Key, class Comparator>
        void SkipList<Key,Comparator>::Insert(const Key& key, uint64_t s=0) {
#else
            template<typename Key, class Comparator>
            void SkipList<Key,Comparator>::Insert(const Key& key) {
#endif
                // TODO(opt): We can use a barrier-free variant of FindGreaterOrEqual()
                // here since Insert() is externally synchronized.
                Node* prev[kMaxHeight];
                Node* x = FindGreaterOrEqual(key, prev);

                // Update sequence number before updating data
#ifdef ENABLE_RECOVERY
                if (arena_->nvmarena_) {
                    *sequence = s;
                }
#endif

                // Our data structure does not allow duplicate insertion
#if defined(USE_OFFSETS)
                assert(x == NULL || !Equal(key, reinterpret_cast<Key>((intptr_t)x - (intptr_t)x->key_offset)));
#else
                assert(x == NULL || !Equal(key, x->key));
#endif

                int height = RandomHeight();
                if (height > GetMaxHeight()) {
                    for (int i = GetMaxHeight(); i < height; i++) {
                        prev[i] = head_;
                    }
                    //fprintf(stderr, "Change height from %d to %d\n", max_height_, height);

                    // It is ok to mutate max_height_ without any synchronization
                    // with concurrent readers.  A concurrent reader that observes
                    // the new value of max_height_ will see either the old value of
                    // new level pointers from head_ (NULL), or a new value set in
                    // the loop below.  In the former case the reader will
                    // immediately drop to the next level since NULL sorts after all
                    // keys.  In the latter case the reader will use the new node.
                    max_height_.NoBarrier_Store(reinterpret_cast<void*>(height));
                }

                x = NewNode(key, height, false);
                for (int i = 0; i < height; i++) {
                    // NoBarrier_SetNext() suffices since we will add a barrier when
                    // we publish a pointer to "x" in prev[i].
                    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
                    prev[i]->SetNext(i, x);
                    if (arena_->nvmarena_ == true) {
                        flush_cache((void *)x, sizeof(Node));
                        flush_cache((void *)prev[i], sizeof(Node));
                    }
                }
#ifdef ENABLE_RECOVERY
                if (arena_->nvmarena_) {
                    *alloc_rem = arena_->getAllocRem();
                    // Set max_height after insertion to ensure correctness.
                    // If NovelSM crashes before updating this, it would just
                    // lead to inefficient lookups (O(n) vs O(logn)).
                    *m_height = GetMaxHeight();
                }
#endif
            }

            template<typename Key, class Comparator>
            bool SkipList<Key,Comparator>::Contains(const Key& key) const {
                Node* x = FindGreaterOrEqual(key, NULL);
#if defined(USE_OFFSETS)
                if (x != NULL && Equal(key, reinterpret_cast<Key>((intptr_t)x - (intptr_t)x->key_offset))) {
#else
                    if (x != NULL && Equal(key, x->key)) {
#endif
                        return true;
                    } else {
                        return false;
                    }
                }
                template<typename Key, class Comparator>
                void SkipList<Key,Comparator>::SetHead(void *ptr){
                    head_ = reinterpret_cast<Node *>(ptr);
                    head_offset_ = (reinterpret_cast<void*>(arena_->CalculateOffset(static_cast<void*>(head_))));
                }

            }  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_SKIPLIST_H_


