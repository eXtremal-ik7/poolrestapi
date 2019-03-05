project(poolrestapi)
cmake_minimum_required(VERSION 3.0)

set (CMAKE_CXX_STANDARD 11)
option(SANITIZER_ENABLED "Build with address sanitizer" OFF)
option(GPROF_ENABLED "Build with GNU profiler (use gprof ./exename -p > out.txt)" OFF)

if (SANITIZER_ENABLED)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

if (GPROF_ENABLED)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

find_package(libp2p)
find_package(poolcore)
find_package(config4cpp)

include_directories(
  ${AIO_INCLUDE_DIR}
  ${POOLCORE_INCLUDE_DIR}
  ${CONFIG4CPP_INCLUDE_DIR}
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_executable(poolrestapi
  poolrestapi.cpp
  Query.cpp
  FoundBlocksQuery.cpp
)

target_link_libraries(poolrestapi
  ${AIO_LIBRARY}
  ${P2P_LIBRARY}
  ${P2PUTILS_LIBRARY}
  ${POOLCOMMON_LIBRARY}
  ${POOLCORE_LIBRARY}
  ${POOLCORE_LOGURU_LIBRARY}
  ${CONFIG4CPP_LIBRARY}
  jansson
)

if (WIN32)
  target_link_libraries(poolrestapi ws2_32 mswsock)
else()
  target_link_libraries(poolrestapi dl)
endif()
