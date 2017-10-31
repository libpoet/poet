# Release Notes

## [Unreleased]


## [v2.0.1] - 2017-10-31
### Added
 * VERSION and SOVERSION properties to shared object library
 * Multiarch support (use GNU standard installation directories)

### Changed
 * Refactor this RELEASES.md file

### Fixed
 * Fixed warnings with stricter compile flags


## [v2.0.0] - 2017-10-02
### Added
 * Added optional dependencies on heartbeats-simple and energymon in tests
 * Added Travis CI build configuration for testing

### Changed
 * Changed build system from Makefile to CMake
 * Changed log_buffer struct to poet_record struct which uses fewer memory allocations
 * Changed some label names in log files
 * Reduced logging in poet_config_linux.c:apply_cpu_config_taskset
 * Changed README to use Markdown

### Removed
 * Removed dependency on Heartbeats 2.0
 * Removed Android CMake toolchain file: cmake-toolchain/android.toolchain.cmake
 * Removed deprecated Python test wrapper

### Fixed
 * Bug fixes and optimizations


## [v1.0.1] - 2016-12-12
### Added
 * Added configuration examples for Intel Xeon E5-2690 processors
 * Added RTAS 2015 and MCSoC 2016 publication details

### Fixed
 * Fixed [#2]: Removed poet_math.h dependency in poet.h
 * Fixed [#4]: Free log buffer in poet_destroy


## 1.0 - 2015-05-29

 * Initial public release

[Unreleased]: https://github.com/libpoet/poet/compare/v2.0.1...HEAD
[v2.0.1]: https://github.com/libpoet/poet/compare/v2.0.0...v2.0.1
[v2.0.0]: https://github.com/libpoet/poet/compare/v1.0.1...v2.0.0
[v1.0.1]: https://github.com/libpoet/poet/compare/1.0...v1.0.1
[#4]: https://github.com/libpoet/poet/issues/4
[#2]: https://github.com/libpoet/poet/issues/2
