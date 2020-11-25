#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/stat.h>

// chen123

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() \
    cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
    cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define BUFFER_SIZE 4096
#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
    execvp((path), (arg));

string _ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
    return _rtrim(_ltrim(s));
}
int _parseCommandLineChar(const char *cmd_line, char **args)
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args[i] = (char *)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}
int _parseCommandLine(const string cmd_line, vector<string> *args)
{
    //FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(cmd_line);
    for (std::string s; iss >> s;)
    {
        args->push_back(s);
        i++;
    }
    return i;

    //FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&')
    {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void ShowPidCommand::execute()
{
    cout << "smash pid is " << SmallShell::getInstance().smash_pid << endl;
}

#pragma region JobsList

JobsList::~JobsList()
{
    for (auto it = this->job_list->begin(); it != this->job_list->end();)
    {
        this->job_list->erase(it++);
        ++it;
    }
    delete this->job_list;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped)
{
    JobEntry *job_entry = new JobEntry();
    JobEntry *temp = getJobByProcessId(pid);

    if (temp == nullptr)
    {
        vector<string> *args = new vector<string>();
        _parseCommandLine(cmd->cmd_line, args);
        job_entry->job_id = this->getMaximalJobId() + 1;
        job_entry->command = cmd->cmd_line;
        job_entry->insertion_time = time(NULL);
        job_entry->process_id = pid;
        job_entry->stopped = isStopped;
        pair<int, JobEntry *> _pair = make_pair(job_entry->job_id, job_entry);
        this->job_list->insert(_pair);
    }
    else
    {
        temp->insertion_time = time(NULL);
        temp->stopped = isStopped;
        delete job_entry;
    }
}

int JobsList::getMaximalJobId()
{
    int max = 0;
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        if (it->first > max)
        {
            max = it->first;
        }
        it++;
    }
    return max;
}

void JobsList::printJobsList()
{
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    { //example of command: sleep 100&
        cout << "[" << it->second->job_id << "] " << it->second->command << " : "
             << it->second->process_id << " " << difftime(time(nullptr), it->second->insertion_time)
             << " secs ";
        if (it->second->stopped)
        {
            cout << "(stopped)";
        }
        cout << endl;
        it++;
    }
}

void JobsList::printJobsListForQuit()
{
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    { //example of command: sleep 100&
        cout << it->second->process_id << ": " << it->second->command << endl;
        it++;
    }
}

void JobsList::killAllJobs()
{
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        kill(it->second->process_id, SIGKILL);
        it++;
    }
}

void JobsList::setFinished(int jobId)
{
    this->getJobById(jobId)->finished = true;
}

void JobsList::removeFinishedJobs()
{
    for (auto it = this->job_list->begin(); it != this->job_list->end();)
    {
        // if (it->second->finished == true)
        if (waitpid(it->second->process_id, NULL, WNOHANG) != 0)
        {
            this->job_list->erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
    auto it = this->job_list->find(jobId);
    if (it == this->job_list->end())
    {
        return nullptr;
    }
    return it->second;
}

void JobsList::removeJobById(int jobId)
{
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        if (this->isEmpty())
        {
            break;
        }
        if (it->first == jobId)
        {
            this->job_list->erase(it);
        }
    }
}

void JobsList::removeJobByProcessIdid(int processId)
{
    JobEntry *entry = this->getJobByProcessId(processId);
    if (entry != nullptr)
    {
        this->removeJobById(entry->job_id);
    }
}

JobsList::JobEntry *JobsList::getJobByProcessId(int processId)
{
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        if (it->second->process_id == processId)
        {
            return it->second;
        }
        it++;
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId)
{

    JobEntry *job;
    job = getJobById(*lastJobId);
    return job;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId)

{
    JobEntry *job;
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        if (it->first == *jobId && it->second->stopped == true)
        {
            job = it->second;
            return job;
        }
        it++;
    }
    return nullptr;
}

#pragma endregion

#pragma region JobsCommand

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs)
{
    this->cmd_line = cmd_line;
    this->job_list_ref = jobs;
}

