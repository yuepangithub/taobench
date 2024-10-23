#ifndef DB_WRAPPER_H_
#define DB_WRAPPER_H_

#include <string>
#include <vector>
#include <iostream>
#include <cassert>

#include "db.h"
#include "measurements.h"
#include "timer.h"
#include "utils.h"
#include "db_factory.h"

namespace benchmark {

// Wrapper Class around DB; times and logs each Execute and ExecuteTransaction operation.
class DBWrapper : public DB {
 public:
  DBWrapper(DB *db, Measurements *measurements, MemcacheWrapper *memcache) :
    db_(db) , measurements_(measurements) , memcache_(memcache) {}
  ~DBWrapper() {
    delete db_;
  }
  void Init() {
    db_->Init();
  }
  void Cleanup() {
    db_->Cleanup();
    if (memcache_ != nullptr) {
      memcache_-> Cleanup();
    }
  }
  Status Read(DataTable table, const std::vector<Field> &key,
              std::vector<TimestampValue> &buffer) {
    throw std::invalid_argument("DBWrapper Read method should never be called.");
  }

  Status Scan(DataTable table, const std::vector<Field> &key, int n,
              std::vector<TimestampValue> &buffer) {
    throw std::invalid_argument("DBWrapper Scan method should never be called.");
  }

  Status Update(DataTable table, const std::vector<Field> &key, const TimestampValue &value) {
    throw std::invalid_argument("DBWrapper Update method should never be called.");
  }

  Status Insert(DataTable table, const std::vector<Field> &key, const TimestampValue &value) {
    throw std::invalid_argument("DBWrapper Insert method should never be called.");
  }

  Status Delete(DataTable table, const std::vector<Field> &key, const TimestampValue &value) {
    throw std::invalid_argument("DBWrapper Delete method should never be called.");
  }

  Status Execute(const DB_Operation &operation,
                 std::vector<TimestampValue> &read_buffer,
                 bool txn_op = false) {
    bool read_only = operation.operation == Operation::READ;

    timer_.Start();
    MemcacheRequest req{std::vector<DB_Operation>{operation}, txn_op, read_only};
    MemcacheResponse resp = memcache_->Execute(req);
    read_buffer.insert(read_buffer.end(), resp.read_buffer.begin(), resp.read_buffer.end());
    uint64_t elapsed = timer_.End();

    if (resp.s == Status::kOK) {
      measurements_->Report(operation.operation, elapsed);
      measurements_->ReportRead(resp.hit_count, resp.read_count);
    }
    return resp.s;
  }

  Status ExecuteTransaction(const std::vector<DB_Operation> &operations,
                            std::vector<TimestampValue> &read_buffer,
                            bool read_only = false)
  {
    timer_.Start();
    MemcacheRequest req{operations, true, read_only};
    MemcacheResponse resp = memcache_->ExecuteTransaction(req);
    read_buffer.insert(read_buffer.end(), resp.read_buffer.begin(), resp.read_buffer.end());
    uint64_t elapsed = timer_.End();

    if (resp.s != Status::kOK) {
      return resp.s;
    }
    if (read_only) {
      measurements_->Report(Operation::READTRANSACTION, elapsed);
      measurements_->ReportRead(resp.hit_count, resp.read_count);
    } else {
      measurements_->Report(Operation::WRITETRANSACTION, elapsed);
    }
    return resp.s;
  }

  Status BatchInsert(DataTable table, 
                     const std::vector<std::vector<Field>> &keys, 
                     const std::vector<TimestampValue> &values) 
  {
    return db_->BatchInsert(table, keys, values);
  }

  Status BatchRead(DataTable table,
                   const std::vector<Field> & floor,
                   const std::vector<Field> & ceil,
                   int n,
                   std::vector<std::vector<Field>> &key_buffer)
  {
    return db_->BatchRead(table, floor, ceil, n, key_buffer);
  }

 private:
  DB *db_;
  Measurements *measurements_;
  utils::Timer<uint64_t, std::nano> timer_;
  MemcacheWrapper *memcache_;
};

} // benchmark

#endif // DB_WRAPPER_H_
