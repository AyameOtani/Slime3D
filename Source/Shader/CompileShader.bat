@echo off
cd /d "%~dp0"

echo ================================
echo Vertex Shader Compile
echo ================================
ShaderCompiler.exe /Tvs_3_0 SlimeRefractionVS.hlsl

if errorlevel 1 (
    echo.
    echo Vertex Shader compile ERROR
    pause
    exit /b
)

echo.
echo ================================
echo Pixel Shader Compile
echo ================================
ShaderCompiler.exe /Tps_3_0 SlimeRefractionPS.hlsl

if errorlevel 1 (
    echo.
    echo Pixel Shader compile ERROR
    pause
    exit /b
)

echo.
echo ================================
echo Compile Complete
echo ================================
echo SlimeRefractionVS.vso
echo SlimeRefractionPS.pso
echo ================================

pause
