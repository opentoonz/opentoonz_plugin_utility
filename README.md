OpenToonz Plugin Utility
============

[日本語](./doc/README_ja.md)

You can develop raster effects worked on FX Schematic using COM-like low-level interfaces.
`opentoonz_plugin_utility` is a wrapper library of the interfaces.
The library makes it easy to create new plugin effects.

## How to install created plugins

### Build

This section introduce how to build plugins from source codes.
Skip this section, if you already had `.plugin` files. 

0. Download source codes.
0. Build them (see their reference).
0. Rename file extensions to `.plugin`, if their extensions were `.dll`, `.dylib` or `.so`.

### Install

0. Move `.plugin` files to `${path-to-opentoonz-stuff}/plugins` directory.
0. Restert `OpenToonz`.
0. Plugins are loaded, and you can choose those plugin effects in a FX Schematic window.

`${path-to-opentoonz-stuff}` is `/Applications/OpenToonz/OpenToonz_1.0_stuff/plugins/` (OSX) or `C:\OpenToonz 1.0 stuff\plugins` (Windows) by default.

## How to develop plugins using the library

see [here](./doc/opentoonz_plugin_utility.md).
