
add_executable(fits2img fitsproc.c fits2img.c)
target_link_libraries(fits2img swap p2sc)
install(TARGETS fits2img DESTINATION bin)
