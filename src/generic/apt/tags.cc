// tags.cc
//
//   Copyright (C) 2005, 2007-2008, 2010 Daniel Burrows
//   Copyright (C) 2014 Daniel Hartwig
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
//   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//   Boston, MA 02111-1307, USA.

#include "tags.h"

#include <aptitude.h>

#include "apt.h"

#include "config_signal.h"

#include <algorithm>
#include <map>
#include <utility>

#include <ctype.h>
#include <string.h>

#include <sigc++/functors/mem_fun.h>

#include <apt-pkg/error.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/tagfile.h>

#include <cwidget/generic/util/eassert.h>

using namespace std;
using aptitude::apt::tag;
using aptitude::apt::tag_set;

class tag_list
{
  // The string to parse.
  std::string s;
public:
  class const_iterator
  {
    std::string::const_iterator start, finish, limit;
  public:
    const_iterator(const std::string::const_iterator &_start,
		   const std::string::const_iterator &_finish,
		   const std::string::const_iterator &_limit)
      :start(_start), finish(_finish), limit(_limit)
    {
    }

    const_iterator operator=(const const_iterator &other)
    {
      start = other.start;
      finish = other.finish;
      limit = other.limit;

      return *this;
    }

    bool operator==(const const_iterator &other)
    {
      return (other.start == start &&
              other.finish == finish &&
              other.limit == limit);
    }

    bool operator!=(const const_iterator &other)
    {
      return (other.start != start ||
              other.finish != finish ||
              other.limit != limit);
    }

    const_iterator &operator++()
    {
      start = finish;

      while(start != limit && (*start) != ',')
        ++start;

      if(start != limit) // Push past the comma.
        ++start;

      while(start != limit && isspace(*start)) // Skip whitespace.
        ++start;

      if(start == limit)
        finish = limit;
      else
        {
          // Eat everything up to the next comma.
          finish = start + 1;
          while(finish != limit && (*finish) != ',')
            ++finish;
        }

      return *this;
    }

    tag operator*()
    {
      return tag(start, finish);
    }
  };

  tag_list(const char *start, const char *finish)
    :s(start, finish)
  {
  }

  tag_list &operator=(const tag_list &other)
  {
    s = other.s;

    return *this;
  }

  const_iterator begin() const
  {
    std::string::const_iterator endfirst = s.begin();
    while(endfirst != s.end() && (*endfirst) != ',')
      ++endfirst;

    const_iterator rval(s.begin(), endfirst, s.end());
    return rval;
  }

  const_iterator end() const
  {
    return const_iterator(s.end(), s.end(), s.end());
  }
};

// The database is built eagerly, since the common use case is
// to scan everything in sight right away and this makes it easy
// to provide a progress bar to the user.
tag_set *tagDB;

static void insert_tags(const pkgCache::VerIterator &ver,
			const pkgCache::VerFileIterator &vf)
{
  tag_set *tags = tagDB + ver.ParentPkg().Group()->ID;

  const char *recstart=0, *recend=0;
  const char *tagstart, *tagend;
  pkgTagSection sec;

  eassert(apt_package_records);
  eassert(tagDB);

  apt_package_records->Lookup(vf).GetRec(recstart, recend);
  if(!recstart || !recend)
    return;
  if(!sec.Scan(recstart, recend-recstart+1))
    return;

  if(!sec.Find("Tag", tagstart, tagend))
    return;

  tag_list lst(tagstart, tagend);

  for(tag_list::const_iterator t=lst.begin(); t!=lst.end(); ++t)
    tags->insert(*t);
}

static void reset_tags()
{
  delete[] tagDB;
  tagDB = NULL;
}

const tag_set aptitude::apt::get_tags(const pkgCache::PkgIterator &pkg)
{
  if(!apt_cache_file || !tagDB)
    return tag_set();

  return tagDB[pkg.Group()->ID];
}

static bool load_tags_from_debtags(OpProgress *progress)
{
  const string filename(aptcfg->FindFile("Debtags::Package-Tags",
                                         "/var/lib/debtags/package-tags"));
  _error->PushToStack(); // Ignore no-such-file errors.
  FileFd F(filename, FileFd::ReadOnly);
  _error->RevertToStack();

  if(F.IsOpen() == false)
    {
      // Fail silently; debtags need not be installed.
      return false;
    }

  const unsigned long long m = F.Size();

  if(progress != NULL)
    progress->OverallProgress(0, m, 1,
                              _("Building tag database"));

  const unsigned long long buf_size = 4096;
  char buf[buf_size];
  while(F.ReadLine(buf, buf_size) != NULL)
    {
      if(progress != NULL)
        progress->OverallProgress(F.Tell(), m, 1,
                                  _("Building tag database"));

      const char *sep = strstr(buf, ": ");
      if(sep == NULL)
        continue;

      const string pkg_name(buf, sep - buf);
      const pkgCache::GrpIterator grp((*apt_cache_file)->FindGrp(pkg_name));
      if(grp.end() == true)
        continue;

      tag_set *tags = tagDB + grp->ID;

      const char *tagstart = sep + 2;
      const char *tagend = tagstart;
      while((*tagend) != '\0')
        ++tagend;

      tag_list ts(tagstart, tagend);
      for(tag_list::const_iterator t = ts.begin();
          t != ts.end();
          ++t)
        tags->insert(*t);
    }

  if(progress != NULL)
    progress->Done();

  return true;
}

