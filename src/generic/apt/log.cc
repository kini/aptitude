// log.cc
//
//   Copyright (C) 2005-2008, 2010 Daniel Burrows
//   Copyright (C) 2014-2016 Manuel A. Fernandez Montecelo
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

#include "log.h"

#include "apt.h"
#include "config_signal.h"

#include <aptitude.h>

#include <generic/util/util.h>

#include <apt-pkg/error.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/strutl.h>

#include <errno.h>
#include <stdio.h>

#include <algorithm>
#include <locale>

using namespace std;

typedef std::pair<pkgCache::PkgIterator, pkg_action_state> logitem;
typedef std::vector<logitem> loglist;

bool do_log(const string &log,
	    const loglist &changed_packages,
	    bool localize)
{
  FILE *f = NULL;

  if(log[0] == '|')
    f = popen(log.c_str()+1, "w");
  else
    f = fopen(log.c_str(), "a");

  if(!f)
    {
      _error->Errno("do_log", _("Unable to open %s to log actions"), log.c_str());

      return false;
    }

  // see #357828, #596221 -- check if we should use the "classic" locale
  //
  // it can throw an exception if the locale defined in the environment is not
  // valid
  try {
    if (!localize && (std::locale("") != std::locale::classic()))
      {
	std::locale::global(std::locale::classic());
      }
  } catch (...) {
    // ignore
  }

  time_t curtime = time(NULL);
  tm ltime;
  string timestr;

  if(localtime_r(&curtime, &ltime) != NULL)
    // TRANSLATORS: This is a date and time format.  See strftime(3).
    timestr = sstrftime(_("%a, %b %e %Y %T %z"), &ltime);
  else
    timestr = ssprintf(_("Error generating local time (%s)"),
		       sstrerror(errno).c_str());

  fprintf(f, "Aptitude " VERSION ": %s\n%s\n\n",
	  _("log report"), timestr.c_str());
  fprintf(f, _("  IMPORTANT: this log only lists intended actions; actions which fail\n  due to dpkg problems may not be completed.\n\n"));
  fprintf(f, _("Will install %li packages, and remove %li packages.\n"),
	  (*apt_cache_file)->InstCount(), (*apt_cache_file)->DelCount());

  if((*apt_cache_file)->UsrSize() > 0)
    fprintf(f, _("%sB of disk space will be used\n"),
	    SizeToStr((*apt_cache_file)->UsrSize()).c_str());
  else if((*apt_cache_file)->UsrSize() < 0)
    fprintf(f, _("%sB of disk space will be freed\n"),
	    SizeToStr((*apt_cache_file)->UsrSize()).c_str());

  fprintf(f, "========================================\n");

  for(loglist::const_iterator i = changed_packages.begin();
      i != changed_packages.end(); ++i)
    {
      std::string action_tag;
      switch (i->second)
	{
	case pkg_install:       action_tag = _("INSTALL"); break;
	case pkg_remove:        action_tag = _("REMOVE"); break;
	case pkg_upgrade:       action_tag = _("UPGRADE"); break;
	case pkg_downgrade:     action_tag = _("DOWNGRADE"); break;
	case pkg_reinstall:     action_tag = _("REINSTALL"); break;
	case pkg_hold:          action_tag = _("HOLD"); break;
	case pkg_broken:        action_tag = _("BROKEN"); break;
	case pkg_unused_remove:	action_tag = _("REMOVE, NOT USED"); break;
	case pkg_auto_remove:   action_tag = _("REMOVE, DEPENDENCIES"); break;
	case pkg_auto_install:  action_tag = _("INSTALL, DEPENDENCIES"); break;
	case pkg_auto_hold:     action_tag = _("HOLD, DEPENDENCIES"); break;
	case pkg_unconfigured:  action_tag = _("UNCONFIGURED"); break;
	default:                action_tag = _("????????"); break;
	}

      // distinguish between remove and purge, see #705612
      //
      // The ultimate cause is that "pkg_action_state" doesn't distinguish
      // between these and doesn't have it as "variation" (like
      // "pkg_auto_remove", for example it could be "pkg_remove_purge"), so we
      // have to do this extra check.
      //
      // "pkg_action_state" is a quite central structure so it cannot be changed
      // lightly, there are multiple implications.
      if (i->second == pkg_remove &&
	  (*apt_cache_file)->get_ext_state(i->first).selection_state == pkgCache::State::Purge)
	{
	  action_tag += _(" (PURGE)");
	}

      std::string cur_verstr  = _("(no version found)");
      std::string cand_verstr = _("(no version found)");
      auto candver_it = (*apt_cache_file)[i->first].CandidateVerIter(*apt_cache_file);
      if (i->first.CurVersion())
	{
	  cur_verstr = i->first.CurVersion();
	}
      if (! candver_it.end() && candver_it.VerStr())
	{
	  cand_verstr = candver_it.VerStr();
	}

      switch (i->second)
	{
	case pkg_install:
	case pkg_auto_install:
	  // version: candidate
	  fprintf(f, _("[%s] %s %s\n"),
		  action_tag.c_str(),
		  i->first.FullName(false).c_str(),
		  cand_verstr.c_str());
	  break;
	case pkg_reinstall:
	case pkg_remove:
	case pkg_unused_remove:
	case pkg_auto_remove:
	case pkg_hold:
	case pkg_auto_hold:
	case pkg_unconfigured:
	  // version: current
	  fprintf(f, _("[%s] %s %s\n"),
		  action_tag.c_str(),
		  i->first.FullName(false).c_str(),
		  cur_verstr.c_str());
	  break;
	case pkg_upgrade:
	case pkg_downgrade:
	  // version: current + candidate
	  fprintf(f, _("[%s] %s %s -> %s\n"),
		  action_tag.c_str(),
		  i->first.FullName(false).c_str(),
		  cur_verstr.c_str(),
		  cand_verstr.c_str());
	  break;
	case pkg_broken:
	default:
	  // version: none
	  fprintf(f, _("[%s] %s\n"),
		  action_tag.c_str(),
		  i->first.FullName(false).c_str());
	}
    }
  fprintf(f, "========================================\n");

  fprintf(f, "\n%s\n", _("Log complete."));
  fprintf(f, "\n===============================================================================\n\n");

  if(log[0] == '|')
    pclose(f);
  else
    fclose(f);

  // try to restore locale -- it can throw an exception if the locale defined in
  // the environment is not valid
  try {
    if (std::locale("") != std::locale())
      {
	std::locale::global(std::locale(""));
      }
  } catch (...) {
    // ignore
  }

  return true;
}

