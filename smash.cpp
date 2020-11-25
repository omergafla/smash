#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[])
{
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction signalAction;
    signalAction.sa_handler = alarmHandler;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART; // SA_RESTART;
    sigaction(SIGALRM, &signalAction, NULL);

    //TODO: setup sig alarm handler

    SmallShell &smash = SmallShell::getInstance();
    while (smash.alive)
    {
        std::cout << smash.promptName << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
