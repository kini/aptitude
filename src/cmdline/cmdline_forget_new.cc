// cmdline_forget_new.cc
//
// Copyright (C) 2004, 2010 Daniel Burrows
// Copyright (C) 2015-2016 Manuel A. Fernandez Montecelo
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


// Local includes:
#include "cmdline_forget_new.h"

#include "terminal.h"
#include "text_progress.h"

#include <aptitude.h>

#include "cmdline_util.h"
#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/matching/pattern.h>
#include <generic/util/util.h>


// System includes:
#include <apt-pkg/error.h>

#include <memory>

#include <stdio.h>

using namespace std;

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;


int cmdline_forget_new(int argc, char *argv[],
		       const char *status_fname, bool simulate)
{
  const std::shared_ptr<terminal_io> term = create_terminal();

  aptitude::cmdline::on_apt_errors_print_and_die();

  std::shared_ptr<OpProgress> progress = make_text_progress(false, term, term, term);
  bool operation_needs_lock = true;
  apt_init(progress.get(), false, operation_needs_lock, status_fname);

  aptitude::cmdline::on_apt_errors_print_and_die();

  // In case we aren't root.
  if(!simulate)
    apt_cache_file->GainLock();
  else
    apt_cache_file->ReleaseLock();

  aptitude::cmdline::on_apt_errors_print_and_die();

  bool all_ok = true;

  if(simulate)
    printf(_("Would forget what packages are new\n"));
  else
    {
      std::vector<std::string> args;
      int argc_start = 1;
      for (int i = argc_start; i < argc; ++i)
	{
	  args.push_back(argv[i]);
	}

      std::vector<pkgCache::PkgIterator> pkg_its = aptitude::cmdline::get_packages_from_set_of_strings(args, all_ok);

      // if the was some problem, stop here
      if (!all_ok)
	return -1;

      // whether packages/patterns were given, or if it should forget all new
      if (argc == 1)
	(*apt_cache_file)->forget_new(NULL);
      else
	(*apt_cache_file)->forget_new(NULL, pkg_its);

      (*apt_cache_file)->save_selection_list(progress.get());

      aptitude::cmdline::on_apt_errors_print_and_die();
    }

  return (all_ok ? 0 : -1);
}
