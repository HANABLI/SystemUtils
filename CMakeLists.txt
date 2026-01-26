# CMackeLists.txt for SystemUtils
#
# © 2024 by Hatem Nabli


cmake_minimum_required(VERSION 3.20)
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
    include/SystemUtils/CryptoRandom.hpp

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

if(WIN32)
    list(APPEND Sources
        src/Win32/NetworkEndPointWin32.hpp
        src/Win32/NetworkEndPointWin32.cpp
        src/Win32/NetworkConnectionWin32.hpp
        src/Win32/NetworkConnectionWin32.cpp
        src/Win32/DirectoryMonitorWin32.cpp
        src/Win32/DynamicLibraryWin32.cpp
        src/Win32/SubprocessWin32.cpp
        src/Win32/TargetInfoWin32.cpp
        src/Win32/CryptoRandomWin32.cpp
        src/Win32/FileWin32.cpp
        src/Win32/TimeWin32.cpp
    )
    set(TargetFolders Win32)
    add_definitions(
        -DUNICODE -D_UNICODE
        -D_CRT_SOURCE_NO_WARNINGS
    )
elseif(LINUX)
    list(APPEND Sources
        src/Linux/DynamicLibraryImpl.cpp
    )
endif()

if(UNIX)
    list(APPEND Sources
        src/Posix/CryptoRandomPosix.cpp
        src/Posix/DynamicLibrayImpl.hpp
        src/Posix/DynamicLibraryPosix.cpp
    )
endif()


add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES
    FOLDER Libraries
)
target_link_libraries(${this} PUBLIC
    StringUtils
)


target_include_directories(${this} PUBLIC include)
if(WIN32)
  add_subdirectory(test)
endif()
