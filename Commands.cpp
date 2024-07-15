#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <iomanip>
#include <stdio.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 
Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line) {}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    char** args = new char*[COMMAND_MAX_ARGS];
    if (args == NULL) {
        perror("smash error: malloc failed");
        return;
    }
    SmallShell &shell = SmallShell::getInstance();
    int len =_parseCommandLine(cmd_line,args);
    if(len==1)
    {
      shell.prompt="smash";
    }
    else
    {
      shell.prompt=args[1];
    }
    delete [] args;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute(){
  SmallShell &shell= SmallShell::getInstance();
  std::cout << "smash pid is " << shell.pid << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute(){
   char buff[1024];
   char* cwd = getcwd(buff,1024);
   if(cwd != nullptr){
     string pwd = string(buff);
     std::cout << pwd << std::endl;
   }
   /*
   perror("smash error: getcwd failed");
   eturn;*/
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd)
{}

void ChangeDirCommand::execute() {
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  if (length > 2) 
  {
      cerr << "smash error: cd: too many arguments" << endl;
  }
  else if (length==1)
  {
  //make sure what error to do
  cerr << "smash error: cd: no arguments" << endl;
  }
  //here this is ok situation
  else
  {
  char* path = args[1];
  //temp argument to save last location
  char* buff = new char[1024];
  getcwd(buff,1024);
  //string pwd = string(buff);
  //////////
  if (strcmp(path,"-")==0)
    {
      if(*plastPwd==nullptr)
      {
        cerr << "smash error: cd: OLDPWD not set" << endl;
      }
      else //there a directory before
      {
        if(chdir(*plastPwd) == -1)
        {
          perror("smash error: chdir failed");
        }
        else
        {
          strcpy(*plastPwd, buff);
        }
      }
    }
  /*else if (strcmp(path,"..")==0)
    {
      //  later do
    }*/
    
  else {
        if(chdir(path) == -1)
        {
          perror("smash error: chdir failed");
        }
        else
        {
          //printf("plast before: %s \n", *plastPwd);
          //*plastPwd = cwd;
          if(*plastPwd==nullptr)
            {
              *plastPwd= new char[1024];
            }
          strcpy(*plastPwd, buff);
          //printf("plast after: %s \n", *plastPwd);
        }
    }
 }
 delete []args;
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){}

void ForegroundCommand::execute() 
{
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  if(length == 1)
  {
    SmallShell& smash = SmallShell::getInstance();
    int lastJobId;
    JobsList::JobEntry *job = smash.jobs->getLastJob(&lastJobId);
    if (!job)
    {
      cerr << "smash error: fg: jobs list is empty" << endl;
    }
    else
    {
      
      if (job->isStopped)
      {
        if (kill(job->job_pid, SIGCONT) == -1) 
        {
          perror("smash error: kill failed");
          delete [] args;
          return;
        }
      }
      int status;
      cout << job->command << " : " << job->job_pid << endl;
      smash.curr_process = job->command;
      smash.curr_pid = job->job_pid;
      smash.curr_job_id = job->job_id;
      job->is_fg = true;
      int tmp = waitpid(job->job_pid,&status,WUNTRACED);
      if(WIFEXITED(status)|| WIFSIGNALED(status))
      {
        smash.jobs->getJobById(job->job_id)->isFinished = true;
      }

      if(tmp == -1)
      {
        perror("smash error: waitpid failed");
        delete [] args;
        return;
      }
      smash.curr_pid = -1;
      smash.curr_process = "";
    }    
  }

  else if(length == 2)
  {
    int wantedJobId;
    _removeBackgroundSign(args[1]);
    wantedJobId = atoi(args[1]);
    if(wantedJobId==0)
    {
      //make sure what is the error
        cerr << "smash error: fg: invalid arguments" << endl;
        delete [] args;
        return; 
    }
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry *job = smash.jobs->getJobById(wantedJobId);  
    if (!job)
    {
      cerr << "smash error: fg: job-id " << wantedJobId << " does not exist" << endl;
    }
    else
    {
      if (job->isStopped)
      {
        if (kill(job->job_pid, SIGCONT) == -1) 
        {
          perror("smash error: kill failed");
          delete [] args;
          return;
        }
      }
      int status;
      cout << job->command << " : " << job->job_pid << endl;
      smash.curr_process = job->command;
      smash.curr_pid = job->job_pid;
      smash.curr_job_id = job->job_id;
      job->is_fg = true;
      int tmp = waitpid(job->job_pid,&status,WUNTRACED);
      if(WIFEXITED(status)|| WIFSIGNALED(status))
      {
        smash.jobs->getJobById(job->job_id)->isFinished = true;
      }
      if(tmp == -1)
      {
        perror("smash error: waitpid failed");
        delete [] args;
        return;
      }
      smash.curr_pid = -1;
      smash.curr_process = "";
    }    
  }

  else
  {
    cerr << "smash error: fg: invalid arguments" << endl;
  }
  delete [] args;
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() 
{
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  if(length == 1)
  {
    SmallShell& smash = SmallShell::getInstance();
    int lastJobId;
    JobsList::JobEntry *job = smash.jobs->getLastStoppedJob(&lastJobId);
    if (!job)
    {
      cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
      delete [] args;
      return; 
    }
    smash.jobs->removeJobFromStoppedJobs(lastJobId);
    job->isStopped=false;
    std::cout<< job->command<< " : "<< job->job_pid<< std::endl;
    int tmp=kill(job->job_pid,SIGCONT);
    if(tmp==-1)
    {
      perror("smash error: kill failed");
      delete [] args;
      return;
    }
  }
  else if (length==2)
  {
    int wantedJobId;
    _removeBackgroundSign(args[1]);
    wantedJobId = atoi(args[1]);
    if(wantedJobId==0)
    {
      //make sure what is the error
        cerr << "smash error: bg: invalid arguments" << endl;
        delete [] args;
        return; 
    }
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry *job = smash.jobs->getJobById(wantedJobId);  
    if (!job)
    {
      cerr << "smash error: bg: job-id " << wantedJobId << " does not exist" << endl;
      delete [] args;
      return; 
    }
    if(!job->isStopped)
    {
      cerr << "smash error: bg: job-id " << wantedJobId << " is already running in the background" << endl;
      delete [] args;
      return; 
    }
    smash.jobs->removeJobFromStoppedJobs(wantedJobId);
    job->isStopped=false;
    std::cout<< job->command<< " : "<< job->job_pid<< std::endl;
    int tmp=kill(job->job_pid,SIGCONT);
    if(tmp==-1)
    {
      perror("smash error: kill failed");
      delete [] args;
      return;
    }

  }
  else
  {
    cerr << "smash error: bg: invalid arguments" << endl;
  }
  delete [] args;
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line) {}


void QuitCommand::execute()
{
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  char* second_word = args[1];
  //_removeBackgroundSign(args[0]);
  SmallShell& smash = SmallShell::getInstance();
  if (length >= 2 && (strcmp(second_word,"kill") == 0 || strcmp(second_word,"kill&") == 0 ))
  {
    cout << "smash: sending SIGKILL signal to " << smash.jobs->all_jobs_list.size() << " jobs:" << endl;
    smash.jobs->killAllJobs();
  }  
  delete[] args;
  //yan add here: delete this. not sure if we need it
  exit(0);
}

KillCommand::KillCommand(const char *cmd_line,  JobsList* jobs) : BuiltInCommand(cmd_line) {}

void KillCommand::execute()
{
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  if(length != 3)
  {
    cerr << "smash error: kill: invalid arguments" << endl;
  } 
  else
  {
    _removeBackgroundSign(args[2]);
    int wantedJobId = atoi(args[2]);
    if(wantedJobId==0)
    {
      cerr << "smash error: kill: invalid arguments" << endl;
      delete [] args;
      return; 
    }
    int signum=atoi(args[1])*(-1);
    //need to check if need to add this, make sense but they didn't write about it
    if(signum<=0 || signum>64)
    {
      cerr << "smash error: kill: invalid arguments" << endl;
      delete [] args;
      return; 
    }
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry *job = smash.jobs->getJobById(wantedJobId);
    if (!job)
    {
      cerr << "smash error: kill: job-id " << wantedJobId << " does not exist" << endl;
      delete [] args;
      return; 
    }
    int temp=kill(job->job_pid,signum);
    if(temp==-1)
    {
      perror("smash error: kill failed");
      delete [] args;
      return; 
    }
    std::cout << "signal number " << signum << " was sent to pid " << job->job_pid << std::endl;
    //Yoni didn't do it but Yan did and I think we shoud do it but we can check it
    if (signum == SIGTSTP) 
    {
        if(job->isStopped != true)
        {
          job->isStopped = true;
          smash.jobs->addJobToStoppedJobs(wantedJobId);
        }
    } 
    else if (signum == SIGCONT) 
    {
      if(job->isStopped)
      {
        job->isStopped = false;
        smash.jobs->removeJobFromStoppedJobs(wantedJobId);
      }
    }
  }
  delete [] args;
}

TimeoutCommand::TimeoutCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void TimeoutCommand::execute()
{
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  if (length < 3)
  {
    cerr << "smash error: timeout: invalid arguments" << endl;
    return;
  }
  string number = string(args[1]);
  std::string::const_iterator it = number.begin();
  while (it != number.end())
  {
    if(!(std::isdigit(*it)))
    {
      cerr << "smash error: timeout: invalid arguments" << endl;
      return;
    }
    it++;
  }
  SmallShell &smash = SmallShell::getInstance();
  string timeout_part = string(args[0]) + " " + string(args[1]);
  //create the parameters to the alarmEntry
  smash.alarm_duration = atoi(args[1]);
  smash.curr_alarm_time = time(nullptr);
  //create the command to execute
  //Command* command = smash.CreateCommand(cmd_line+timeout_part.length(),timeout_part);
  //AlarmEntry* new_alarm = new AlarmEntry();
  //new_alarm->duration = atoi(args[1]);
  //new_alarm->command = string(cmd_line+timeout_part.length());
  //new_alarm->timestamp = time(nullptr);
  //dynamic_cast<ExternalCommand*>(command)->alarm = new_alarm;
  //printf("command: %s \n", command->get_cmd_line());
  smash.curr_alarm_cmd = cmd_line+timeout_part.length();
  string new_cmd_line = cmd_line+timeout_part.length();
  smash.full_alarm_cmd = string(cmd_line);
  smash.executeCommand(new_cmd_line.c_str(), true);
  //command->execute();
}
//external commands:

ExternalCommand::ExternalCommand(const char *cmd_line,bool is_alarm) : Command(cmd_line),is_alarm(is_alarm) {}

void ExternalCommand::execute(){
  //printf("cmd line: %s \n", cmd_line);
  char** args = new char*[COMMAND_MAX_ARGS];
  int length =_parseCommandLine(cmd_line,args);
  bool is_simple = true;
  //check if this is complex command  
  for (int i = 1; ((i < length) && (is_simple)); i++)
  {
    string tmp_arg = string(args[i]);
    //here found it
    if ((tmp_arg.find("*") != std::string::npos)||(tmp_arg.find("?") != std::string::npos))
    {
      is_simple=false;
    }
  }
  if (args[0] != nullptr)
  {
    string first_arg = string(args[0]);
    const char* first_arg_v2 = first_arg.c_str();
    const int first_arg_len = first_arg.length();
    //printf("first arg len %d \n", first_arg_len);
    char** param_args = new char*[length-1];
    for (int i = 0; i < length-1; i++)
    {
      param_args[i]=new char[first_arg_len];
    }
    
    //printf("check 4 length %d \n", length);
    for (int i = 0; i < length-1; i++)
    {
      strcpy(param_args[i], args[i+1]);
    }
    if (is_simple)
    {
      //printf("this is simple! \n");
      //printf("please work \n");
      int pid = fork();
      if(pid==-1)
      {
        perror("smash error: fork failed");
        return;
      }
      if(pid==0)
      {
        if(setpgrp()==-1)
        {
          perror("smash error: setpgrp failed");
          return;
        }
        bool is_background = _isBackgroundComamnd(cmd_line);
        if (is_background)
        {
          char* tmp_string_check = new char[strlen(cmd_line)+1];
          strcpy(tmp_string_check, cmd_line);
          if (is_background)
          {
            _removeBackgroundSign(tmp_string_check);
          }
          length =_parseCommandLine(tmp_string_check,args);
        }
         //printf("s: %s \n", first_arg_v2); 
        if(execvp(first_arg_v2, args) == -1)
        {
          perror("smash error: execvp failed");
          exit(0);
          //return;
        }
      }

      else // father
      {
        SmallShell& smash= SmallShell::getInstance();
        if(is_alarm)
        {
          AlarmEntry* new_alarm;
          new_alarm = new AlarmEntry(smash.curr_alarm_time,smash.alarm_duration,pid,smash.curr_alarm_cmd,smash.full_alarm_cmd);
          smash.addAlarm(new_alarm);
        }
        bool is_background = _isBackgroundComamnd(cmd_line);
        if (!is_background) //no &
        {
          smash.curr_pid = pid;
          smash.curr_process = this->cmd_line;
          smash.curr_job_id = -1;
          //note - need to change somthing here
          if(is_alarm)
          {
            ExternalCommand* tmp_command = new ExternalCommand((smash.full_alarm_cmd).c_str(), true);
            //printf("the tmp commands: %s \n ", tmp_command->get_cmd_line());
            smash.jobs->addJob(tmp_command, pid,false, true);
          }
          else
          {
            smash.jobs->addJob(this, pid,false, true);
          }
          int status;
          if (waitpid(pid, &status, WUNTRACED) == -1)
          {
            perror("smash error: waitpid failed");
            return;
          }
          if(WIFEXITED(status) || WIFSIGNALED(status))
          {
            smash.jobs->getJobById(-1)->isFinished=true;
          }
          smash.curr_pid = -1;
          smash.curr_process = "";
        }
        else
        {
          if (is_alarm)
          {
            ExternalCommand* tmp_command = new ExternalCommand((smash.full_alarm_cmd).c_str(), true);
            smash.jobs->addJob(tmp_command, pid);
          }
          else
          {
              smash.jobs->addJob(this, pid);
          }
        }
      }
    }

    //not simple command - with bash and stuff
    else
    {
      //printf("this is not simple \n");
      int pid = fork();
      if(pid==-1)
      {
        perror("smash error: fork failed");
        return;
      }
      if(pid==0)
      {
        if(setpgrp()==-1)
        {
          perror("smash error: setpgrp failed");
          return;
        }        
        //char* cmd_line_arg = new char[strlen(cmd_line)+1];
        //strcpy(cmd_line_arg, cmd_line);
        //char *argv[] = {"/bin/bash", "-c", cmd_line_arg, nullptr};

        char** argv = new char*[4];
        argv[0] = new char[12];
        strcpy(argv[0],"/bin/bash");
        argv[1]= new char[3];
        strcpy(argv[1],"-c");
        argv[2] = new char[strlen(cmd_line)+1];
        strcpy(argv[2], cmd_line);

        bool is_background = _isBackgroundComamnd(cmd_line);
        if (is_background)
        {
            _removeBackgroundSign(argv[2]);
        }
         //printf("s: %s \n", argv[2]); 
        if(execv("/bin/bash",argv) == -1)
        {
          perror("smash error: execv failed");
          return;
        }
      }
      else // father
      {
          SmallShell& smash= SmallShell::getInstance();
          if(is_alarm)
          {
            AlarmEntry* new_alarm;
            new_alarm = new AlarmEntry(smash.curr_alarm_time,smash.alarm_duration,pid,smash.curr_alarm_cmd,smash.full_alarm_cmd);
            smash.addAlarm(new_alarm);
          }
          bool is_background = _isBackgroundComamnd(cmd_line);
          if (!is_background) //no &
          {
            smash.curr_pid = pid;
            smash.curr_process = this->cmd_line;
            smash.curr_job_id = -1;
            if(is_alarm)
            {
              ExternalCommand* tmp_command = new ExternalCommand((smash.full_alarm_cmd).c_str(), true);
              smash.jobs->addJob(tmp_command, pid,false, true);
            }
            else
            {
              smash.jobs->addJob(this, pid,false, true);
            }
            int status;
            if (waitpid(pid, &status, WUNTRACED) == -1)
            {
              perror("smash error: waitpid failed");
              return;
            }
            if(WIFEXITED(status) || WIFSIGNALED(status))
            {
              smash.jobs->getJobById(-1)->isFinished=true;
            }
            smash.curr_pid = -1;
            smash.curr_process = "";
          }
          else
          {
            if (is_alarm)
            {
              ExternalCommand* tmp_command = new ExternalCommand((smash.full_alarm_cmd).c_str(), true);
              smash.jobs->addJob(tmp_command, pid);
            }
            else
            {
                smash.jobs->addJob(this, pid);
            }
          }

          
      }
  }
}
}

//pipe and i/o
RedirectionCommand::RedirectionCommand(const char* cmd_line, bool append): Command(cmd_line),append(append)
{
  if(!append)
  {
    string all_line = _trim(string(cmd_line));
    real_command =_trim(all_line.substr(0,all_line.find_first_of(">")));
    filename =_trim(all_line.substr(all_line.find_first_of(">")+1,all_line.length()-1));
  }
  else
  {
    string all_line = _trim(string(cmd_line));
    real_command = _trim(all_line.substr(0,all_line.find_first_of(">>")));
    filename =_trim(all_line.substr(all_line.find_first_of(">>")+2,all_line.length()-1));
  }
}

void RedirectionCommand::execute()
{
  int tmp_screen = dup(1);
  if(tmp_screen == -1)
  {
    perror("smash error: dup failed");
    return;
  }
  if(close(1) == -1)
  {
    perror("smash error: close failed");
    return;
  }
  int fd;
  if (append) 
  {
    fd = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0655);
  } 
  else
  {
    fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
  }
  if(fd==-1)
  {
    dup(tmp_screen);
    close(tmp_screen);
    perror("smash error: open failed");
    return;
  }
  SmallShell&  smash = SmallShell::getInstance();
  Command* command = smash.CreateCommand(real_command.c_str(),false);
  command->execute();
  delete command; //to check why Yoni  and Yan did it
  if(close(1) == -1)
  {
    perror("smash error: close failed");
    return;
  }
  if(dup(tmp_screen) == -1)
  {
    perror("smash error: dup failed");
    return;
  }
  if(close(tmp_screen) == -1)
  {
    perror("smash error: close failed");
    return;
  }
}


PipeCommand::PipeCommand(const char* cmd_line, bool is_err): Command(cmd_line), is_err(is_err)
{
  if(!is_err)
  {
  string all_line = _trim(string(cmd_line));
  command_1 =_trim(all_line.substr(0,all_line.find_first_of("|")));
  command_2 =_trim(all_line.substr(all_line.find_first_of("|")+1,all_line.length()-1));
  }
  else
  {
  string all_line = _trim(string(cmd_line));
  command_1 = _trim(all_line.substr(0,all_line.find_first_of("|&")));
  command_2 =_trim(all_line.substr(all_line.find_first_of("|&")+2,all_line.length()-1));
  }
}

void PipeCommand::execute()
{
  int num;
  if (is_err)
  {
    num =2;
  }
  else
  {
    num=1;
  }
  int fields[2];
  if(pipe(fields)== -1)
  {
    perror("smash error: pipe failed");
    return;
  }
  SmallShell &shell = SmallShell::getInstance();
  int first = fork();
  if(first == -1)
  {
    perror("smash error: fork failed");
    return;
  }
  if(first == 0)
  {
    if (close(num)== -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (dup(fields[1]) == -1)
    {
      perror("smash error: dup failed");
      return;
    }
    if (close(fields[1])== -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (close(fields[0])== -1)
    {
      perror("smash error: close failed");
      return;
    }
    shell.executeCommand(command_1.c_str());
    exit(0);
  }

  //second child
  int second = fork();
  if(second == -1)
  {
    perror("smash error: fork failed");
    return;
  }
  if(second==0)
  {
    if (close(0)== -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (dup(fields[0]) == -1)
    {
      perror("smash error: dup failed");
      return;
    }
    if (close(fields[0])== -1)
    {
      perror("smash error: close failed");
      return;
    }
    if (close(fields[1])== -1)
    {
      perror("smash error: close failed");
      return;
    }
    shell.executeCommand(command_2.c_str());
    exit(0);
  }
  if (close(fields[0])== -1)
  {
    perror("smash error: close failed");
    return;
  }
  if (close(fields[1])== -1)
  {
    perror("smash error: close failed");
    return;
  }
  int status;
  if(waitpid(first,&status,0)==-1)
  {
    perror("smash error: waitpid failed");
    return;
  }
  if(waitpid(second,&status,0)==-1)
  {
    perror("smash error: waitpid failed");
    return;
  }
}

SmallShell::SmallShell(): prompt("smash"), last_pwd(nullptr), curr_pid(-1), curr_job_id(-1), curr_process(), curr_alarm_cmd(),full_alarm_cmd(), alarm_duration(0),curr_alarm_time(0){
  pid = getpid();
  jobs = new JobsList();
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line,bool is_alarm) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  if(cmd_s.find("|&",0) != string::npos)
  {
    return new PipeCommand(cmd_line, true);
  }
  else if(cmd_s.find("|",0) != string::npos)
  {
    return new PipeCommand(cmd_line,false);
  }
  else if(cmd_s.find(">>",0) != string::npos)
  {
    return new RedirectionCommand(cmd_line, true);
  }
  else if (cmd_s.find(">",0) != string::npos)
  {
    return new RedirectionCommand(cmd_line,false);
  }
  else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0){
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt")==0 || firstWord.compare("chprompt&")==0){
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord == "cd" || firstWord.compare("cd&") == 0) {
        return new ChangeDirCommand(cmd_line, &last_pwd);
  }
  else if (firstWord.compare("jobs")==0 || firstWord.compare("jobs&")==0){
    return new JobsCommand(cmd_line,jobs);
  }
  else if(firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0)
  {
    return new ForegroundCommand(cmd_line,jobs);
  }
  else if(firstWord.compare("bg") == 0 || firstWord.compare("bg&") == 0)
  {
    return new BackgroundCommand(cmd_line,jobs);
  }
  else if (firstWord.compare("quit")==0 || firstWord.compare("quit&")==0)
  {
   return new QuitCommand(cmd_line,jobs);
  }
  else if (firstWord.compare("kill")==0 || firstWord.compare("kill&")==0) //* to check why Yoni did it)
  {
   return new KillCommand(cmd_line,jobs);
  }
  else if (firstWord.compare("timeout")==0)
  {
    return new TimeoutCommand(cmd_line);
  }
  else 
  {
    return new ExternalCommand(cmd_line,is_alarm);
  }
  
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line, bool alarm) {
  // TODO: Add your implementation here
  // for example:
  jobs->removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line,alarm);
  cmd->execute();
  //delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

//----Jobs class--------
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};

void JobsCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  shell.jobs->removeFinishedJobs();
  shell.jobs->printJobsList();
}


