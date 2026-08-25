# 미리보기 for OBS 32.2.2

Windows 64-bit OBS dock plugin providing:

- Live scene preview cards
- Click a card to switch the current scene
- Red border for the active scene
- Drag a card directly onto another card to reorder the dashboard only
- Per-scene-collection order persistence across OBS restarts
- Adjustable preview-card size (80-360 px)
- Uniform card and thumbnail dimensions for scenes with or without media
- Seek bar when a scene contains a playable media source
- New scenes appended automatically; deleted scenes removed automatically

The plugin never changes the order of OBS's built-in Scenes dock.

## Build

This project is designed to be placed into the official OBS Plugin Template or
built against an OBS 32.2.2 development tree with the `libobs` and
`obs-frontend-api` CMake packages available.

Required Windows tools:

- Visual Studio 2022 with Desktop development with C++
- CMake 3.30.x
- Qt 6 version used by OBS 32.2.2
- OBS Studio 32.2.2 development dependencies

Configure and build an x64 Release target. The output file is
`vmix-dashboard-core.dll`.

Install it to:

`C:\Program Files\obs-studio\obs-plugins\64bit\vmix-dashboard-core.dll`

Keep the older `vmix-panel-obs.dll` installed only if you want both docks. To
avoid duplicate dashboards, close OBS and move the older DLL out of the OBS
plugin directory before testing this one.

## Use

Open OBS, then enable **Docks > 미리보기**. Click a card to switch scenes.
Hold the left mouse button on any part of a card, drag it, and drop it directly
on another card. The new dashboard order is saved automatically.

Scenes containing a playable media source show a seek bar. Other scenes reserve
the same row height so all cards stay aligned.
