#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "== Compilando componentes =="
make clean
make all

echo
echo "== Binarios generados =="
ls -lh "$ROOT_DIR/bin"
