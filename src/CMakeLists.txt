set(dca_SOURCES
	dca.c
)

add_library(dcalib ${dca_SOURCES})
set_target_properties(dcalib PROPERTIES OUTPUT_NAME dca)

add_executable(dca main.c)
target_link_libraries(dca dcalib)
