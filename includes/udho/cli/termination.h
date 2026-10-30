#ifndef UDHO_CLI_TERMINATION_H
#define UDHO_CLI_TERMINATION_H

#include <csignal>
#include <utility>
#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

namespace udho{
namespace cli{

class termination{
public:
    explicit termination(boost::asio::io_context& io): _signals(io, SIGINT, SIGTERM) { }

    termination(const termination&) = delete;
    termination& operator=(const termination&) = delete;
    termination(termination&&) = delete;
    termination& operator=(termination&&) = delete;

    template <typename Function>
    void async_wait(Function&& function) {
        _signals.async_wait([function = std::forward<Function>(function)](const boost::system::error_code& error, int signal_number) mutable {
             if(!error) { 
                std::cout << "\r**Interrupted**";
                std::cout.flush();
                function(signal_number);
             } 
        });
    }

private:
    boost::asio::signal_set _signals;
};

}
}

#endif // UDHO_CLI_TERMINATION_H
