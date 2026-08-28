cmake_minimum_required(VERSION 3.15)
project(latituderuler VERSION 0.3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PACKAGE_NAME "latituderuler_pi")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(OCPN_API_VERSION "1.18")

set(LATITUDERULER_ICON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler-white.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler-navy.svg
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${LATITUDERULER_ICON_FILES}
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler.svg
    LATITUDERULER_ICON_DAY_SVG)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler-white.svg
    LATITUDERULER_ICON_WHITE_SVG)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/latituderuler-navy.svg
    LATITUDERULER_ICON_NAVY_SVG)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/in-files/latituderuler_icons.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/latituderuler_icons.h
    @ONLY
)

find_package(wxWidgets REQUIRED COMPONENTS base core aui)
find_package(OpenGL REQUIRED)
include(${wxWidgets_USE_FILE})

add_library(latituderuler_pi SHARED
    src/latituderuler_pi.cpp
    include/latituderuler_pi.h
)

target_include_directories(latituderuler_pi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

add_subdirectory(opencpn-libs/api-18)
target_link_libraries(latituderuler_pi PRIVATE ocpn::api ${wxWidgets_LIBRARIES} OpenGL::GL)

if(WIN32)
  add_subdirectory(opencpn-libs/WindowsHeaders)
  target_link_libraries(latituderuler_pi PRIVATE windows::headers)
endif()

if(MSVC)
  target_compile_definitions(latituderuler_pi PRIVATE
      MAKING_PLUGIN
      __MSVC__
      _CRT_NONSTDC_NO_DEPRECATE
      _CRT_SECURE_NO_DEPRECATE
  )
  target_compile_options(latituderuler_pi PRIVATE /utf-8)
endif()

set_target_properties(latituderuler_pi PROPERTIES
    PREFIX ""
    OUTPUT_NAME "latituderuler_pi"
)

if(MSVC)
  set(OCPN_PACKAGE_TARGET "msvc-wx32")
  set(OCPN_BUILD_TARGET "msvc")
else()
  set(OCPN_PACKAGE_TARGET "unknown")
  set(OCPN_BUILD_TARGET "unknown")
endif()

if(CMAKE_VS_PLATFORM_NAME)
  string(TOLOWER "${CMAKE_VS_PLATFORM_NAME}" _platform_name)
  if(_platform_name STREQUAL "win32")
    set(OCPN_TARGET_ARCH "x86")
  elseif(_platform_name STREQUAL "x64")
    set(OCPN_TARGET_ARCH "x86_64")
  elseif(_platform_name STREQUAL "arm64")
    set(OCPN_TARGET_ARCH "arm64")
  else()
    set(OCPN_TARGET_ARCH "${_platform_name}")
  endif()
else()
  set(OCPN_TARGET_ARCH "unknown")
endif()

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
  set(OCPN_TARGET_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
elseif(CMAKE_SYSTEM_VERSION)
  set(OCPN_TARGET_VERSION "${CMAKE_SYSTEM_VERSION}")
else()
  set(OCPN_TARGET_VERSION "unknown")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/metadata.xml
    @ONLY
)

install(TARGETS latituderuler_pi
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/metadata.xml DESTINATION .)

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "${PACKAGE_NAME}-${PACKAGE_VERSION}-${OCPN_BUILD_TARGET}-${OCPN_TARGET_ARCH}-${OCPN_TARGET_VERSION}"
)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
