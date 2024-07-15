#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class AlarmEntry{
  public:
  time_t timestamp;
  int duration;
  pid_t pid;
  std::string command;
  std::string timeout_command;
  AlarmEntry(time_t timestamp,int duration,pid_t pid,std::string command,std::string timeout_command);
  friend class ExternalCommand;
  friend class TimeoutCommand;
  friend class SmallShell;
};

class Command {
// TODO: Add your data members
protected:
    const char* cmd_line;
    //std::string cmd_line;
 public:
  Command(const char* cmd_line);
  //yan did here default - check later
  virtual ~Command()=default;
  virtual void execute() = 0;
  const char* get_cmd_line(){return cmd_line;}
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  //yan did here default - check later
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  bool is_alarm;
  ExternalCommand(const char* cmd_line, bool is_alarm);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  bool is_err;
  std::string command_1;
  std::string command_2;
 public:
  PipeCommand(const char* cmd_line,bool is_err);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
bool append;
std::string real_command;
std::string filename;
 public:
  explicit RedirectionCommand(const char* cmd_line, bool append);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

//
// our new classes of built in commands
//

class ChpromptCommand: public BuiltInCommand {
 public:
  ChpromptCommand(const char* cmd_line);
  //yan did here default - check later
  virtual ~ChpromptCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  char **plastPwd;
  ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  JobsList* jobs;
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
  public:
    int job_id;
    pid_t job_pid;
    time_t start_time;
    std::string command;
    bool isStopped;
    bool is_fg;
    bool isFinished;
    JobEntry(int job_id, pid_t job_pid, time_t time_created, std::string& command, bool isStopped, bool is_fg);
  };
  std::vector<JobEntry*> all_jobs_list;
  std::vector<JobEntry*> stopped_jobs;

  int max_job_id;
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, pid_t pid, bool isStopped = false, bool is_fg = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  void removeJobFromStoppedJobs(int jobId);
  void addJobToStoppedJobs(int jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
    std::string prompt;
    pid_t pid;
    char* last_pwd;
    pid_t curr_pid;
    pid_t curr_job_id;
    std::string curr_process;
    std::string curr_alarm_cmd;
    std::string full_alarm_cmd;
    int alarm_duration;
    time_t curr_alarm_time;
    JobsList* jobs;
    std::vector<AlarmEntry*> alarms;
    
    
  Command *CreateCommand(const char* cmd_line, bool is_alarm);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  friend void alarmHandler(int signum);
  void addAlarm(AlarmEntry* new_alarm);
  AlarmEntry* findMinAlarm();
  AlarmEntry* removeAlarm();
  ~SmallShell();
  void executeCommand(const char* cmd_line, bool alarm = false);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
