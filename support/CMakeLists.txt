if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LDFLAGS LDFLAGS=-Wl,--rpath=<INSTALL_DIR>/lib)
endif()

add_subdirectory(cfitsio)
add_subdirectory(genx)
add_subdirectory(jpeg)
add_subdirectory(openjpeg)
