#ifndef UDHO_LOGGING_SETUP_H
#define UDHO_LOGGING_SETUP_H

#include <string>
#include <unordered_set>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <udho/logging/producer.h>
#include <udho/logging/consumer.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <atomic>
#include <udho/logging/macros.h>
#include <udho/logging/formatter.h>
#include <boost/config.hpp>

namespace udho{
namespace logging{

/** @addtogroup DoxyG_logging
 *  @{
 */

struct fixed_file {
    static auto apply(const std::string& path = "server.log") {
        auto sink = boost::log::add_file_log(
            boost::log::keywords::file_name  = path,
            boost::log::keywords::open_mode  = std::ios_base::out | std::ios_base::trunc,
            boost::log::keywords::auto_flush = true
        );

        sink->set_formatter(udho::logging::formatter{});
        return sink;
    }
};

struct rotating_file{
    static auto apply(const std::string& prefix = "server") {
        auto sink = boost::log::add_file_log(
            boost::log::keywords::file_name           = (prefix+"_%Y-%m-%d_%H-%M-%S.log"),
            boost::log::keywords::rotation_size       = 10 * 1024 * 1024,
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0)
        );

        sink->set_formatter(udho::logging::formatter{});
        return sink;
    }
};

/**
 * @brief Process-level controller for the logger child and producer activation.
 *
 * This class owns the global logging runtime for the current process:
 * - creates the IPC message queue used by producers and the consumer
 * - forks the logger child process
 * - starts the consumer loop in the child
 * - activates producer-side access in the parent
 * - stops the logger child and deactivates the producer during shutdown
 *
 * The class is intentionally static-only in usage: all state is stored in the
 * class-level members @c _pid and @c _should_stop.
 *
 * @tparam ConsumerConfigurationT Sink configuration applied in the logger child
 *         before entering the consumer loop.
 *
 * Typical usage is through the RAII wrapper returned by @ref start rather than
 * calling @ref apply and @ref stop manually.
 *
 * @note This class manages one logger child per process.
 * @note After successful startup, the parent process returns from @ref apply
 *       while the child process enters the consumer loop.
 */
template <typename ConsumerConfigurationT = rotating_file>
struct setup{

    /**
     * @brief Create the IPC queue, fork the logger child, and activate the producer.
     * @param ipc_mq_name Name of the IPC message queue; @c nullptr selects default.
     * @param cmd_socket_path Filesystem path of the consumer admin socket.
     * @return Child PID on success, or -1 on startup failure.
     *
     * Startup sequence:
     * - creates the IPC queue
     * - forks the logger child
     * - redirects the child stdout/stderr to @c consumer.stdout
     * - constructs the consumer in the child
     * - waits for a one-byte readiness notification from the child
     * - activates producer-side queue access in the parent
     *
     * @note Successful return requires a valid readiness byte from the child.
     */
    static pid_t apply(const char* ipc_mq_name = 0x0, const char* cmd_socket_path = 0x0) {
        if(_pid != -1) return _pid;

        int io_fds[2]  = {-1, -1};
        if (::pipe(io_fds) == -1) {
            ::perror("pipe io");
        }

        int io_out, io_in;
        io_out  = io_fds[0];
        io_in   = io_fds[1];

        auto queue = udho::logging::detail::ipc_queue::create(ipc_mq_name);

        _pid = ::fork();
        if (_pid == 0) {
            ::close(io_out);

            int fd = ::open("consumer.stdout", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd != -1) {
                ::dup2(fd, STDOUT_FILENO);
                ::dup2(fd, STDERR_FILENO);
                if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
                    ::close(fd);
                }
            }

            _should_stop.store(false, std::memory_order_relaxed);
            run_child(io_in, cmd_socket_path, ipc_mq_name);
            return 0;
        } else if (_pid > 0) {
            ::close(io_in);

            char ok = 0;
            ssize_t n = read(io_out, &ok, 1);
            ::close(io_out);

            if(n < 1 || ok != 1) {
                throw std::runtime_error{"Handshake with log consumer process failed"};
            }

            producer::activate(ipc_mq_name);

            UDHO_LOG_INFO("logging", "Logger started");

            return _pid;
        } else {
            ::close(io_out);
            ::close(io_in);

            return _pid;
        }
    }

    /**
     * @brief Stop the logger process.
     *
     * Shutdown sequence:
     * - emits a local "Logger stopping" message
     * - blocks new producer admissions
     * - waits for in-flight producer calls to leave the counted region
     * - sends SIGTERM to the logger child
     * - waits for child termination
     */
    static void stop() {
        if(_pid <= 0) return;
        UDHO_LOG_INFO("logging", "Logger stopping");
        producer::deactivate();

        ::kill(_pid, SIGTERM);
        int status = -1;
        ::waitpid(_pid, &status, 0);
        _pid = -1;
    }

    /**
     * @brief Check whether the logger child process is currently running.
     * @return @c true if the child process is still alive, otherwise @c false.
     *
     * This method performs a non-blocking @c waitpid on the stored child PID.
     *
     * @note The current implementation only reports liveness; it does not yet
     *       clear stale PID state when the child has already exited.
     */
    static bool running() {
        if(_pid <= 0) return false;

        int status = -1;
        pid_t result = ::waitpid(_pid, &status, WNOHANG);
        return (result == 0);
    }

