Introduction
============
DDS is a double-dummy solver of bridge hands.  It is provided as a Windows DLL and as C++ source code suitable for a number of operating systems.  It supports single-threading and multi-threading  for improved performance.

DDS offers a wide range of functions, including par-score calculations.

Please refer to the [home page](http://privat.bahnhof.se/wb758135) for details.

The current version is DDS 2.9.0, released in August 2018 and licensed under the Apache 2.0 license in the LICENSE FILE.

Release notes are in the ChangeLog file.

(c) Bo Haglund 2006-2014, (c) Bo Haglund / Soren Hein 2014-2018.


Credits
=======
Many people have generously contributed ideas, code and time to make DDS a great program.  While leaving out many people, we thank the following here.

The code in Par.cpp for calculating par scores and contracts is based on Matthew Kidd's perl code for ACBLmerge.  He has kindly given permission to include a C++ adaptation in DDS.

Alex Martelli cleaned up and ported code to Linux and to Mac OS X in 2006.  The code grew a bit outdated over time, and in 2014 Matthew Kidd contributed updates.

Brian Dickens found bugs in v2.7 and encouraged us to look at GitHub.  He also set up the entire historical archive and supervised our first baby steps on GitHub.

Foppe Hemminga maintains DDS on ArchLinux.  He also contributed a version of the documentation file completely in .md mark-up language.

Pierre Cossard contributed the code for multi-threading on the Mac using GDS.

Soren Hein made a number of contributions before becoming a co-author starting with v2.8 in 2014.


Fork Overview
========

This is a fork of the original repository [dds](https://github.com/dds-bridge/dds/tree/develop) and provides a wrapper on top of the original functions.

This wrapper utilizes the emscripten library to compile the functions to WebAssembly. This allows the calculation functions to be utilized in the browser by a client.
This fork also provides transformation wrapper functions on top of the original package functions to allow for more abstraction from the client.

The [Emscripten package](https://emscripten.org/docs/getting_started/downloads.html) must also be downloaded and configured to compile and use the wrapper with WebAssembly
The compilation can be done using the below command:
```
emcc -sINITIAL_MEMORY=52428800 -sNO_EXIT_RUNTIME=1 -sEXPORTED_FUNCTIONS="_free,_malloc,_do_dds_solve_board, _dds_init" -sEXPORTED_RUNTIME_METHODS="getValue,ccall,allocateUTF8" -sEXPORT_ES6=1 -sMODULARIZE=1 src/dds.cpp src/dump.cpp src/ABsearch.cpp src/ABstats.cpp src/CalcTables.cpp src/DealerPar.cpp src/File.cpp src/Init.cpp src/LaterTricks.cpp src/Memory.cpp src/Moves.cpp src/Par.cpp src/PlayAnalyser.cpp src/PBN.cpp src/QuickTricks.cpp src/Scheduler.cpp src/SolveBoard.cpp src/SolverIF.cpp src/System.cpp src/ThreadMgr.cpp src/Timer.cpp src/TimerGroup.cpp src/TimerList.cpp src/TimeStat.cpp src/TimeStatList.cpp src/TransTableS.cpp src/TransTableL.cpp src/DDSWrapper.c -I./include -I./src -o dds.js
```

