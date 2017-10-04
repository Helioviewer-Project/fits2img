
include_before(include)

add_executable(fits_verify
    fits_verify.c
    fverify.c
    fvrf_data.c
    fvrf_file.c
    fvrf_head.c
    fvrf_key.c
    fvrf_misc.c
    lib/cftools.c
)

target_link_libraries(fits_verify cfitsio)
install(TARGETS fits_verify DESTINATION support)