JobsList::JobEntry::JobEntry(int job_id, pid_t job_pid, time_t start_time, std::string &command, bool isStopped,bool is_fg)
        : job_id(job_id),
          job_pid(job_pid),
          start_time(start_time),
          command(command),
          isStopped(isStopped),
          is_fg(is_fg),
          isFinished(false){}


JobsList::JobsList(): max_job_id(0) {
}

void JobsList::addJob(Command *cmd, pid_t pid,bool isStopped,bool is_fg)
{
  this->removeFinishedJobs();
  string cmd_line(cmd->get_cmd_line());
  time_t start_time = time(nullptr);
  if(start_time==-1)
  {
    perror("smash error: time failed");
    return;
  }
  JobEntry* new_job;
  if(is_fg)
  {
    new_job = new JobEntry(-1,pid,start_time,cmd_line,isStopped,is_fg);
  }
  else
  {
    new_job = new JobEntry(max_job_id+1,pid,start_time,cmd_line,isStopped,is_fg);
    max_job_id++;
  }
  all_jobs_list.push_back(new_job);
  if(isStopped)
  {
    stopped_jobs.push_back(new_job);
  }
}

void JobsList:: removeFinishedJobs()
{
  if(all_jobs_list.empty())
  {
    max_job_id=0;
  }
  else
  {
    std::vector<JobEntry*>::iterator it= all_jobs_list.begin();
    int status;
    while(it != all_jobs_list.end())
    {
      if(!(*it)->is_fg)
      {
        int pid = waitpid((*it)->job_pid,&status,WNOHANG);
        if(pid==-1)
        {
          perror("smash error: waitpid failed");
          continue;
        }
        if(pid!=0)
        {
          (*it)->isFinished=true;
          it=all_jobs_list.erase(it);
        }
        else
        {
          it++;
        }
      }
      else
      {
        if((*it)->isFinished)
        {
          it=all_jobs_list.erase(it);
        }
        else
        {
          it++;
        }
      } 
    }
    it = stopped_jobs.begin();
    while(it != stopped_jobs.end())
    {
      if((*it)->isFinished)
      {
        it=stopped_jobs.erase(it);
      }
      else
      {
        it++;
      }
    }
    if(all_jobs_list.empty())
    {
      max_job_id=0;
    }
    else
    {
      max_job_id = all_jobs_list.back()->job_id;
    }
  }
}

