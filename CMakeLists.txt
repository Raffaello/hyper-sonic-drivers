cmake_minimum_required (VERSION 3.24)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()


project ("hyper-sonic-drivers" VERSION 0.19.1 DESCRIPTION "Hyper-Sonic Drivers for emulating old soundcards")
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
  message(STATUS "BIG_ENDIAN")
  add_compile_definitions(-DIS_BIG_ENDIAN)
else()
  message(STATUS "LITTLE_ENDIAN")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

################################ Packages ####################################
set (CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPACK_SYSTEM_NAME win64)
  else()
    set(CPACK_SYSTEM_NAME win32)
  endif()
endif()
set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CPACK_SYSTEM_NAME}-${CMAKE_BUILD_TYPE})
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_GENERATOR "ZIP")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CMAKE_PROJECT_DESCRIPTION})
include(CPack)

################################# Options ####################################
option(USE_SDL3 "USE SDL3" OFF)
option(USE_SDL2 "USE_SDL2" OFF)
option(USE_MT32 "USE MT32Emu" ON)
option(USE_SPDLOG "USE spdlog" OFF)
option(GENERATE_CODE_COVERAGE "Generate Code Coverage with gcov" OFF)
option(BUILD_EXAMPLES "build examples" ON)
include(CTest)

# SDL3 over SDL2
if(USE_SDL3)
  set(USE_SDL2 OFF CACHE BOOL "")
endif()

message(STATUS "USE_SDL3  : ${USE_SDL3}")
message(STATUS "USE_SDL2  : ${USE_SDL2}")
message(STATUS "USE_MT32  : ${USE_MT32}")
message(STATUS "USE_SPDLOG: ${USE_SPDLOG}")
message(STATUS "GENERATE_CODE_COVERAGE: ${GENERATE_CODE_COVERAGE}")

##############################################################################

# Include sub-projects.
add_subdirectory(libs)
add_subdirectory ("hyper-sonic-drivers")