struct log_sorter
{
  pkg_name_lt plt;
public:
  inline bool operator()(const logitem &a, const logitem &b)
  {
    if(a.second<b.second)
      return true;
    else if(a.second>b.second)
      return false;
    else
      return plt(a.first, b.first);
  }
};

void log_changes()
{
  vector<string> logs;

  string main_log = aptcfg->Find(PACKAGE "::Log", "/var/log/" PACKAGE);

  if(!main_log.empty())
    logs.push_back(main_log);

  const Configuration::Item *parent = aptcfg->Tree(PACKAGE "::Log");

  if(parent != NULL)
    for(const Configuration::Item *curr = parent->Child;
	curr != NULL; curr = curr->Next)
      {
	if(!curr->Value.empty())
	  logs.push_back(curr->Value);
      }

  bool localize_log = aptcfg->FindB(PACKAGE "::Localize-Log", false);

  if(!logs.empty())
    {
      loglist changed_packages;
      for(pkgCache::PkgIterator i = (*apt_cache_file)->PkgBegin(); !i.end(); ++i)
	{
	  pkg_action_state s = find_pkg_state(i, *apt_cache_file);
	  if(s != pkg_unchanged)
	    changed_packages.push_back(logitem(i, s));
	}

      sort(changed_packages.begin(), changed_packages.end(), log_sorter());

      for(vector<string>::const_iterator i = logs.begin(); i != logs.end(); ++i)
	do_log(*i, changed_packages, localize_log);
    }
}
