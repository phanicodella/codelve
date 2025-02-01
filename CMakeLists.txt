# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(CodeLve VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# Find required packages
find_package(Threads REQUIRED)

# Collect source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# Define the executable
add_executable(codelve ${SOURCES})

# Link libraries
target_link_libraries(codelve PRIVATE
    Threads::Threads
)

# Set Windows-specific properties
if(WIN32)
    target_link_libraries(codelve PRIVATE
        shlwapi
    )
    # Set executable icon (will add later)
    # set_target_properties(codelve PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Installation
install(TARGETS codelve DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ 
        DESTINATION resources)

# Create directory for external dependencies
add_custom_target(create_external_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/external
    COMMENT "Creating external directory for dependencies"
)

# Add custom target for downloading dependencies (to be expanded later)
add_custom_target(download_dependencies
    COMMENT "Downloading dependencies"
)

# Add dependencies between targets
add_dependencies(codelve create_external_dir)