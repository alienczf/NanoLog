/* Copyright (c) 2016-2018 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * This file implements a basic benchmarking utility for NanoLog for internal
 * use. It breaks all abstractions to retrieve hidden metrics. It is NOT
 * recommended to use any of the code here as examples.
 */

#define EXPOSE_PRIVATES

#include <pthread.h>
#include <stdint.h>
#include <xmmintrin.h>

#include <functional>
#include <thread>
#include <vector>

#include "Cycles.h"
#include "NanoLogCpp17.h"
#include "TimeTrace.h"

static const uint32_t BENCHMARK_THREADS = 5;
static const uint64_t ITERATIONS = 100000000;

using namespace NanoLog::LogLevels;

template <typename T>
void runBenchmark(int id, pthread_barrier_t* barrier, T& bench_ops) {
  // Number of messages to log repeatedly and take the average latency
  uint64_t start, stop;
  double time;

  // PerfUtils::Util::pinThreadToCore(1);

  // Optional optimization: pre-allocates thread-local data structures
  // needed by NanoLog. This should be invoked once per new
  // thread that will use the NanoLog system.
  PerfUtils::TimeTrace::record("Thread[%d]: Preallocation", id);
  PerfUtils::TimeTrace::record("Thread[%d]: Preallocation", id);
  NanoLog::preallocate();
  PerfUtils::TimeTrace::record("Thread[%d]: Preallocation Done", id);

  PerfUtils::TimeTrace::record("Thread[%d]: Waiting for barrier...", id);
  pthread_barrier_wait(barrier);

  PerfUtils::TimeTrace::record("Thread[%d]: Starting benchmark", id);
  start = PerfUtils::Cycles::rdtsc();

  for (size_t i = 0; i < ITERATIONS; ++i) {
    bench_ops();
  }
  stop = PerfUtils::Cycles::rdtsc();
  PerfUtils::TimeTrace::record("Thread[%d]: Benchmark Done", id);

  time = PerfUtils::Cycles::toSeconds(stop - start);
  printf(
      "Thread[%d]: The total time spent invoking BENCH_OP %lu "
      "times took %0.2lf seconds (%0.2lf ns/op average)\r\n",
      id, ITERATIONS, time, (time / ITERATIONS) * 1e9);

  // Break abstraction to bring metrics on cycles blocked.
  uint32_t nBlocks =
      NanoLogInternal::RuntimeLogger::stagingBuffer->numTimesProducerBlocked;
  printf("Thread[%d]: Times producer was stuck:%u\r\n", id, nBlocks);
}

