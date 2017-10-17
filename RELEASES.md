# Release Notes

## Unreleased

 * Added VERSION and SOVERSION properties to shared object library
 * Added support for stricter compile flags
 * Use GNU standard installation directories (multiarch support)

## v2.0.0 - 2017-10-02

 * Removed dependency on Heartbeats 2.0
 * Added optional dependencies on heartbeats-simple and energymon in tests
 * Changed build system from Makefile to CMake
 * Reduced logging in poet_config_linux.c:apply_cpu_config_taskset
 * Changed log_buffer struct to poet_record struct which uses fewer memory allocations
 * Changed some label names in log files
 * Removed Android CMake toolchain file: cmake-toolchain/android.toolchain.cmake
 * Removed deprecated Python test wrapper
 * Changed README to use Markdown
 * Added Travis CI build configuration for testing
 * Bug fixes and optimizations

## v1.0.1 - 2016-12-12

 * Fixed #2: Removed poet_math.h dependency in poet.h
 * Fixed #4: Free log buffer in poet_destroy
 * Added configuration examples for Intel Xeon E5-2690 processors
 * Added RTAS 2015 and MCSoC 2016 publication details

## v1.0.0 - 2015-05-29

 * Initial public release
