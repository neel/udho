#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "${script_dir}/../.." && pwd)"
certificate_dir="${UDHO_HELLO_HTTPS_CERTIFICATE_DIR:-${script_dir}/certificates}"
certificate="${certificate_dir}/localhost-cert.pem"
private_key="${certificate_dir}/localhost-key.pem"
binary="${UDHO_HELLO_HTTPS_BINARY:-${project_dir}/build/Desktop-Debug/examples/hello.https/udho-examples-hello-https}"

if [[ ! -e "${certificate}" && ! -e "${private_key}" ]]; then
    "${script_dir}/generate-dev-certificate.sh"
elif [[ ! -f "${certificate}" || ! -f "${private_key}" ]]; then
    echo "The HTTPS credential pair in ${certificate_dir} is incomplete" >&2
    echo "Run ${script_dir}/generate-dev-certificate.sh --force to replace it" >&2
    exit 1
fi

if [[ ! -x "${binary}" ]]; then
    echo "Hello HTTPS example binary is not executable: ${binary}" >&2
    echo "Set UDHO_HELLO_HTTPS_BINARY to the path of your built udho-examples-hello-https binary" >&2
    exit 1
fi

cd -- "${script_dir}"
exec "${binary}" \
    --tls-certificate "${certificate}" \
    --tls-private-key "${private_key}" \
    "$@"
