// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_
#include <unistd.h>
#include <deque>
#include <set>
#include "db/dbformat.h"
#include "db/log_writer.h"
#include "db/snapshot.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "port/port.h"
#include "port/thread_annotations.h"
#include "db/memtable.h"
#include "util/thpool.h"

#define NUMEMTABLE 0
#define NUMEMTABLE_NVM 10


namespace leveldb {

class MemTable;
class TableCache;
class Version;
class VersionEdit;
class VersionSet;

class MultiMem {
    MemTable **mem_;

public:
    int idx_start_;
    int idx_end_;
    int num_mem_;
    int size;
    MultiMem(int mem_size) {
        num_mem_ = mem_size;
        idx_start_ = idx_end_ = -1;
        mem_ = new MemTable*[mem_size];
        size = 0;
    }

    MemTable* getCompactionMem();
    int AssignHeadIndex(MemTable *);
    bool MemIsFull();
    bool MemIsEmpty();
    MemTable** getMemList(int *sz) {
        *sz = size;
        return mem_;
    }

    void reset() {
        idx_start_ = idx_end_ = -1;
        size = 0;
    }
};

class DBImpl : public DB {
public:
    DBImpl(const Options& options, const std::string& dbname_disk, const std::string& dbname_mem);
    virtual ~DBImpl();

    // Implementations of the DB interface
    virtual Status Put(const WriteOptions&, const Slice& key, const Slice& value);
    virtual Status Delete(const WriteOptions&, const Slice& key);
    virtual Status Write(const WriteOptions& options, WriteBatch* updates);
    virtual Status Get(const ReadOptions& options,
            const Slice& key,
            std::string* value);
    virtual Iterator* NewIterator(const ReadOptions&);
    virtual const Snapshot* GetSnapshot();
    virtual void ReleaseSnapshot(const Snapshot* snapshot);
    virtual bool GetProperty(const Slice& property, std::string* value);
    virtual void GetApproximateSizes(const Range* range, int n, uint64_t* sizes);
    virtual void CompactRange(const Slice* begin, const Slice* end);

    // Extra methods (for testing) that are not in the public DB interface

    // Compact any files in the named level that overlap [*begin,*end]
    void TEST_CompactRange(int level, const Slice* begin, const Slice* end);

    // Force current memtable contents to be compacted.
    Status TEST_CompactMemTable();

    // Return an internal iterator over the current state of the database.
    // The keys of this iterator are internal keys (see format.h).
    // The returned iterator should be deleted when no longer needed.
    Iterator* TEST_NewInternalIterator();

    // Return the maximum overlapping data (in bytes) at next level for any
    // file at a level >= 1.
    int64_t TEST_MaxNextLevelOverlappingBytes();

    // Record a sample of bytes read at the specified internal key.
    // Samples are taken approximately once every config::kReadBytesPeriod
    // bytes.
    void RecordReadSample(Slice key);
    bool search_thread_multilevel (int val, LookupKey *lkey, std::string *value, Status *s);
    bool CheckSearchCondition(MemTable* mem);

    //NoveLSM Mem2 creation
    MemTable* CreateNVMtable(bool assign_map = false);
    MemTable* CreateMemTable(void);

    void DebugMemTable(MemTable *mem);

    //For hit stats accounting
    void IncrementHitFlag();
    //Clearing flags. Should be called before get
    void ClearThreadFlags();
    //Function to alternate between DRAM and NVM memtable
    int SwapMemtables();

    typedef struct read_struct {
        int val;
        LookupKey *lkey;
        DBImpl *db;
        //port::CondVar *rt_cv_;
        //port::Mutex *my_mutex_;
        //volatile uint64_t *complete;
        uint64_t *result;
        std::string *value;
        Status *s;
        bool *have_stat_update;
        bool done;
        Version* current;
        void *stats;
        //const leveldb::ReadOptions *myoptions;
    }read_struct;

    //NoveLSM Swap/Alternate between NVM and DRAM arena
    size_t drambuff_;
    size_t nvmbuff_;

private:
    friend class DB;
    struct CompactionState;
    struct Writer;

    std::string getDBName(int level);

    //NoveLSM Also  take file number as input to decide where t
    //to place the file
    std::string getDBNameFilenum(int level, uint64_t filenumber);

    Iterator* NewInternalIterator(const ReadOptions&,
            SequenceNumber* latest_snapshot,
            uint32_t* seed);

    Status NewDB();

    // Recover the descriptor from persistent storage.  May do a significant
    // amount of work to recover recently logged updates.  Any changes to
    // be made to the descriptor are added to *edit.
    Status Recover(VersionEdit* edit, bool* save_manifest)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    void MaybeIgnoreError(Status* s) const;

    // Delete any unneeded files and stale in-memory entries.
    void DeleteObsoleteFiles();

