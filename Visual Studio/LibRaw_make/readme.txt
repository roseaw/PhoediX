This project will build the LibRaw library.

Ignore the below as it is just some hints on how to configure the project itself.

Building under Windows has three steps:

Unpack the distribution package (if you have got no tar+gzip, take the LibRaw distribution package in the .ZIP format) and go to folder LibRaw-X.YYY.
Set the environment parameters so that the compiler/linker would find the libraries and include-files. For Visual C++, this is done by running vcvars32.bat.
Run
nmake -f Makefile.msvc
If all paths are set correctly and the include-files/libraries have been found, then the following will be compiled:

Library libraw_static.lib in folder lib
Dynamic library bin/libraw.dll and linking library for it lib/libraw.lib
Examples in folder bin/.
Only the thread-safe library is built under Win32, but it can be used with non-threaded applications as well. All examples are linked with the dynamic library (DLL); if static linking is necessary, one should link applications with library libraw_static.lib and set the preprocessor option /DLIBRAW_NODLL during compilation.

Windows-version compiles without LCMS support for now.

During building of DLL, all public functions are exported; further, the exported subset may be reduced.

Unfortunately, paths to include/ libraries depend on the way Visual C (or other compiler) is installed; therefore, it is impossible to specify some standard paths in Makefile.msvc.