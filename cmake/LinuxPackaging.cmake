# Issue #61: native Debian package; depends on system Qt6 libraries/plugins.
# No bundled Qt binaries, developer packages, or post-install scripts.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Linux packaging requires a Linux build host")
endif()
if(NOT RATIONALSTUDIO_BUILD_GUI OR NOT TARGET RationalStudio)
    message(FATAL_ERROR "Linux packaging requires RATIONALSTUDIO_BUILD_GUI=ON")
endif()
if(NOT QT_VERSION_MAJOR EQUAL 6)
    message(FATAL_ERROR "The Issue #61 .deb is qualified only for Qt6")
endif()
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "Issue #61 distributable packages require CMAKE_BUILD_TYPE=Release")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/LICENSE.MD")
    message(FATAL_ERROR "LICENSE.MD is missing. Restore the real tracked MIT license before packaging; do not create substitute licensing text.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/README.md")
    message(FATAL_ERROR "README.md is missing from package source")
endif()
include(GNUInstallDirs)
# Explicit paths because Debian packages must install to /usr, not /usr/local.
install(TARGETS RationalStudio RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
if(TARGET RationalCLI)
    install(TARGETS RationalCLI RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
endif()
install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/rationalstudio.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications" COMPONENT Runtime)
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.MD"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rationalstudio"
    RENAME copyright COMPONENT Runtime)
install(FILES "${PROJECT_SOURCE_DIR}/README.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rationalstudio" COMPONENT Runtime)

# M5 / Issue #64 — install notices and unmodified upstream license texts.
# These are first-party package metadata, not bundled Qt shared libraries.
install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/THIRD_PARTY_NOTICES.txt"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rationalstudio"
    COMPONENT Runtime)
install(FILES
    "${PROJECT_SOURCE_DIR}/packaging/licenses/LGPL-3.0.txt"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/GPL-3.0.txt"
    "${PROJECT_SOURCE_DIR}/packaging/licenses/BSL-1.0.txt"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/rationalstudio/licenses"
    COMPONENT Runtime)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "rationalstudio")
set(CPACK_PACKAGE_VENDOR "Rational Studio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Scientific and graphing calculator with exact rational arithmetic")
set(CPACK_PACKAGE_CONTACT "Rational Studio project maintainers")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/SpilledMilkFactory/RationalStudio")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_SET_DESTDIR OFF)
set(CPACK_DEBIAN_PACKAGE_NAME "rationalstudio")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Rational Studio project maintainers")
set(CPACK_DEBIAN_PACKAGE_SECTION "math")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Qt dynamically loads the platform plugin: dpkg-shlibdeps cannot discover it.
# This is a system-Qt package, not an AppImage or statically bundled Qt.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "Interactive scientific, graphing, and exact-fraction calculator.\\n Features numerical calculus, matrix operations, and statistics.")
include(CPack)
