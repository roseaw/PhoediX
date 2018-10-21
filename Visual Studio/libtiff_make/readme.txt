This project will build the libtiff library.

Ignore the below as it is just some hints on how to configure the project itself.

With Microsoft Visual C++ installed, and properly configured for commandline use (you will likely need to source VCVARS32.BAT in AUTOEXEC.bAT or somewhere similar) you should be able to use the provided makefile.vc.
The source package is delivered using Unix line termination conventions, which work with MSVC but do not work with Windows 'notepad'. If you use unzip from the Info-Zip package, you can extract the files using Windows normal line termination conventions with a command similar to:

  unzip -aa -a tiff-4.0.5.zip
By default the nmake-based libtiff build does not depend on any additional libraries. Normally libtiff should be built with at least JPEG and ZIP support so that it can open JPEG and ZIP-compressed TIFF files. In order to add additional libraries (e.g. libjpeg, zlib, jbigkit), build those libraries according to their own particular build instructions, and then edit 'nmake.opt' (using a capable plain-text editor) to enable use of the libraries, including specifying where the libraries are installed. It is also necessary to edit libtiff/tiffconf.vc.h to enable the related configuration defines (JPEG_SUPPORT, OJPEG_SUPPORT, PIXARLOG_SUPPORT, ZIP_SUPPORT), or to disable features which are normally included by default. Ignore the comment at the top of the libtiff/tiffconf.vc.h file which says that it has no influence on the build, because the statement is not true for Windows. Please note that the nmake build copies tiffconf.vc.h to tiffconf.h, and copies tif_config.vc.h to tif_config.h, overwriting any files which may be present. Likewise, the 'nmake clean' step removes those files.

To build using the provided makefile.vc you may use:

  C:\tiff-4.0.5> nmake /f makefile.vc clean
  C:\tiff-4.0.5> nmake /f makefile.vc

    or (the hard way)

  C:\tiff-4.0.5> cd port
  C:\tiff-4.0.5\port> nmake /f makefile.vc clean
  C:\tiff-4.0.5\port> nmake /f makefile.vc
  C:\tiff-4.0.5> cd ../libtiff
  C:\tiff-4.0.5\libtiff> nmake /f makefile.vc clean
  C:\tiff-4.0.5\libtiff> nmake /f makefile.vc
  C:\tiff-4.0.5\libtiff> cd ..\tools
  C:\tiff-4.0.5\tools> nmake /f makefile.vc clean
  C:\tiff-4.0.5\tools> nmake /f makefile.vc
This will build the library file libtiff\libtiff\libtiff.lib.

The makefile also builds a DLL (libtiff.dll) with an associated import library (libtiff_i.lib). Any builds using libtiff will need to include the LIBTIFF\LIBTIFF directory in the include path.

The libtiff\tools\makefile.vc should build .exe's for all the standard TIFF tool programs.

