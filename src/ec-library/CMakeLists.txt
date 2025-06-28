cmake_minimum_required( VERSION 3.1 )

project(ECLib)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -mmmx -msse -mpclmul -msse4.2 -DINTEL_SSE4 -mavx")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

### Header ###

include_directories (include)


### Source Code ###

add_subdirectory ( src )

### External Project ###
include( ExternalProject )

### Third-party libraries ###

## Setup third-party library
set (PROJECT_THIRD_PARTY_LIB_DIR ${PROJECT_SOURCE_DIR}/third-party)
set (THIRD_PARTY_INCLUDE_CONFIG ${PROJECT_BINARY_DIR}/include)
set (THIRD_PARTY_LIB_CONFIG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set (THIRD_PARTY_BIN_CONFIG ${PROJECT_BINARY_DIR}/bin/third-party)
set (THIRD_PARTY_MAN_CONFIG ${PROJECT_BINARY_DIR}/man/third-party)
set (THIRD_PARTY_CONFIG --includedir=${THIRD_PARTY_INCLUDE_CONFIG} --libdir=${THIRD_PARTY_LIB_CONFIG} --bindir=${THIRD_PARTY_BIN_CONFIG} --mandir=${THIRD_PARTY_MAN_CONFIG})

link_directories (${THIRD_PARTY_LIB_CONFIG})
include_directories (${THIRD_PARTY_INCLUDE_CONFIG})

include(thirdPartyLibs.cmake)
