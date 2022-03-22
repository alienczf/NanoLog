set(TARGET NanoLogIntegrationTest)

file(GLOB SOURCES "*.h" "*.cc")
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE
  NanoLogCore
)

target_executable(${TARGET})