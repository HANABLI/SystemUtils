# CMakeLists.txt for UriTests
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this SystemUtilsTests)

set(Sources 
    src/StringFileTests.cpp
    src/FileTests.cpp
    src/DynamicLibraryTests.cpp
    src/DirectoryMonitorTests.cpp
    src/DiagnosticsSenderTests.cpp
    src/DiagnosticsContextTests.cpp
    src/DiagnosticsStreamReporterTests.cpp
    src/NetworkConnectionTests.cpp
    src/NetworkEndPointTests.cpp
)

add_executable(${this} ${Sources})
set_target_properties(${this} PROPERTIES 
    FOLDER Tests
)

target_include_directories(${this} PRIVATE ../src)

target_link_libraries(${this} PUBLIC 
    gtest_main 
    SystemUtils
    StringUtils
)

add_subdirectory(MockDynamicLibrary)
add_custom_command(TARGET ${this} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MockDynamicLibrary> $<TARGET_FILE_DIR:${this}>
)

add_subdirectory(MockSubprocess)
add_custom_command(TARGET ${this} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MockSubprocess> $<TARGET_FILE_DIR:${this}>
)

add_test(
    NAME ${this} 
    COMMAND ${this}
)