void JobsCommand::execute()
{
    bool is_exited;
    bool is_stopped;
    bool is_continued;
    int status = -1;
    pid_t returned_pid;
    auto it = this->job_list_ref->job_list->begin();
    while (it != this->job_list_ref->job_list->end())
    {
        returned_pid = waitpid(it->second->process_id, &status, WCONTINUED | WUNTRACED | WNOHANG);
        is_exited = WIFEXITED(status);
        is_stopped = WIFSTOPPED(status);
        is_continued = WIFCONTINUED(status);
        if (is_exited)
        {
            it->second->finished = true;
        }
        if (is_continued)
        {
            it->second->stopped = false;
        }
        if (is_stopped)
        {
            it->second->stopped = true;
        }
        it++;
    }
    this->job_list_ref->removeFinishedJobs();
    this->job_list_ref->printJobsList();
}
#pragma endregion

#pragma region ChangeDirCommand

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string plastPwd)
{
    this->cmd_line = cmd_line;
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(cmd_line, args);
    if (result < 2)
    {
        this->path = "";
    }
    else
    {

        this->path = args->at(1);
    }

    if (result == 2 && args->at(1) == "-" && plastPwd.empty())
    {
        this->is_error = true;
        this->err_message = "smash error: cd: OLDPWD not set";
    }
    else if (result > 2)
    {
        this->is_error = true;
        this->err_message = "smash error: cd: too many arguments";
    }
    else if (result == 2 && args->at(1) == "-")
    {
        this->path = plastPwd;
    }
    delete args;
}

void ChangeDirCommand::execute()
{
    if (this->is_error == true)
    {
        cout << this->err_message << endl;
    }
    else
    {
        int result = chdir(path.c_str());
        if (result == -1)
        {
            perror("smash error: chdir failed");
        }
    }
}

#pragma endregion

#pragma region KillCommand

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)
{
    job_list_ref = jobs;
    this->cmd_line = cmd_line;
}

void KillCommand::execute()
{
    int job_id;
    int signum;
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(this->cmd_line, args);
    try //If no conversion could be performed
    {
        job_id = stoi(args->at(2));
        signum = stoi(args->at(1));
    }
    catch (invalid_argument)
    {
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }
    // If sig_num is not negative
    if (args->at(1).at(0) != '-')
    {
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry *job = this->job_list_ref->getJobById(job_id);
    //If job_id doesn't exist in the job_list
    if (job == nullptr)
    {
        cout << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        return;
    }
    pid_t pid = this->job_list_ref->getJobById(job_id)->process_id;
    int res = kill(pid, abs(signum));
    if (res == -1)
    {
        perror("smash error: kill failed");
    }
    else
    {
        cout << "signal number " << abs(signum) << " was sent to pid " << pid << endl;
    }
}
#pragma endregion

#pragma region QuitCommand

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs, bool kill)
{
    this->cmd_line = cmd_line;
    this->job_list_ref = jobs;
    this->if_kill = kill;
}

void QuitCommand::execute()
{
    if (this->if_kill == true)
    {
        cout << "smash: sending SIGKILL signal to " << this->job_list_ref->job_list->size() << " jobs:" << endl;
        this->job_list_ref->removeFinishedJobs();
        this->job_list_ref->printJobsListForQuit();
        this->job_list_ref->killAllJobs();
    }
    SmallShell::getInstance().alive = false;
}

#pragma endregion

#pragma region RedirectionCommand

RedirectionCommand::RedirectionCommand(const char *cmd_line)
{
    this->cmd_line = cmd_line;
}

void deleteLastCellsOfArray(char **args_char, int res)
{
    int flag = 0;
    for (int i = 0; i < res; i++)
    {
        if (flag == 1)
        {
            args_char[i] = NULL;
            continue;
        }
        if (strcmp(args_char[i], ">") == 0 || strcmp(args_char[i], ">>") == 0)
        {
            flag = 1;
            args_char[i] = NULL;
        }
    }
}

void remove_spaces(char *s)
{
    const char *d = s;
    do
    {
        while (*d == ' ')
        {
            ++d;
        }
    } while (*s++ = *d++);
}

void RedirectionCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    string str(this->cmd_line);
    int savestdout;
    char *no_space_name = new char[str.size()];
    const char *name = new char[str.size()];
    char **args_char = new char *[COMMAND_MAX_ARGS];
    int res = _parseCommandLineChar(cmd_line, args_char);
    int fd;
    int pos;
    if (smash.append)
    {
        pos = str.find(">>");
        name = (str.substr(pos + 2)).c_str();
        str = str.substr(0, pos);
    }
    else
    {
        pos = str.find(">");
        name = (str.substr(pos + 1)).c_str();
        str = str.substr(0, pos);
        //strcpy(name, str.substr(pos + 1).c_str());
    }
    strcpy(no_space_name, name);
    remove_spaces(no_space_name);
    //Deal with no spaces around > or >> : as in date>temp.txt, becuase in real shell it works.
    Command *cmd = smash.CreateCommand(str.c_str());
    if (smash.append)
        fd = open(no_space_name, O_CREAT | O_APPEND | O_WRONLY, 0666);
    else
        fd = open(no_space_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd == -1)
    {
        perror("smash error: open failed");
    }
    savestdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    if (close(fd) == -1)
    {
        perror("smash error: close failed");
    };
    cmd->execute();
    if (close(STDOUT_FILENO))
    {
        perror("smash error: close failed");
    }
    dup2(savestdout, STDOUT_FILENO);
    if (close(savestdout) == -1)
    {
        perror("smash error: close failed");
    }
    smash.append = false;
    //cout << "Hi"<< endl;
}

#pragma endregion

#pragma region PipeCommand

bool CheckBackground(const char *command)
{
    bool bg = false;
    string argument(command);
    if (argument.at(argument.size() - 1) == '&')
        return true;
    else
        return false;
}

PipeCommand::PipeCommand(const char *cmd_line)
{
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(cmd_line, args);
    this->cmd_line = cmd_line;
    if (std::count(args->begin(), args->end(), "|&") != 0)
    {
        this->is_amper = true;
    }
    else
        this->is_amper = false;
}

string prepare(string str)
{
    int pos = str.find_last_of("&");
    if (pos != -1)
    {
        str = str.substr(0, pos);
        str = _trim(str);
        str = str + "&";
    }
    return str;
}

void PipeCommand::execute()
{
    bool bg_pipe = false;
    SmallShell &smash = SmallShell::getInstance();
    Command *cmd1, *cmd2;
    string cmd1_name, cmd2_name;
    string str(this->cmd_line);
    str = prepare(str);
    int pos = str.find("|");
    cmd1_name = str.substr(0, pos);
    if (this->is_amper)
    {
        cmd2_name = _trim(str.substr(pos + 2));
    }
    else
    {
        cmd2_name = _trim(str.substr(pos + 1));
    }

    //cmd1_name = _trim(cmd1_name);
    //cmd2_name = _trim(cmd2_name);

    if (CheckBackground(cmd2_name.c_str()))
        bg_pipe = true;
    cmd1 = smash.CreateCommand(cmd1_name.c_str());
    cmd2 = smash.CreateCommand(cmd2_name.c_str());

    ChPrompt *changePrompt = dynamic_cast<ChPrompt *>(cmd1);
    if (changePrompt != nullptr)
    {
        changePrompt->execute();
    }
    smash.forked = true;
    pid_t pipe_pid = fork();
    if (pipe_pid == -1)
    {
        perror("smash error: fork faild");
        exit(0);
    }
    if (pipe_pid == 0)
    {
        setpgrp();
        int mypipe[2];
        pipe(mypipe);

        pid_t pid_command1 = fork();
        if (pid_command1 == -1)
        {
            perror("smash error: fork faild");
            exit(0);
        }
        if (pid_command1 == 0)
        {
            if (this->is_amper)
            {
                if (dup2(mypipe[1], STDERR_FILENO) == -1)
                {
                    perror("smash error: dup2 failed");
                }
            }
            else
            {
                if (dup2(mypipe[1], STDOUT_FILENO) == -1)
                {
                    perror("smash error: dup2 failed");
                }
            }
            if (close(mypipe[0]) == -1)
            {

                perror("smash error: close failed");
            }
            if (close(mypipe[1]) == -1)
            {

                perror("smash error: close failed");
            }
            cmd1->execute();
            //kill(getpid(), SIGKILL);
            exit(0);
        }

        pid_t pid_command2 = fork();
        if (pid_command2 == -1)
        {
            perror("smash error: fork faild");
            exit(0);
        }
        if (pid_command2 == 0)
        {
            if (this->is_amper)
            {
                if (dup2(mypipe[0], STDERR_FILENO) == -1)
                {
                    perror("smash error: dup2 failed");
                }
            }
            else
            {
                if (dup2(mypipe[0], STDIN_FILENO) == -1)
                {
                    perror("smash error: dup2 failed");
                }
            }
            if (close(mypipe[0]) == -1)
            {
                perror("smash error: close failed");
            }
            if (close(mypipe[1]) == -1)
            {
                perror("smash error: close failed");
            }
            cmd2->execute();
            //kill(getpid(), SIGKILL);
            exit(0);
        }

        if (close(mypipe[0]) == -1)
        {
            perror("smash error: close failed");
        }
        if (close(mypipe[1]) == -1)
        {
            perror("smash error: close failed");
        }
        if (waitpid(pid_command1, NULL, WUNTRACED) == -1)
        {
            perror("smash error: waitpaid failed");
        }
        if (waitpid(pid_command2, NULL, WUNTRACED) == -1)
        {
            perror("smash error: waitpaid failed");
        }
        //kill(getpid(), SIGKILL);
        //cout << "pipe pid2 :" << getpid() << endl;
        exit(0);
    }

    else
    { // Smash here
        //cout << "smash pid waiting for pipe :" << getpid() << endl;
        if (!bg_pipe)
            if (waitpid(pipe_pid, NULL, WUNTRACED) == -1)
            {
                perror("smash error: waitpaid failed");
            }
            else
                smash.jobList->addJob(this, pipe_pid);
    }
}

