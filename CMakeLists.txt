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
# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(route-handler STATIC)

add_subdirectory(extern/erkir)
add_subdirectory(extern/fmt)

target_sources(route-handler
    PRIVATE
        src/RouteHandler.cpp
        src/Navdata.cpp
        src/Parser.cpp
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
)

install(TARGETS route-handler
    EXPORT route-handler-config
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

export(EXPORT route-handler-config
    FILE "${CMAKE_CURRENT_BINARY_DIR}/route-handler-config.cmake"
    NAMESPACE route-handler::
)

install(EXPORT route-handler-config
    FILE route-handler-config.cmake
    NAMESPACE route-handler::
    DESTINATION lib/cmake/route-handler
)

add_subdirectory(tests)