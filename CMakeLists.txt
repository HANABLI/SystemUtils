# CMackeLists.txt for SystemUtils
#
# © 2024 by Hatem Nabli


cmake_minimum_required(VERSION 3.8)
set(this SystemUtils)

set(Headers 
    include/SystemUtils/IFile.hpp
    include/SystemUtils/IFileSystemEntry.hpp
    include/SystemUtils/File.hpp
    include/SystemUtils/StringFile.hpp
    include/SystemUtils/Time.hpp
    include/SystemUtils/DirectoryMonitor.hpp
    
)

set(Sources 
    src/File.cpp
    src/FileImpl.hpp
    src/StringFile.cpp   
)

if(MSVC)
    list(APPEND Sources 
        src/Win32/DirectoryMonitorWin32.cpp
        src/Win32/FileWin32.cpp 
        src/Win32/TimeWin32.cpp  
    )
    set(TargetFolders Win32) 
    add_definitions(
        -DUNICODE -D_UNICODE
        -D_CRT_SOURCE_NO_WARNINGS
    )
endif()


add_library(${this} STATIC ${Sources} ${Headers})
set_target_properties(${this} PROPERTIES 
    FOLDER Libraries
)
target_link_libraries(${this} PUBLIC
    StringUtils
)


target_include_directories(${this} PUBLIC include)

add_subdirectory(test)
