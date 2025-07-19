# Automatically discover all .cpp sources in subdirectories.
# CONFIGURE_DEPENDS: Reruns CMake when files are added or removed.
file(GLOB_RECURSE CORE_SOURCES
    CONFIGURE_DEPENDS
    "*.cpp"
)

add_library(bedrock_core ${CORE_SOURCES})

# Set public include directories.
target_include_directories(bedrock_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})