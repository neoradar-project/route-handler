set(PROJECT_NAME erkir)
project(erkir)

# SOURCE GROUPS
set(SRC_GEO
    src/geo/cartesianpoint.cpp
    src/geo/coordinate.cpp
    src/geo/datum.cpp
    src/geo/ellipsoidalpoint.cpp
    src/geo/point.cpp
    src/geo/sphericalpoint.cpp
    src/geo/TriangleCollide.cpp
    src/geo/vector3d.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${SRC_GEO}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE ERKIR_STATIC)

# Export the library
export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_NAME}Targets.cmake)

# Install the library
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install the headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
)

# Install the export file
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)