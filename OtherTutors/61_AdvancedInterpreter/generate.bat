@echo off
flex\bin\flex -olexer.cpp lexer.l

REM bisonnak sajnos kell az m4 ezert hulyulni kell
set CURR_DIR=%CD%

cd bison\bin
bison -o %CURR_DIR%/parser.cpp -d %CURR_DIR%/parser.y
cd %CURR_DIR%

pause
