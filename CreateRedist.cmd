@echo off
rem Copies application, help and ini files to redist folders

@rd /S /Q Redist

@mkdir "Redist\32_Bit"
@mkdir "Redist\64_Bit"

@copy Doc\Handbuch.pdf "Redist\32_Bit"
@copy Doc\Handbuch.pdf "Redist\64_Bit"
@copy Doc\Manual.pdf "Redist\32_Bit"
@copy Doc\Manual.pdf "Redist\64_Bit"

@copy gsbatchprint.ini "Redist\32_Bit"
@copy gsbatchprint.ini "Redist\64_Bit"

@copy x64\gsbatchprint\Release\gsbatchprint.exe "Redist\64_Bit"
@copy x64\gsbatchprintc\Release\gsbatchprintc.exe "Redist\64_Bit"

@copy Win32\gsbatchprint\Release\gsbatchprint.exe "Redist\32_Bit"
@copy Win32\gsbatchprintc\Release\gsbatchprintc.exe "Redist\32_Bit"

rem Compress under Windows 10 if powershell is enabled
@powershell -Command "Compress-Archive -Path Redist\32_Bit\* -DestinationPath Redist\GSBatchPrint32.zip"
@powershell -Command "Compress-Archive -Path Redist\64_Bit\* -DestinationPath Redist\GSBatchPrint64.zip"
