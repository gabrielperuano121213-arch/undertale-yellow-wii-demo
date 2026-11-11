@echo off
where docker >nul 2>nul || (echo Necesitas Docker Desktop: https://www.docker.com/products/docker-desktop & pause & exit /b 1)
docker run --rm -v "%cd%":/project -w /project devkitpro/devkitppc make package
if exist apps\UndertaleYellow\boot.dol (echo. & echo LISTO: copia la carpeta apps\UndertaleYellow a SD:\apps\) else (echo ERROR: no se genero boot.dol)
pause
