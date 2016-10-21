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

There is a simple example program called "ssc_lua_example" (it builds on
the bin folder of the build/stage folder) which takes a Lua simulation
script and allows to send data to the simulation from the command line.

A good simulation file to start playing might be "example.lua", which is
an overview of the available features. It is located on the same path as
the "ssc_lua_example" console program.

The program has the limitation that it can just send data to the fiber
group 0 (referred as 1 from the Lua/simulation side).

Current status
==============

Very basic testing done. Everything looks fine so far.

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
