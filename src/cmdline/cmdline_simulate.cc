// cmdline_simulate.cc
//
// Copyright (C) 2004, 2010 Daniel Burrows
// Copyright (C) 2015 Manuel A. Fernandez Montecelo
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
#include "cmdline_simulate.h"

#include "cmdline_common.h"
#include "cmdline_prompt.h"
#include "terminal.h"

#include <aptitude.h>

#include <generic/apt/apt.h>


// System includes:
#include <apt-pkg/algorithms.h>
#include <apt-pkg/error.h>

#include <stdio.h>

using aptitude::cmdline::terminal_metrics;

int cmdline_simulate(bool as_upgrade,
		     pkgset &to_install, pkgset &to_hold, pkgset &to_remove,
		     pkgset &to_purge,
		     bool showvers, bool showdeps,
		     bool showsize, bool showwhy,
		     bool always_prompt, int verbose,
		     bool assume_yes, bool force_no_change,
		     pkgPolicy &policy, bool arch_only, bool download_only,
                     const std::shared_ptr<terminal_metrics> &term_metrics)
{
  bool simulate_only = true;

  if(!cmdline_do_prompt(as_upgrade,
			to_install, to_hold, to_remove, to_purge,
			showvers, showdeps, showsize, showwhy,
			always_prompt, verbose,
			assume_yes, force_no_change,
			policy, arch_only, download_only, simulate_only,
                        term_metrics))
    {
      printf(_("Abort.\n"));
      return 1;
    }

  if(verbose==0)
    {
      printf(_("Would download/install/remove packages.\n"));
      return 0;
    }

  pkgSimulate PM(*apt_cache_file);
  pkgPackageManager::OrderResult Res = PM.DoInstall(nullptr);

  if(Res==pkgPackageManager::Failed)
    return -1;
  else if(Res!=pkgPackageManager::Completed)
    {
      _error->Error(_("Internal Error, Ordering didn't finish"));
      return -1;
    }
  else
    return 0;
}
