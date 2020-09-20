
aux_source_directory(. DIR_SRCS)

add_library(minitest ${DIR_SRCS})

add_subdirectory(test)