void JobsList::printJobsList()
{
  time_t current_time= time(nullptr);
  if(current_time==-1)
  {
    perror("smash error: time failed");
  }
  else
  {
    for(JobEntry* tmp : all_jobs_list)
    {
      if(tmp->job_id==-1 || tmp->is_fg)
      {
        continue;
      }
      std::cout<<"["<<tmp->job_id<<"] "<< tmp->command<<" : "<<tmp->job_pid<<" "<< difftime(current_time,tmp->start_time)<< " secs";
      if(tmp->isStopped==true)
      {
        std::cout<<" (stopped)"<<std::endl;
      }
      else
      {
        std::cout<<std::endl;
      }
    }
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  for(JobEntry* tmp : all_jobs_list)
  {
    if(tmp->job_id == jobId)
    {
      return tmp;
    }
  }
  return nullptr; //if does not exist
}


JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) 
{
  if (all_jobs_list.size() == 0)
  {
    return nullptr;
  }
  JobEntry* last_job = all_jobs_list.back();
  *lastJobId = last_job->job_id;
  return last_job;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *lastJobId) 
{
  if (stopped_jobs.size() == 0)
  {
    return nullptr;
  }
  JobEntry* last_job = stopped_jobs.back();
  *lastJobId = last_job->job_id;
  return last_job;
}

