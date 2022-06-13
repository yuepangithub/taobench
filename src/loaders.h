#pragma once
#include "workload_loader.h"

namespace benchmark {

  // Function run on each thread for batch reads.
  int BatchReadThread(std::shared_ptr<WorkloadLoader> loader) {

    // random offset for each thread so that the DB isn't hit by all threads at once
    std::this_thread::sleep_for(std::chrono::microseconds(std::rand() % 100000));
    return loader->LoadFromDB();
  }

  // Function run on each thread for batch inserts.
  int BatchInsertThread(std::shared_ptr<WorkloadLoader> loader, TraceGeneratorWorkload *wl, long num_ops) {
    // random offset for each thread so that the DB isn't hit by all threads at once
    std::this_thread::sleep_for(std::chrono::microseconds(std::rand() % 100000));
    int failed_ops = 0;
    for (long i = 0; i < num_ops; ++i) {
      failed_ops += wl->LoadRow(*loader);
    }
    failed_ops += loader->FlushObjectBuffer() + loader->FlushEdgeBuffer();
    return failed_ops;
  }
}