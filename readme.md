Simulator Scheduler Lua (ssc_lua)
=================================

LUA binding for the simulator scheduler.

Special credit
==============

This project is part of an unrealeased and unfinished project at my former
employer [Diadrom AB](http://diadrom.se/) which they kindly accepted
to release as open source with a BSD license.

Description
============

A working implementation of ssc where the simulations are written in LUA.
LuaJIT is used as Lua implementation.

TODO: Add example program

Current status
==============

Very basic testing done.

Build (Linux)
=============

Be aware that cmocka used on base_library requires CMake.

git submodule update --init --recursive
cd dependencies
./prepare_dependencies.sh
cd ../build/premake
premake5 gmake
make -C ../linux config=release verbose=yes

Build (Windows)
===============

Hopefully with Windows bash the steps above will work. the premake5 command
should be run to generate Visual Studio solutions. (e.g.: premake5 vs2015).
