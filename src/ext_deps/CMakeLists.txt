include_directories(
	libsurvive/include/libsurvive
	libsurvive/redist
)
set(survive_source_files
	libsurvive/src/survive_usb.c
	libsurvive/src/survive.c
	libsurvive/src/survive_data.c
	libsurvive/src/survive_process.c
	libsurvive/src/ootx_decoder.c
	libsurvive/src/survive_driverman.c
	libsurvive/src/survive_vive.c
	libsurvive/src/survive_config.c
	libsurvive/src/survive_cal.c
	libsurvive/src/poser_turveytori.c
	libsurvive/src/poser_octavioradii.c
	libsurvive/src/poser_charlesslow.c
	libsurvive/src/poser_daveortho.c
	libsurvive/src/poser_dummy.c
	libsurvive/redist/json_helpers.c
	libsurvive/redist/linmath.c
	libsurvive/redist/jsmn.c
	libsurvive/redist/os_generic.c
	libsurvive/redist/crc32.c
	libsurvive/redist/puff.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-variable -Wno-unused-but-set-variable") #-pedantic -Wextra")
add_definitions(-DUSE_DOUBLE)

add_library(survive SHARED ${survive_source_files})
set_property(TARGET survive PROPERTY C_STANDARD 11)
target_link_libraries(survive z)
