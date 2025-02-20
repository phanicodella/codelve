# E:\codelve\CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(CodeLve VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp
)

# Add source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

# Create executable
add_executable(CodeLve WIN32 ${SOURCES})

# Set Windows subsystem
if(MSVC)
    set_target_properties(CodeLve PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# Add required libraries
target_link_libraries(CodeLve
    PRIVATE
    comctl32
    shlwapi
    comdlg32
    riched20
)

# Add llama.cpp as a dependency
add_subdirectory(external/llama.cpp)
target_link_libraries(CodeLve PRIVATE llama)

# Copy configuration and resources
add_custom_command(TARGET CodeLve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config
)

# Create directory for logs
add_custom_command(TARGET CodeLve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/logs
)

# Create directory for models
add_custom_command(TARGET CodeLve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models
)

# Install targets
install(TARGETS CodeLve DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/ DESTINATION config)
install(DIRECTORY DESTINATION logs)
install(DIRECTORY DESTINATION models)

# Package with CPack
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "CodeLve")
set(CPACK_PACKAGE_VENDOR "CodeLve Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline Code Analysis Tool")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CodeLve")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
include(CPack)

# Print configuration summary
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")