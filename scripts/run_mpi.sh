#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

NP="${1:-3}"

if ! [[ "$NP" =~ ^[0-9]+$ ]] || [ "$NP" -lt 1 ]; then
    echo "ERROR: el numero de procesos debe ser un entero positivo" >&2
    exit 1
fi

docker compose exec -T master bash -lc "cd /app && mpirun --allow-run-as-root --hostfile /app/config/hostfile --map-by node --bind-to none --mca btl ^openib --oversubscribe -np ${NP} /app/bin/tsp_mpi"
