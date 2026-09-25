#!/bin/bash
# ============================================================
#  Compila o Feira Sort para rodar no navegador (WebAssembly).
#  O resultado vai para a pasta docs/, publicada pelo GitHub Pages.
# ============================================================
set -e

source ~/emsdk/emsdk_env.sh > /dev/null 2>&1
RAYLIB=~/raylib/src
RAYLIB_WEB=~/raylib-web

# 1) Compila a raylib para web (so na primeira vez)
if [ ! -f "$RAYLIB_WEB/libraylib.a" ]; then
    echo "Compilando a raylib para web (so na primeira vez)..."
    mkdir -p "$RAYLIB_WEB"
    for arquivo in rcore rshapes rtextures rtext rmodels utils raudio; do
        emcc -c "$RAYLIB/$arquivo.c" -o "$RAYLIB_WEB/$arquivo.o" -Os -w -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    done
    emar rcs "$RAYLIB_WEB/libraylib.a" "$RAYLIB_WEB"/*.o
fi

# 2) Compila o jogo
echo "Compilando o Feira Sort para web..."
mkdir -p docs
emcc src/main.cpp -o docs/index.html -std=c++17 -Os -DPLATFORM_WEB \
    -I "$RAYLIB" "$RAYLIB_WEB/libraylib.a" \
    -sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 \
    --preload-file Poppins-Bold.ttf --shell-file web/shell.html

echo ""
echo "Pronto! Arquivos gerados em docs/"
echo "Para testar aqui: cd docs && python3 -m http.server 8080"
echo "Depois abra no navegador: http://localhost:8080"
