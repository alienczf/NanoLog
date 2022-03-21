
# # compiler parallelism and caching
# find_program(CCACHE_CMD ccache ENV CCACHE)
# message(STATUS "ccache enabled: CCACHE_PREFIX=${CCACHE_CMD}")
# set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "CCACHE_PREFIX=${CCACHE_CMD}")

execute_process(
  COMMAND nproc
  OUTPUT_VARIABLE NPROC
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PARALLEL_COMPILE_JOBS "${NPROC}" CACHE STRING
  "Define the maximum number of concurrent compilation jobs.")
if(PARALLEL_COMPILE_JOBS)
  message(STATUS "Running at most ${PARALLEL_COMPILE_JOBS} parallel compile jobs")
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=${PARALLEL_COMPILE_JOBS})
  set(CMAKE_JOB_POOL_COMPILE compile_job_pool)
endif()

set(PARALLEL_LINK_JOBS "${NPROC}" CACHE STRING
  "Define the maximum number of concurrent link jobs.")
if(PARALLEL_LINK_JOBS)
  message(STATUS "Running at most ${PARALLEL_LINK_JOBS} parallel link jobs")
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${PARALLEL_LINK_JOBS})
  set(CMAKE_JOB_POOL_LINK link_job_pool)
endif()