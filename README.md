# Viper Browser

A lightweight web browser built with the Qt framework and WebKit

Licensed under GPLv3

# Note

This project is done in my spare time and is far from being complete. It is probably a bad idea to use this browser as a substitute for any of the mainstream ones.

# Features

* Built-in AdBlock Plus support, with uBlock Origin filter compatibility (mostly compatible)
* Greasemonkey-style UserScript support
* Multi-tab style web browser
* Ability to customize the user agent
* Support for manual cookie addition / removal / modifications
* Toggle JavaScript on or off in settings
* Compatible with NPAPI plugins

# Building

The browser software uses the qmake build system. The project can be built with QtCreator or through the command line by running 'qmake-qt5' or 'qmake', followed by 'make'

If it is your first time running the program, you will have to manually copy the file 'search_engines.json' into the directory ~/.config/Vaccarelli/

# Thanks

This project is possible thanks to the work of others, including those involved in the following projects:

* Qt Framework
* PDF.js (license in file PDFJS-APACHE-LICENSE)
* Arora QT Browser (licenses in files LICENSE.GPL2, LICENSE.LGPL3)
* Qt Tab Browser example (From Qt 5.5 Webkit example archive)
* Code Editor example (From Qt examples, license in file CodeEditor-LICENSE-BSD)
* Qupzilla - for parts of AdBlockPlus implementation (license in file GPLv3)
* Otter Browser - for parts of AdBlockPlus implementation (license in file GPLv3)
