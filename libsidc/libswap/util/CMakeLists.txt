
add_executable(fits2pgm fits2pgm.c)
target_link_libraries(fits2pgm swap)
install(TARGETS fits2pgm DESTINATION support)

add_executable(fits_test fits_test.c)
target_link_libraries(fits_test p2sc)
install(TARGETS fits_test DESTINATION support)