int main(int argc, char** argv) {
  // Optional: Set the output location for the NanoLog system. By default
  // the log will be output to /tmp/compressedLog
  std::vector<std::pair<std::string, std::function<void()>>> ops{
      {"staticString",
       []() {
         NANO_LOG(NOTICE, "Starting backup replica garbage collector thread");
       }},
      {"stringConcat",
       []() {
         NANO_LOG(NOTICE, "Opened session with coordinator at %s",
                  "basic+udp:host=192.168.1.140,port=12246");
       }},
      {"singleInteger",
       []() {
         NANO_LOG(NOTICE, "Backup storage speeds (min): %d MB/s read", 181);
       }},
      {"twoIntegers",
       []() {
         NANO_LOG(NOTICE,
                  "buffer has consumed %lu bytes of extra storage, current "
                  "allocation: %lu bytes",
                  1032024lu, 1016544lu);
       }},
      {"singleDouble",
       []() {
         NANO_LOG(NOTICE, "Using tombstone ratio balancer with ratio = %0.6lf",
                  0.400000);
       }},
      {"complexFormat", []() {
         NANO_LOG(NOTICE,
                  "Initialized InfUdDriver buffers: %lu receive buffers (%u "
                  "MB), %u transmit buffers (%u MB), took %0.1lf ms",
                  50000lu, 97, 50, 0, 26.2);
       }}};

  std::vector<std::tuple<uint64_t, uint64_t, double, double, double>> results;
  for (auto& op : ops) {
    const std::string output_fn = "/tmp/benchmark_" + op.first + ".log";
    const uint64_t preEvents =
        NanoLogInternal::RuntimeLogger::nanoLogSingleton.logsProcessed;
    const uint64_t preAlloctions =
        NanoLogInternal::RuntimeLogger::stagingBuffer
            ? NanoLogInternal::RuntimeLogger::stagingBuffer->numAllocations
            : 0;

    NanoLog::setLogFile(output_fn.c_str());
    printf("NanoLog Bench for: %s\r\n", op.first.c_str());

    pthread_barrier_t barrier;
    if (pthread_barrier_init(&barrier, NULL, BENCHMARK_THREADS)) {
      printf("pthread error\r\n");
    }

    std::vector<std::thread> threads;
    threads.reserve(BENCHMARK_THREADS);
    for (size_t i = 1; i < BENCHMARK_THREADS; ++i)
      threads.emplace_back(
          [&, i = i]() { runBenchmark(i, &barrier, op.second); });

    uint64_t start = PerfUtils::Cycles::rdtsc();
    runBenchmark(0, &barrier, op.second);

    for (size_t i = 0; i < threads.size(); ++i)
      if (threads[i].joinable()) threads.at(i).join();

    uint64_t syncStart = PerfUtils::Cycles::rdtsc();
    // Flush all pending log messages to disk
    NanoLog::sync();
    uint64_t stop = PerfUtils::Cycles::rdtsc();

    double time = PerfUtils::Cycles::toSeconds(stop - syncStart);
    printf(
        "Flushing the log statements to disk took an additional %0.2lf "
        "secs\r\n",
        time);

    uint64_t totalEvents =
        NanoLogInternal::RuntimeLogger::nanoLogSingleton.logsProcessed -
        preEvents;
    uint64_t totalAllocations =
        NanoLogInternal::RuntimeLogger::stagingBuffer->numAllocations -
        preAlloctions;
    double totalTime = PerfUtils::Cycles::toSeconds(stop - start);
    double recordTimeEstimated = PerfUtils::Cycles::toSeconds(stop - start);
    double recordNsEstimated =
        recordTimeEstimated * 1.0e9 /
        NanoLogInternal::RuntimeLogger::stagingBuffer->numAllocations;
    double compressionTime = PerfUtils::Cycles::toSeconds(
        NanoLogInternal::RuntimeLogger::nanoLogSingleton.cyclesCompressing);
    printf(
        "Took %0.2lf seconds to log %lu operations\r\nThroughput: %0.2lf op/s "
        "(%0.2lf Mop/s)\r\n",
        totalTime, totalEvents, totalEvents / totalTime,
        totalEvents / (totalTime * 1e6));
    results.push_back({totalEvents, totalAllocations, totalTime,
                       recordNsEstimated, compressionTime});

    // Prints various statistics gathered by the NanoLog system to stdout
    printf("%s", NanoLog::getStats().c_str());
    NanoLog::printConfig();
  }

  printf(
      "# Note: record()* time is estimated based on one thread's "
      "performance\r\n");
  printf("# %8s %10s %10s %10s %10s %10s %10s\r\n", "Mlogs/s", "Ops", "Time",
         "record()*", "compress()", "Threads", "BenchOp");
  for (size_t i = 0; i < results.size(); ++i) {
    auto& [totalEvents, totalAllocations, totalTime, recordNsEstimated,
           compressionTime] = results[i];
    printf("%10.2lf %10lu %10.6lf %10.2lf %10.2lf %10d %10s\r\n",
           totalEvents / (totalTime * 1e6), totalEvents, totalTime,
           recordNsEstimated, compressionTime * 1.0e9 / totalEvents,
           BENCHMARK_THREADS, ops[i].first.c_str());
  }

  // This is useful for when output is disabled and our metrics from the
  // consumer aren't correct
  printf("# Same as the above, but guestimated from the producer side\r\n");
  printf("# %8s %10s %10s %10s %10s %10s %10s\r\n", "Mlogs/s", "Ops", "Time",
         "record()*", "compress()", "Threads", "BenchOp");
  for (size_t i = 0; i < results.size(); ++i) {
    auto& [totalEvents, totalAllocations, totalTime, recordNsEstimated,
           compressionTime] = results[i];
    printf("%10.2lf %10lu %10.6lf %10.2lf %10.2lf %10d %10s\r\n",
           totalAllocations * BENCHMARK_THREADS / (totalTime * 1e6),
           totalAllocations * BENCHMARK_THREADS, totalTime, recordNsEstimated,
           compressionTime * 1.0e9 / totalAllocations / BENCHMARK_THREADS,
           BENCHMARK_THREADS, ops[i].first.c_str());
  }
}
