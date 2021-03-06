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
    //std::istringstream iss(cmd_line);
    std::istringstream iss(_trim(string(cmd_line)).c_str());
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

void ShowPidCommand::execute()
{
    //cout << "smash pid is " << SmallShell::getInstance().smash_pid << endl;
    cout << "smash pid is " << SmallShell::getInstance().smash_pid << endl;
}

#pragma region JobsList

JobsList::~JobsList()
{

    // for (auto it = this->job_list->begin(); it != this->job_list->end();)
    // {
    //     if (!this->isEmpty())
    //     {
    //         it = this->job_list->erase(it);
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }
    this->job_list->clear();
    delete this->job_list;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped, bool timeout)
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
        job_entry->timeout = timeout;
        pair<int, JobEntry *> _pair = make_pair(job_entry->job_id, job_entry);
        this->job_list->insert(_pair);
        delete args;
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

        cout << "[" << it->second->job_id << "] ";
        if (it->second->timeout)
        {
            cout << "timeout ";
        }
        cout << _trim(it->second->command) << " : "
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
            break;
        }
        it++;
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
    // bool is_exited;
    // bool is_stopped;
    // bool is_continued;
    // int status = -1;
    // pid_t returned_pid;
    // auto it = this->job_list_ref->job_list->begin();
    // while (it != this->job_list_ref->job_list->end())
    // {
    //     returned_pid = waitpid(it->second->process_id, &status, WCONTINUED | WUNTRACED | WNOHANG);
    //     if (returned_pid == -1)
    //     {
    //         perror("smash error: waitpid failed");
    //     }
    //     is_exited = WIFEXITED(status);
    //     is_stopped = WIFSTOPPED(status);
    //     is_continued = WIFCONTINUED(status);
    //     if (is_exited)
    //     {
    //         it->second->finished = true;
    //     }
    //     if (is_stopped)
    //     {
    //         it->second->stopped = true;
    //     }
    //     if (is_continued)
    //     {
    //         it->second->stopped = false;
    //     }

    //     it++;
    // }
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
            SmallShell::getInstance().previousDirectory = this->backup_previous;
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
    if (result != 3)
    {
        cout << "smash error: kill: invalid arguments" << endl;
        delete args;
        return;
    }
    try //If no conversion could be performed
    {
        job_id = stoi(args->at(2));
        signum = stoi(args->at(1));
    }
    catch (invalid_argument)
    {
        cout << "smash error: kill: invalid arguments" << endl;
        delete args;
        return;
    }
    // If sig_num is not negative
    if (args->at(1).at(0) != '-')
    {
        cout << "smash error: kill: invalid arguments" << endl;
        delete args;

        return;
    }

    JobsList::JobEntry *job = this->job_list_ref->getJobById(job_id);
    //If job_id doesn't exist in the job_list
    if (job == nullptr)
    {
        cout << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        delete args;

        return;
    }
    pid_t pid = this->job_list_ref->getJobById(job_id)->process_id;
    int res = killpg(pid, abs(signum));
    if (res == -1)
    {
        perror("smash error: kill failed");
    }
    else
    {
        cout << "signal number " << abs(signum) << " was sent to pid " << pid << endl;
        //https://piazza.com/class/kg7wfbyqnoc73t?cid=53
        // if (abs(signum) == SIGSTOP)
        // {
        //     job->stopped = true;
        // }
    }
    delete args;
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
    bool bg = _isBackgroundComamnd(this->cmd_line);
    if (bg)
        str = prepare_no_ampersand(str);
    //char *no_space_name = new char[str.size()];
    string name;
    char *args_char[COMMAND_MAX_ARGS];
    int res = _parseCommandLineChar(cmd_line, args_char);
    int fd, pos, savestdout;
    if (smash.append)
    {
        pos = str.find(">>");
        name = str.substr(pos + 2);
        str = str.substr(0, pos);
    }
    else
    {
        pos = str.find(">");
        name = str.substr(pos + 1);
        str = str.substr(0, pos);
        //strcpy(name, str.substr(pos + 1).c_str());
    }
    if (bg == true)
    {
        str = str + "&";
    }

    name = _trim(name);

    //Deal with no spaces around > or >> : as in date>temp.txt, becuase in real shell it works.
    Command *cmd = smash.CreateCommand(str.c_str());
    QuitCommand *quit = dynamic_cast<QuitCommand *>(cmd);
    if (quit != nullptr)
    {
        quit->execute();
        return;
    }

    if (smash.append)
        fd = open(name.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0666);
    else
        fd = open(name.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd == -1)
    {
        perror("smash error: open failed");
        return;
    }
    savestdout = dup(STDOUT_FILENO);
    if (savestdout == -1)
    {
        perror("smash error: dup failed");
        return;
    }
    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(fd) == -1)
    {
        perror("smash error: close failed");
        return;
    }

    cmd->execute();

    if (close(STDOUT_FILENO))
    {
        perror("smash error: close failed");
        return;
    }
    if (dup2(savestdout, STDOUT_FILENO) == -1)
    {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(savestdout) == -1)
    {
        perror("smash error: close failed");
        return;
    }
    //delete[] no_space_name;
    //delete [] name;
    smash.append = false;
    //cout << "Hi"<< endl;
}