void JobsList::removeJobFromStoppedJobs(int jobId)
{
  std::vector<JobEntry*>::iterator it= stopped_jobs.begin();
  while(it != stopped_jobs.end())
  {
    if((*it)->job_id == jobId)
    {
      it=stopped_jobs.erase(it);
      break;
    }
    else
    {
      it++;
    }
  }
}

void JobsList::addJobToStoppedJobs(int jobId)
{
  JobEntry* to_add = getJobById(jobId);
  if(stopped_jobs.empty())
  {
    stopped_jobs.push_back(to_add);
    return;
  }
  if(to_add->job_id > stopped_jobs.back()->job_id)
  {
    stopped_jobs.push_back(to_add);
    return;
  }
  std::vector<JobEntry*>::iterator it= stopped_jobs.begin();
  if(to_add->job_id < stopped_jobs.front()->job_id)
  {
    stopped_jobs.insert(it,to_add);
    return;
  }
  while(it != stopped_jobs.end()-1)
  {
    if(to_add->job_id < (*(it+1))->job_id && to_add->job_id> (*it)->job_id)
    {
      stopped_jobs.insert(it+1,to_add);
      break;
    }
    it++;
  }
}

void JobsList::killAllJobs()
{
  for(JobEntry* tmp : all_jobs_list)
  {
    int result = kill(tmp->job_pid,SIGKILL);
    if(result == -1)
    {
      perror("smash error: kill failed");
      return;
    }
    std::cout << tmp->job_pid << ": " << tmp->command <<  std::endl;
  }
  //yoni added here removeFinishedJobs();
  // do we really need it?
}

