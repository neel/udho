#ifndef UDHO_CLI_COMMANDS_LOG_H
#define UDHO_CLI_COMMANDS_LOG_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <sys/types.h>

#include <boost/program_options.hpp>
#include <udho/logging/commander.h>
#include <udho/utils/filesystem.h>

namespace udho{
namespace cli{
namespace commands{

class log{
public:
    log(std::string process_name, udho::utils::filesystem::path log_sock_dir): _process_name(std::move(process_name)), _log_sock_dir(std::move(log_sock_dir)), _pid(0) {
    }

    int run(int argc, char** argv) {
        namespace options = boost::program_options;

        bool help = false;
        std::vector<std::string> arguments;
        for(int index = 0; index < argc; ++index) arguments.emplace_back(argv[index]);

        options::options_description visible("Log command options");
        auto add = visible.add_options();
        add("pid,p", options::value<pid_t>(&_pid), "Server process ID");
        add("socket,s", options::value<std::string>(&_socket), "Logging consumer socket");
        add("log-sock-dir", options::value<udho::utils::filesystem::path>(&_log_sock_dir)->default_value(_log_sock_dir, _log_sock_dir.string()), "Logging command socket directory");
        add("help,h", options::bool_switch(&help), "Show log command help");

        options::options_description hidden;
        hidden.add_options()("command", options::value<std::vector<std::string>>(&_command), "Logging command");

        options::options_description all;
        all.add(visible).add(hidden);

        options::positional_options_description positional;
        positional.add("command", -1);

        options::variables_map variables;
        try {
            options::store(options::command_line_parser(arguments).options(all).positional(positional).run(), variables);
            options::notify(variables);
        } catch(const options::error& error) {
            std::cerr << "Error: " << error.what() << std::endl;
            show_help(visible);
            return 2;
        }

        if(help) {
            show_help(visible);
            return EXIT_SUCCESS;
        }

        bool has_pid = variables.count("pid") != 0;
        bool has_socket = variables.count("socket") != 0;
        if(has_pid == has_socket) return usage_error("Provide exactly one of --pid or --socket", visible);
        if(has_pid && _pid <= 0) return usage_error("PID must be greater than zero", visible);
        if(has_socket && _socket.empty()) return usage_error("Socket path must not be empty", visible);
        if(_command.empty()) return usage_error("A logging command is required", visible);

        std::string socket_path = has_socket ? _socket : (_log_sock_dir / ("udho-" + _process_name + "-" + std::to_string(_pid) + ".sock")).string();
        udho::logging::commander commander(std::move(socket_path));
        return dispatch(commander, visible);
    }

private:
    int dispatch(udho::logging::commander& commander, const boost::program_options::options_description& options) const {
        if(_command[0] == "filter") {
            if(_command.size() == 2 && _command[1] == "show") return report(commander.filter_show());
            if(_command.size() == 2 && _command[1] == "unset") return report(commander.filter_unset());
            if(_command.size() >= 3 && _command[1] == "set") return report(commander.filter_set(join(2)));
            return usage_error("Expected filter set <expression>, filter unset, or filter show", options);
        }
        if(_command.size() == 1 && _command[0] == "enable") return report(commander.temporary_enable());
        if(_command.size() == 1 && _command[0] == "disable") return report(commander.temporary_disable());
        if(_command[0] == "sink") {
            if(_command.size() == 1) return report(commander.sink());
            if(_command.size() == 2) return report(commander.sink(_command[1]));
            return usage_error("Expected sink or sink <property>", options);
        }
        return usage_error("Unknown logging command: " + _command[0], options);
    }

    std::string join(std::size_t offset) const {
        std::string value = _command[offset];
        for(std::size_t index = offset + 1; index < _command.size(); ++index) value += " " + _command[index];
        return value;
    }

    int report(const udho::logging::result& result) const {
        if(result) {
            if(!result.message.empty()) std::cout << result.message << std::endl;
            return EXIT_SUCCESS;
        }
        std::cerr << "Error: " << result.message << std::endl;
        return EXIT_FAILURE;
    }

    int usage_error(const std::string& message, const boost::program_options::options_description& options) const {
        std::cerr << "Error: " << message << std::endl;
        show_help(options);
        return 2;
    }

    void show_help(const boost::program_options::options_description& options) const {
        std::cout << "Usage: " << _process_name << " log [options] command [arguments...]" << std::endl;
        std::cout << options << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  filter set <expression>  Apply a logging filter" << std::endl;
        std::cout << "  filter unset             Remove the logging filter" << std::endl;
        std::cout << "  filter show              Show the current logging filter" << std::endl;
        std::cout << "  enable                   Enable logging temporarily" << std::endl;
        std::cout << "  disable                  Disable logging temporarily" << std::endl;
        std::cout << "  sink [property]          List or read sink properties" << std::endl;
    }

    std::string _process_name;
    udho::utils::filesystem::path _log_sock_dir;
    pid_t _pid;
    std::string _socket;
    std::vector<std::string> _command;
};

}
}
}

#endif // UDHO_CLI_COMMANDS_LOG_H
