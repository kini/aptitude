// cmdline_search.cc
//
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
#include "cmdline_search.h"

#include "cmdline_common.h"
#include "cmdline_progress_display.h"
#include "cmdline_search_progress.h"
#include "cmdline_util.h"
#include "terminal.h"
#include "text_progress.h"

#include <aptitude.h>
#include <load_sortpolicy.h>
#include <loggers.h>
#include <pkg_columnizer.h>
#include <pkg_item.h>
#include <pkg_sortpolicy.h>

#include <generic/apt/apt.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>
#include <generic/apt/matching/serialize.h>
#include <generic/util/progress_info.h>
#include <generic/util/throttle.h>
#include <generic/views/progress.h>

#include <cwidget/config/column_definition.h>
#include <cwidget/generic/util/transcode.h>


// System includes:
#include <apt-pkg/error.h>
#include <apt-pkg/strutl.h>

#include <boost/format.hpp>

#include <sigc++/bind.h>

#include <algorithm>
#include <memory>

using namespace std;
namespace cw = cwidget;
using aptitude::Loggers;
using aptitude::cmdline::create_search_progress;
using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;
using aptitude::cmdline::terminal_locale;
using aptitude::cmdline::terminal_metrics;
using aptitude::cmdline::terminal_output;
using aptitude::matching::serialize_pattern;
using aptitude::util::create_throttle;
using aptitude::util::progress_info;
using aptitude::util::progress_type_bar;
using aptitude::util::progress_type_none;
using aptitude::util::progress_type_pulse;
using aptitude::util::throttle;
using aptitude::views::progress;
using boost::format;
using cwidget::util::ref_ptr;
using cwidget::util::transcode;
using namespace aptitude::matching;
using namespace cwidget::config;

namespace
{
  int do_search_packages(const std::vector<ref_ptr<pattern> > &patterns,
                         pkg_sortpolicy *sort_policy,
                         const column_definition_list &columns,
                         int width,
                         bool disable_columns,
                         bool debug,
                         const std::shared_ptr<terminal_locale> &term_locale,
                         const std::shared_ptr<terminal_metrics> &term_metrics,
                         const std::shared_ptr<terminal_output> &term_output)
  {
    typedef std::vector<std::pair<pkgCache::PkgIterator, ref_ptr<structural_match> > >
      results_list;

    const std::shared_ptr<progress> search_progress_display =
      create_progress_display(term_locale, term_metrics, term_output);
    const std::shared_ptr<throttle> search_progress_throttle =
      create_throttle();

    results_list output;
    ref_ptr<search_cache> search_info(search_cache::create());
    for(std::vector<ref_ptr<pattern> >::const_iterator pIt = patterns.begin();
        pIt != patterns.end(); ++pIt)
      {
        const std::shared_ptr<progress> search_progress =
          create_search_progress(serialize_pattern(*pIt),
                                 search_progress_display,
                                 search_progress_throttle);

        // Q: should I just wrap an ?or around them all?
        aptitude::matching::search(*pIt,
                                   search_info,
                                   output,
                                   *apt_cache_file,
                                   *apt_package_records,
                                   debug,
                                   sigc::mem_fun(*search_progress,
                                                 &progress::set_progress));
      }

    search_progress_display->done();

    int exit_status = 0;

    aptitude::cmdline::on_apt_errors_print_and_die();

    if (output.empty())
      {
	exit_status = 1;
      }
    else
      {
	std::sort(output.begin(), output.end(),
		  aptitude::cmdline::package_results_lt(sort_policy));

	output.erase(std::unique(output.begin(), output.end(),
				 aptitude::cmdline::package_results_eq()),
		     output.end());

	for(results_list::const_iterator it = output.begin(); it != output.end(); ++it)
	  {
	    column_parameters *p =
	      new aptitude::cmdline::search_result_column_parameters(it->second);

	    // get candidate or, if does not exist, current version
	    auto ver_it = get_candidate_version(it->first);
	    if (ver_it.end())
	      {
		ver_it = it->first.CurrentVer();
	      }

	    pkg_item::pkg_columnizer columnizer(it->first,
						ver_it,
						columns,
						0);

	    std::wstring line;
	    if (disable_columns)
	      line = aptitude::cmdline::de_columnize(columns, columnizer, *p);
	    else
	      line = columnizer.layout_columns(width, *p);
	    printf("%ls\n", line.c_str());

	    // Note that this deletes the whole result, so we can't re-use
	    // the list.
	    delete p;
	  }
      }

    return exit_status;
  }
}

// FIXME: apt-cache does lots of tricks to make this fast.  Should I?
int cmdline_search(int argc, char *argv[], const char *status_fname,
		   string display_format, string width_cfg, string sort,
		   bool disable_columns, bool debug)
{
  std::shared_ptr<terminal_io> term = create_terminal();

  pkg_item::pkg_columnizer::setup_columns();

  pkg_sortpolicy *s=parse_sortpolicy(sort);

  if(!s)
    {
      _error->DumpErrors();
      return -1;
    }

  _error->DumpErrors();

  // do not truncate to 80 cols on redirections, pipes, etc -- see #445206,
  // #496728, #815690.  Either use explicit width set by the
  // command-line/config, or disable columns.
  int explicit_width = -1;
  if (!width_cfg.empty())
    {
      explicit_width = std::stoi(width_cfg);
    }
  if (!term->output_is_a_terminal() && !(explicit_width > 0))
    {
      disable_columns = true;
    }
  int width = (explicit_width > 0) ? explicit_width : term->get_screen_width();

  if(!disable_columns && !pkg_item::pkg_columnizer::check_valid_display_format(display_format, PACKAGE "::CmdLine::Package-Display-Format" " (or -F)"))
    {
      _error->DumpErrors();
      return -1;
    }

  wstring wdisplay_format;

  if(!cw::util::transcode(display_format.c_str(), wdisplay_format))
    {
      _error->DumpErrors();
      fprintf(stderr, _("iconv of %s failed.\n"), display_format.c_str());
      return -1;
    }

  std::unique_ptr<column_definition_list> columns;
  columns.reset(parse_columns(wdisplay_format,
                              pkg_item::pkg_columnizer::parse_column_type,
                              pkg_item::pkg_columnizer::defaults));

  if(columns.get() == NULL)
    {
      fprintf(stderr, _("%s: Could not parse column definitions: '%ls'\n"), "search", wdisplay_format.c_str());
      _error->DumpErrors();
      return -1;
    }

  if(argc<=1)
    {
      fprintf(stderr, _("search: You must provide at least one search term\n"));
      return -1;
    }

  std::shared_ptr<OpProgress> progress =
    make_text_progress(true, term, term, term);

  bool operation_needs_lock = false;
  apt_init(progress.get(), true, operation_needs_lock, status_fname);

  aptitude::cmdline::on_apt_errors_print_and_die();

  vector<ref_ptr<pattern> > matchers;

  for(int i=1; i<argc; ++i)
    {
      const char * const arg = argv[i];

      ref_ptr<pattern> m = parse(arg);
      if(!m.valid())
        {
          _error->DumpErrors();

          return -1;
        }

      matchers.push_back(m);
    }

  return do_search_packages(matchers,
                            s,
                            *columns,
                            width,
                            disable_columns,
                            debug,
                            term,
                            term,
                            term);
}
