
include_before(${CFITSIO})

add_definitions(
    -Dunix
    -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1
    -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1
    -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_MATH_H=1 -DHAVE_LIMITS_H=1
    -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DHAVE_FTRUNCATE=1 -DHAVE_LONGLONG=1
)

set(SRC_FILES
    buffers.c cfileio.c checksum.c
    drvrfile.c drvrmem.c drvrnet.c drvrsmem.c
    editcol.c edithdu.c eval_f.c eval_l.c eval_y.c
    f77_wrap1.c f77_wrap2.c f77_wrap3.c f77_wrap4.c
    fits_hcompress.c fits_hdecompress.c fitscore.c
    getcol.c getcolb.c getcold.c getcole.c getcoli.c getcolj.c getcolk.c
    getcoll.c getcols.c getcolsb.c getcolui.c getcoluj.c getcoluk.c
    getkey.c group.c grparser.c histo.c imcompress.c iraffits.c modkey.c
    pliocomp.c
    putcol.c putcolb.c putcold.c putcole.c putcoli.c putcolj.c putcolk.c
    putcoll.c putcols.c putcolsb.c putcolu.c putcolui.c putcoluj.c putcoluk.c
    putkey.c quantize.c region.c ricecomp.c scalnull.c simplerng.c swapproc.c
    wcssub.c wcsutil.c zcompress.c zuncompress.c)

foreach(SRC ${SRC_FILES})
    list(APPEND FITS_SRCS ${CFITSIO}/${SRC})
endforeach()

add_library(cfitsio STATIC ${FITS_SRCS})
target_link_libraries(cfitsio m z)
sidc_install_lib(cfitsio)

add_executable(speed ${CFITSIO}/speed.c)
target_link_libraries(speed cfitsio)

add_executable(fpack ${CFITSIO}/fpackutil.c ${CFITSIO}/fpack.c)
target_link_libraries(fpack cfitsio)

add_executable(funpack ${CFITSIO}/fpackutil.c ${CFITSIO}/funpack.c)
target_link_libraries(funpack cfitsio)

add_executable(fits_listhead listhead.c)
target_link_libraries(fits_listhead cfitsio)
install(TARGETS fits_listhead DESTINATION support)

add_executable(fits_imcopy ${CFITSIO}/imcopy.c)
target_link_libraries(fits_imcopy cfitsio)
install(TARGETS fits_imcopy DESTINATION support)

add_executable(fitscopy ${CFITSIO}/fitscopy.c)
target_link_libraries(fitscopy cfitsio)
install(TARGETS fitscopy DESTINATION support)

add_subdirectory(fverify)
