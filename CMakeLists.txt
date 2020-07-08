cmake_minimum_required(VERSION 2.8)

project(myf2fs)

set(F2FS_SRCS main.c super.c)

add_executable(myf2fs ${F2FS_SRCS})

add_subdirectory(crc32)
set(EXTLIB ${EXTLIB} crc32)
include_directories("crc32")

add_subdirectory(utils)
set(EXTLIB ${EXTLIB} utils)
include_directories("utils")

target_link_libraries(myf2fs ${EXTLIB})

