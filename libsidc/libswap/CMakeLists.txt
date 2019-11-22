
include_before(${JPEG} ${OPENJPEG} ${GENX})

pkg_check_modules(SWAP_PKG REQUIRED freetype2 libpng>=1.2)
include_directories(${SWAP_PKG_INCLUDE_DIRS})
link_directories(${SWAP_PKG_LIBRARY_DIRS})

add_library(swap SHARED
    fitsproc.c
    opj_index.c
    swap_color.c
    swap_coord.c
    swap_draw.c
    swap_file.c
    swap_file_j2k.c
    swap_math.c
    swap_meta.c
    swap_qlook.c
    swap_vliet.c
    swap_vliet8.c
    swap_warp.c)

set_target_properties(swap PROPERTIES COMPILE_FLAGS "-ffast-math -Wno-deprecated-declarations" COMPILE_DEFINITIONS SIDC_INSTALL_LIB="${SIDC_INSTALL_LIB}")
target_link_libraries(swap p2sc ${SWAP_PKG_LIBRARIES} p2scjpeg openjpeg m)
sidc_install_lib(swap)

install(DIRECTORY data DESTINATION lib PATTERN ".svn" EXCLUDE)

add_subdirectory(util)
