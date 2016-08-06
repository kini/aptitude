// Copyright (C) 2016 Manuel A. Fernandez Montecelo
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "cmdline_apt_proxy.h"

#include "aptitude.h"
#include "generic/apt/matching/match.h"
#include "generic/apt/matching/parse.h"
#include "generic/apt/matching/pattern.h"

#include <apt-pkg/error.h>

#include <algorithm>
#include <string>
#include <vector>

#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/** Build the command line as a string
 */
std::string build_cmdline_as_string(const std::vector<std::string>& args)
{
  std::string exec_cmdline_str;
  for (const auto& s : args)
    {
      if (! exec_cmdline_str.empty())
	exec_cmdline_str += " ";

      exec_cmdline_str += s;
    }

  return exec_cmdline_str;
}


/** Execute the given command
 */
int exec_command(const std::vector<std::string>& cmdline_args)
{
  // create the child process
  pid_t pid = fork();
  if (pid < (pid_t) 0)
    {
      _error->Error(_("Failed to fork process."));
      return EXIT_FAILURE;
    }
  else if (pid == (pid_t) 0)
    {
      // handle signals with default values for this process
      signal(SIGTERM, SIG_DFL);
      signal(SIGINT,  SIG_DFL);
      signal(SIGHUP,  SIG_DFL);
      signal(SIGILL,  SIG_DFL);
      signal(SIGSEGV, SIG_DFL);
      signal(SIGBUS,  SIG_DFL);
      signal(SIGABRT, SIG_DFL);

      // prepare exec*()
      size_t args_with_NULL_size = cmdline_args.size() + 1;
      char* args[args_with_NULL_size];
      for (size_t i = 0; i < cmdline_args.size(); ++i)
	{
	  char* dupstr = strndup(cmdline_args[i].c_str(), cmdline_args[i].size());
	  if (dupstr == NULL)
	    {
	      _error->Error(_("Failed to copy string (strndup): %d: '%s'"),
			    errno, strerror(errno));
	      exit(errno);
	    }

	  args[i] = dupstr;
	}
      args[args_with_NULL_size - 1] = NULL;

      // do exec*()
      execvp(args[0], args);

      // deallocating, even if not needed -- shouldn't return
      for (size_t i = 0; i < cmdline_args.size(); ++i)
	{
	  delete[] args[i];
	  args[i] = nullptr;
	}

      // returns from exec*() only of error has occurred, sets errno instead of
      // exit status (exit status always -1 on error)
      int exec_errno = errno;
      exit(exec_errno);
    }
  else
    {
      // parent process

      std::string exec_cmdline_str = build_cmdline_as_string(cmdline_args);

      // wait for child to complete
      int child_exit_status = -1;
      int waitpid_exit_status = waitpid(pid, &child_exit_status, 0);
      if (waitpid_exit_status != pid)
	{
	  // waitpid error
	  _error->Error(_("Failed to execute:\n  '%s'"),
			exec_cmdline_str.c_str());

	  _error->Error(_("waitpid error: %d: '%s'"),
			errno, strerror(errno));
	  return EXIT_FAILURE;
	}
      else if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) == 0)
	{
	  // execution success!
	  return EXIT_SUCCESS;
	}
      else
	{
	  // execution, problem
	  _error->Error(_("Failed to execute:\n  '%s'"),
			exec_cmdline_str.c_str());

	  if (WIFEXITED(child_exit_status))
	    {
	      _error->Error(_("The process or trying to execute it exited with status/errno: %d"),
			    WEXITSTATUS(child_exit_status));
	    }
	  else if (WIFSIGNALED(child_exit_status))
	    {
	      _error->Error(_("Killed by signal: %d"),
			    WTERMSIG(child_exit_status));
	    }
	  else
	    {
	      _error->Error(_("Exited with unknown problem"));
	    }

	  return EXIT_FAILURE;
	}
    }
}


int cmdline_apt_proxy(int argc, char* argv[])
{
  if (argc <= 2)
    {
      fprintf(stdout, _("Error: not enough arguments: %d\n"), argc);
      return EXIT_FAILURE;
    }
  else if (argc > 1024)
    {
      // sanity check, too many can complicate things handling string copies,
      // too many arguments for exec, etc.
      fprintf(stdout, _("Error: too many arguments: %d\n"), argc);
      return EXIT_FAILURE;
    }
  else
    {
      std::vector<std::string> exec_cmdline_args;

      exec_cmdline_args.push_back("apt"); // argv[0] is command

      // check for subcommands supported: source, download, ...
      bool is_subcommand_supported = false;
      std::vector<std::string> subcommands_supported = { "download", "showsrc", "source" };

      // add the arguments
      for (int argi = 1; argi < argc; ++argi)
	{
	  std::string arg = argv[argi];

	  if (aptitude::matching::is_pattern(arg))
	    {
	      fprintf(stdout, _("Error: pattern not supported by this subcommand: '%s'\n"), arg.c_str());
	      return EXIT_FAILURE;
	    }

	  if (!is_subcommand_supported)
	    {
	      for (const auto& subcommand_supported : subcommands_supported)
		{
		  if (arg == subcommand_supported)
		    {
		      is_subcommand_supported = true;
		      break;
		    }
		}
	    }

	  exec_cmdline_args.push_back(arg);
	}

      if (!is_subcommand_supported)
	{
	  fprintf(stdout, _("Supported subcommand not found, needs one of: "));
	  for (const auto& subcommand_supported : subcommands_supported)
	    {
	      fprintf(stdout, "%s ", subcommand_supported.c_str());
	    }
	  fprintf(stdout, "\n");
	  return EXIT_FAILURE;
	}

      // build the command line as a string
      std::string exec_cmdline_str = build_cmdline_as_string(exec_cmdline_args);

      // fprintf(stdout, "DEBUG: '%s %s'\n", argv[0], argv[1]);
      fprintf(stdout, _("Executing '%s'\n\n"), exec_cmdline_str.c_str());

      int status = exec_command(exec_cmdline_args);

      if (_error->PendingError())
	{
	  _error->DumpErrors();
	}

      return status;
    }
}
