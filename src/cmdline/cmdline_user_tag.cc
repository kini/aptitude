// cmdline_user_tag.cc
//
//   Copyright (C) 2008-2010 Daniel Burrows
//   Copyright (C) 2015-2016 Manuel A. Fernandez Montecelo
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; see the file COPYING.  If not, write to
//   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//   Boston, MA 02110-1301, USA.


// Local includes:
#include "cmdline_user_tag.h"

#include "cmdline_util.h"
#include "terminal.h"
#include "text_progress.h"

#include <aptitude.h>

#include <apt-pkg/error.h>

#include <generic/apt/apt.h>
#include <generic/apt/aptcache.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>
#include <generic/util/util.h>

#include <stdio.h>
#include <string.h>

#include <memory>

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;

namespace aptitude
{
  namespace cmdline
  {
    namespace
    {
      enum user_tag_action { action_add, action_remove };

      void print_result(user_tag_action act,
			const std::string& tag,
			const pkgCache::PkgIterator& pkg,
			int verbose,
			bool operation_result)
      {
	const char* action_str = (act == action_add) ? _("add") : _("remove");
	if (operation_result && verbose>0)
	  {
	    fprintf(stderr, _("Applied user-tag operation '%s' '%s' to: %s\n"),
		    action_str, tag.c_str(), pkg.FullName(true).c_str());
	  }
	else if (!operation_result)
	  {
	    fprintf(stderr, _("Failed to apply user-tag operation '%s' '%s' to: %s\n"),
		    action_str, tag.c_str(), pkg.FullName(true).c_str());
	  }
      }

      bool do_user_tag(user_tag_action act,
		       const std::string &tag,
		       const pkgCache::PkgIterator &pkg,
		       int verbose)
      {
	bool op_result = false;
	switch(act)
	  {
	  case action_add:
	    op_result = (*apt_cache_file)->attach_user_tag(pkg, tag, NULL);
	    break;
	  case action_remove:
	    op_result = (*apt_cache_file)->detach_user_tag(pkg, tag, NULL);
	    break;
	  default:
	    fprintf(stderr, "Internal error: bad user tag action %d", act);
	    return false;
	    break;
	  }

	print_result(act, tag, pkg, verbose, op_result);
	return op_result;
      }
    }

    int cmdline_user_tag(int argc, char *argv[], int quiet, int verbose)
    {
      const std::shared_ptr<terminal_io> term = create_terminal();

      user_tag_action action = (user_tag_action)-1;
      if (strcmp(argv[0], "add-user-tag") == 0)
	{
	  action = action_add;
	}
      else if (strcmp(argv[0], "remove-user-tag") == 0)
	{
	  action = action_remove;
	}
      else
	{
	  fprintf(stderr, "Internal error: cmdline_user_tag encountered an unknown command name \"%s\"\n",
		  argv[0]);
	  return -1;
	}

      if (argc < 3)
	{
	  fprintf(stderr,
		  _("%s: too few arguments; expected at least a tag name and a package or pattern\n"),
		  argv[0]);
	  return -1;
	}

      aptitude::cmdline::on_apt_errors_print_and_die();

      OpProgress progress;
      bool operation_needs_lock = true;
      apt_init(&progress, true, operation_needs_lock, nullptr);

      aptitude::cmdline::on_apt_errors_print_and_die();

      std::string tag = argv[1];
      int argc_start = 2;

      bool all_ok = true;
      for (int i = argc_start; i < argc; ++i)
	{
	  if(!aptitude::matching::is_pattern(argv[i]))
	    {
	      pkgCache::PkgIterator pkg = (*apt_cache_file)->FindPkg(argv[i]);
	      if(pkg.end())
		{
		  if(quiet == 0)
                    std::cerr << ssprintf(_("No such package \"%s\""), argv[i])
                              << std::endl;
		  all_ok = false;
		}
	      else
		{
		  bool result = do_user_tag(action, tag, pkg, verbose);
		  if (!result)
		    {
		      all_ok = false;
		    }
		}
	    }
	  else
	    {
	      using namespace aptitude::matching;
	      using cwidget::util::ref_ptr;

	      ref_ptr<pattern> p(parse(argv[i]));

	      if(!p.valid())
		{
		  _error->DumpErrors();
		  all_ok = false;
		}
	      else
		{
		  std::vector<std::pair<pkgCache::PkgIterator, ref_ptr<structural_match> > > matches;
		  ref_ptr<search_cache> search_info(search_cache::create());
		  search(p, search_info,
			 matches,
			 *apt_cache_file,
			 *apt_package_records);

		  for(std::vector<std::pair<pkgCache::PkgIterator, ref_ptr<structural_match> > >::const_iterator
			it = matches.begin(); it != matches.end(); ++it)
		    {
		      const pkgCache::PkgIterator& pkg = it->first;
		      bool result = do_user_tag(action, tag, pkg, verbose);
		      if (!result)
			{
			  all_ok = false;
			}
		    }
		}
	    }
	}

      // check for root/permissions -- needs to lock before
      // (*apt_cache_file)->save_selection_list() below, see #725272
      {
	bool could_lock = apt_cache_file->GainLock();

	if (!could_lock || _error->PendingError())
	  {
	    _error->DumpErrors();
	    return -1;
	  }
      }

      int exit_status = 0;

      if (!(*apt_cache_file)->save_selection_list(&progress))
	{
	  exit_status = 1;
	}

      if (!all_ok)
	{
	  exit_status = 2;
	}

      if (!_error->empty(GlobalError::MsgType::NOTICE))
	{
	  _error->DumpErrors(GlobalError::MsgType::NOTICE);
	}

      return exit_status;
    }
  }
}
