# Latitude Ruler for OpenCPN

A small OpenCPN 1.18 plugin which draws a nautical latitude ruler along the left edge of the chart canvas.

## What it does

- Shows latitude marks on the left side of the chart.
- Adapts major and minor tick spacing automatically to the current zoom level.
- Labels major ticks as degrees/minutes/seconds as appropriate.
- Uses the OpenCPN viewport transformation, so the scale follows chart panning and zooming.
- Supports both normal wxDC rendering and OpenGL chart rendering.
- Can be toggled from the OpenCPN toolbar.
- Adapts its colors to Day, Dusk and Night schemes.

## Build on Windows

This repository follows the same lightweight API-18 structure used by the other OpenCPN plugins in this account.

```bat
git clone --recurse-submodules https://github.com/jkuhnen/latituderuler_pi.git
cd latituderuler_pi
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
cpack -C Release
```

If the repository was cloned without submodules:

```bat
git submodule update --init --recursive
```

## Status

Version 0.1.0.0 is the first functional implementation. The visual design can be tuned after testing it inside the actual OpenCPN canvas.
