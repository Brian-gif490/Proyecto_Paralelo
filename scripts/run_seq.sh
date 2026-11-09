#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

DATA_FILE="${1:-data/puntos_yacuiba.csv}"

docker compose exec -T master bash -lc "cd /app && ./bin/tsp_seq \"${DATA_FILE}\""