    /**
     * @brief Return the stored PID of the logger child process.
     * @return PID of the logger child, or a non-positive value when inactive.
     *
     * @note This returns the process-wide static logger PID managed by @ref setup.
     */
    static pid_t pid() { return _pid; }

private:

    /**
     * @brief Child-process entry point after a successful fork.
     * @param notify_fd Write end of the startup handshake pipe.
     * @param cmd_socket_path Filesystem path of the consumer admin socket.
     * @param ipc_mq_name IPC message queue name to open in the consumer.
     *
     * This function:
     * - installs signal handlers that request shutdown through @c _should_stop
     * - applies the selected consumer sink configuration
     * - constructs the log consumer
     * - notifies the parent that startup completed
     * - runs the consumer loop until shutdown is requested
     * - flushes and removes all Boost.Log sinks before returning
     *
     * This function is intended to run only in the logger child process.
     */
    static void run_child(int notify_fd, const char* cmd_socket_path, const char* ipc_mq_name) {
        struct sigaction sa;
        sa.sa_handler = &setup<ConsumerConfigurationT>::callback;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT,  &sa, nullptr);

        assert(!_should_stop);
        ConsumerConfigurationT::apply();
        udho::logging::consumer consumer(cmd_socket_path, ipc_mq_name);

        char ok = 1;
        if (::write(notify_fd, &ok, 1) != 1) {
            std::exit(1);
        }

        consumer.consume(_should_stop);
        boost::log::core::get()->flush();
        boost::log::core::get()->remove_all_sinks();
    }

    /**
     * @brief Signal handler used by the logger child to request shutdown.
     * @param signum Signal number received by the child process.
     *
     * The handler does not perform shutdown directly. It only sets the shared
     * stop flag observed by the consumer loop.
     */
    static void callback(int signum) {
        _should_stop = true;
    }


private:
    static pid_t _pid;
    static std::atomic_bool _should_stop;
};

template <typename ConsumerConfigurationT>
pid_t setup<ConsumerConfigurationT>::_pid = -1;

template <typename ConsumerConfigurationT>
std::atomic_bool setup<ConsumerConfigurationT>::_should_stop = false;

/**
 * @brief Owning RAII controller for the logging subsystem.
 *
 * Constructing this object starts the logger process through @ref setup and
 * activates producer-side logging in the parent process. Destroying this object
 * stops the logger process and deactivates producer-side logging.
 *
 * Intended usage:
 * @code
 * int main() {
 *     auto log = udho::logging::start<>();
 *     // application code
 * }
 * @endcode
 *
 * @note Keep this object alive for as long as logging is required.
 * @note If other local objects may emit logs from their destructors, declare
 *       this object before them so that it is destroyed last.
 * @warning This type should have unique ownership semantics and should not be copied.
 */
template <typename ConsumerConfigurationT = rotating_file>
struct BOOST_ATTRIBUTE_NODISCARD logger{
    using setup_type = setup<ConsumerConfigurationT>;

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;

    logger(logger&& other) = delete;
    logger& operator=(logger&& other) = delete;

    /**
     * @brief Start the logging subsystem and take ownership of its lifetime.
     * @param name IPC queue name; @c nullptr selects the default queue name.
     * @param cmd_socket_path Filesystem path of the consumer admin socket.
     *
     * In the parent process this constructor returns normally and the created object
     * owns shutdown responsibility. In the child process the underlying setup path
     * enters the consumer loop and does not continue application execution.
     *
     * @throws std::runtime_error if logger startup handshake fails.
     */
    inline explicit logger(const char* name = 0x0, const char* cmd_socket_path = 0x0): _pid(0) {
        _pid = setup_type::apply(name, cmd_socket_path);
        if(0 == _pid){
            ::exit(0);
        }
    }

    /**
     * @brief Check whether the logger child process is still running.
     * @return @c true if the logger child is alive, otherwise @c false.
     *
     * This forwards to the shared @ref setup state.
     */
    bool running() const { return setup_type::running(); }

    /**
     * @brief Return the logger child PID.
     * @return PID of the logger child process, or a non-positive value if inactive.
     *
     * Prefer querying the shared @ref setup state rather than relying on a cached
     * construction-time value.
     */
    pid_t pid() const { return _pid; }

    /**
     * @brief Stop the logging subsystem if this object still owns it.
     *
     * This destructor is intended to provide automatic shutdown during scope exit,
     * including normal return from @c main.
     */
    ~logger() {
        if(_pid) {
            setup_type::stop();
        }
    }
private:
    pid_t _pid;
};

/**
 * @brief Start logging and return an owning RAII handle.
 * @param name IPC queue name; @c nullptr selects the default queue name.
 * @param cmd_socket_path Filesystem path of the consumer admin socket.
 * @return Owning logger handle.
 *
 * The returned object must be kept alive for as long as logging is desired.
 * Destroying it stops the logger process.
 *
 * @warning Discarding the returned object immediately will also stop logging
 *          immediately at the end of the full expression.
 */
template <typename ConsumerConfigurationT = rotating_file>
BOOST_ATTRIBUTE_NODISCARD logger<ConsumerConfigurationT> start(const char* name = 0x0, const char* cmd_socket_path = 0x0) {
    return logger<ConsumerConfigurationT>(name, cmd_socket_path);
}

/** @} */

}
}

#endif // UDHO_LOGGING_SETUP_H