static bool load_tags_from_verfiles(OpProgress *progress)
{
  std::vector<loc_pair> verfiles;

  for(pkgCache::PkgIterator p = (*apt_cache_file)->PkgBegin();
      !p.end(); ++p)
    for(pkgCache::VerIterator v = p.VersionList(); !v.end(); ++v)
      for(pkgCache::VerFileIterator vf = v.FileList();
	  !vf.end(); ++vf)
	verfiles.push_back(loc_pair(v, vf));

  sort(verfiles.begin(), verfiles.end(), location_compare());

  progress->OverallProgress(0, verfiles.size(), 1,
                            _("Building tag database"));
  size_t n=0;
  for(std::vector<loc_pair>::iterator i=verfiles.begin();
      i!=verfiles.end(); ++i)
    {
      insert_tags(i->first, i->second);
      ++n;
      progress->OverallProgress(n, verfiles.size(), 1,
                                _("Building tag database"));
    }

  progress->Done();

  return true;
}

bool initialized_reset_signal;
void aptitude::apt::load_tags(OpProgress *progress)
{
  eassert(apt_cache_file && apt_package_records);

  if(!initialized_reset_signal)
    {
      cache_closed.connect(sigc::ptr_fun(reset_tags));
      cache_reload_failed.connect(sigc::ptr_fun(reset_tags));
      initialized_reset_signal = true;
    }

  tagDB = new tag_set[(*apt_cache_file)->Head().GroupCount];

  if(load_tags_from_debtags(progress) == true)
    return;

  load_tags_from_verfiles(progress);
}

// TAG VOCABULARY FILE
typedef map<string, string> facet_description_map;
typedef map<string, string> tag_description_map;

facet_description_map *facet_descriptions;
tag_description_map *tag_descriptions;

static void init_vocabulary()
{
  if(facet_descriptions != NULL)
    {
      eassert(tag_descriptions != NULL);
      return;
    }

  facet_descriptions = new facet_description_map;
  tag_descriptions = new tag_description_map;

  _error->PushToStack(); // Ignore no-such-file errors.
  FileFd F(aptcfg->FindFile("DebTags::Vocabulary", "/var/lib/debtags/vocabulary"),
	   FileFd::ReadOnly);
  _error->RevertToStack();

  if(!F.IsOpen())
    {
      //_error->Warning(_("Unable to load debtags vocabulary, perhaps debtags is not installed?"));
      // Fail silently; debtags need not be installed.
      return;
    }

  pkgTagFile tagfile(&F);

  pkgTagSection sec;

  while(tagfile.Step(sec))
    {
      string facet;
      const char *start, *end;

      if(sec.Find("Facet", start, end))
	{
	  facet.assign(start, end-start);

	  if(sec.Find("Description", start, end))
	    (*facet_descriptions)[facet] = string(start, end-start);
	}
      else if(sec.Find("Tag", start, end))
	{
	  string tag(start, end-start);

	  if(sec.Find("Description", start, end))
	    (*tag_descriptions)[tag] = string(start, end-start);
	}
    }
}

std::string aptitude::apt::get_facet_long_description(const tag &t)
{
  init_vocabulary();

  const string facet(get_facet_name(t));

  facet_description_map::const_iterator found =
    facet_descriptions->find(facet);

  if(found == facet_descriptions->end())
    return string();
  else
    return found->second;
}

std::string aptitude::apt::get_facet_short_description(const tag &t)
{
  const string desc(get_facet_long_description(t));
  return string(desc, 0, desc.find('\n'));
}

std::string aptitude::apt::get_tag_long_description(const tag &t)
{
  init_vocabulary();

  tag_description_map::const_iterator found =
    tag_descriptions->find(t);

  if(found == tag_descriptions->end())
    return string();
  else
    return found->second;
}

std::string aptitude::apt::get_tag_short_description(const tag &t)
{
  const string desc = get_tag_long_description(t);
  return string(desc, 0, desc.find('\n'));
}

std::string aptitude::apt::get_facet_name(const tag &t)
{
  const string name = get_fullname(t);
  string::size_type split_pos = name.find("::");
  if(split_pos == string::npos)
    return _("legacy");
  else
    return std::string(name, 0, split_pos);
}

std::string aptitude::apt::get_tag_name(const tag &t)
{
  const string name = get_fullname(t);
  string::size_type split_pos = name.find("::");
  if(split_pos == string::npos)
    return name;
  else
    return std::string(name, split_pos + 2);
}
