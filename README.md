# Qt Web Browser

A lightweight web browser built with the Qt framework and WebKit

Partially derived from the Arora Qt browser and the Tab Browser from the Qt 5.5 WebKit example archive 

# Note

This project is done in my spare time and is very far from being complete. It is probably a bad idea to use this browser as a substitute for any of the mainstream ones.

# Features

* Built-in advertisement blocking, based on a hosts file
* Multi-tab style web browser
* Ability to customize the user agent
* Support for manual cookie addition / removal / modifications
* Toggle JavaScript on or off in settings
* Compatible with NPAPI plugins

# Building

The browser software uses the qmake build system. The project can be built with QtCreator or through the command line by running 'qmake-qt5' or 'qmake', followed by 'make'

If it is your first time running the program, you will have to manually copy the files 'hosts_adblock.txt' and 'search_engines.json' into the directory ~/.config/Vaccarelli/

