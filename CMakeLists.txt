cmake_minimum_required(VERSION 3.29)
project(mcClient2 C)

set(CMAKE_C_STANDARD 11)

# Add the executable target
add_executable(mcClient2
        main.c
        libs/sharedmain.c
        libs/simpleSocket.c
        libs/debug/file_io.c
        libs/debug/file_io.h
        libs/debug/localErrno.c
        libs/zlib/uncompr.c
        libs/zlib/inflate.c
        libs/zlib/adler32.c
        libs/zlib/inftrees.c
        libs/zlib/crc32.c
        libs/zlib/zutil.c
        libs/zlib/inffast.c
        packetDefs-1_12_2.c
        clientDef.h
)

# Link the Ws2_32 library on Windows
if (WIN32)
    target_link_libraries(mcClient2 PRIVATE Ws2_32    )
endif()
