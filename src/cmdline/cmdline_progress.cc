/** \file cmdline_progress.cc */    // -*-c++-*-

// Copyright (C) 2010 Daniel Burrows
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
#include "cmdline_progress.h"

#include "cmdline_download_progress_display.h"
#include "terminal.h"
#include "transient_message.h"

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/download_signal_log.h>

#include <generic/controllers/acquire_download_progress.h>

#include <generic/util/util.h>

#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/functors/ptr_fun.h>

using aptitude::cmdline::download_status_display;
using aptitude::controllers::acquire_download_progress;
using aptitude::controllers::create_acquire_download_progress;

namespace aptitude
{
  namespace cmdline
  {
    std::pair<download_signal_log *, std::shared_ptr<acquire_download_progress> >
    create_cmdline_download_progress(const std::shared_ptr<terminal_input> &term_input,
                                     const std::shared_ptr<terminal_locale> &term_locale,
                                     const std::shared_ptr<terminal_metrics> &term_metrics,
                                     const std::shared_ptr<terminal_output> &term_output)
    {
      download_signal_log * const log = new download_signal_log;

      const int quiet = aptcfg->FindI("Quiet", 0);
      const bool display_messages = quiet <= 1;
      const bool hide_status = (quiet > 0) || aptitude::util::is_dumb_terminal();

      const std::shared_ptr<transient_message> message =
        create_transient_message(term_locale, term_metrics, term_output);

      const std::shared_ptr<download_status_display> download_status =
        create_cmdline_download_status_display(message,
                                               term_locale,
                                               term_metrics,
                                               hide_status);

      const std::shared_ptr<views::download_progress> download_progress =
        create_download_progress_display(message,
                                         download_status,
                                         term_input,
                                         display_messages);

      const std::shared_ptr<acquire_download_progress> controller =
        create_acquire_download_progress(log, download_progress);

      return std::make_pair(log, controller);
    }
  }
}