    // Compact the in-memory write buffer to disk.  Switches to a new
    // log-file/memtable and writes a new descriptor iff successful.
    // Errors are recorded in bg_error_.
    void CompactBottomMemTable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    void CompactTopMemTable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void CompactTopMemTable_Norelease() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    static void *read_thread(void *arg);
    bool thread_task(read_struct *str, std::string *value);

    Status RecoverLogFile(uint64_t log_number, bool last_log, bool* save_manifest,
            VersionEdit* edit, SequenceNumber* max_sequence)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
#ifdef ENABLE_RECOVERY
    Status RecoverMapFile(uint64_t map_number, bool* save_manifest,
            VersionEdit* edit, SequenceNumber* max_sequence)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
#endif
    Status WriteLevel0Table(MemTable* mem, VersionEdit* edit, Version* base)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    Status MakeRoomForWrite(bool force /* compact even if there is room? */)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    WriteBatch* BuildBatchGroup(Writer** last_writer);

    void RecordBackgroundError(const Status& s);

    void MaybeScheduleCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void ScheduleCompactionNow();
    static void BGWork(void* db);
    void BackgroundCall();
    void  BackgroundCompaction() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void CleanupCompaction(CompactionState* compact)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    Status DoCompactionWork(CompactionState* compact)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void IterateMemAndPrint(MemTable *mem);
    Status OpenCompactionOutputFile(CompactionState* compact);
    Status FinishCompactionOutputFile(CompactionState* compact, Iterator* input);
    Status InstallCompactionResults(CompactionState* compact)
    EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    bool needsCompaction();
    bool NVMemIsFull();
    int AssignHeadIndex(MemTable *mem);
    MemTable * getCompactionMem();

    // Constant after construction
    Env* const env_;
    const InternalKeyComparator internal_comparator_;
    const InternalFilterPolicy internal_filter_policy_;

    //NoveLSM: Dirty hack to alternate between DRAM and NVM tables
    Options options_;

    bool owns_info_log_;
    bool owns_cache_;
    const std::string dbname_disk_;
    const std::string dbname_secndry_disk_;
    const std::string dbname_mem_;

    // table_cache_ provides its own synchronization
    TableCache* table_cache_;

    // Lock over the persistent DB state.  Non-NULL iff successfully acquired.
    FileLock* db_lock_;

    // State below is protected by mutex_
    port::Mutex mutex_;
    port::AtomicPointer shutting_down_;
    port::CondVar bg_cv_;          // Signalled when background work finishes
    MemTable* mem_;
    MemTable* imm_;                // Memtable being compacted
    port::AtomicPointer has_imm_;  // So bg thread can detect non-NULL imm_
    WritableFile* logfile_;
    uint64_t logfile_number_;

    //NoveLSM: Recovery related code
    uint64_t mapfile_number_;

    log::Writer* log_;
    uint32_t seed_;                // For sampling.
    bool use_multiple_levels;
    threadpool thpool;

    // Queue of writers.
    std::deque<Writer*> writers_;
    WriteBatch* tmp_batch_;

    SnapshotList snapshots_;

    // Set of table files to protect from deletion because they are
    // part of ongoing compactions.
    std::set<uint64_t> pending_outputs_;

    // Has a background compaction been scheduled or is running?
    bool bg_compaction_scheduled_;

    // Information for a manual compaction
    struct ManualCompaction {
        int level;
        bool done;
        const InternalKey* begin;   // NULL means beginning of key range
        const InternalKey* end;     // NULL means end of key range
        InternalKey tmp_storage;    // Used to keep track of compaction progress
    };
    ManualCompaction* manual_compaction_;

    VersionSet* versions_;

    // Have we encountered a background error in paranoid mode?
    Status bg_error_;

    // Per level compaction stats.  stats_[level] stores the stats for
    // compactions that produced data for the specified "level".
    struct CompactionStats {
        int64_t micros;
        int64_t bytes_read;
        int64_t bytes_written;

        CompactionStats() : micros(0), bytes_read(0), bytes_written(0) { }

        void Add(const CompactionStats& c) {
            this->micros += c.micros;
            this->bytes_read += c.bytes_read;
            this->bytes_written += c.bytes_written;
        }
    };
    CompactionStats stats_[config::kNumLevels];

    // No copying allowed
    DBImpl(const DBImpl&);
    void operator=(const DBImpl&);

    const Comparator* user_comparator() const {
        return internal_comparator_.user_comparator();
    }
};

// Sanitize db options.  The caller should delete result.info_log if
// it is not equal to src.info_log.
extern Options SanitizeOptions(const std::string& db,
        const InternalKeyComparator* icmp,
        const InternalFilterPolicy* ipolicy,
        const Options& src);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_DB_IMPL_H_
