cmake_minimum_required(VERSION 3.14)

project(route-handler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)  # Change this from 17 to 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
foreach(OUTPUT_CONFIG DEBUG RELEASE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} ${CMAKE_BINARY_DIR}/${OUTPUT_CONFIG})
endforeach()
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE) 

find_package(OpenMP)

add_library(route-handler STATIC)

# Try to find an installed version first
find_package(erkir CONFIG QUIET)

if(NOT erkir_FOUND)
    # If not found, build from source
    if(NOT TARGET erkir::erkir)
        add_subdirectory(extern/erkir)
    endif()
endif()

find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    # If not found, build from source
    if(NOT TARGET nlohmann_json::nlohmann_json)
        add_subdirectory(extern/json)
    endif()
endif()

add_subdirectory(extern/fmt)
find_package(absl CONFIG QUIET)
if(NOT absl_FOUND)
    set(ABSL_PROPAGATE_CXX_STD ON)
    set(ABSL_ENABLE_INSTALL ON)
    add_subdirectory(extern/abseil-cpp)
endif()
add_subdirectory(extern/mio)
add_subdirectory(extern/SQLiteCpp)

target_sources(route-handler
    PRIVATE
        src/RouteHandler.cpp
        src/Navdata.cpp
        src/Parser.cpp
        src/AirwayNetwork.cpp
        src/AirportNetwork.cpp
        )

target_include_directories(route-handler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/compile-time-regular-expressions/single-header>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/types>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(route-handler
    PUBLIC
        SQLiteCpp
        erkir::erkir
        fmt
        absl::strings
        absl::flat_hash_map
        mio
        nlohmann_json::nlohmann_json
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