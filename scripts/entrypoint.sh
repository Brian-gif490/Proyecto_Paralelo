#!/usr/bin/env bash
set -euo pipefail

NODE_NAME="${NODE_NAME:-$(hostname)}"
ROLE="${ROLE:-worker}"
PEER_HOSTS="${PEER_HOSTS:-}"
SSH_DIR="/root/.ssh"
KEY_DIR="/ssh-keys"
PUB_DIR="${KEY_DIR}/pubkeys"

log() { echo "[entrypoint][${NODE_NAME}] $*"; }

mkdir -p "$SSH_DIR" "$PUB_DIR" /run/sshd
chmod 700 "$SSH_DIR"
chmod 755 "$KEY_DIR" "$PUB_DIR"

KEY_PATH="${KEY_DIR}/id_rsa_${NODE_NAME}"
if [ ! -f "$KEY_PATH" ]; then
    log "Generando par de llaves SSH"
    ssh-keygen -t rsa -b 4096 -N "" -f "$KEY_PATH" -C "${NODE_NAME}@tel420" >/dev/null
fi

install -m 600 "$KEY_PATH" "$SSH_DIR/id_rsa"
install -m 644 "${KEY_PATH}.pub" "$SSH_DIR/id_rsa.pub"
install -m 644 "${KEY_PATH}.pub" "${PUB_DIR}/${NODE_NAME}.pub"

cat > "$SSH_DIR/config" <<'EOF'
Host *
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
    LogLevel ERROR
    ServerAliveInterval 30
    ConnectTimeout 5
EOF
chmod 600 "$SSH_DIR/config"

EXPECTED=("$NODE_NAME")
PEERS=()
if [ -n "$PEER_HOSTS" ]; then
    read -r -a PEERS <<< "$PEER_HOSTS"
    EXPECTED+=("${PEERS[@]}")
fi

log "Esperando llaves publicas de: ${EXPECTED[*]}"
deadline=$((SECONDS + 60))
while [ "$SECONDS" -lt "$deadline" ]; do
    missing=0
    for host in "${EXPECTED[@]}"; do
        [ -f "${PUB_DIR}/${host}.pub" ] || missing=1
    done
    [ "$missing" -eq 0 ] && break
    sleep 1
done

tmp_keys="$(mktemp)"
cat "${PUB_DIR}"/*.pub > "$tmp_keys" 2>/dev/null || true
install -m 600 "$tmp_keys" "$SSH_DIR/authorized_keys"
rm -f "$tmp_keys"

ssh-keygen -A >/dev/null 2>&1 || true
if ! pgrep -x sshd >/dev/null 2>&1; then
    /usr/sbin/sshd
fi
log "sshd iniciado"

if [ "$ROLE" = "master" ]; then
    for host in "${PEERS[@]}"; do
        log "Esperando SSH en ${host} ..."
        ok=0
        for _ in $(seq 1 60); do
            if ssh -o BatchMode=yes -o ConnectTimeout=2 "$host" true >/dev/null 2>&1; then
                ok=1
                break
            fi
            sleep 1
        done
        if [ "$ok" -eq 1 ]; then
            log "SSH listo: ${host}"
        else
            log "ADVERTENCIA: no se pudo conectar por SSH a ${host}"
        fi
    done
fi

log "Nodo listo (rol=${ROLE})"
exec "$@"