#pragma endregion

#pragma region SmallShell

SmallShell::SmallShell()
{
    this->current_pid = getpid();
    this->jobList = new JobsList();
    this->alive = true;
    this->smash_pid = getpid();
    this->timedList = new TimedList();
}

SmallShell::~SmallShell()
{
    delete this->jobList;
    delete this->timedList;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line)
{
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(cmd_line, args);
    if (result == 0)
    {
        return nullptr;
    }

    if (std::count(args->begin(), args->end(), ">>") != 0 || std::count(args->begin(), args->end(), ">") != 0)
    {
        delete args;
        return new RedirectionCommand(cmd_line);
    }

    if (std::count(args->begin(), args->end(), "|") != 0 || std::count(args->begin(), args->end(), "|&") != 0)
    {
        delete args;
        return new PipeCommand(cmd_line);
    }

    if (args->at(0) == "timeout")
    {
        delete args;
        return new TimeoutCommand(cmd_line);
    }

    if (args->at(0) == "cp")
    {
        delete args;
        return new CpCommand(cmd_line);
    }

    if (args->at(0) == "chprompt")
    {
        std::string name = "smash";
        if (result >= 2)
            name = args->at(1);
        delete args;
        return new ChPrompt(name);
    }

    if (args->at(0) == "ls")
    {
        delete args;
        return new LsCommand(cmd_line);
    }

    if (args->at(0) == "pwd" && result == 1)
    {

        delete args;
        return new GetCurrDirCommand(cmd_line);
    }

    if (args->at(0) == "showpid")
    {
        delete args;
        return new ShowPidCommand(cmd_line);
    }

    if (args->at(0) == "jobs")
    {
        delete args;
        return new JobsCommand(cmd_line, this->jobList);
    }

    if (args->at(0) == "kill")
    {
        delete args;
        return new KillCommand(cmd_line, this->jobList);
    }

    if (args->at(0) == "quit")
    {
        if (result == 2 && args->at(1) == "kill")
        {
            delete args;
            return new QuitCommand(cmd_line, this->jobList, true);
        }
        delete args;
        return new QuitCommand(cmd_line, this->jobList, false);
    }

    if (args->at(0) == "fg")
    {
        delete args;
        return new ForegroundCommand(cmd_line, this->jobList);
    }

    if (args->at(0) == "bg")
    {
        delete args;
        return new BackgroundCommand(cmd_line, this->jobList);
    }

    if (args->at(0) == "cd")
    {
        ChangeDirCommand *cd;
        char *buffer = new char[1024];
        string curr = getcwd(buffer, COMMAND_ARGS_MAX_LENGTH);
        if (this->previousDirectory.empty())
        {
            if (result == 2 && args->at(1) == "-")
            {
                cd = new ChangeDirCommand(cmd_line, "");
            }
            else
            {
                cd = new ChangeDirCommand(cmd_line, curr);
                this->previousDirectory = curr;
            }
        }
        else
        {
            //string lastPwd = this->dirHistory.top();
            cd = new ChangeDirCommand(cmd_line, this->previousDirectory);
            if (result == 2 && args->at(1) == "-")
            {
                this->previousDirectory = curr;
            }
        }
        delete[] buffer;
        delete args;
        return cd;
    }

    else
    {
        delete args;
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

const char **convertStringToChar(vector<string> *args)

{
    const char **args_new = new const char *[args->size()];
    for (int i = 0; i < args->size(); i++)
    {
        args_new[i] = args->at(i).c_str();
    }
    return args_new;
}

void CheckBuiltIn(vector<string> *args)
{
    string builtins[] = {"chprompt", "ls", "showpid", "pwd", "cd", "jobs", "kill", "fg", "bg", "quit"};
    for (int i = 0; i < 10; i++)
    {
        if (args->at(0) == builtins[i])
        {
            if (args->at(0) == "fg")
            {
                SmallShell::getInstance().current_pid = getpid();
            }
            SmallShell::getInstance().external = false;
            break;
        }
    }
}

void CheckRedirection(vector<string> *args)
{
    SmallShell &smash = SmallShell::getInstance();
    if (args->size() == 3)
    {
        if (args->at(1) == ">>")
        {
            smash.redirection = true;
            smash.append = true;
        }
        if (args->at(1) == ">")
        {
            smash.redirection = true;
        }
    }
}

void CheckBackground(vector<string> *args, char **args_char)
{
    SmallShell &smash = SmallShell::getInstance();
    smash.background = false;
    string argument = args->at(args->size() - 1);
    if (argument.at(argument.size() - 1) == '&')
    {
        smash.background = true;
        int i;
        char *temp = args_char[args->size() - 1];

        char *new_word = new char[argument.size() - 1];
        for (i = 0; i < argument.size() - 1; i++)
        {
            new_word[i] = temp[i];
        }
        new_word[i] = '\0';
        args_char[args->size() - 1] = new_word;
    }
}

void CheckTimeout(vector<string> *args, char **args_char)
{
    SmallShell &smash = SmallShell::getInstance();
    smash.timeout = false;
    string argument = args->at(0);
    if (argument == "timeout")
    {
        smash.timeout = true;
    }
}

void setToDefault()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.redirection = false;
    smash.append = false;
    smash.external = true;
    smash.background = false;
    smash.forked = false;
    smash.piped = false;
    smash.timeout = false;
}

void SmallShell::executeCommand(const char *cmd_line)
{
    Command *cmd = CreateCommand(cmd_line);
    this->command = cmd;
    vector<string> *args = new vector<string>();
    char **args_char = new char *[COMMAND_MAX_ARGS];
    _parseCommandLineChar(cmd_line, args_char);
    int result = _parseCommandLine(cmd_line, args);
    if (result == 0)
    {
        delete args;
        // for (int i = 0; i < COMMAND_MAX_ARGS; i++)
        // {
        //     free(args_char[i]);
        // }
        delete[] args_char;
        return;
    }

    CheckBuiltIn(args);
    CheckRedirection(args);
    CheckBackground(args, args_char);
    CheckTimeout(args, args_char);

    cmd->execute();

    delete args;
    // for (int i = 0; i < COMMAND_MAX_ARGS; i++)
    // {
    //     free(args_char[i]);
    // }
    delete[] args_char;
    delete cmd;

    setToDefault();
}

#pragma endregion

string prepare_no_ampersand(string str)
{
    int pos = str.find_last_of("&");
    if (pos != -1)
    {
        str = str.substr(0, pos);
        str = _trim(str);
    }
    return str;
}

void ExternalCommand::execute()
{
    pid_t pid;
    char *cmd_copy = new char[COMMAND_MAX_ARGS];
    strcpy(cmd_copy, this->cmd_line);
    SmallShell &smash = SmallShell::getInstance();
    vector<string> *args = new vector<string>();
    char **args_char = new char *[COMMAND_MAX_ARGS];
    _parseCommandLineChar(cmd_line, args_char);
    int result = _parseCommandLine(cmd_line, args);
    CheckBackground(args, args_char);
    if (smash.background)
    {
        _removeBackgroundSign(cmd_copy);
    }

    char *arguments_for_execv[] = {"/bin/bash", "-c", cmd_copy, NULL};
    int status;
    pid = fork();
    if (pid < 0)
    {
        perror("smash error: fork failed");
    }
    else
    {
        if (pid == 0) //child
        {
            if (!smash.forked)
                setpgrp();

            if (smash.timeout)
            {
                string s;
                std::vector<std::string>::const_iterator i = args->begin();
                i++;
                for (i; i != args->end(); ++i)
                    s = s + *i + " ";
                s = prepare_no_ampersand(s);
                const char *cmd_line_temp = s.c_str();
                strcpy(cmd_copy, cmd_line_temp);
                arguments_for_execv[2] = cmd_copy;
            }

            if (execvp("/bin/bash", arguments_for_execv) == -1)
            {
                perror("something went wrong");
                for (int i = 0; i < COMMAND_MAX_ARGS; i++)
                {
                    free(args_char[i]);
                }
                delete[] args_char;
                exit(0);
            }
        }

        else
        { //parent
            if (smash.timeout)
            {
                //cout << getpid() << endl;
                int duration = stoi(args->at(0));
                alarm(duration);
                smash.timedList->addTimed(cmd_line, pid, time(nullptr), duration);
            }
            if (smash.background == false)
            {
                smash.current_pid = pid;
                waitpid(pid, &status, WUNTRACED);
                smash.current_pid = getpid();
            }
            else
            {
                smash.jobList->addJob(this, pid);
            }
            if (smash.forked)
            {
                //kill(getpid(), SIGKILL);
                exit(0);
            }
        }
    }
}

ChPrompt::ChPrompt(string name)
{
    this->newPromptName = name;
}

void ChPrompt::execute()
{
    pid_t pid = getpid();
    SmallShell::getInstance().changePrompt(this->newPromptName);
}

void LsCommand::execute()
{
    // DIR *dir;
    // struct dirent *dp;
    // char *file_name;
    // dir = opendir(".");
    // while((dp = readdir(dir)) != NULL){
    //   if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") )

    // }
    struct dirent **namelist;
    int n;
    int i = 0;
    n = scandir(".", &namelist, NULL, alphasort);

    if (n > 0)
    {
        while (i < n)
        {
            char *current = namelist[i]->d_name;
            if (strcmp(current, ".") != 0 && strcmp(current, "..") != 0)
                cout << namelist[i]->d_name << endl;
            free(namelist[i]);
            i++;
        }
        free(namelist);
    }
}

bool JobsList::isEmpty()
{
    return this->job_list->size() == 0;
}

void ForegroundCommand::execute()
{
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(this->cmd_line, args);
    int job_id = -1;
    if (result == 1)
    {
        if (this->joblist->isEmpty())
        {
            cout << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        job_id = this->joblist->getMaximalJobId();
    }
    else
    {
        if (result > 2)
        {
            cout << "smash error: fg: invalid arguments" << endl;
            return;
        }
        try
        {
            job_id = stoi(args->at(1));
        }
        catch (invalid_argument)
        {
            cout << "smash error: fg: invalid arguments" << endl;
            return;
        }
    }

    //int process_id = this->joblist->getJobById(job_id)->process_id;
    JobsList::JobEntry *job_entry = this->joblist->getJobById(job_id);
    if (job_entry == nullptr)
    {
        cout << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    int process_id = job_entry->process_id;
    kill(process_id, SIGCONT);
    SmallShell::getInstance().current_pid = process_id;
    int status;
    waitpid(process_id, &status, WUNTRACED);
    bool is_exited = WIFEXITED(status);

    if (is_exited)
    {
        //it->second->finished = true;
        this->joblist->setFinished(job_id);
        SmallShell::getInstance().current_pid = getpid();
    }
}

void GetCurrDirCommand::execute()
{
    char *buffer = new char[1024];
    cout << getcwd(buffer, COMMAND_ARGS_MAX_LENGTH) << "\n";
    delete[] buffer;
}

void BackgroundCommand::execute()
{
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(this->cmd_line, args);
    int job_id = -1;
    if (result == 1)
    {
        if (this->joblist->isEmpty())
        {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        job_id = this->joblist->getMaximalStoppedJobId();
        if (job_id == 0)
        {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
    else
    {
        if (result > 2)
        {
            cout << "smash error: bg: invalid arguments" << endl;
            return;
        }
        try
        {
            job_id = stoi(args->at(1));
        }
        catch (invalid_argument)
        {
            cout << "smash error: bg: invalid arguments" << endl;
            return;
        }
    }
    if (result == 1)
        job_id = this->joblist->getMaximalStoppedJobId();
    else
        job_id = stoi(args->at(1));
    //int process_id = this->joblist->getJobById(job_id)->process_id;
    JobsList::JobEntry *job_entry = this->joblist->getJobById(job_id);
    if (job_entry == nullptr)
    {
        cout << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    if (!this->joblist->getJobById(job_id)->stopped)
    {
        cout << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        return;
    }

    int process_id = job_entry->process_id;
    cout << job_entry->command << " : " << job_entry->process_id << endl;
    kill(process_id, SIGCONT);
    job_entry->stopped = false;
}

int JobsList::getMaximalStoppedJobId()
{

    int max = 0;
    auto it = this->job_list->begin();
    while (it != this->job_list->end())
    {
        if (it->first > max && it->second->stopped)
        {
            max = it->first;
        }
        it++;
    }
    return max;
}

void TimeoutCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    vector<string> *args = new vector<string>();
    _parseCommandLine(this->cmd_line, args);
    std::string s;
    std::vector<std::string>::const_iterator i = args->begin();
    i++;
    for (i; i != args->end(); ++i)
        s = s + *i + " ";

    const char *cmd_line = s.c_str();
    Command *cmd = smash.CreateCommand(cmd_line);
    //int duration = stoi(args->at(1));
    //alarm(duration);
    cmd->execute();
}

#pragma region CopyCommand

CpCommand::CpCommand(const char *cmd_line)
{
    this->cmd_line = cmd_line;
    vector<string> *args = new vector<string>();
    _parseCommandLine(cmd_line, args);
    this->source = args->at(1);
    this->destination = args->at(2);
    delete args;
}

void CpCommand::execute()
{
    int content, wrote;
    pid_t copy_pid = fork();
    if (copy_pid == -1)
    {
        perror("smash error: fork failed");
    }
    if (copy_pid == 0)
    {
        int fd_read = open(this->source.c_str(), O_RDONLY, 0666);
        int fd_write = open(this->destination.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_read == -1 || fd_write == -1)
        {
            perror("smash error: open failed");
            exit(0);
        }
        // struct stat st;
        // fstat(fd_read, &st);
        // size_t size = st.st_size;
        char buffer[BUFFER_SIZE];
        while ((content = read(fd_read, buffer, BUFFER_SIZE)) > 0)
        {
            if ((wrote = write(fd_write, buffer, BUFFER_SIZE)) != content)
            {
                perror("smash error: write failed");
                break;
            }
            if (content != -1)
            {
                perror("smash error: read failed");
                break;
            }
        }
        if (close(fd_read) == -1)
        {
            perror("smash error: close failed");
        }
        if (close(fd_write) == -1)
        {
            perror("smash error: close failed");
        }
        exit(0);
    }
    else
    {
        waitpid(copy_pid, NULL, WUNTRACED);
    }
    //Background
}

#pragma endregion
