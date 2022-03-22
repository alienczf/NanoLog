set(TARGET NanoLogDecompressor)

file(GLOB SOURCES "*.cc")
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE
  NanoLogCore
)

target_executable(${TARGET})