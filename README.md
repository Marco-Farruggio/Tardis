Tardis
======

Tardis is a Windows python based program which bundles a directory into a single .exe
Provides a C bootloader which is the bulk of the .exe, and appends the zipped directory onto itself,
allowing for it to be unzipped to Window's temporary file system. Packages a python version with the bundle,
runs an entry point script to initiate the directory's code running process. Fetches data from a single tardis.config file
in the directory provided. Supports compression in zipping and provides python scripts with sys._TARDIS_PATH, which returns
the current absolute path to the temporary file for internal path navigation.
