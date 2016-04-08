//
// Copyright 2015-2016 Manuel A. Fernandez Montecelo
//
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA 02110-1301, USA.


#include "dpkg_selections.h"

#include "aptitude.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>

#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


namespace aptitude {
namespace apt {
namespace dpkg {


DpkgSelections::DpkgSelections()
{
}


DpkgSelections::~DpkgSelections()
{
}


void DpkgSelections::clear()
{
  selections.clear();
}


bool DpkgSelections::save_selections()
{
  if (selections.empty())
    {
      return true;
    }
  else
    {
      return save_to_dpkg(selections);
    }
}


bool DpkgSelections::save_to_dpkg(const std::string& selections)
{
  // create the pipe
  int dpkg_pipe[2];
  if (pipe(dpkg_pipe))
    {
      _error->Error(_("Failed to create pipe for dpkg selections."));
      return false;
    }
  
  // create the child process
  pid_t pid = fork();
  if (pid < (pid_t) 0)
    {
      _error->Error(_("Failed to fork process for dpkg selections."));
      return false;
    }
  else if (pid == (pid_t) 0)
    {
      // child process, close other end first
      close(dpkg_pipe[1]);

      // handle signals with default values for this process
      signal(SIGTERM, SIG_DFL);
      signal(SIGINT,  SIG_DFL);
      signal(SIGHUP,  SIG_DFL);
      signal(SIGILL,  SIG_DFL);
      signal(SIGSEGV, SIG_DFL);
      signal(SIGBUS,  SIG_DFL);
      signal(SIGABRT, SIG_DFL);

      // set input and output for this process
      dup2(dpkg_pipe[0], STDIN_FILENO);
      int fd_null = open("/dev/null", O_RDONLY);
      dup2(fd_null, STDOUT_FILENO);
      dup2(fd_null, STDERR_FILENO);

      // get command and options
      std::string dpkg_bin = _config->Find("Dir::Bin::dpkg", "dpkg");
      std::vector<std::string> dpkg_args = _config->FindVector("dpkg::options", "");
      // specific option for this action
      dpkg_args.insert(dpkg_args.begin(), "--set-selections");
      // command has to be the first of the args
      dpkg_args.insert(dpkg_args.begin(), dpkg_bin);

      // chroot?
      std::string chroot_dir = _config->FindDir("dpkg::Chroot-Directory", "/");
      int chroot_status = chroot(chroot_dir.c_str());
      if (chroot_dir != "/" && chroot_status != 0)
	{
	  //_error->Error(_("Couldn't chroot for dpkg selections"));
	  int errno_chroot = errno;
	  exit(errno_chroot);
	}

      // prepare exec*()
      const char* command = dpkg_bin.c_str();
      size_t args_with_NULL_size = dpkg_args.size() + 1;
      const char* args[args_with_NULL_size];
      for (size_t i = 0; i < dpkg_args.size(); ++i)
	{
	  args[i] = dpkg_args[i].c_str();
	}
      args[args_with_NULL_size - 1] = NULL;

      // do exec*()
      execvp(command, const_cast<char**>(args));

      // returns from exec*() only of error has occurred, sets errno instead of
      // exit status (exit status always -1 on error)
      int exec_errno = errno;
      //_error->Error(_("Couldn't execute dpkg for selections"));
      exit(exec_errno);
    }
  else
    {
      // parent process, close other end first
      close(dpkg_pipe[0]);

      // write selections to the pipe, then close
      FILE* out_stream = fdopen(dpkg_pipe[1], "w");
      fprintf(out_stream, "%s\n", selections.c_str());
      fclose(out_stream);
      close(dpkg_pipe[1]);

      // wait for child to complete
      int child_exit_status = -1;
      int waitpid_exit_status = waitpid(pid, &child_exit_status, 0);
      if (waitpid_exit_status != pid)
	{
	  // waitpid error
	  _error->Error(_("Failed to execute process to save dpkg selections, waitpid error: %d: '%s'"),
			errno, strerror(errno));
	  return false;
	}
      else if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) == 0)
	{
	  // execution success!
	  return true;
	}
      else
	{
	  // execution, problem
	  if (WIFEXITED(child_exit_status))
	    {
	      _error->Error(_("Failed to execute process to save dpkg selections, dpkg or trying to execute it exited with status/errno: %d"),
			    WEXITSTATUS(child_exit_status));
	    }
	  else if (WIFSIGNALED(child_exit_status))
	    {
	      _error->Error(_("Failed to execute process to save dpkg selections, dpkg killed by signal: %d"),
			    WTERMSIG(child_exit_status));
	    }
	  else
	    {
	      _error->Error(_("Failed to execute process to save dpkg selections, dpkg exited with unknown problem"));
	    }

	  return false;
	}
    }
}


}
}
}
