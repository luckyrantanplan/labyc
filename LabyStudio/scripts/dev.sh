#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

web_host="${LABYSTUDIO_WEB_HOST:-0.0.0.0}"
web_port="${LABYSTUDIO_WEB_PORT:-4173}"
server_port="${LABYSTUDIO_PORT:-4310}"

pids=()

cleanup() {
    local exit_code=$?

    trap - EXIT INT TERM

    for pid in "${pids[@]:-}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    wait "${pids[@]:-}" 2>/dev/null || true
    exit "$exit_code"
}

trap cleanup EXIT INT TERM

cd "$repo_root"

echo "Building shared workspace package..."
npm run build --workspace @labystudio/shared

echo "Starting LabyStudio server on http://0.0.0.0:${server_port}"
LABYSTUDIO_PORT="$server_port" npm run dev --workspace @labystudio/server &
pids+=("$!")

echo "Starting LabyStudio web app on http://${web_host}:${web_port}"
npm run dev --workspace @labystudio/web -- --host "$web_host" --port "$web_port" &
pids+=("$!")

echo "LabyStudio is starting. Press Ctrl+C to stop both processes."

wait -n "${pids[@]}"