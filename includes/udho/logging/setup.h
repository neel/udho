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

namespace udho{
namespace logging{


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

template <typename ConsumerConfigurationT = rotating_file>
struct setup{
    static pid_t apply(const char* ipc_mq_name = 0x0, const char* cmd_socket_path = "/tmp/udho-logger.sock") {
        if(_pid != -1) return _pid;

        int io_fds[2]  = {-1, -1};
        if (::pipe(io_fds) == -1) {
            ::perror("pipe io");
        }

        int io_out, io_in;
        io_out  = io_fds[0];
        io_in   = io_fds[1];

        int cmd_fds[2]  = {-1, -1};
        if (::pipe2(cmd_fds, O_NONBLOCK) == -1) {
            ::perror("pipe cmd");
        }

        auto queue = udho::logging::detail::ipc_queue::create(ipc_mq_name);
        producer::activate(ipc_mq_name);

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

            run_child(io_in, cmd_socket_path, ipc_mq_name);
            return 0;
        } else if (_pid > 0) {
            ::close(io_in);

            char ok = 0;
            ssize_t n = read(io_out, &ok, 1);
            ::close(io_out);

            UDHO_LOG_INFO("logging", "Logger started");

            return _pid;
        } else {
            ::close(io_out);
            ::close(io_in);

            return _pid;
        }
    }

    static void stop() {
        if(_pid <= 0) return;
        UDHO_LOG_INFO("logging", "Logger stopping");
        producer::deactivate();

        ::kill(_pid, SIGTERM);
        int status = -1;
        ::waitpid(_pid, &status, 0);
        _pid = -1;
    }

    static bool running() {
        if(_pid <= 0) return false;

        int status = -1;
        pid_t result = ::waitpid(_pid, &status, WNOHANG);
        return (result == 0);
    }

private:
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
