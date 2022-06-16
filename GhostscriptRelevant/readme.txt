Ghostscript is licensed under the GNU AFFERO GENERAL PUBLIC LICENSE.
Please consult the file COPYING for details about the license.

To build Ghostscript for yourself, download a source package of Ghostscript from the internet:

Copy the changed source file "gdevwpr2.c" into the src-directory of the extracted
Ghostscript package. The contained "gdevwpr2.c" file was tested with version 9.15 but will most probably
also work with later versions.

If necessary change the makefile "msvc32.mak". e.g set "Debug=1" or "COMPILE_INITS 1".
Invoke the compiler from the Visual Studio Command Prompt (here 2005) with:
nmake -a -f src\msvc32.mak MSVC_VERSION=8 DEVSTUDIO="d:\vs2005"
To rebuild everything use "-a"; adapt the DEVSTUDIO-entry to your needs.
Afterwards the bin-path contains the new "gsdll32.dll"s.