#pragma endregion

#pragma region PipeCommand

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
    delete args;
}

string prepare(string str)
{
    int pos = str.find("|");
    size_t n = std::count(str.begin(), str.end(), '&');
    if (str.at(pos + 1) == '&' && n == 2)
    {
        pos = str.find_last_of("&");
        if (pos != -1)
        {
            str = str.substr(0, pos);
            str = _trim(str);
            str = str + "&";
        }
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
        string tmp = str.substr(pos + 2);
        cmd2_name = _trim(tmp);
    }
    else
    {
        cmd2_name = _trim(str.substr(pos + 1));
    }

    if (CheckBackground(cmd2_name.c_str()))
        bg_pipe = true;
    cmd1 = smash.CreateCommand(cmd1_name.c_str());
    cmd2 = smash.CreateCommand(cmd2_name.c_str());

    ChPrompt *changePrompt = dynamic_cast<ChPrompt *>(cmd1);
    if (changePrompt != nullptr)
    {
        changePrompt->execute();
    }

    QuitCommand *quit = dynamic_cast<QuitCommand *>(cmd1);
    if (quit != nullptr)
    {
        quit->execute();
        return;
    }

    smash.forked = true;
    pid_t pipe_pid = fork();
    if (pipe_pid == -1)
    {
        perror("smash error: fork failed");
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
            perror("smash error: fork failed");
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
            // if (this->is_amper)
            // {
            //     if (dup2(mypipe[0], STDERR_FILENO) == -1)
            //     {
            //         perror("smash error: dup2 failed");
            //     }
            // }
            // else
            // {
            if (dup2(mypipe[0], STDIN_FILENO) == -1)
            {
                perror("smash error: dup2 failed");
            }
            //}
            if (close(mypipe[0]) == -1)
            {
                perror("smash error: close failed");
            }
            if (close(mypipe[1]) == -1)
            {
                perror("smash error: close failed");
            }

            cmd2->execute();
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
    this->alarmList = new AlarmsList();
}

SmallShell::~SmallShell()
{
    delete this->jobList;
    delete this->timedList;
    delete this->alarmList;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line)
{
    this->jobList->removeFinishedJobs();
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(cmd_line, args);
    string s(cmd_line);

    if (result == 0)
    {
        delete args;
        return nullptr;
    }

    auto it1 = s.find(">>");
    auto it2 = s.find(">");

    //if (std::count(args->begin(), args->end(), ">>") != 0 || std::count(args->begin(), args->end(), ">") != 0)
    if (it1 != std::string::npos || it2 != std::string::npos)
    {
        delete args;
        return new RedirectionCommand(cmd_line);
    }

    it1 = s.find("|");
    it2 = s.find("|&");
    //if (std::count(args->begin(), args->end(), "|") != 0 || std::count(args->begin(), args->end(), "|&") != 0)
    if (it1 != std::string::npos || it2 != std::string::npos)
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
        char buffer[1024];
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
            cd = new ChangeDirCommand(cmd_line, this->previousDirectory);
            cd->backup_previous = this->previousDirectory;
            this->previousDirectory = curr;
        }
        // }
        // else
        // {
        //     //string lastPwd = this->dirHistory.top();
        //     cd = new ChangeDirCommand(cmd_line, this->previousDirectory);
        //     //if (result == 2 && args->at(1) == "-")
        //     //{
        //         this->previousDirectory = curr;
        //     //}
        // }
        //cd = new ChangeDirCommand(cmd_line, this->previousDirectory);
        //this->previousDirectory = curr;
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

void CheckRedirection(const char *cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();
    string s(cmd_line);
    auto it = s.find(">>");
    if (it != std::string::npos)
    {
        smash.append = true;
    }
    else
    {
        smash.append = false;
    }
}

bool CheckBackground(const char *cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();
    if (_isBackgroundComamnd(cmd_line) == true)
    {
        smash.background = true;
    }
    else
    {
        smash.background = false;
    }
    return smash.background;
}

void CheckTimeout(vector<string> *args)
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
    //smash.redirection = false;
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
    int result = _parseCommandLine(cmd_line, args);
    if (result == 0)
    {
        delete args;
        setToDefault();
        return;
    }
    CheckBuiltIn(args);
    CheckRedirection(cmd_line);
    CheckBackground(cmd_line);
    CheckTimeout(args);

    cmd->execute();

    delete args;
    delete cmd;
    setToDefault();
}

