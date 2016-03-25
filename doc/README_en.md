OpenToonz Plugin
===

You can develop raster effects worked on `FX Schematic` using COM-like low-level interfaces.
Here, we introduce "How to install plugins".

## How To Install Plugins

### Build

This section introduce how to build plugins from source codes.
Skip this section, if you already had `.plugin` files. 

0. Download source codes.
0. Build them (see their reference).
0. Rename file extensions to `.plugin`, if their extensions were `.dll`, `.dylib` or `.so`.

### Install

0. Move `.plugin` files to `${TOONZ_STUFF}/plugins` directory.
0. Restert `OpenToonz`.
0. Plugins are loaded, and you can choose those plugin effects in a FX Schematic window.

`${TOONZ_STUFF}` is a path to a stuff directory (the stuff directory is `C:/OpenToonz 1.0 stuff` on Windows by default).

## opentoonz_plugin_utility

`opentoonz_plugin_utility` is a wrapper library of low-level interfaces (see [opentoonz_plugin_utility](./opentoonz_plugin_utility_en.md)).
