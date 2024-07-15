#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
	std::cout <<"smash: got ctrl-Z"<<std::endl;
  SmallShell& smash = SmallShell::getInstance();
  int curr_pid = smash.curr_pid;
  std::string curr_process= smash.curr_process;
  if(curr_pid != -1)
  {
    int curr_job_id=smash.curr_job_id;
    JobsList::JobEntry* job = smash.jobs->getJobById(curr_job_id);
    //Yoni did but why? yan didn't 
    if(curr_job_id==-1)
    {
      job->job_id=smash.jobs->max_job_id+1;
    }
    int temp=kill(curr_pid,SIGSTOP);
    if(temp == -1)
    {
      perror("smash errorkill failed");
      return;
    }
    //job->start_time=time(NULL); Yoni did but why?
    job->isStopped = true;
    job->is_fg = false;
    smash.jobs->addJobToStoppedJobs(job->job_id);
    std::cout <<"smash: process "<<curr_pid<<" was stopped"<< std::endl;
  }
}

void ctrlCHandler(int sig_num) {
  std::cout <<"smash: got ctrl-C"  << std::endl;
  SmallShell& smash = SmallShell::getInstance();
  int curr_pid = smash.curr_pid;
  if(curr_pid != -1)
  {
    //Yoni didn't do it but Yan did , I think maybe we should not do it because we didn't need it in kill function (I checked)
    smash.curr_pid = -1;
    smash.curr_process = "";
    int temp = kill(curr_pid,SIGKILL);
    if(temp == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    std::cout << "smash: process "<< curr_pid<< " was killed"<< std::endl;
  }
}

void alarmHandler(int sig_num) {
  std::cout << "smash:" << " got an alarm" << std::endl;
  SmallShell& smash = SmallShell::getInstance();
  AlarmEntry* alarm_to_remove = smash.removeAlarm();
  smash.jobs->removeFinishedJobs();
  if(kill(alarm_to_remove->pid,SIGKILL) == -1)
  {
    return;
  }
  std::cout << "smash: " << alarm_to_remove->timeout_command << " timed out!" << std::endl;
  delete alarm_to_remove;
}

