cmake_minimum_required(VERSION 3.0.0)
project(LrnWebServer VERSION 0.1.0 LANGUAGES C CXX)

set (DIR_SRC "${PROJECT_SOURCE_DIR}/src")
include_directories(${DIR_SRC})
aux_source_directory(${DIR_SRC} SRC)
add_library(webserver ${SRC})

add_executable(lrnwebserver main.cpp)
target_link_libraries(lrnwebserver webserver)