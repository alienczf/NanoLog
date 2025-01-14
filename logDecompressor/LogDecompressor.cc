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

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#include "Cycles.h"
#include "Log.h"

// File generated by the NanoLog preprocessor that contains all the
// compression and decompression functions.
#include "GeneratedCode.h"

using namespace NanoLogInternal::Log;

/**
 * Produces a GNUPlot graphable reverse CDF graph to stdout given a vector of
 * rdtsc time deltas a conversion factor for tsc to wall time seconds.
 * This is primarily used by NanoLog to visualize extreme tail latency behavior.
 *
 * \param timeDeltas
 *      vector of rdtsc time differences
 *
 * \param cyclesPerSecond
 *      conversion factor of tsc counts to seconds
 */
void runRCDF(std::vector<uint64_t> timeDeltas, double cyclesPerSecond) {
  printf("# Aggregating...\r\n");
  std::sort(timeDeltas.begin(), timeDeltas.end());
  printf("# Done; printing rcdf\r\n");
  printf("#   Latency     Percentage of Operations\r\n");

  uint64_t sum = 0;
  double boundary = 1.0e-10;  // 1 decimal points into nanoseconds
  uint64_t bound = PerfUtils::Cycles::fromSeconds(boundary, cyclesPerSecond);
  double size = double(timeDeltas.size());

  printf(
      "%8.2lf    %11.10lf\r\n",
      1e9 * PerfUtils::Cycles::toSeconds(timeDeltas.front(), cyclesPerSecond),
      1.0);

  uint64_t lastPrinted = timeDeltas.front();
  for (uint64_t i = 1; i < timeDeltas.size(); ++i) {
    sum += timeDeltas[i];
    if (timeDeltas[i] - lastPrinted > bound) {
      printf("%8.2lf    %11.10lf\r\n",
             1e9 * PerfUtils::Cycles::toSeconds(lastPrinted, cyclesPerSecond),
             1.0 - double(i) / size);
      lastPrinted = timeDeltas[i];
    }
  }

  printf("%8.2lf    %11.10lf\r\n",
         1e9 * PerfUtils::Cycles::toSeconds(timeDeltas.back(), cyclesPerSecond),
         1 / size);

  printf("\r\n# The mean was %0.2lf ns\r\n",
         1e9 * PerfUtils::Cycles::toSeconds(sum / timeDeltas.size(),
                                            cyclesPerSecond));
}

/**
 * Prints the usage information to stdout.
 *
 * \param exe
 *      Name of the executable
 */
void printHelp(const char* exe) {
  printf(
      "Decompress/Aggregate log files produced by "
      "the NanoLog System\r\n\r\n");

  printf("Decompress the log file into a human-readable format:\r\n");
  printf("\t%s decompress <logFile>\r\n\r\n", exe);

  printf(
      "Decompress the log file into a sorted human-readable format \r\n"
      "without sorting the messages by time:\r\n");
  printf("\t%s decompressUnordered <logFile>\r\n\r\n", exe);

  printf("Create an RCDF of the inter-log invocation times. Only works\r\n");
  printf("when there is one runtime logging thread:\r\n");
  printf("\t%s rcdfTime <logFile>\r\n\r\n", exe);
}

/**
 * Simple program to decompress log files produced by the NanoLog System.
 * Note that this executable must be compiled with the same BufferStuffer.h
 * as the LogCompressor that generated the compressedLog for this to work.
 */
