project(CFITSIO C)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Release)
set(MINIMAL true)

set(CFITSIO ../cfitsio-4.2.0)
set(ZLIB    zlib-1.2.13)

set(SIDC_PROJECT_NAME   ${CMAKE_PROJECT_NAME})
set(SIDC_INSTALL_LIB    ${CMAKE_INSTALL_PREFIX}/lib)

macro(include_before ...)
    include_directories(BEFORE ${ARGV})
endmacro()

macro(sidc_install_lib target)
    set_target_properties(${target} PROPERTIES VERSION 1.0 SOVERSION 1.0 INSTALL_NAME_DIR ${SIDC_INSTALL_LIB})
    install(TARGETS ${target} EXPORT ${SIDC_PROJECT_NAME} DESTINATION ${SIDC_INSTALL_LIB})
    install(EXPORT ${SIDC_PROJECT_NAME} DESTINATION ${SIDC_INSTALL_LIB})
endmacro()

include_before(${ZLIB})

set(ZLIB_FILES
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

foreach(SRC ${ZLIB_FILES})
    list(APPEND ZLIB_SRCS ${ZLIB}/${SRC})
endforeach()

add_library(z STATIC ${ZLIB_SRCS})
sidc_install_lib(z)

include(../CMakeLists.txt)
