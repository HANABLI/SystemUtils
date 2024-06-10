# CMakeLists.txt for MockSubprocess
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this MockSubprocess)

set(Sources 
    src/main.cpp
)

add_executable(${this} ${Sources})
set_target_properties(${this} PROPERTIES
    FOLDER TestHelpers
)

target_link_libraries(${this} PUBLIC
    SystemUtils
)