#pragma endregion

void ExternalCommand::execute()
{
    pid_t pid;
    char cmd_copy[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_copy, this->cmd_line);
    SmallShell &smash = SmallShell::getInstance();
    vector<string> *args = new vector<string>();
    int result = _parseCommandLine(cmd_line, args);
    CheckBackground(this->cmd_line);
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
            delete args;
            if (execv("/bin/bash", arguments_for_execv) == -1)
            {
                perror("something went wrong");
                exit(0);
            }
        }

        else
        { //parent
            if (smash.timeout)
            {
                int duration = stoi(args->at(0));
                //alarm(duration);
                smash.alarmList->addAlarm(time(nullptr), duration);
                if (!smash.alarmList->alarms_list->empty())
                {
                    smash.timedList->addTimed(cmd_line, pid, time(nullptr), duration);
                }
            }
            if (smash.background == false)
            {
                smash.current_pid = pid;

                if (waitpid(pid, &status, WUNTRACED) == -1)
                {
                    perror("smash error: waitpid failed");
                }
                smash.current_pid = getpid();
            }
            else
            {
                bool timeout = smash.timeout;
                smash.jobList->addJob(this, pid, false, timeout);
            }
            if (smash.forked)
            {
                delete args;
                exit(0);
            }
            delete args;
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
    if(n == -1){
        perror("smash error: scandir failed");
    }
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
            delete args;
            return;
        }
        job_id = this->joblist->getMaximalJobId();
    }
    else
    {
        if (result > 2)
        {
            cout << "smash error: fg: invalid arguments" << endl;
            delete args;
            return;
        }
        try
        {
            job_id = stoi(args->at(1));
        }
        catch (invalid_argument)
        {
            cout << "smash error: fg: invalid arguments" << endl;
            delete args;
            return;
        }
    }

    //int process_id = this->joblist->getJobById(job_id)->process_id;
    JobsList::JobEntry *job_entry = this->joblist->getJobById(job_id);
    if (job_entry == nullptr)
    {
        cout << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        delete args;
        return;
    }
    if (job_entry->timeout)
    {
        cout << "timeout ";
    }
    cout << _trim(job_entry->command) << " : " << job_entry->process_id << endl;
    int process_id = job_entry->process_id;
    kill(process_id, SIGCONT);
    SmallShell::getInstance().current_pid = process_id;
    int status;
    if (waitpid(process_id, &status, WUNTRACED) == -1)
    {
        perror("smash error: waitpid failed");
    }
    bool is_exited = WIFEXITED(status);

    if (is_exited)
    {
        //it->second->finished = true;
        this->joblist->setFinished(job_id);
        SmallShell::getInstance().current_pid = getpid();
    }
    delete args;
}

