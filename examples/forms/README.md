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
| `POST /submit` | URL-encoded `name`, `age`, optional `newsletter` | HTML field table, or form errors with HTTP 400 | 16 KiB |
| `POST /json` | Raw `application/json` body | Parsed JSON echoed as JSON | 16 KiB |
| `POST /upload` | Multipart upload under field name `file` | HTML file details | 1 MiB |

Open the home page to submit a name (2–60 bytes), age (18–120), and optional
newsletter checkbox, choose a file, or send the JSON textarea contents. Only the
JSON form uses JavaScript; it sends the raw textarea value with `fetch` and
displays the JSON response or error as text. Invalid JSON can therefore be tested
directly in the UI. The other forms use ordinary browser submissions.

The form action uses `udho::forms` to parse typed fields and apply constraints:

```cpp
#include <udho/forms/forms.h>

using namespace udho::forms;
auto name = required<std::string>("name")
    .constrain<constraints::gte>(2, "Use at least 2 bytes.")
    .constrain<constraints::lte>(60, "Use at most 60 bytes.");
auto age = required<int>("age")
    .constrain<constraints::gte>(18, "Age must be at least 18.")
    .constrain<constraints::lte>(120, "Age must be at most 120.");
auto newsletter = optional<bool>("newsletter", false);
auto validation = validate(context.portal().body().form());
validation >> name >> age >> newsletter;
```

The action checks `validation.valid()` before using `*name`, `*age`, and
`*newsletter`. A valid submission shows the typed name, age, and newsletter choice
in a table. An absent checkbox defaults to `false`; its HTML value is `1` when
checked. String constraints measure `std::string::size()`, so the name limits
count bytes.

Invalid fields return HTTP 400 and render the home form again with the accumulated
errors and a message beside each invalid input. Each input is associated with its
message through `aria-describedby`. The form preserves the submitted name and age
text, including malformed values, and the parsed newsletter choice. The age input
uses a numeric keyboard hint without browser range restrictions, so server-side
age errors can be tried in the form. Each request creates fresh fields because a
field caches its first fetch.

The Lua views escape dynamic titles, messages, errors, field names, values, and
attribute values with `udho.utils.htmlescape`. Scalar validation reads the first
occurrence of a repeated name. Uploads are inspected for their temporary names
and sizes, then deleted. The UI selects one file, and the upload handler accepts
all repeated `file` parts.

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
with missing, malformed, or out-of-range required fields, or an upload without a
file, returns HTTP 400.

Routes configure `body_memory_limit`, `field_memory_limit`, and a 30-second
`body_time_limit`. The 1 MiB upload cap includes multipart boundaries and headers,
so the file itself must be smaller. Uploaded files are temporary and are not
retained as permanent storage.

The action context includes the transport-neutral `udho::www::components::request`
component, exposing `context.portal().request()` and `context.portal().body()`.
The normal form action validates `body().form()` with `udho::forms`; uploads use
the parsed multipart fields, and raw JSON is parsed through `body().json()`.
