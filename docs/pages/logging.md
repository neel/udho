Logging {#LoggingPage}
========

@brief High-level guide for application developers using the UDHO logging subsystem.

@tableofcontents
@section logging_overview Overview
The UDHO logging subsystem is designed around a multi-producer / single-consumer model.
Application code emits log records through lightweight producer-side calls such as
@ref UDHO_LOG_INFO or @ref UDHO_LOG_ERROR. These records are serialized and sent
over an interprocess message queue to a dedicated logger process. The logger process
reconstructs the records and forwards them to the configured output sinks.
In normal application code you usually need only three things:
- start the logging subsystem at program startup
- emit records through the logging macros
- keep the returned RAII logger handle alive until program shutdown
@section logging_quick_start Quick start
The preferred usage pattern is to create the logger handle near the beginning of
@c main and let it stop automatically on scope exit.

@code
#include <udho/logging/setup.h>
#include <udho/logging/macros.h>
int main() {
    auto log = udho::logging::start<>();
    UDHO_LOG_INFO("app", "Application started");
    UDHO_LOG_WARNING("http", "Slow request observed");
    UDHO_LOG_ERROR("db", "Database connection failed");
    return 0;
}
@endcode
The object returned by @ref udho::logging::start owns the lifetime of the logging
subsystem. When it is destroyed, logging is stopped automatically.
@section logging_lifetime Logger lifetime and RAII
Logging is normally started through @ref udho::logging::start, which returns an
owning RAII handle.
Keep that handle alive for as long as logging is needed.
Recommended pattern:
@code
int main() {
    auto log = udho::logging::start<>();
    // application objects created after this point may safely log
    // during normal execution
    return run_application();
}
@endcode
If other local objects may emit logs from their destructors, declare the logger
handle before those objects so that it is destroyed last.
Example:
@code
int main() {
    auto log = udho::logging::start<>();
    MyApplication app;      // if app destructor logs, this is the safer order
    return app.run();
}
@endcode
@section logging_emit Emitting log records
The simplest way to emit a record is to use one of the severity-specific macros:
- @ref UDHO_LOG_TRACE
- @ref UDHO_LOG_DEBUG
- @ref UDHO_LOG_INFO
- @ref UDHO_LOG_WARNING
- @ref UDHO_LOG_ERROR
- @ref UDHO_LOG_FATAL
Each macro captures:
- source file
- source line
- source function
automatically.
Basic examples:
@code
UDHO_LOG_INFO("http", "Request received");
UDHO_LOG_DEBUG("router", "Matched route");
UDHO_LOG_ERROR("session", "Session lookup failed");
@endcode
The first argument is the logical subsystem name.
The second argument is the final message text.
@section logging_return_value Return value of logging calls
Logging calls return a count-like value of type @c std::size_t.
This value represents the number of messages transferred to the IPC queue during
that call. The count may include previously backlogged messages flushed during the
same call.
In particular:
- a value greater than zero means at least one message reached the IPC queue
- a value of zero does not necessarily mean the current message was permanently lost
- the current message may have been accepted into the producer-side backlog if the
  IPC queue was temporarily full
Typical usage:
@code
std::size_t transferred = UDHO_LOG_INFO("http", "Request received");
if (transferred == 0) {
    // no immediate transfer happened during this call
}
@endcode
@section logging_optional_fields Optional structured fields
In addition to the mandatory fields, you may attach typed optional fields to a log record.
Optional fields are passed as strongly typed wrappers from @ref udho::logging::params.
Examples:
@code
UDHO_LOG_INFO(
    "http",
    "Request completed",
    udho::logging::params::request_id("req-123"),
    udho::logging::params::status_code(200),
    udho::logging::params::bytes_sent(4096),
    udho::logging::params::latency(std::chrono::milliseconds(12))
);
@endcode
Common optional fields include:
- @c request_id
- @c flow_id
- @c socket_id
- @c session_id
- @c user_id
- @c client
- @c host
- @c method
- @c uri
- @c route
- @c query
- @c agent
- @c status_code
- @c bytes_sent
- @c latency
- @c retry_count
These fields are optional. Only fields provided by the caller are attached to the record.
@section logging_required_fields Mandatory fields added automatically
Every record created through the public logging macros automatically includes:
- a process-local monotonically increasing record ID
- timestamp
- severity
- thread identifier
- process ID
- subsystem
- message text
- source file
- source function
- source line
This means application code usually does not need to populate those fields manually.
@section logging_flow Data flow
The logging subsystem has two sides:
@subsection logging_flow_producer Producer side
The application process is the producer side. It:
- constructs log records
- serializes them
- attempts to send them to the IPC queue
- may temporarily keep accepted records in an in-memory backlog when the queue is full
@subsection logging_flow_consumer Consumer side
The logger child process is the consumer side. It:
- reads serialized records from the IPC queue
- reconstructs their fields
- forwards them into Boost.Log
- writes formatted records to the configured sink
@section logging_sink Sink configuration
The default startup form
@code
auto log = udho::logging::start<>();
@endcode
uses the default consumer configuration template argument, which is typically
the rotating file sink configuration.
You may select another sink configuration type when starting logging:
@code
auto log = udho::logging::start<udho::logging::fixed_file>();
@endcode
Available built-in sink helpers include:
- @ref udho::logging::fixed_file
- @ref udho::logging::rotating_file
@section logging_format Output format
The consumer formats reconstructed records into a human-readable log line.
Required fields are printed first.
Optional fields, when present, are appended in a compact tagged form.
Typical output shape:
@code
<line-id> PID(<pid>) TID(0x<thread>) <timestamp> [<severity>] {<subsystem>} <message> <: Key(value) Key(value) :>
@endcode
Exact formatting depends on the active formatter configuration.
@section logging_admin Administrative control
The logger process also exposes a local administrative command socket.
Administrative clients may connect through @ref udho::logging::commander to:
- set a consumer-side Boost.Log filter
- remove the filter
- inspect the current filter
- temporarily enable or disable delivery from the consumer into Boost.Log
Example:
@code
udho::logging::commander cmd("/tmp/udho-log.sock");
auto r = cmd.filter_set("%Severity% >= warning");
if (!r) {
    // inspect r.message
}
@endcode
Administrative filtering operates on the consumer side.
@section logging_priorities Prioritized delivery
The producer can be configured to use severity as message-queue priority.
When prioritized delivery is enabled, higher-severity records may be received
before lower-severity records, so output order may differ from production order.
This improves urgency handling, but the resulting log file may no longer be in
strict production order.
@section logging_threading Threading model
The intended design is:
- many concurrent producer threads or processes
- one consumer process
- one consumer thread
Application code may log concurrently from multiple threads.
@section logging_caveats Important caveats
- Keep the RAII logger handle alive for the full lifetime of logging.
- Declare the logger handle before objects whose destructors may log.
- A return value of zero from a logging macro means no immediate IPC transfer
  happened during that call; it does not necessarily imply permanent loss.
- If prioritized delivery is enabled, receive order may differ from production order.
- Shutdown behavior depends on the implementation policy in the current build;
  application code should not assume that every queued record has already reached
  the final sink unless the shutdown path is explicitly documented as a full flush.
@section logging_example_full Full example
@code
#include <chrono>
#include <udho/logging/setup.h>
#include <udho/logging/macros.h>
#include <udho/logging/params.h>
int main() {
    auto log = udho::logging::start<>();
    UDHO_LOG_INFO("app", "Application started");
    UDHO_LOG_INFO(
        "http",
        "Request completed",
        udho::logging::params::request_id("req-42"),
        udho::logging::params::status_code(200),
        udho::logging::params::bytes_sent(1024),
        udho::logging::params::latency(std::chrono::milliseconds(8))
    );
    UDHO_LOG_WARNING("cache", "Cache miss rate increased");
    UDHO_LOG_ERROR("db", "Failed to execute query");
    return 0;
}
@endcode
@section logging_summary Summary
For normal application code:
- start logging once near the beginning of @c main
- keep the returned RAII handle alive
- use @ref UDHO_LOG_INFO and friends to emit records
- attach typed optional fields when useful
- let the RAII handle stop logging automatically during shutdown