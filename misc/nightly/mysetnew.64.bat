call %~dp0base_64.bat

nmake /f makefile_vc build USEDEPS=1 MP_LIMIT=1