int main(int argc, char** argv) {
  if (argc < 3) {
    printHelp(argv[0]);
    exit(1);
  }

  const char* command = argv[1];
  const char* logFileName = argv[2];
  bool find = false;
  bool sorted = false;
  bool doRCDF = false;
  FILE* outputFd = NULL;
  int filterId = -1;

  if (strcmp(command, "decompress") == 0) {
    outputFd = stdout;
    sorted = true;
  } else if (strcmp(command, "decompressUnordered") == 0) {
    outputFd = stdout;
  } else if (strcmp(command, "rcdfTime") == 0) {
    doRCDF = true;
  } else {
    printHelp(argv[0]);
    exit(1);
  }

  Decoder decoder;
  if (!decoder.open(logFileName)) {
    printf("Unable to open file %s\r\n", logFileName);
    exit(1);
  }

  if (find) {
    return 0;
  }

  LogMessage args;
  if (doRCDF) {
    uint64_t start = PerfUtils::Cycles::rdtsc();
    std::vector<uint64_t> interLogTimes;
    interLogTimes.reserve(1000000000);
    uint64_t stop = PerfUtils::Cycles::rdtsc();
    double reserveTime = PerfUtils::Cycles::toSeconds(stop - start);

    uint64_t lastTimestamp = 0;
    start = PerfUtils::Cycles::rdtsc();
    if (decoder.getNextLogStatement(args)) {
      lastTimestamp = args.getTimestamp();
      while (decoder.getNextLogStatement(args)) {
        interLogTimes.push_back(args.getTimestamp() - lastTimestamp);
        lastTimestamp = args.getTimestamp();
      }
    }
    stop = PerfUtils::Cycles::rdtsc();
    double decodeTime = PerfUtils::Cycles::toSeconds(stop - start);

    start = PerfUtils::Cycles::rdtsc();
    runRCDF(interLogTimes, PerfUtils::Cycles::getCyclesPerSec());
    stop = PerfUtils::Cycles::rdtsc();
    double rcdfTime = PerfUtils::Cycles::toSeconds(stop - start);

    double totalTime = reserveTime + decodeTime + rcdfTime;
    printf(
        "# Took %0.2lf seconds to aggregate %lu time entries "
        "(%0.2lf ns/event avg)\r\n",
        totalTime, interLogTimes.size(),
        1.0e9 * totalTime / double(interLogTimes.size()));

    printf(
        "# On average, thats..\r\n"
        "#\t%0.2lf seconds allocate large vector (%0.2lf ns/event)\r\n"
        "#\t%0.2lf seconds decompressing events (%0.2lf ns/event)\r\n"
        "#\t%0.2lf seconds sorting/rcdf-ing (%0.2lf ns/event)\r\n",
        reserveTime, 1.0e9 * reserveTime / double(interLogTimes.size()),
        decodeTime, 1.0e9 * decodeTime / double(interLogTimes.size()), rcdfTime,
        1.0e9 * rcdfTime / double(interLogTimes.size()));
    return 0;
  }

  if (sorted) {
    decoder.decompressTo(outputFd);
    // if (outputFd)
    //   fprintf(outputFd,
    //           "\r\n\r\n# Decompression Complete after printing "
    //           "%ld log messages\r\n",
    //           numLogMsgs);
    return 0;
  }

  // Perform no aggregation but decompress unsorted.
  if (filterId < 0) {
    int64_t numLogMsgs = 0;
    while (decoder.getNextLogStatement(args, outputFd)) ++numLogMsgs;

    // if (outputFd)
    //   fprintf(outputFd,
    //           "\r\n\r\n# Decompression Complete after printing "
    //           "%ld log messages\r\n",
    //           numLogMsgs);
    return 0;
  }

  // Perform unsorted aggregation
  long total = 0;
  long count = 0;
  long max = 0;
  long min = 0;
  uint64_t numLogs = 0;
  uint64_t start = PerfUtils::Cycles::rdtsc();
  while (decoder.getNextLogStatement(args, nullptr)) {
    ++numLogs;

    if (static_cast<int>(args.getLogId()) != filterId) continue;

    int elem = args.get<int>(0);
    if (count == 0) max = min = elem;

    if (max < elem) max = elem;

    if (min > elem) min = elem;

    total += elem;
    ++count;
  }

  uint64_t stop = PerfUtils::Cycles::rdtsc();
  double time = PerfUtils::Cycles::toSeconds(stop - start);

  printf("Logs Encountered: %lu\r\n", numLogs);
  printf("Matching Logs: %ld (%0.2lf%%)\r\n", count,
         (100.0 * (double)count) / (double)numLogs);
  printf("Min: %ld\r\n", min);
  printf("Max: %ld\r\n", max);
  printf("Mean: %ld\r\n", total / count);
  printf("Total: %ld\r\n", total);

  printf(
      "\r\nThe aggregation took %0.2lf seconds over "
      "%ld elements (%0.2lf ns avg)\r\n",
      time, count, (1.0e9 * time) / (double)count);

  return 0;
}
