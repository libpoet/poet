# POET - The Performance with Optimal Energy Toolkit

[http://poet.cs.uchicago.edu/](http://poet.cs.uchicago.edu/)

The Performance with Optimal Energy Toolkit (POET) provides a means to monitor and adjust application performance while minimizing energy consumption during application runtime.

For details, please see the following and reference as appropriate:

* Connor Imes, David H. K. Kim, Martina Maggio, and Henry Hoffmann. "POET: A Portable Approach to Minimizing Energy Under Soft Real-time Constraints". In: RTAS. 2015.
* Connor Imes, David H. K. Kim, Martina Maggio, and Henry Hoffmann. "Portable Multicore Resource Management for Applications with Performance Constraints". In: MCSoC. 2016.

See the `baseline-1.0` branch for the version of the API defined in these publications.


## Dependencies

The POET library no longer has any direct dependencies, but some tests/examples depend on the following:

* [heartbeats-simple](https://github.com/libheartbeats/heartbeats-simple)
* [energymon](https://github.com/energymon/energymon)

CMake will skip building some tests/examples if these dependencies are not found.

The `baseline-1.0` branch has a direct dependency on:

* [Heartbeats 2.0](https://github.com/libheartbeats/heartbeats)


## Building

This project uses CMake.

To build, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```


## Running POET Examples

Run the tests from the build directory:

``` sh
./[test_binary] [num_beats] [target_rate] [window_size]
```

The num\_beats is the number of total heartbeats the application will cycle
through before exiting. The target_rate is the number of heartbeats per second,
and the window size is the number of heartbeats used to calculate the heartbeat
rate. It doesn't make sense to give the test binary a target heartbeat rate
without knowing the normal heartbeat rate of the application on your system. To
find this, run:

``` sh
./[test_binary] [num_beats] 0 [window_size]
```

If you aren't sure what parameters to use, a good place to start is 200 beats
and a window size of 20.


## Installing

To install, run with proper privileges:

``` sh
make install
```

Headers are installed to /usr/local/include/poet.
The library is installed to /usr/local/lib.


## Uninstalling

Install must be run before uninstalling in order to have a manifest.
To uninstall, run with proper privileges:

``` sh
make uninstall
```


## Directory Structure

.  
├── config    -- Default and example configuration files  
├── inc       -- Header files  
├── src       -- Source files  
└── test      -- Test source files
