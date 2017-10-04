if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LDFLAGS LDFLAGS=-Wl,--rpath=<INSTALL_DIR>/lib)
endif()

if(DEFINED ENV{PYTHON_OSX_OPENSSL_INCLUDE})
    set(PYTHON_BUILD_CFLAGS "CFLAGS=-I$ENV{PYTHON_OSX_OPENSSL_INCLUDE}")
endif()

add_subdirectory(cfitsio)
add_subdirectory(genx)
add_subdirectory(jpeg)
add_subdirectory(libzdb)
add_subdirectory(openjpeg)
