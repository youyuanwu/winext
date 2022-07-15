

set (CMAKE_CXX_STANDARD 20)

message(STATUS "Configuring examples")

file(GLOB SOURCES
echoserver_coro.cpp
echoserver2.cpp
echoserver.cpp
echoclient.cpp
*.cpp
)

# set(SOURCES echoserver.cpp)

foreach(exe_file ${SOURCES})
    get_filename_component(exe_name ${exe_file} NAME_WE)
    add_executable(${exe_name} ${exe_file})
    target_link_libraries(${exe_name} PRIVATE winext)
endforeach()