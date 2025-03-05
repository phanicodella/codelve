cmake_minimum_required(VERSION 3.14)
project(CodeLve VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/external/llama.cpp)

# Configure llama.cpp
set(LLAMA_STANDALONE OFF CACHE BOOL "LLAMA_STANDALONE" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "LLAMA_BUILD_EXAMPLES" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "LLAMA_BUILD_TESTS" FORCE)
set(LLAMA_NATIVE OFF CACHE BOOL "LLAMA_NATIVE" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "BUILD_SHARED_LIBS" FORCE)

# Add llama.cpp as a subdirectory
add_subdirectory(external/llama.cpp)

# Define source files with explicit list to avoid including any unwanted files
file(GLOB_RECURSE SOURCE_FILES 
    "src/*.cpp"
)

# Filter out the problematic engine_functions.cpp if it exists
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*engine_functions\\.cpp$")

# Create executable
add_executable(CodeLve WIN32 ${SOURCE_FILES} "src/llm/mock_llm.h" "src/llm/mock_llm.cpp" "src/core/engine_functions.cpp")

# Add Windows library dependencies
target_link_libraries(CodeLve PRIVATE comctl32 gdi32 user32 shell32 shlwapi)

# Link with llama.cpp
target_link_libraries(CodeLve PRIVATE llama)

# Copy configuration files to the build directory
add_custom_command(TARGET CodeLve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/logs
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config/codelve.cfg
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/codelve.cfg
)

# Define USE_MOCK_LLM for testing without the real model
target_compile_definitions(CodeLve PRIVATE USE_MOCK_LLM=1)

# Set compiler options for MSVC
if(MSVC)
    # Add windows subsystem flag
    target_link_options(CodeLve PRIVATE /SUBSYSTEM:WINDOWS)
    
    # Add compiler flags
    target_compile_options(CodeLve PRIVATE /W4)
endif()