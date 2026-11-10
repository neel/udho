#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
certificate_dir="${UDHO_HELLO_HTTPS_CERTIFICATE_DIR:-${script_dir}/certificates}"
certificate="${certificate_dir}/localhost-cert.pem"
url="${1:-https://localhost:${PORT:-9999}/}"

if ! command -v curl >/dev/null 2>&1; then
    echo "curl is required to test the HTTPS endpoint" >&2
    exit 1
fi

if [[ ! -f "${certificate}" ]]; then
    echo "Certificate not found: ${certificate}" >&2
    echo "Run ${script_dir}/generate-dev-certificate.sh first" >&2
    exit 1
fi

exec curl \
    --fail \
    --show-error \
    --silent \
    --cacert "${certificate}" \
    "${url}"
