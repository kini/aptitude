// cmdline_changelog.cc
//
// Copyright (C) 2004, 2010 Daniel Burrows
// Copyright (C) 2014-2016 Manuel A. Fernandez Montecelo
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
#include "cmdline_changelog.h"

#include "cmdline_common.h"
#include "cmdline_main_loop.h"
#include "cmdline_progress.h"
#include "cmdline_util.h"
#include "terminal.h"

#include <aptitude.h>

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/download_queue.h>
#include <generic/apt/pkg_changelog.h>
#include <generic/util/util.h>

// System includes:
#include <cwidget/generic/util/ssprintf.h>

#include <apt-pkg/error.h>
#include <apt-pkg/metaindex.h>
#include <apt-pkg/progress.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/srcrecords.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <sigc++/adaptors/bind.h>

#include <signal.h>
#include <stdlib.h>

#include <sstream>

using namespace std;

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::terminal_io;
using aptitude::cmdline::terminal_metrics;

namespace fs = boost::filesystem;

namespace
{

  // Temporary hack to display download progress for a single file.
  //
  // Eventually there should be a generic class to track the progress
  // of a group of downloads, but I don't have enough infrastructure
  // for that yet.
  class single_download_progress : public aptitude::download_callbacks
  {
    std::string description;
    bool quiet;
    std::shared_ptr<terminal_metrics> term_metrics;

  public:
    single_download_progress(const std::string &_description,
			     bool _quiet,
                             const std::shared_ptr<terminal_metrics> &_term_metrics)
      : description(_description),
        quiet(_quiet),
        term_metrics(_term_metrics)
    {
    }

    void partial_download(const std::string& filename,
			  unsigned long long currentSize,
			  unsigned long long totalSize)
    {
      // Hack: ripped this code from AcqTextStatus.

      if (quiet)
	return;

      const int screen_width = term_metrics->get_screen_width();

      unsigned long long TotalBytes = totalSize;
      unsigned long long CurrentBytes = currentSize;

      std::ostringstream bufferStream;

      bufferStream << long(double(CurrentBytes * 100.0) / double(TotalBytes));
      bufferStream << "% [" << description.c_str() << "]";

      std::string Buffer(bufferStream.str());

      sigset_t Sigs,OldSigs;
      sigemptyset(&Sigs);
      sigaddset(&Sigs,SIGWINCH);
      sigprocmask(SIG_BLOCK,&Sigs,&OldSigs);

      int Len = (int)Buffer.size();
      if (Len < screen_width)
	Buffer.insert(Buffer.end(), screen_width - Len, ' ');
      std::string BlankLine(screen_width, ' ');
      sigprocmask(SIG_SETMASK,&OldSigs,0);

      // Draw the current status
      if (Buffer.size() == BlankLine.size())
	std::cout << '\r' << Buffer << std::flush;
      else
	std::cout << '\r' << BlankLine << '\r' << Buffer << std::flush;
    }

    void success(const std::string& filename)
    {
      if(!quiet)
	std::cout << "\r"
                  << std::string(term_metrics->get_screen_width(), ' ')
                  << "\r";

      std::cout << _("Get:") << " " << description << std::endl;
    }

    void failure(const std::string &msg)
    {
      if(!quiet)
	std::cout << "\r"
                  << std::string(term_metrics->get_screen_width(), ' ')
                  << "\r";

      std::cout << _("Err") << " " << description << std::endl;
    }
  };

  class changelog_download_callbacks : public single_download_progress
  {
  public:
    // Where to store the changelog file, if any.
    std::string out_changelog_file;

    changelog_download_callbacks(const std::string &short_description,
                                 const std::shared_ptr<terminal_metrics> &term_metrics)
      : single_download_progress(short_description,
				 aptcfg->FindI("Quiet", 0) > 0,
                                 term_metrics)
    {
    }

    void success(const std::string& filename)
    {
      single_download_progress::success(filename);

      out_changelog_file = filename;

      aptitude::cmdline::exit_main();
    }

    void failure(const std::string &msg)
    {
      single_download_progress::failure(msg);

      _error->Error(_("Changelog download failed: %s"), msg.c_str());
      _error->DumpErrors();

      aptitude::cmdline::exit_main();
    }
  };


  /** \brief Get a package's changelog.
   *
   *  \param info Structure with the information needed to get the changelog
   *  \param out_changelog_file  Where to store the resulting file.
   *  \param term_metrics Data about the terminal
   *
   *  @return File path of the changelog, empty if invalid
   *
   *  This routine exits once the download is complete.
   */
  std::string get_changelog(const std::shared_ptr<aptitude::apt::changelog_info>& info,
			    const std::shared_ptr<terminal_metrics> &term_metrics)
  {
    const std::string short_description = cwidget::util::ssprintf(_("Changelog of %s"), info->get_display_name().c_str());

    auto callbacks = std::make_shared<changelog_download_callbacks>(short_description,
								    term_metrics);

    aptitude::apt::get_changelog(info, callbacks, aptitude::cmdline::post_thunk);

    aptitude::cmdline::main_loop();

    std::string changelog_filepath = callbacks->out_changelog_file;
    if (fs::is_regular_file(changelog_filepath))
      return changelog_filepath;
    else
      return "";
  }

  /** \brief Get a package's changelog.
   *
   *  \param ver Version iterator for which to get the changelog
   *  \param term_metrics Data about the terminal
   *
   *  @return File path of the changelog, empty if invalid
   *
   *  This routine exits once the download is complete.
   */
  std::string get_changelog(const pkgCache::VerIterator &ver,
			    const std::shared_ptr<terminal_metrics> &term_metrics)
  {
    std::shared_ptr<aptitude::apt::changelog_info> info =
      aptitude::apt::changelog_info::create(ver);

    return get_changelog(info, term_metrics);
  }

