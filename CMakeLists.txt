cmake_minimum_required(VERSION 2.8)

project(myf2fs)

set(F2FS_SRCS main.c super.c)

add_executable(myf2fs ${F2FS_SRCS})

add_subdirectory(crc32)
set(EXTLIB ${EXTLIB} crc32)

target_link_libraries(myf2fs ${EXTLIB})

