cmake_minimum_required(VERSION 3.14)

project(winext)

find_package(Boost REQUIRED)
# header only
add_definitions("-DBOOST_ALL_NO_LIB")

add_library(winext INTERFACE)
target_include_directories(winext INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
)

add_subdirectory(examples)
