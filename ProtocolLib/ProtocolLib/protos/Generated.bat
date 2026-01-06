@echo off
pushd "%~dp0"

set PROTOC=./protoc/protoc.exe
set PROTO_DIR=./
set OUT_DIR=../Protocols

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

for %%f in (*.proto) do (
    echo Compiling %%~nxf ...
    "%PROTOC%" -I="%PROTO_DIR%" --cpp_out="%OUT_DIR%" "%%f"
)

IF ERRORLEVEL 1 PAUSE

popd