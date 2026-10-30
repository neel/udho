#ifndef UDHO_CLI_CONTROL_H
#define UDHO_CLI_CONTROL_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/program_options.hpp>
#include <udho/cli/commands/log.h>
#include <udho/logging/commander.h>
#include <udho/logging/setup.h>
#include <udho/utils/filesystem.h>

namespace udho{
namespace cli{

template <typename FrameworkT, typename StoreT, typename UrlsT>
class server;

namespace defaults{

static const std::string host = "0.0.0.0";
static const unsigned short port = 9999;
static const int threads = 0;
static const std::string log = "fixed";
static const udho::utils::filesystem::path log_dir = "/tmp";
static const udho::utils::filesystem::path log_sock_dir = "/tmp";

}

class control{
public:
    struct logger_holder_base {
        virtual ~logger_holder_base() = default;
        virtual pid_t pid() const = 0;
        virtual bool running() const = 0;
        virtual const std::string& ipc_mq_name() const = 0;
        virtual const std::string& cmd_socket_path() const = 0;
    };

private:
    struct fixed_log_file {
        using description = udho::logging::fixed_file::description;
        inline static std::string path;

        static description apply() { return udho::logging::fixed_file::apply(path); }
    };

    struct rotating_log_file {
        using description = udho::logging::rotating_file::description;
        inline static std::string prefix;

        static description apply() { return udho::logging::rotating_file::apply(prefix); }
    };

    template <typename ConsumerConfigurationT>
    struct logger_holder: logger_holder_base {
        logger_holder(std::string ipc_mq_name, std::string cmd_socket_path): _ipc_mq_name(std::move(ipc_mq_name)), _cmd_socket_path(std::move(cmd_socket_path)), value(udho::logging::start<ConsumerConfigurationT>(_ipc_mq_name.c_str(), _cmd_socket_path.c_str())) {}

        pid_t pid() const override { return value.pid(); }
        bool running() const override { return value.running(); }
        const std::string& ipc_mq_name() const override { return _ipc_mq_name; }
        const std::string& cmd_socket_path() const override { return _cmd_socket_path; }

        std::string _ipc_mq_name;
        std::string _cmd_socket_path;
        udho::logging::logger<ConsumerConfigurationT> value;
    };

public:
    control(): _description("Command-line options"), _host(defaults::host), _port(defaults::port), _threads(defaults::threads), _log(defaults::log), _log_dir(defaults::log_dir), _log_sock_dir(defaults::log_sock_dir), _process_name("process"), _argc(0), _argv(nullptr), _admin(false){
        namespace options = boost::program_options;

        auto add = _description.add_options();
        add("host,H", options::value<std::string>(&_host)->default_value(defaults::host), "Host");
        add("port,P", options::value<unsigned short>(&_port)->default_value(defaults::port), "Port");
        add("threads,T", options::value<int>(&_threads)->default_value(defaults::threads), "Number of threads");
        add("log,L", options::value<std::string>(&_log)->default_value(defaults::log), "Logging mode: fixed or rotating");
        add("log-dir", options::value<udho::utils::filesystem::path>(&_log_dir)->default_value(defaults::log_dir, defaults::log_dir.string()), "Log output directory");
        add("log-sock-dir", options::value<udho::utils::filesystem::path>(&_log_sock_dir)->default_value(defaults::log_sock_dir, defaults::log_sock_dir.string()), "Logging command socket directory");
        add("help,h", options::bool_switch(), "Show command-line help");
    }

    control(int argc, char** argv): control(){
        parse(argc, argv);
    }

    template <typename FrameworkT, typename StoreT, typename UrlsT, typename... Components>
    server<FrameworkT, StoreT, std::decay_t<UrlsT>> prepare(StoreT& store, UrlsT&& urls, Components&&... components) & {
        using server_type = server<FrameworkT, StoreT, std::decay_t<UrlsT>>;
        return server_type(*this, store, std::forward<UrlsT>(urls), std::forward<Components>(components)...);
    }

    template <typename T>
    control& option(const char* name, T& target, const char* description){
        _description.add_options()(name, boost::program_options::value<T>(&target), description);
        return *this;
    }

    template <typename T>
    control& option(const char* name, T& target, const T& default_value, const char* description){
        _description.add_options()(name, boost::program_options::value<T>(&target)->default_value(default_value), description);
        return *this;
    }

    control& flag(const char* name, bool& target, const char* description){
        _description.add_options()(name, boost::program_options::bool_switch(&target), description);
        return *this;
    }

    control& parse(int argc, char** argv) {
        namespace options = boost::program_options;

        if(argc > 0 && argv && argv[0]) _process_name = udho::utils::filesystem::path(argv[0]).filename().string();
        _argc = argc;
        _argv = argv;
        _admin = argc > 1 && argv && argv[1] && std::string(argv[1]) == "log";
        if(_admin) return *this;
        _variables.clear();
        options::store(options::parse_command_line(argc, argv, _description), _variables);
        if(_variables.count("help") && _variables["help"].as<bool>()) {
            show_help();
            std::exit(EXIT_SUCCESS);
        }
        options::notify(_variables);
        start_logger();
        return *this;
    }

