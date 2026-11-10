#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
certificate_dir="${UDHO_HELLO_HTTPS_CERTIFICATE_DIR:-${script_dir}/certificates}"
certificate="${certificate_dir}/localhost-cert.pem"
private_key="${certificate_dir}/localhost-key.pem"
force=false

case "${1:-}" in
    "") ;;
    --force) force=true ;;
    *)
        echo "Usage: $0 [--force]" >&2
        exit 2
        ;;
esac

if ! command -v openssl >/dev/null 2>&1; then
    echo "openssl is required to generate the development certificate" >&2
    exit 1
fi

if [[ -e "${certificate}" || -e "${private_key}" ]]; then
    if [[ "${force}" != true ]]; then
        echo "Development credentials already exist in ${certificate_dir}" >&2
        echo "Run $0 --force to replace them" >&2
        exit 1
    fi
fi

mkdir -p -- "${certificate_dir}"
umask 077

openssl req \
    -x509 \
    -newkey rsa:2048 \
    -sha256 \
    -nodes \
    -days "${DAYS:-30}" \
    -keyout "${private_key}" \
    -out "${certificate}" \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1" \
    -addext "basicConstraints=critical,CA:FALSE" \
    -addext "keyUsage=critical,digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth"

chmod 0600 "${private_key}"
chmod 0644 "${certificate}"

openssl pkey -in "${private_key}" -check -noout
openssl verify -CAfile "${certificate}" "${certificate}"
openssl x509 -in "${certificate}" -noout -subject -dates -ext subjectAltName

echo
echo "Certificate: ${certificate}"
echo "Private key: ${private_key}"
echo "These credentials are for local development only."
