cmake_minimum_required(VERSION 3.1)

aux_source_directory(. DIR_SRCS)

include_directories(SYSTEM ${PROJECT_BINARY_DIR}/include)
link_directories(${PROJECT_BINARY_DIR}/lib)

file(GLOB EC_SRC galois.c jerasure.c ErasureCode.cc RSCode.cc LESS.cc)
file(GLOB UTIL_SRC Util.cc)

add_executable(CodeTest CodeTest.cc ${EC_SRC} ${UTIL_SRC} ${EXTRA_SRC})
target_link_libraries(CodeTest gf_complete isal)