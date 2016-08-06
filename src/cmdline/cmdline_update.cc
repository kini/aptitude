// cmdline_update.cc
//
//   Copyright (C) 2004-2008, 2010 Daniel Burrows
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
#include "cmdline_update.h"

#include "cmdline_util.h"

#include "terminal.h"

#include <aptitude.h>

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/download_update_manager.h>


// System includes:
#include <apt-pkg/error.h>

#include <memory>
#include <stdio.h>

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::terminal_io;

void print_autoclean_msg()
{
  printf("%s\n", _("Deleting obsolete downloaded files"));
}

int cmdline_update(int argc, char *argv[], int verbose)
{
  std::shared_ptr<terminal_io> term = create_terminal();

  if(argc!=1)
    {
      fprintf(stderr, _("E: The update command takes no arguments\n"));
      return -1;
    }

  // Don't exit if there's an error: it probably means that there
  // was a problem loading the package lists, so go ahead and try to
  // download new ones.
  _error->DumpErrors();

  download_update_manager m;
  m.pre_autoclean_hook.connect(sigc::ptr_fun(print_autoclean_msg));
  int rval =
    (cmdline_do_download(&m, verbose, term, term, term, term)
     == download_manager::success ? 0 : -1);

  aptitude::cmdline::on_apt_errors_print_and_die();

  return rval;
}

