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
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


#include "usertags.h"

#include <aptitude.h>

#include <apt-pkg/error.h>

#include <cctype>


// User tag syntax:
// 
// Tag-List ::= Tag+
// Tag      ::= regexp([^[:space:]])
bool parse_user_tag(std::string& out,
		    const char *& start, const char* end,
		    const std::string& package_name)
{
  // consume whitespace
  while (start != end && isspace(*start))
    ++start;

  if (start == end)
    {
      return false;
    }
  else
    {
      while (start != end && !isspace(*start))
	{
	  out += *start;
	  ++start;
	}
      return true;
    }
}


bool user_tag_collection::check_valid(const std::string& tag, std::string::size_type& error_position)
{
  // empty is invalid
  if (tag.empty())
    {
      _error->Error(_("Invalid empty user-tag"));
      error_position = 0;
      return false;
    }

  // check for invalid/unallowed characters
  for (size_t i = 0; i < tag.size(); ++i)
    {
      if (! std::isgraph(tag[i]))
	{
	  _error->Error(_("Invalid character in user-tag at position %zu (use only printable characters and no spaces)"), i);
	  error_position = i;
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

      typedef std::map<std::string, user_tag_reference>::const_iterator	user_tags_index_iterator;
      user_tags_index_iterator found = user_tags_index.find(tag);
      if (found == user_tags_index.end())
	{
	  user_tag_reference loc(user_tags.size());
	  user_tags.push_back(tag);
	  std::pair<user_tags_index_iterator, bool> tmp(user_tags_index.insert(std::make_pair(tag, loc)));
	  found = tmp.first;
	}
      tags.insert(user_tag(found->second));
    }

  return true;
}