//Alarm functions
AlarmEntry::AlarmEntry(time_t timestamp,int duration,pid_t pid,std::string command, std::string timeout_command):
    timestamp(timestamp),
    duration(duration),
    pid(pid),
    command(command),
    timeout_command(timeout_command){}

AlarmEntry* SmallShell::findMinAlarm()
{
  AlarmEntry* min_alarm = *(alarms.begin());
  for(AlarmEntry* it : alarms)
  {
    if(it->duration < min_alarm->duration)
    {
      min_alarm = it;
    }
  }
  return min_alarm;
}

void SmallShell::addAlarm(AlarmEntry* new_alarm)
{
  time_t current_time = time(NULL);
  for(AlarmEntry* it : alarms)
  {
    it->duration -= (int)difftime(current_time,it->timestamp);
    it->timestamp=current_time;
  }
  //new_alarm->timestamp = time(NULL);
  alarms.push_back(new_alarm);
  AlarmEntry* min_alarm = findMinAlarm();
  alarm(min_alarm->duration);
}


 AlarmEntry* SmallShell::removeAlarm()
 {
    std::vector<AlarmEntry*>::iterator min_alarm = alarms.begin();
    std::vector<AlarmEntry*>::iterator tmp_alarm = alarms.begin();
    while (tmp_alarm != alarms.end())
    {
      if((*min_alarm)->duration > (*tmp_alarm)->duration)
      {
        min_alarm = tmp_alarm;
      }
      tmp_alarm++;
    }
    AlarmEntry* timedout_alarm = *(min_alarm);
    //AlarmEntry tmp_alarm = *(min_alarm);
    alarms.erase(min_alarm);
    if(alarms.empty())
    {
      return timedout_alarm;
    }
    time_t current_time = time(NULL);
    for(AlarmEntry* it : alarms)
    {
      it->duration -= (int)difftime(current_time,it->timestamp);
      it->timestamp=current_time;
    }
    AlarmEntry* updated_alarm = findMinAlarm();
    alarm(updated_alarm->duration);
    return timedout_alarm;
 }
