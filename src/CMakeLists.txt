cmake_minimum_required(VERSION 3.0)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

find_package(libp2p)
find_package(poolcore)
find_package(config4cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
  ${AIO_INCLUDE_DIR}
  ${POOLCORE_INCLUDE_DIR}
  ${CONFIG4CPP_INCLUDE_DIR}
)

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
  ${CONFIG4CPP_LIBRARY}
  jansson
)
