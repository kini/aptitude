// Copyright (C) 2016-2017 Manuel A. Fernandez Montecelo
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

#include "cmdline_mark.h"

#include "terminal.h"
#include "text_progress.h"
#include "cmdline_util.h"

#include "aptitude.h"
#include "generic/apt/matching/match.h"
#include "generic/apt/matching/parse.h"
#include "generic/apt/matching/pattern.h"
#include "generic/apt/apt.h"
#include "generic/apt/config_signal.h"
#include "generic/util/util.h"

#include <aptitude.h>

#include <string>
#include <vector>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;


int cmdline_mark(int argc, char *argv[], const char* status_fname, bool simulate, int verbose)
{
  aptitude::cmdline::on_apt_errors_print_and_die();

  const std::shared_ptr<terminal_io> term = create_terminal();
  std::shared_ptr<OpProgress> progress = make_text_progress(false, term, term, term);

  bool operation_needs_lock = true;
  apt_init(progress.get(), false, operation_needs_lock, status_fname);
  aptitude::cmdline::on_apt_errors_print_and_die();

  // In case we aren't root.
  if (!simulate)
    apt_cache_file->GainLock();
  else
    apt_cache_file->ReleaseLock();
  aptitude::cmdline::on_apt_errors_print_and_die();

  bool all_ok = true;

  // Instantiate action group, so all changes get collected and actions happen
  // only once.  Otherwise, every change triggers a reevaluation in
  // aptitudeDepCache::end_action_group() of what has changed ("mark and sweep",
  // duplicate of cache, triggers of packages state changes... etc), for every
  // package marked as "keep" (all of them), and thus causing problems such as
  // #842707 -- "keep-all hangs forever"
  aptitudeDepCache::action_group group(*apt_cache_file, NULL);

  if (std::string(argv[0]) == "keep-all")
    {
      if (argc != 1)
	{
	  std::cerr << _("Unexpected argument following \"keep-all\"") << std::endl;
	  return -1;
	}

      if (verbose > 0)
	printf(_("Marking all packages as keep (%s)\n"), argv[0]);

      if (simulate)
	{
	  printf(_("Would mark all packages as keep\n"));
	}
      else
	{
	  for (pkgCache::PkgIterator it = (*apt_cache_file)->PkgBegin(); !it.end(); ++it)
	    {
	      (*apt_cache_file)->mark_keep(it, is_auto_installed(it), false, NULL);
	    }
	}
    }
  else if (std::string(argv[0]) == "forbid-version")
    {
      if (argc < 2)
	{
	  std::cerr << _("Subcommand needs arguments: ") << argv[0] << std::endl;
	  return -1;
	}

      std::vector<std::string> pkg_args;

      int argc_start = 1;
      for (int i = argc_start; i < argc; ++i)
	{
	  std::vector<pkgCache::PkgIterator> pkgs_from_args = aptitude::cmdline::get_packages_from_string(argv[i]);

	  if (pkgs_from_args.empty())
	    {
	      // problem parsing command line or finding packages

	      // check whether argument is pkgname=version
	      cmdline_version_source source = cmdline_version_cand;
	      string sourcestr, package;
	      if (cmdline_parse_source(argv[i], source, package, sourcestr))
		{
		  pkg_args.push_back(argv[i]);
		}
	      else
		{
		  all_ok = false;

		  int quiet = aptcfg->FindI("quiet", 0);
		  if (quiet == 0)
		    {
		      if (aptitude::matching::is_pattern(argv[i]))
			std::cerr << ssprintf(_("No packages match pattern \"%s\""), argv[i]) << std::endl;
		      else
			std::cerr << ssprintf(_("No such package \"%s\""), argv[i]) << std::endl;
		    }
		}
	    }
	  else
	    {
	      // append packages
	      for (const auto& it : pkgs_from_args)
		{
		  pkg_args.push_back(it.FullName());
		}
	    }
	}

      for (std::string pkgstr : pkg_args)
	{
	  // get version to apply
	  cmdline_version_source source = cmdline_version_cand;
	  string sourcestr, package;
	  if (!cmdline_parse_source(pkgstr, source, package, sourcestr))
	    {
	      printf(_("Could not parse version for %s: %s\n"), argv[0], pkgstr.c_str());
	      return -1;
	    }

	  pkgCache::PkgIterator pkg = (*apt_cache_file)->FindPkg(package);
	  if (pkg.end())
	    {
	      printf(_("Package not found: %s, from %s\n"), package.c_str(), pkgstr.c_str());
	      return -1;
	    }

	  pkgDepCache::StateCache& pkg_state((*apt_cache_file)[pkg]);

	  if (source != cmdline_version_cand)
	    {
	      if (verbose > 0)
		printf(_("Marking version %s of package %s as forbidden\n"), sourcestr.c_str(), pkg.FullName(true).c_str());

	      if (simulate)
		{
		  printf(_("Would mark package %s as forbid to upgrade to version %s\n"), pkg.FullName(true).c_str(), sourcestr.c_str());
		}
	      else
		{
		  (*apt_cache_file)->forbid_upgrade(pkg, sourcestr, NULL);
		}
	    }
	  else
	    {
	      pkgCache::VerIterator curver = pkg.CurrentVer();
	      pkgCache::VerIterator candver = pkg_state.CandidateVerIter(*apt_cache_file);
	      if (!curver.end() && !candver.end() && (curver != candver))
		{
		  if (verbose > 0)
		    printf(_("Marking version %s of package %s as forbidden\n"), candver.VerStr(), pkg.FullName(true).c_str());

		  if (simulate)
		    {
		      printf(_("Would mark package %s as forbid to upgrade to version %s\n"), pkg.FullName(true).c_str(), candver.VerStr());
		    }
		  else
		    {
		      (*apt_cache_file)->forbid_upgrade(pkg, candver.VerStr(), NULL);
		    }
		}
	      else if (pkg.CurrentVer().end())
		printf(_("Package %s is not installed, cannot forbid an upgrade\n"),
		       pkg.FullName(true).c_str());
	      else if (!pkg_state.Upgradable())
		printf(_("Package %s is not upgradable, cannot forbid an upgrade\n"),
		       pkg.FullName(true).c_str());
	    }
	}
    }
  else
    {
      if (argc < 2)
	{
	  std::cerr << _("Subcommand needs arguments: ") << argv[0] << std::endl;
	  return -1;
	}

      std::vector<std::string> args;
      int argc_start = 1;
      for (int i = argc_start; i < argc; ++i)
	{
	  args.push_back(argv[i]);
	}

      std::vector<pkgCache::PkgIterator> pkg_its = aptitude::cmdline::get_packages_from_set_of_strings(args, all_ok);

      // convert action
      cmdline_pkgaction_type subcommand;
      if (std::string(argv[0]) == "markauto")
	subcommand = cmdline_markauto;
      else if (std::string(argv[0]) == "unmarkauto")
	subcommand = cmdline_unmarkauto;
      else if (std::string(argv[0]) == "hold")
	subcommand = cmdline_hold;
      else if (std::string(argv[0]) == "unhold")
	subcommand = cmdline_unhold;
      else if (std::string(argv[0]) == "keep")
	subcommand = cmdline_keep;
      else
	{
	  std::cerr << _("Subcommand unknown/unhandled: ") << argv[0] << std::endl;
	  return -1;
	}

      // process
      for (const auto& pkg : pkg_its)
	{
	  if (verbose > 0)
	    printf(_("Marking package %s (%s)\n"), pkg.FullName(true).c_str(), argv[0]);

	  switch (subcommand)
	    {
	    case cmdline_markauto:
	    case cmdline_unmarkauto:
	      {
		if (pkg.CurrentVer().end())
		  printf(_("Package %s is not installed, cannot be marked/unmarked as automatically installed\n"),
			 pkg.FullName(true).c_str());

		bool auto_value = (subcommand == cmdline_markauto) ? true : false;
		if (simulate)
		  {
		    printf(_("Would mark package %s as auto-installed=%d\n"), pkg.FullName(true).c_str(), auto_value);
		  }
		else
		  {
		    (*apt_cache_file)->mark_auto_installed(pkg, auto_value, NULL);
		  }
	      }
	      break;
	    case cmdline_hold:
	      {
		if (simulate)
		  {
		    printf(_("Would mark package %s as hold\n"), pkg.FullName(true).c_str());
		  }
		else
		  {
		    (*apt_cache_file)->mark_keep(pkg, is_auto_installed(pkg), true, NULL);
		  }
	      }
	      break;
	    case cmdline_unhold:
	    case cmdline_keep:
	      {
		if (simulate)
		  {
		    printf(_("Would mark package %s as keep (or unhold)\n"), pkg.FullName(true).c_str());
		  }
		else
		  {
		    (*apt_cache_file)->mark_keep(pkg, is_auto_installed(pkg), false, NULL);
		  }
	      }
	      break;
	    default:
	      std::cerr << _("Subcommand unknown/unhandled: ") << argv[0] << std::endl;
	      return -1;
	      break;
	    }
	}
    }

  // if the was some problem, stop here
  if (!all_ok)
    return -1;
  aptitude::cmdline::on_apt_errors_print_and_die();

  (*apt_cache_file)->save_selection_list(progress.get());

  aptitude::cmdline::on_apt_errors_print_and_die();

  return (all_ok ? 0 : -1);
}
