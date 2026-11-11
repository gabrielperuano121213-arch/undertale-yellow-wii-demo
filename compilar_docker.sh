#!/bin/sh
# Compila con la imagen oficial de devkitPro (requiere Docker). Resultado: apps/UndertaleYellow/
set -e
docker run --rm -v "$PWD":/project -w /project devkitpro/devkitppc make package
python3 tools/verificar.py apps/UndertaleYellow
