//
// Copyright 1999-2011 Daniel Burrows
// Copyright 2015 Manuel A. Fernandez Montecelo
//
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA 02110-1301, USA.


#include "usertags.h"

#include <aptitude.h>

#include <apt-pkg/error.h>

#include <cctype>


// User tag syntax:
// 
// Tag-List ::= Tag+
// Tag      ::= regexp([^[:space:]]) | regexp([^[,]])
bool parse_user_tag(std::string& out,
		    const char *& start, const char* end,
		    const std::string& package_name)
{
  // consume whitespace
  while (start != end && (isspace(*start) || (*start == ',')))
    ++start;

  if (start == end)
    {
      return false;
    }
  else
    {
      while (start != end && !isspace(*start) && (*start != ','))
	{
	  out += *start;
	  ++start;
	}
      return true;
    }
}


bool user_tag_collection::check_valid(const std::string& tag)
{
  // check valid characters, it is a reasonable request that "tags" don't
  // contain tabs, newlines or even spaces (see #792601)

  // empty is invalid
  if (tag.empty())
    {
      _error->Error(_("Invalid empty user-tag"));
      return false;
    }

  // check for invalid/unallowed characters
  for (size_t i = 0; i < tag.size(); ++i)
    {
      if (! std::isgraph(tag[i]) || tag[i] == ',')
	{
	  _error->Error(_("Invalid character in user-tag at position %zu (use only printable characters and no spaces or comma)"), i);
	  return false;
	}
    }

  return true;
}


bool user_tag_collection::parse(std::set<user_tag>& tags,
				const char *& start, const char* end,
				const std::string& package_name)
{
  while (start != end)
    {
      std::string tag;
      bool parsed_tag = parse_user_tag(tag, start, end, package_name);
      if (!parsed_tag)
	{
	  return false;
	}
      else
	{
	  user_tag_reference ref;
	  add(tag, ref);
	  tags.insert(user_tag{ref});
	}
    }

  return true;
}


bool user_tag_collection::add(const std::string& tag, user_tag_reference& ref)
{
  bool added = false;

  typedef std::map<std::string, user_tag_reference>::const_iterator user_tags_index_iterator;

  // try to find if already there, otherwise insert
  user_tags_index_iterator found = user_tags_index.find(tag);
  if (found == user_tags_index.end())
    {
      user_tag_reference loc(user_tags.size());
      user_tags.push_back(tag);

      std::pair<user_tags_index_iterator, bool> insert_result(user_tags_index.insert(std::make_pair(tag, loc)));
      found = insert_result.first;
      added = insert_result.second;
    }

  // either the found one or the newly inserted
  ref = found->second;

  return added;
}
