# E:\CodeLve\CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(CodeLve VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Windows-specific compile options
if(WIN32)
    add_compile_definitions(WIN32 _WINDOWS)
    # Set Windows subsystem
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Create a directory for external dependencies
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
file(MAKE_DIRECTORY ${EXTERNAL_DIR})

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${EXTERNAL_DIR}
)

# Find source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# Main executable
add_executable(codelve WIN32 ${SOURCES} "src/core/engine_shim.cpp" "src/ui/main_window_shim.cpp" "src/utils/logger_initialize.cpp" "src/core/engine_functions.cpp")

# Link Windows libraries for the UI
if(WIN32)
    target_link_libraries(codelve PRIVATE
        comctl32
        gdi32
        user32
        shell32
        ole32
        comdlg32
    )
endif()

# Install rules
install(TARGETS codelve
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Copy resources
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
    DESTINATION bin/resources
)