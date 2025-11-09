#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void executeCommand(std::vector<std::string> args) {
    pid_t pid = fork();

    if (pid == 0) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == ">") {
                int fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args.erase(args.begin() + i, args.begin() + i + 2);
                break;
            }
            else if (args[i] == "<") {
                int fd = open(args[i + 1].c_str(), O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
                args.erase(args.begin() + i, args.begin() + i + 2);
                break;
            }
        }

        char *argv[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char *>(args[i].c_str());
        }
        argv[args.size()] = nullptr;

        if (execvp(argv[0], argv) == -1) {
            perror("Error executing command");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Fork failed");
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void handlePipe(std::vector<std::string> commands) {
    int pipeFD[2];
    pid_t pid1, pid2;

    pipe(pipeFD);

    if ((pid1 = fork()) == 0) { 
        dup2(pipeFD[1], STDOUT_FILENO);
        close(pipeFD[1]);
        close(pipeFD[0]);

        std::vector<std::string> args1 = {commands.begin(), commands.begin() + commands.size()/2};
        executeCommand(args1);
        exit(0);
    }

    if ((pid2 = fork()) == 0) {
        dup2(pipeFD[0], STDIN_FILENO);
        close(pipeFD[1]);
        close(pipeFD[0]);

        std::vector<std::string> args2 = {commands.begin() + commands.size()/2, commands.end()};
        executeCommand(args2);
        exit(0);
    }

    close(pipeFD[0]);
    close(pipeFD[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
}

int main() {
    std::string command;

    while (true) {
        std::cout << "Custom Shell> ";
        std::getline(std::cin, command);
        
        if (command == "exit") {
            break;
        }

        std::istringstream iss(command);
        std::vector<std::string> commands;
        std::string cmd;

        while (getline(iss, cmd, '|')) {
            commands.push_back(cmd);
        }

        if (commands.size() == 1) {
            std::istringstream argStream(commands[0]);
            std::vector<std::string> args;
            std::string arg;
            while (argStream >> arg) {
                args.push_back(arg);
            }
            executeCommand(args);
        } else {
            handlePipe(commands);
        }
    }

    return 0;
}
