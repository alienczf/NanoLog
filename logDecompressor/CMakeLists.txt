set(TARGET NanoLogDecompressor)

file(GLOB SOURCES "*.cc")
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE
  NanoLog
)

target_executable(${TARGET})