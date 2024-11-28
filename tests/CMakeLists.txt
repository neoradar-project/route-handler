set (PROJECT_NAME route-handler-tests)

include(CTest)
enable_testing()

set(TEST_USE_REALDATA ON)

# CORE TESTS
set(test_core
    core/RouteHandlerTest.cpp
    core/RouteHandlerPerformanceTest.cpp
    core/Data/SampleNavdata.cpp
    # core/TestDataParserTest.cpp
)

if (TEST_USE_REALDATA)
    set(test_core
        ${test_core}
        core/TestDataParserTest.cpp
    )
endif()

if(DEFINED VCPKG_TARGET_TRIPLET)
    # We're building as part of FDPS with vcpkg
    find_package(GTest CONFIG REQUIRED)
    set(GTEST_LIBS GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
else()
    # Standalone build - use source version
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../extern/googletest 
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest)
    set(GTEST_LIBS gtest gtest_main gmock gmock_main)
endif()

# Test executable
add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${test_core}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        route-handler
        ${GTEST_LIBS}
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

add_custom_command(TARGET route-handler-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:route-handler-tests>/testdata/
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata/airways.db
        $<TARGET_FILE_DIR:route-handler-tests>/testdata/airways.db
)

if (TEST_USE_REALDATA)
add_custom_command(TARGET route-handler-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:route-handler-tests>/testdata/
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata/gng.db
        $<TARGET_FILE_DIR:route-handler-tests>/testdata/gng.db
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata/navdata.db
        $<TARGET_FILE_DIR:route-handler-tests>/testdata/navdata.db
)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${TEST_USE_TESTDATA}>:TEST_USE_TESTDATA>
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})