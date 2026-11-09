#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

echo "=== Contenedores del cluster ==="
docker compose ps
echo
echo "=== Direcciones IP ==="
for container in tel420-master tel420-worker1 tel420-worker2; do
    printf "%-18s " "$container"
    docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$container" 2>/dev/null || echo "no disponible"
    echo
done
echo
echo "=== Prueba SSH master -> workers ==="
docker compose exec -T master bash -lc '
for host in worker1 worker2; do
    printf "%-10s " "$host"
    if ssh -o BatchMode=yes -o ConnectTimeout=3 "$host" "hostname" 2>/dev/null; then
        echo " [OK]"
    else
        echo " [FALLO]"
    fi
done'
