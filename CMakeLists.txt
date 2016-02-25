cmake_minimum_required(VERSION 2.8.11)

project(poet)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas -Wextra -pedantic -pedantic-errors -std=gnu99")

include_directories(${PROJECT_SOURCE_DIR}/inc)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Determine if we should link with librt for libraries that use "clock_gettime"
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
  if(NOT HAVE_CLOCK_GETTIME)
    find_library(LIBRT NAMES rt)
  endif()
endif()


# Library

# OVERFLOW flag, causing POET fixed point to check for overflows print error statements
if(${OVERFLOW})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPOET_MATH_OVERFLOW -DOVERFLOW_WARNING -DUNDERFLOW_WARNING")
endif()

# FIXED_POINT flag, for compiling the fixed point version of POET
if(${FIXED_POINT})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIXED_POINT")
endif()

add_library(poet src/poet.c src/poet_config_linux.c)


# Tests

add_executable(math_ut test/math_ut.c)

add_executable(poet_config_test test/poet_config_test.c)
target_link_libraries(poet_config_test poet)

add_executable(double_loop_test test/double_loop_test.c)
target_link_libraries(double_loop_test poet hb-acc-pow-shared ${LIBRT})

add_executable(processor_speed_test test/processor_speed_test.c)
target_link_libraries(processor_speed_test poet hb-acc-pow-shared ${LIBRT})


# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/${PROJECT_NAME}")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "${PROJECT_NAME}")
set(PKG_CONFIG_DESCRIPTION "Performance with Optimal Energy Toolkit")
set(PKG_CONFIG_LIBS "-L\${libdir} -lpoet")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/poet.pc
)


# Install

install(TARGETS poet DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/inc/poet.h ${CMAKE_CURRENT_SOURCE_DIR}/inc/poet_config.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)


# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
