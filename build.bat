@echo off

if NOT "%Platform%" == "X64" IF NOT "%Platform%" == "x64" (call vcvarsall x64)

set exe_name=TGFPA
set compile_flags= -nologo /Zi /FC /I ../include/ /W4
set linker_flags= glfw3dll.lib gdi32.lib user32.lib kernel32.lib opengl32.lib Comdlg32.lib Shell32.lib
set linker_path="../lib/"

set main_file="../src/main.cpp"

if "%1" == "release" (
    echo "BUILDING RELEASE!"
    set compile_flags=%compile_flags% /O2
    set linker_flags=%linker_flags% /SUBSYSTEM:windows /ENTRY:mainCRTStartup
) else (
    set compile_flags=%compile_flags% /fsanitize=address
)

if "%1" == "trace" (
    set compile_flags=%compile_flags% -DMTR_ENABLED
)


if not exist build mkdir build
pushd build

start /b /wait "" "cl.exe" %compile_flags%  %main_file% /link %linker_flags% /libpath:%linker_path% /out:%exe_name%.exe

copy ..\lib\* . >NUL
rem copy ..\resources\* . >NUL
xcopy /E /I /Y ..\resources\* .
popd
