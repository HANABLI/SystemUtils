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
    include/SystemUtils/DynamicLibrary.hpp
    include/SystemUtils/DirectoryMonitor.hpp
    include/SystemUtils/DiagnosticsSender.hpp
    include/SystemUtils/DiagnosticsContext.hpp
    include/SystemUtils/DiagnosticsStreamReporter.hpp
    include/SystemUtils/INetworkConnection.hpp
    include/SystemUtils/NetworkConnection.hpp
    include/SystemUtils/NetworkEndPoint.hpp
    include/SystemUtils/Subprocess.hpp
    include/SystemUtils/TargetInfo.hpp
    
)

set(Sources 
    src/File.cpp
    src/FileImpl.hpp
    src/StringFile.cpp
    src/DataQueue.hpp
    src/DataQueue.cpp
    src/DiagnosticsSender.cpp   
    src/DiagnosticsContext.cpp
    src/DiagnosticsStreamReporter.cpp
    src/NetworkConnectionImpl.hpp
    src/NetworkConnection.cpp
    src/NetworkEndPointImpl.hpp
    src/NetworkEndPoint.cpp
    src/SubprocessInternal.hpp
)

if(MSVC)
    list(APPEND Sources 
        src/Win32/NetworkEndPointWin32.hpp
        src/Win32/NetworkEndPointWin32.cpp
        src/Win32/NetworkConnectionWin32.hpp
        src/Win32/NetworkConnectionWin32.cpp
        src/Win32/DirectoryMonitorWin32.cpp
        src/Win32/DynamicLibraryWin32.cpp
        src/Win32/SubprocessWin32.cpp
        src/Win32/TargetInfoWin32.cpp
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
