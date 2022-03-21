#! /bin/bash -e

function runCommonTests() {
  pushd ./_run
  rm -f ./testLog
  ./bin/NanoLogIntegrationTest > /dev/null
  chmod 666 ./testLog

  # Test for string special case
  test -n "$$(find ./testLog -size -100000c)" || \
    (printf "\r\n\033[0;31mError: ./testLog is very large, suggesting a failure of the 'Special case string precision' test in main.cc\033[0m" \
    && echo "" && exit 1)

  # Run the decompressor with embedded functions and cut out the timestamps before the ':'
  printf "Checking normal decompression..."
  ./bin/NanoLogDecompressor decompress ./testLog | cut -d':' -f5- > output.txt
  diff -w ../integrationTest/expected/regularRun.txt output.txt
  printf " OK!\r\n"

  # Run the app once more as if appended to a log file and decompress again
  printf "Checking decompression with appended logs..."
  ./bin/NanoLogIntegrationTest > /dev/null
  ./bin/NanoLogDecompressor decompress ./testLog | cut -d':' -f5- > appended_output.txt
  diff -w ../integrationTest/expected/appendedFile.txt appended_output.txt
  printf " OK!\r\n"

  # Empty file tests
  printf "Checking decompression error handling on malformed files..."
  touch emptyFile
  printf "Error: Could not read initial checkpoint, the compressed log may be corrupted.\r\n" > expected.txt
  printf "Unable to open file emptyFile\r\n" >> expected.txt

  ./bin/NanoLogDecompressor decompress emptyFile > output.txt 2>&1 || true
  diff -w expected.txt output.txt
  printf " OK!\r\n"

  # clean up
  rm -f ./testLog basic_output.txt output.txt appended_output.txt dictionary_lookup.txt basic_unordered.txt expected.txt emptyFile
  popd
}

printf "Running Tests...\r\n"
runCommonTests
printf "\r\nIntegration Tests completed without error for C++17 NanoLog!\r\n"
