# Issue #62: Windows portable staging; the PowerShell release qualifier performs
# windeployqt, selects licensed app-local MSVC redist DLLs, audits and zips.
# This CMake file installs only owned executables and documentary source files.
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Windows packaging requires native x64 MSVC")
endif()
if(NOT RATIONALSTUDIO_BUILD_GUI OR NOT TARGET RationalStudio)
    message(FATAL_ERROR "Windows packaging requires the desktop GUI target")
endif()
if(NOT QT_VERSION_MAJOR EQUAL 6)
    message(FATAL_ERROR "Windows packaging requires Qt 6 (MSVC 2022 x64 kit)")
endif()
foreach(required IN ITEMS
        LICENSE.MD README.md THIRD_PARTY_DEPENDENCIES.md
        packaging/windows/THIRD_PARTY_NOTICES.txt
        packaging/licenses/LGPL-3.0.txt
        packaging/licenses/GPL-3.0.txt
        packaging/licenses/BSL-1.0.txt)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/${required}")
        message(FATAL_ERROR "Required package notice/source missing: ${required}")
    endif()
endforeach()

install(TARGETS RationalStudio RUNTIME DESTINATION bin CONFIGURATIONS Release COMPONENT Runtime)
if(TARGET RationalCLI)
    install(TARGETS RationalCLI RUNTIME DESTINATION bin CONFIGURATIONS Release COMPONENT Runtime)
endif()
install(FILES
    "${PROJECT_SOURCE_DIR}/LICENSE.MD"
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/THIRD_PARTY_DEPENDENCIES.md"
    "${PROJECT_SOURCE_DIR}/packaging/windows/THIRD_PARTY_NOTICES.txt"
    DESTINATION share/doc/rationalstudio CONFIGURATIONS Release COMPONENT Runtime)
install(FILES
    "${PROJECT_SOURCE_DIR}/packaging/licenses/LGPL-3.0.txt"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/GPL-3.0.txt"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/BSL-1.0.txt"
    DESTINATION share/doc/rationalstudio/licenses CONFIGURATIONS Release COMPONENT Runtime)
