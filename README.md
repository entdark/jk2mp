JK2MP
=====

# Introduction #
This is the source code of Star Wars Jedi Knight 2: Jedi Outcast Multiplayer v1.04. The goal of this source is to be compilable in Microsoft Visual Studio 2010 (and therefore later versions). The code is cleaned a bit and has some minor fixes.

# Building #
By default it builds 2 applications (.exe): jk2mp and jk2Ded; and 3 dinamic libraries (.dll): cgamex86, uix86 and jk2mpgamex86. It doesn't build QVMs but the code of them is left so it is possible to build them as well.
To build open a solution file (.sln) in CODE-mp folder and then press F7 (Build). It can build both Debug and Release versions without errors.

# Features #
* removed cd check
* the developer console can now be open with ` (holding SHIFT is not necessary anymore)
* the game doesn't crash anymore on creating local server with custom resolution
* increased fov limit to 180 degrees

# Issues #
For some reason game crashes either in Sys_Init or in QuickMemTest. I commented QuickMemTest and timeBeginPeriod(1) in Sys_Init to avoid crashes. I am not sure if those are important.
Also I could not link openal32.dll to jk2mp.exe therefore openal is not supported (s_UseOpenAL = false).
