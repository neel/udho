#!/usr/bin/env bash

set -e

if ! command -v openssl >/dev/null 2>&1; then
    echo "openssl is required to generate the development certificate" >&2
    exit 1
fi

umask 077

openssl req \
    -x509 \
    -newkey rsa:2048 \
    -sha256 \
    -nodes \
    -days 30 \
    -keyout localhost-key.pem \
    -out localhost-cert.pem \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1" \
    -addext "basicConstraints=critical,CA:FALSE" \
    -addext "keyUsage=critical,digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth"

chmod 0600 localhost-key.pem
chmod 0644 localhost-cert.pem

openssl pkey -in localhost-key.pem -check -noout
openssl verify -CAfile localhost-cert.pem localhost-cert.pem
openssl x509 -in localhost-cert.pem -noout -subject -dates -ext subjectAltName

echo
echo "Generated localhost-cert.pem and localhost-key.pem."
echo "These credentials are for local development only."
