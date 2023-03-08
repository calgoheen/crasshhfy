# Set up resampler library
add_library(r8b STATIC)
target_sources(r8b PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/r8b/r8bbase.cpp")

# SYSTEM keyword causes warnings to be suppressed
target_include_directories(r8b SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
