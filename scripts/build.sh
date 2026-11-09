#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

if ! docker compose ps --status running --services 2>/dev/null | grep -qx "master"; then
    echo "ERROR: el nodo master no esta en ejecucion." >&2
    echo "Ejecute primero: docker compose up -d --build" >&2
    exit 1
fi

docker compose exec -T master bash /app/scripts/compile.sh
