cmake_minimum_required(VERSION 3.14)

project(route-handler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
foreach(OUTPUT_CONFIG DEBUG RELEASE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
endforeach()
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE) 

find_package(OpenMP)

# Add this to make Abseil installation configs available
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_ENABLE_INSTALL ON)

add_library(route-handler STATIC)

add_subdirectory(extern/erkir)
add_subdirectory(extern/fmt)
add_subdirectory(extern/abseil-cpp)

target_sources(route-handler
    PRIVATE
        src/RouteHandler.cpp
        src/Navdata.cpp
        src/Parser.cpp
        src/AirwayParser.cpp

        src/types/AirwayNetwork.cpp
        )

target_include_directories(route-handler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/compile-time-regular-expressions/single-header>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(route-handler
    PUBLIC
        erkir
        fmt
        absl::strings
        absl::flat_hash_map
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/route-handler-config.cmake
    @ONLY
)

install(TARGETS route-handler erkir fmt
    EXPORT route-handler-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT route-handler-targets
    FILE route-handler-targets.cmake
    NAMESPACE route-handler::
    DESTINATION lib/cmake/route-handler
)

# Install the config file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/config.cmake
    DESTINATION lib/cmake/route-handler
)

add_subdirectory(tests)