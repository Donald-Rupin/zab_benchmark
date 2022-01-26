#  MMM"""AMV       db      `7MM"""Yp,
#  M'   AMV       ;MM:       MM    Yb
#  '   AMV       ,V^MM.      MM    dP
#     AMV       ,M  `MM      MM"""bg.
#    AMV   ,    AbmmmqMA     MM    `Y
#   AMV   ,M   A'     VML    MM    ,9
#  AMVmmmmMM .AMA.   .AMMA..JMMmmmd9

cmake_minimum_required(VERSION 3.7...3.18 FATAL_ERROR)

project(ZAB, CXX)


include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_SUPPORT OUTPUT ERROR)

if( LTO_SUPPORT )
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DRELEASE -O3 -g")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -fuse-linker-plugin -D_FILE_OFFSET_BITS=64")

message(STATUS "Compiling with flags ${CMAKE_CXX_FLAGS}")

add_subdirectory(zab)

macro(add_zab_build name file)

    message(STATUS "Adding zab file ${file}")

    add_executable(${name} src/${file}.cpp)

    target_compile_options(${name} PUBLIC
        -fcoroutines
        -pthread
        -Wall
        -Wextra
        -g
    )

    target_include_directories(${name} PUBLIC
        includes
        zab/includes
        ${CMAKE_CURRENT_SOURCE_DIR}/zab/liburing/src/include
    )

    target_link_libraries(
        ${name} PUBLIC
         zab -lpthread -latomic uring
    )

    target_link_directories(
        ${name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/zab/liburing/src
    )

endmacro()

macro(add_asio_build name file)

    message(STATUS "Adding asio file ${file}")

    add_executable(${name} src/${file}.cpp)

    target_compile_options(${name} PUBLIC
        -pthread
        -Wall
        -Wextra
        -g
        
    )

    target_include_directories(${name} PUBLIC
        asio/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/zab/liburing/src/include
    )

    target_link_libraries(
        ${name} PUBLIC
         -lpthread -latomic uring
    )

    target_link_directories(
        ${name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/zab/liburing/src
    )
endmacro()

add_zab_build(server echo_server)
add_zab_build(client echo_client)

add_asio_build(asio_server asio_echo_server)
add_asio_build(asio_client asio_echo_client)

