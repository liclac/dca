add_executable(dca main.c config.c)
target_link_libraries(dca dcalib avformat avcodec avutil opus)
