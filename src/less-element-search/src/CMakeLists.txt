cmake_minimum_required(VERSION 3.1)

aux_source_directory(. DIR_SRCS)

include_directories(SYSTEM ${PROJECT_BINARY_DIR}/include)
link_directories(${PROJECT_BINARY_DIR}/lib)

file(GLOB LESS_MDS galois.c jerasure.c comb.cc element.cc LESSMDS.cc progressBar.cc)

add_executable(LESS_search LESS_search.cc ${LESS_MDS} ${EXTRA_SRC})
target_link_libraries(LESS_search  gf_complete)

add_executable(LESS_verify LESS_verify.cc ${LESS_MDS} ${EXTRA_SRC})
target_link_libraries(LESS_verify gf_complete)