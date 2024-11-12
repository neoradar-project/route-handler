set (PROJECT_NAME router-handler-tests)

include(CTest)
enable_testing()

set(TEST_USE_TESTDATA OFF)

# CORE TESTS
set(test_core
    core/RouteHandlerTest.cpp
    core/RouteHandlerPerformanceTest.cpp
    core/Data/SampleNavdata.cpp
    core/AirwayParserTest.cpp
)

# Configure Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../extern/googletest 
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest)

# Test executable
add_executable(${PROJECT_NAME})

# Add test sources
target_sources(${PROJECT_NAME}
    PRIVATE
        ${test_core}  # Add your test source files here
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        route-handler
        gtest_main
        gmock_main
)

# Platform-specific configuration
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IO_KIT IoKit)
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE 
            ${CORE_FOUNDATION}
            ${IO_KIT}
            ${COCOA_LIBRARY}
    )
endif()

if (TEST_USE_TESTDATA)
add_custom_command(TARGET router-handler-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:router-handler-tests>/testdata/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
        $<TARGET_FILE_DIR:router-handler-tests>/testdata
)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${TEST_USE_TESTDATA}>:TEST_USE_TESTDATA>
)

# Register test
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
    