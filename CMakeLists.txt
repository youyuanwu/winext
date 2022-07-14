cmake_minimum_required(VERSION 3.14)

project(winext)

add_library(winext INTERFACE)
target_include_directories(winext INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


message(STATUS "Fetching asio")
include(FetchContent)
FetchContent_Declare(asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG f0a1e1c7c0387ad16358c81eb52528f190df625c)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE
    ${asio_SOURCE_DIR}/asio/include
)

target_link_libraries(winext INTERFACE asio)

add_subdirectory(examples)