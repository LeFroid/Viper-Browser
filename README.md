# Viper Browser

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/faef22a2f30e4f1cbea3b15db777cab3)](https://www.codacy.com/app/LeFroid/Viper-Browser?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=LeFroid/Viper-Browser&amp;utm_campaign=Badge_Grade)

A lightweight web browser built with the Qt framework and QtWebEngine

Licensed under GPLv3

## Features

*   Bookmark management
*   Built-in ad blocker, compatible with AdBlock Plus and uBlock Origin filters
*   Cookie viewer, editor, and support for cookie filters (QtWebEngine 5.11+ only)
*   Compatible with Pepper Plugin API
*   Custom user agent support
*   Fast and lightweight
*   Fullscreen support
*   Granular control over browser settings and web permissions
*   Gives the user control over their data, no invasions of privacy like other browsers are known to do..
*   GreaseMonkey-style UserScript support
*   PDF.js embedded into the browser
*   Save and restore browsing sessions, local tab history, pinned tabs
*   Secure AutoFill manager (disabled by default)
*   Traditional browser UI design instead of WebUI and chromium-based interfaces

## Building

The browser can be built using the cmake build system, by either importing the root CMakeLists file into your IDE of choice or performing the following commands from a console:

```console
$ cd viper-browser && mkdir build && cd build
$ cmake ..
$ make
```

The binary will be located in the `build/src` folder when following the commands listed above.

## Thanks

This project is possible thanks to the work of others, including those involved in the following projects:

*   Qt Framework
*   PDF.js 
*   Arora QT Browser
*   Qt Tab Browser example (From Qt 5.5 Webkit example archive)
*   Code Editor example
*   Qupzilla - for parts of AdBlockPlus implementation
*   Otter Browser - for parts of AdBlockPlus implementation

All licenses and credits can be found in the file `LICENSE`
