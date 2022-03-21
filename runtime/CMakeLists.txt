set(TARGET NanoLog)
find_package(Threads REQUIRED)
find_package(RT REQUIRED)
file(GLOB_RECURSE SRC "*.h" "*.cc")

add_library(${TARGET} SHARED ${SRC})

target_link_libraries(${TARGET} PUBLIC
  ${CMAKE_THREAD_LIBS_INIT}
  ${RT_LIBRARIES}
)

target_module(${TARGET})