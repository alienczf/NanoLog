# Must be included after project

message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")

# Global compile defines
add_definitions(
  # https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
  # -D_GLIBCXX_USE_CXX11_ABI=0
 -DBOOST_BIND_GLOBAL_PLACEHOLDERS
)

add_compile_options(
  # We *always* want debug info, regardless what the cmake best practices tell you!
  -g
  "$<$<CONFIG:RELEASE>:-O3>"

  # all warnings are errors...
  -Wall
  -Wformat=2
  -Wparentheses
  -Wextra
  -Wcast-align
  -Wcast-qual
  -Wwrite-strings
  -Wnull-dereference
  -Wdangling-else
  -Wmissing-format-attribute
  -Wmissing-declarations
	-Wbad-function-cast
  -Woverloaded-virtual
  -Weffc++
  -Wno-unknown-warning-option
  -Wno-unused-parameter
  -Wno-unused-result
  -Wno-missing-field-initializers
  -Wno-ambiguous-reversed-operator
  -Wno-unused-parameter
  -Wno-unused-result
  # ...but not unused variables warning, as they can get annoying in release builds because asserts
  # get compiled out
  "$<$<CONFIG:RELEASE>:-Wno-unused-variable>"
  "$<$<CONFIG:RELEASE>:-Wno-unused-const-variable>"

  # fancy flags
  -fno-ms-extensions
  # Since we generate binary with different march and mtune now, we record the switches
  # in the binary https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html
  -frecord-gcc-switches
  # see http://gcc.gnu.org/wiki/Visibility
  $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>
  "$<$<CONFIG:DBG>:-fno-omit-frame-pointer>"
  # LTO slows down compile
  "$<$<CONFIG:DBG>:-fno-lto>"
  # Speeds up release binary
  "$<$<CONFIG:RELEASE>:-flto>"
)

add_compile_options(-fdiagnostics-color=always)
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../lib64")


# # COMDAT folding: https://stackoverflow.com/q/15168924
# set(LINKER_FLAGS "-ffunction-sections -Wl,--icf=all ${LINKER_FLAGS}")

# Only link libraries that are actually used
set(LINKER_FLAGS "-Wl,--as-needed ${LINKER_FLAGS}")
set(LINKER_FLAGS "-Wparentheses ${LINKER_FLAGS}")

# add build ids for debuggers
set(LINKER_FLAGS "-Wl,--build-id ${LINKER_FLAGS}")

# Needed (for binutils 2.24+) since we need DT_RPATH rather than
# DT_RUNPATH - the latter is not transitive, breaking shared
# libraries without DT_RUNPATH (like those in vendor) - by using
# DT_RPATH, all paths in the executable are used, even for deps
# of deps.
set(LINKER_FLAGS "-Wl,--disable-new-dtags ${LINKER_FLAGS}")


# For debug builds use lld because it's much faster.
set(LINKER_FLAGS_DBG "-fuse-ld=lld -Wl,--gdb-index -fno-lto ${LINKER_FLAGS_DBG}")
set(LINKER_FLAGS_RELEASE "-fuse-ld=lld -flto ${LINKER_FLAGS_RELEASE}")

set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${LINKER_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_DBG "${LINKER_FLAGS_DBG} ${CMAKE_EXE_LINKER_FLAGS_DBG}")
set(CMAKE_SHARED_LINKER_FLAGS_DBG "${LINKER_FLAGS_DBG} ${CMAKE_SHARED_LINKER_FLAGS_DBG}")
set(CMAKE_MODULE_LINKER_FLAGS_DBG "${LINKER_FLAGS_DBG} ${CMAKE_MODULE_LINKER_FLAGS_DBG}")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} ${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")

if(WITH_ASAN)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-omit-frame-pointer>
  )
  set(LINKER_FLAGS "-lasan ${LINKER_FLAGS}")
  link_libraries(-lasan)
endif()

if(WITH_TSAN)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=thread>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-omit-frame-pointer>
  )
  set(LINKER_FLAGS "-ltsan ${LINKER_FLAGS}")
  link_libraries(-ltsan)
endif()