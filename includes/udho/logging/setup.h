#ifndef UDHO_LOGGING_SETUP_H
#define UDHO_LOGGING_SETUP_H

#include <string>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <udho/logging/producer.h>
#include <udho/logging/consumer.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <atomic>
// #include <boost/log/utility/manipulators/to_log.hpp>

namespace boost::log{
inline formatting_ostream& operator<<(formatting_ostream& stream, const std::chrono::system_clock::time_point& tp ) {
    stream << udho::utils::date_time::format_rfc7231(tp);
    return stream;
}
}

namespace udho{
namespace logging{


struct rotating_file{
    static void apply() {
        namespace keywords    = boost::log::keywords;
        namespace expressions = boost::log::expressions;

        boost::log::add_file_log(
            keywords::file_name             = "server_%Y-%m-%d_%H-%M-%S.log",
            keywords::rotation_size         = 10 * 1024 * 1024,
            keywords::time_based_rotation   = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format = (
                expressions::stream
                << expressions::attr<std::uint64_t>("LocalID") << ":" << expressions::attr<std::uint32_t>("ProcessID") << ":" << std::hex << expressions::attr<std::size_t>("ThreadID") << " " << std::dec
                << "[" << expressions::attr<std::chrono::system_clock::time_point>("TimeStamp")                     << "] "
                << "[" << expressions::attr<std::underlying_type_t<udho::logging::severity>>("Severity")            << "] "
                << "[" << expressions::attr<std::string>("Subsystem")                                               << "] "
                << expressions::attr<std::string>("Message")
            )
        );

        boost::log::add_common_attributes();
    }
};

template <typename ConsumerConfigurationT = rotating_file>
struct setup{
    static pid_t apply(const char* name = 0x0) {
        if(_pid != -1) return _pid;

        auto queue = udho::logging::detail::ipc_queue::create(name);
        producer::activate(name);

        // int std_fds[2] = {-1, -1};
        // if (pipe(std_fds) == -1) {
        //     perror("pipe stdout");
        // }

        int io_fds[2]  = {-1, -1};
        if (pipe(io_fds) == -1) {
            perror("pipe io");
        }

        // int std_out, std_in;
        // std_out = std_fds[0];
        // std_in  = std_fds[1];
        int io_out, io_in;
        io_out  = io_fds[0];
        io_in   = io_fds[1];

        _pid = fork();
        if (_pid == 0) {
            // close(std_out);
            // dup2(std_in, STDOUT_FILENO);
            // dup2(std_in, STDERR_FILENO);
            // close(std_in);
            close(io_out);

            run_child(io_in);
            return 0;
        } else if (_pid > 0) {
            // close(std_in);
            close(io_in);

            char ok = 0;
            ssize_t n = read(io_out, &ok, 1);
            close(io_out);

            return _pid;
        } else {
            close(io_out);
            close(io_in);
            // close(std_out);
            // close(std_in);
            return _pid;
        }
    }

    static void stop() {
        if(_pid <= 0) return;
        producer::deactivate();

        kill(_pid, SIGTERM);
        int status = -1;
        waitpid(_pid, &status, 0);
        _pid = -1;
    }

    static bool running() {
        if(_pid <= 0) return false;

        int status = -1;
        pid_t result = waitpid(_pid, &status, WNOHANG);
        return (result == 0);
    }

private:
    static void run_child(int fd) {
        struct sigaction sa;
        sa.sa_handler = &setup<ConsumerConfigurationT>::callback;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT,  &sa, nullptr);

        assert(!_should_stop);
        ConsumerConfigurationT::apply();
        udho::logging::consumer consumer;

        char ok = 1;
        if (write(fd, &ok, 1) != 1) {
            std::exit(1);
        }

        consumer.consume(_should_stop);
        boost::log::core::get()->flush();
        boost::log::core::get()->remove_all_sinks();
    }

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

template <typename ConsumerConfigurationT = rotating_file>
pid_t init(const char* name = 0x0) {
    return setup<ConsumerConfigurationT>::apply(name);
}

}
}

#endif // UDHO_LOGGING_SETUP_H
