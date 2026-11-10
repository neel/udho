# Hello over HTTP and HTTPS

This directory builds the same application for two transports:

- `udho-examples-hello` starts a plain HTTP server from `http.cpp`.
- `udho-examples-hello-https` starts a TLS server from `https.cpp`.

Both executables link `libexamples-hello-common.a`, which contains the shared
actions and data types. They also use the same routes, views, CSS, and embedded
resource bundle. Only their transport setup differs.

## Structure

- `http.cpp` - HTTP entry point.
- `https.cpp` - HTTPS entry point and TLS credential loading.
- `views/` - HTML views
- `assets/` - embedded resources.
- `hello-https-gen.sh` - creates a self-signed localhost certificate.

CMake copies the certificate generation script beside the executables in the
build directory.

## Run over HTTP

Enter the directory containing the built example executables and run:

```sh
./udho-examples-hello
```

Then open <http://localhost:9999/>.

## Run over HTTPS

From the same build directory:

```sh
./hello-https-gen.sh
./udho-examples-hello-https
```

The generation script creates `localhost-cert.pem` and `localhost-key.pem` in
the current directory. Then open <https://localhost:9999/>.

To use a different certificate and private key, pass their PEM files explicitly:

```sh
./udho-examples-hello-https --cert server-cert.pem --key server-key.pem
```

The generated certificate and private key are for local development only and
remain in the current directory. Run `hello-https-gen.sh` again whenever you
want to replace them.

## CLI Params

Both servers use port 9999 by default, so run them one at a time or pass
`--port` to select a different port. Use `--help` to see all available options:

```sh
./udho-examples-hello --help
# Or
./udho-examples-hello-https --help
```

Sample https output:

```text
Command-line options:
  -H [ --host ] arg (=0.0.0.0)     Host
  -P [ --port ] arg (=9999)        Port
  -T [ --threads ] arg (=0)        Number of threads
  -L [ --log ] arg (=fixed)        Logging mode: fixed or rotating
  --log-dir arg (=/tmp)            Log output directory
  --log-sock-dir arg (=/tmp)       Logging command socket directory
  -h [ --help ]                    Show command-line help
  --cert arg (=localhost-cert.pem) PEM certificate chain
  --key arg (=localhost-key.pem)   PEM private key

Commands:
  log  Control the logging consumer
```
