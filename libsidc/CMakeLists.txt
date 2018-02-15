
project(SIDC_lib C)

# out of P2SC system (no real send to LMAT)
add_definitions(-DSTANDALONE)

add_subdirectory(send2LMAT)
add_subdirectory(libp2sc)
add_subdirectory(libswap)

add_subdirectory(libsidc_msg)