    const std::string& host() const { return _host; }
    unsigned short port() const { return _port; }
    int threads(bool actual = false) const { return actual && _threads == 0 ? hardware_threads() : _threads; }
    const udho::utils::filesystem::path& log_dir() const { return _log_dir; }
    const udho::utils::filesystem::path& log_sock_dir() const { return _log_sock_dir; }
    bool logging() const { return static_cast<bool>(_logger); }
    bool admin() const { return _admin; }

    int operator()() {
        if(!admin()) throw std::logic_error("No administrative command was selected");
        udho::cli::commands::log command(_process_name, defaults::log_sock_dir);
        return command.run(_argc - 2, _argv + 2);
    }

    const logger_holder_base& logger() const {
        if(!logging()) throw std::runtime_error("Logging is not active");
        return *_logger;
    }

    boost::asio::ip::tcp::endpoint ipv4() const {
        return boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(_host), _port);
    }

    boost::asio::ip::tcp::endpoint ipv6() const {
        return boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v6(_host), _port);
    }

    boost::asio::local::stream_protocol::endpoint domain() const {
        return boost::asio::local::stream_protocol::endpoint(_host);
    }

private:
    template <typename FrameworkT, typename StoreT, typename UrlsT>
    friend class server;

    void run(boost::asio::io_context& io) {
        int threads = (_threads == 0 ? hardware_threads() : _threads);
        std::vector<std::thread> pool;
        auto worker = [&io]()  {
            io.run();
        };

        for(int index = 0; index < threads; ++index) {
            pool.emplace_back(worker);
        }

        for(auto& thread: pool) {
            thread.join();
        }

        stop_logger();
    }

    void stop_logger() {
        if(!_logger) return;
        std::cout << " ● " << "Stopping logger \t";
        std::cout.flush();
        _logger.reset();
        std::cout << "done" << std::endl;
    }

    void start_logger() {
        if(_log != "fixed" && _log != "rotating") throw std::invalid_argument("Unknown logging mode: " + _log);

        ensure_directory(_log_dir, "log-dir");
        ensure_directory(_log_sock_dir, "log-sock-dir");

        std::string identifier = "udho-" + _process_name + "-" + std::to_string(::getpid());
        std::string ipc_mq_name = identifier;
        std::string cmd_socket_path = (_log_sock_dir / (identifier + ".sock")).string();

        _logger.reset();
        if(_log == "fixed") {
            fixed_log_file::path = (_log_dir / (identifier + ".log")).string();
            _logger.reset(new logger_holder<fixed_log_file>(std::move(ipc_mq_name), std::move(cmd_socket_path)));
        } else {
            rotating_log_file::prefix = (_log_dir / identifier).string();
            _logger.reset(new logger_holder<rotating_log_file>(std::move(ipc_mq_name), std::move(cmd_socket_path)));
        }

        show_logger();
    }

    void show_logger() const {
        udho::logging::commander commander(_logger->cmd_socket_path());
        udho::logging::result keys = commander.sink();
        std::cout << " ● " << "Logging started" << std::endl;
        if(!keys) {
            std::cout << "    Error: " << keys.message << std::endl;
            return;
        }

        std::istringstream stream(keys.message);
        std::string key;
        while(std::getline(stream, key)) {
            if(key.empty()) continue;
            udho::logging::result value = commander.sink(key);
            std::cout << "    " << key << ": " << (value ? value.message : "Error: " + value.message) << std::endl;
        }
    }

    static void ensure_directory(const udho::utils::filesystem::path& directory, const char* option) {
        if(udho::utils::filesystem::exists(directory)) {
            if(!udho::utils::filesystem::is_directory(directory))
                throw std::invalid_argument(std::string("--") + option + " is not a directory: " + directory.string());
        }
        udho::utils::filesystem::create_directories(directory);
    }

    static int hardware_threads() {
        unsigned int threads = std::thread::hardware_concurrency();
        return threads == 0 ? 1 : static_cast<int>(threads);
    }

    void show_help() const {
        std::cout << _description << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  log  Control the logging consumer" << std::endl;
    }

    boost::program_options::options_description _description;
    boost::program_options::variables_map _variables;
    std::string _host;
    unsigned short _port;
    int _threads;
    std::string _log;
    udho::utils::filesystem::path _log_dir;
    udho::utils::filesystem::path _log_sock_dir;
    std::string _process_name;
    int _argc;
    char** _argv;
    bool _admin;
    std::unique_ptr<logger_holder_base> _logger;
};

}
}

#endif // UDHO_CLI_CONTROL_H
