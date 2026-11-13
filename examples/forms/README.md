# Forms and request bodies

This HTTP example demonstrates normal form submissions, multipart uploads, and a
JSON request sent with AJAX. It uses the stateless Lua framework without sessions.

`actions.cpp` processes request data. `helpers.h` handles rendering and temporary
upload cleanup; `data.h` provides small page and field types. The HTML lives in
`views/`, with CSS and JavaScript in `assets/`. CMake embeds them in the `forms`
resource bundle, served under `/assets/forms/`.

| Route | Input | Response | Request limit |
| --- | --- | --- | --- |
| `GET /` | Three forms | HTML home page | No body |
| `POST /submit` | URL-encoded `name`, `email`, `message` | HTML field table | 16 KiB |
| `POST /json` | Raw `application/json` body | Parsed JSON echoed as JSON | 16 KiB |
| `POST /upload` | Multipart upload under field name `file` | HTML file details | 1 MiB |

Open the home page to submit a required name and optional email/message, choose a
file, or send the JSON textarea contents. Only the JSON form uses JavaScript; it
sends the raw textarea value with `fetch` and displays the JSON response or error
as text. Invalid JSON can therefore be tested directly in the UI. The other forms
use ordinary browser submissions.

The Lua result view escapes dynamic titles, messages, field names, and values
with `udho.utils.htmlescape`. Repeated form fields remain separate table rows.
Uploads are inspected for their temporary names and sizes, then deleted. The UI
selects one file, and the handler also accepts repeated `file` parts.

## Build and run

From the repository root, with examples enabled:

```sh
cmake --build <build-dir> --target udho-examples-forms
```

From `<build-dir>/examples/forms`:

```sh
./udho-examples-forms
```

Open <http://localhost:9999/>. Use `--port` to change the default port, or `--help`
for the standard CLI options.

## JSON and request limits

The JSON endpoint also works with command-line clients:

```sh
curl -H 'Content-Type: application/json' \
     --data '{"name":"Ada","items":[1,2,3]}' \
     http://localhost:9999/json
```

It returns `application/json`. Invalid or empty JSON returns HTTP 400; the wrong
media type returns HTTP 415. JSON errors contain an `error` message. A normal form
without a name or an upload without a file returns HTTP 400.

Routes configure `body_memory_limit`, `field_memory_limit`, and a 30-second
`body_time_limit`. The 1 MiB upload cap includes multipart boundaries and headers,
so the file itself must be smaller. Uploaded files are temporary and are not
retained as permanent storage.

The action context includes the transport-neutral `udho::www::components::request`
component, exposing `context.portal().request()` and `context.portal().body()`.
The actions read URL-encoded and multipart fields through `body().form().fields()`
and parse raw JSON through `body().json()`.