  /** \brief download a source package's changelog.
   *
   *  \param srcpkg the source package name
   *  \param ver the version of the source package
   *  \param section the section of the source package
   *  \param name the name of the package that the user provided
   *              (e.g., the binary package that the changelog command
   *               was executed on)
   *
   *  @return File path of the changelog, empty if invalid
   *
   *  This routine exits once the download is complete.
   */
  std::string get_changelog_from_source(const std::string &srcpkg,
					const std::string &ver,
					const std::string &section,
					const std::string &name,
					const std::shared_ptr<terminal_metrics> &term_metrics)
  {
    std::shared_ptr<aptitude::apt::changelog_info> info =
      aptitude::apt::changelog_info::create(srcpkg, ver, section, name, "");

    return get_changelog(info, term_metrics);
  }

/** Try to find a particular package version without knowing the
 *  section that it occurs in.  The resulting name will be invalid if
 *  no changelog could be found.
 */
std::string changelog_by_version(const std::string &pkg,
				 const std::string &ver,
				 const std::shared_ptr<terminal_metrics> &term_metrics)
{
  // Try forcing the particular version that was
  // selected, using various sections.  FIXME: relies
  // on specialized knowledge about how get_changelog
  // works; in particular, that it only cares whether
  // "section" has a first component.

  std::string filename;

  filename = get_changelog_from_source(pkg, ver, "", pkg, term_metrics);

  if (!fs::is_regular_file(filename))
    {
      filename = get_changelog_from_source(pkg, ver, "contrib/foo", pkg, term_metrics);
    }

  if (!fs::is_regular_file(filename))
    {
      filename = get_changelog_from_source(pkg, ver, "non-free/foo", pkg, term_metrics);
    }

  return filename;
}
}

void do_cmdline_changelog(const vector<string> &packages,
                          const std::shared_ptr<terminal_metrics> &term_metrics)
{
  const char *pager="/usr/bin/sensible-pager";

  if(access("/usr/bin/sensible-pager", X_OK)!=0)
    {
      _error->Warning(_("Can't execute sensible-pager, is this a working Debian system?"));

      pager=getenv("PAGER");

      if(strempty(pager)==true)
	pager="more";
    }

  for(vector<string>::const_iterator i=packages.begin(); i!=packages.end(); ++i)
    {
      // We need to do this because some code (see above) checks
      // PendingError to see whether everything is OK.  In addition,
      // dumping errors means we get sensible error message output
      // (this will be true even if the PendingError check is removed
      // ... which it arguably should be).
      _error->DumpErrors();
      string input=*i;

      cmdline_version_source source;
      string package, sourcestr;

      if(!cmdline_parse_source(input, source, package, sourcestr))
	continue;

      pkgCache::PkgIterator pkg=(*apt_cache_file)->FindPkg(package);

      std::string filename;

      // For real packages/versions, we can do a sanity check on the
      // version and warn the user if it looks like it doesn't have a
      // corresponding source package.
      if(!pkg.end())
	{
	  pkgCache::VerIterator ver=cmdline_find_ver(pkg,
						     source, sourcestr);

	  if (!ver.end())
	    {
	      // check if we know the origin
	      if ( ! aptitude::apt::check_valid_origin(ver) )
		{
		  _error->DumpErrors();
		  continue;
		}

              filename = get_changelog(ver, term_metrics);
	    }
        }

      if (!fs::is_regular_file(filename))
        {
	  aptitude::cmdline::source_package p =
	    aptitude::cmdline::find_source_package(package,
						   source,
						   sourcestr);

	  // Use the source package if one was found; otherwise try to
	  // use an explicit version.
	  if(p.valid())
	    {
	      get_changelog_from_source(p.get_package(),
					p.get_version(),
					p.get_section(),
					p.get_package(),
                                        term_metrics);
	    }
	  else
	    {
	      // We couldn't find a real or source package with the
	      // given name and version.
	      //
	      // If the user didn't specify a version or selected a
	      // candidate and we couldn't find anything, we have no
	      // recourse.  But if they passed a version number, we
	      // can fall back to just blindly guessing that the
	      // version exists.

              // TODO: We can try using the path "current" rather than
              // "SOURCE_VERSION" in the default case, because this is
              // supported on http://packages.debian.org/changelogs to
              // fetch details for the latest version.

	      switch(source)
		{
		case cmdline_version_cand:
		  break;

		case cmdline_version_curr_or_cand:
		  break;

		case cmdline_version_archive:
		  break;

		case cmdline_version_version:
		  filename = changelog_by_version(package, sourcestr, term_metrics);
		  break;
		}
	    }
	}

      if (!fs::is_regular_file(filename))
	_error->Error(_("Couldn't find a changelog for %s"), input.c_str());
      else
	// Run the user's pager.
	if (system((string(pager) + " " + filename).c_str()) != 0)
          _error->Error(_("Couldn't run pager %s"), pager);
    }
}

// TODO: fetch them all in one go.
int cmdline_changelog(int argc, char *argv[])
{
  std::shared_ptr<terminal_io> term = create_terminal();

  _error->DumpErrors();

  OpProgress progress;
  bool operation_needs_lock = false;
  apt_init(&progress, false, operation_needs_lock, nullptr);

  if (_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  vector<string> packages;
  for(int i=1; i<argc; ++i)
    packages.push_back(argv[i]);

  do_cmdline_changelog(packages, term);

  if (_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  return 0;
}
