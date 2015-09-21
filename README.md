EventRacer
==========

A race detection tool for event driven applications.


How to compile (on Linux)
-------------------------

Install prerequisites
   * Google Sparse Hash (http://code.google.com/p/sparsehash/downloads/list)
      * ```sudo apt-get install libsparsehash-dev```
   * Google Flags 2.0 (http://gflags.github.io/gflags/)
      * ```sudo apt-get install libgflags-dev```
      * or, build this from source, cloning git@github.com:gflags/gflags.git
   * CMake 2.8 (http://www.cmake.org/)
      * ```sudo apt-get install cmake```
   * GraphViz is needed to display happens-before graphs.
      * ```sudo apt-get install graphviz```

Compiling:
   * Run: ```./build.sh```

Running
-------

Checking a website for races
   * Obtain a ER_actionlog file by exploring a website with an instrumented browser (see https://github.com/eth-srl/webkit)
      * You can use the binary distribution from http://eventracer.org/ and use only the browser from it.
   * Run the race analyzer
      * <code>bin/eventracer/webapp/raceanalyzer [the ER_actionlog file]</code>
      * The above command starts a web server on port 8000 (can be changed with a --port parameter to the above command)
      * Open http://localhost:8000/ to see the races

Read our paper "Effective Race Detection for Event-Driven Programs" to understand the meaning of
uncovered races and the race filters. Enjoy the tool and develop web applications that provide
a great user experience.


Credits
-------

The EventRacer code includes the mongoose web server (BSD license) and stringprintf and mutex utilities from Google (Apache license).

