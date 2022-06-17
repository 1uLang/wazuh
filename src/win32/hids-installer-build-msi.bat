SETLOCAL
SET PATH=%PATH%;C:\Program Files\Microsoft SDKs\Windows\v7.0\Bin
SET PATH=%PATH%;C:\Program Files (x86)\WiX Toolset v3.11\bin

set VERSION=%1
set REVISION=%2

REM IF VERSION or REVISION are empty, ask for their value
IF [%VERSION%] == [] set /p VERSION=Enter the version of the Hids agent (x.y.z):
IF [%REVISION%] == [] set /p REVISION=Enter the revision of the Hids agent:

SET MSI_NAME=hids-agent-%VERSION%-%REVISION%.msi

candle.exe -nologo "hids-installer.wxs" -out "hids-installer.wixobj" -ext WixUtilExtension -ext WixUiExtension
light.exe "hids-installer.wixobj" -out "%MSI_NAME%"  -ext WixUtilExtension -ext WixUiExtension

signtool sign /f 18c1fd6c56759d7c01eac0c7318280dc.pfx /p 202203 /d "%MSI_NAME%"  "%MSI_NAME%" 

pause
