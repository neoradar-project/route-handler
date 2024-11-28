cmake_minimum_required(VERSION 3.14)
project(erkir VERSION 1.0.0 LANGUAGES CXX)

# Create main library target and an alias target in the erkir:: namespace
add_library(erkir STATIC)
add_library(erkir::erkir ALIAS erkir)

# Define source files
target_sources(erkir
    PRIVATE
        src/geo/cartesianpoint.cpp
        src/geo/coordinate.cpp
        src/geo/datum.cpp
        src/geo/ellipsoidalpoint.cpp
        src/geo/point.cpp
        src/geo/sphericalpoint.cpp
        src/geo/TriangleCollide.cpp
        src/geo/vector3d.cpp
)

# Set include directories using generator expressions
target_include_directories(erkir
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set compile definitions
target_compile_definitions(erkir
    PRIVATE
        ERKIR_STATIC
)

# Install the library and headers
install(TARGETS erkir
    EXPORT erkir-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
)

# Generate and install CMake config files
install(EXPORT erkir-targets
    FILE erkir-targets.cmake
    NAMESPACE erkir::
    DESTINATION lib/cmake/erkir
)

# Generate the config file that includes the exports
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/erkir-config.cmake
    INSTALL_DESTINATION lib/cmake/erkir
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/erkir-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/erkir-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/erkir-config-version.cmake
    DESTINATION lib/cmake/erkir
)