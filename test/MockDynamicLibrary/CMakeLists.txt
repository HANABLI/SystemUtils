# CMakeLists.txt for MockDynamicLibrary
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.20)
set(this MockDynamicLibrary)

set(Sources 
    src/MockDynamicLibrary.cpp
)

add_library(${this} SHARED ${Sources})
set_target_properties(${this} PROPERTIES 
    FOLDER TestHelpers
)

target_link_libraries(${this} PUBLIC 
)