void GetCurrDirCommand::execute()
{
    char buffer[1024];
    cout << getcwd(buffer, COMMAND_ARGS_MAX_LENGTH) << "\n";
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
            delete args;
            return;
        }
        job_id = this->joblist->getMaximalStoppedJobId();
        if (job_id == 0)
        {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            delete args;
            return;
        }
    }
    else
    {
        if (result > 2)
        {
            cout << "smash error: bg: invalid arguments" << endl;
            delete args;
            return;
        }
        try
        {
            job_id = stoi(args->at(1));
        }
        catch (invalid_argument)
        {
            cout << "smash error: bg: invalid arguments" << endl;
            delete args;
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
        delete args;
        return;
    }
    if (!this->joblist->getJobById(job_id)->stopped)
    {
        cout << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        delete args;
        return;
    }
    if (this->joblist->getJobById(job_id)->timeout)
    {
        cout << "timeout ";
    }
    int process_id = job_entry->process_id;
    cout << _trim(job_entry->command) << " : " << job_entry->process_id << endl;
    kill(process_id, SIGCONT);
    //SmallShell::getInstance().jobList->job_list->
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
    int result = _parseCommandLine(this->cmd_line, args);
    if (result <= 2)
    {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    try
    {
        int check_time = stoi(args->at(1));
        if (check_time < 0)
        {
            cout << "smash error: timeout: invalid arguments" << endl;
            return;
        }
    }
    catch (invalid_argument)
    {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }

    string builtins[] = {"chprompt", "ls", "showpid", "pwd", "cd", "jobs", "kill", "fg", "bg", "quit"};
    bool builtin = false;
    string command_name = args->at(2);
    for (int i = 0; i < 10; i++)
    {
        if (builtins[i] == command_name)
        {
            builtin = true;
        }
    }
    if (builtin)
    {
        std::string s;
        std::vector<std::string>::const_iterator i = args->begin();
        i++;
        i++;
        for (i; i != args->end(); ++i)
            s = s + *i + " ";

        const char *cmd_line = s.c_str();
        Command *cmd = smash.CreateCommand(cmd_line);
        //int duration = stoi(args->at(1));
        //alarm(duration);
        smash.alarmList->addAlarm(time(nullptr), stoi(args->at(1)));
        cmd->execute();
    }
    else
    {
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
}

#pragma region CopyCommand

CpCommand::CpCommand(const char *cmd_line)
{
    string s;
    this->cmd_line = cmd_line;
    vector<string> *args = new vector<string>();
    _parseCommandLine(cmd_line, args);
    if (_isBackgroundComamnd(cmd_line) == true)
    {
        s = prepare_no_ampersand(args->at(2));
    }
    else
    {
        s = args->at(2);
    }
    this->same = false;
    this->source = args->at(1);
    this->destination = s;
    try
    {
        string full_source = realpath(this->source.c_str(), NULL);
        string full_dest = realpath(this->destination.c_str(), NULL);
        if (full_source == full_dest)
        {
            same = true;
        }
    }
    catch (exception)
    {
        //same = false;
    }
    delete args;
}

void CpCommand::execute()
{
    int w_flag, r_flag, content, wrote;
    if (same == true)
    {
        cout << "smash: " << this->source << " was copied to " << this->destination << endl;
        return;
    }
    bool bg = _isBackgroundComamnd(cmd_line);

    pid_t copy_pid = fork();
    if (copy_pid == -1)
    {
        perror("smash error: fork failed");
    }
    if (copy_pid == 0)
    {
        setpgrp();
        int fd_read = open(this->source.c_str(), O_RDONLY);
        int fd_write = open(this->destination.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_read == -1 || fd_write == -1)
        {
            perror("smash error: open failed");
            exit(0);
        }
        char buffer[BUFFER_SIZE];
        while ((content = read(fd_read, buffer, BUFFER_SIZE)) > 0)
        {
            r_flag = content;
            if ((wrote = write(fd_write, buffer, content)) != content)
            {
                perror("smash error: write failed");
                break;
                w_flag = 1;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        if (r_flag == -1)
        {
            perror("smash error: read failed");
        }
        if (r_flag != -1 && w_flag != 1)
        {
            cout << "smash: " << this->source << " was copied to " << this->destination << endl;
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
        if (bg == false)
        {
            SmallShell::getInstance().current_pid = copy_pid;
            if (waitpid(copy_pid, NULL, WUNTRACED) == -1)
            {
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().current_pid = getpid();
        }
        else
        {

            SmallShell::getInstance().jobList->addJob(this, copy_pid);
        }
    }
}

#pragma endregion

#pragma region Alarm
void AlarmsList::fireAlarm()
{
    // for(int i = 0; i<this->alarms_list->size(); i++){
    Alarm *current = this->alarms_list->at(0);
    int new_duration = current->scheduled_fire_time - time(nullptr);
    int alarm_result = alarm(new_duration);
    if(alarm_result == -1){
        perror("smash error: alarm failed");
    }
    // }
}

bool alarmCompare(AlarmsList::Alarm *alarm1, AlarmsList::Alarm *alarm2)
{
    return alarm1->scheduled_fire_time < alarm2->scheduled_fire_time;
}
void AlarmsList::addAlarm(time_t timestamp, int duration)
{
    Alarm *entry = new Alarm();
    entry->time_created = timestamp;
    entry->duration = duration;
    entry->scheduled_fire_time = time(nullptr) + duration;
    this->alarms_list->push_back(entry);
    sort(this->alarms_list->begin(), this->alarms_list->end(), alarmCompare);

    fireAlarm();
}
#pragma endregion