# Cube World Server Idle CPU Fix

The original Server.exe from Cube World Alpha runs a world/zone management
thread, that never sleeps or waits. Essentially, it forces one CPU core to run
at 100%. This causes modern CPUs with advanced boost mechanisms to boost as
high as they can, causing temperature hotspots and increased fan speeds.

This mod injects a short sleep period into the thread after each cycle. In
fact, the client runs a nearly identical internal server thread in
singleplayer, but with a short sleep period included.

## Installation

* requires [coremaze Cube-World-Server-Mod-Launcher](https://github.com/coremaze/Cube-World-Server-Mod-Launcher/releases/tag/prerelease2)
* unpack Mod Launcher in the game directory
* put `IdleCPUFix.dll` into `Server_Mods`
* start server via `ServerModLauncher.exe`

## Compilation

Requires a compiler that targets 32-bit Windows. Due to the `asm` style, `MSVC`
will not work, use `GCC` or `Clang`. Run `make` in this directory and specify 
the compiler as `CXX` variable if necessary (e.g. 
`make CXX=i686-w64-mingw32-g++`).
