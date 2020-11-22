#include <unistd.h>
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

// TODO: Add your implementation for classes in Commands.h

void ShowPidCommand::execute()
{
    cout << "smash pid is " << getpid() << endl;
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
        if (it->second->finished == true)
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
        if (it->first == jobId)
        {
            this->job_list->erase(it);
        }
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

#pragma region

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

void RedirectionCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    int savestdout;
    char **args_char = new char *[COMMAND_MAX_ARGS];
    int res = _parseCommandLineChar(cmd_line, args_char);
    int fd;
    string str(this->cmd_line);
    if (smash.append)
    {
        str = str.substr(0, str.find(">>"));
    }
    else
    {
        str = str.substr(0, str.find(">"));
    }
    //Deal with no spaces around > or >> : as in date>temp.txt, becuase in real shell it works.
    Command *cmd = smash.CreateCommand(str.c_str());
    if (smash.append)
        fd = open(args_char[2], O_CREAT | O_APPEND | O_WRONLY, 0666);
    else
        fd = open(args_char[2], O_TRUNC | O_WRONLY | O_CREAT, 0666);
    savestdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    cmd->execute();
    close(STDOUT_FILENO);
    dup2(savestdout, STDOUT_FILENO);
    close(savestdout);
    smash.append = false;
    //cout << "Hi"<< endl;
}

#pragma endregion

//------------------------------- SmallShell ----------------------------------------------------

SmallShell::SmallShell()
{
    this->current_pid = getpid();
    this->jobList = new JobsList();
    this->alive = true;
}

SmallShell::~SmallShell()
{
    delete this->jobList;
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

    if (result == 3 && (args->at(1) == ">" || args->at(1) == ">>"))
    {
        delete args;
        return new RedirectionCommand(cmd_line);
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

void setToDefault(){
    SmallShell &smash = SmallShell::getInstance();
    smash.redirection = false;
    smash.append = false;
    smash.external = true;
    smash.background = false;
    smash.forked = false;
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
        for (int i = 0; i < COMMAND_MAX_ARGS; i++)
        {
            free(args_char[i]);
        }
        delete[] args_char;
        return;
    }

    CheckBuiltIn(args);
    CheckRedirection(args);
    CheckBackground(args, args_char);

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

void ExternalCommand::execute()
{
    pid_t pid;
    SmallShell &smash = SmallShell::getInstance();
    char **args_char = new char *[COMMAND_MAX_ARGS];
    int result = _parseCommandLineChar(cmd_line, args_char);
    int status;
    pid = fork();
    if (pid < 0)
        perror("negative fork");
    else
    {
        if (pid == 0) //child
        {
            setpgrp();
            if (execvp(args_char[0], args_char) == -1)
            {
                perror("something went wrong");
                for (int i = 0; i < COMMAND_MAX_ARGS; i++)
                {
                    free(args_char[i]);
                }
                delete[] args_char;
                kill(getpid(), SIGKILL);
            }
        }
        else
        { //parent
            if (!smash.background)
            {
                cout << "getpid = " << getpid() << ", currentpid = " << smash.current_pid << ", pid = " << pid << endl;
                smash.current_pid = pid;
                pid_t temp = waitpid(pid, &status, WUNTRACED);
                smash.current_pid = getpid();
            }
            else
            {
                smash.jobList->addJob(this, pid);
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
    return this->job_list->empty();
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