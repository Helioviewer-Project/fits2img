
include_before(${CFITSIO} ${LIBZDB} ${GENX})

pkg_check_modules(P2SC_PKG REQUIRED gmodule-2.0)
include_directories(${P2SC_PKG_INCLUDE_DIRS})
link_directories(${P2SC_PKG_LIBRARY_DIRS})

add_library(p2sc SHARED
    p2sc_assert.c
    p2sc_buffer.c
    p2sc_file.c
    p2sc_fits.c
    p2sc_hash.c
    p2sc_math.c
    p2sc_msg.c
    p2sc_name.c
    p2sc_stdlib.c
    p2sc_time.c
    p2sc_xml.c
    p2sc_zdb.c)

target_link_libraries(p2sc send2LMAT ${P2SC_PKG_LIBRARIES} ${PKG_LIBRARIES} cfitsio zdb genx m)
sidc_install_lib(p2sc)
