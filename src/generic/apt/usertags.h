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


#ifndef APTITUDE_GENERIC_APT_USERTAGS_H
#define APTITUDE_GENERIC_APT_USERTAGS_H


/** @file
 *
 * Helpers for user tags
 *
 */

#include <map>
#include <set>
#include <string>
#include <vector>


/** Alias for user tag references
 */
typedef int user_tag_reference;


/** \brief An opaque type used to store references into the user-tags list.
 *
 *  We only store one copy of each tag string, to save space.
 */
class user_tag
{
  friend class aptitudeDepCache;
  friend class user_tag_collection;

 public:
  bool operator==(const user_tag& other) const
  {
    return tag_num == other.tag_num;
  }

  bool operator<(const user_tag& other) const
  {
    return tag_num < other.tag_num;
  }

 private:

  user_tag_reference tag_num;

  explicit user_tag(const user_tag_reference& _tag_num) : tag_num(_tag_num)
  {
  }
};


/** Collection of user tags
 *
 * It is implemented as a collection of strings plus references pointing to
 * them, to save space (many packages can share the same tag(s)).
 */
class user_tag_collection
{
  friend class aptitudeDepCache;

 public:

  /** Get tag string from reference
   *
   * @returns User tag as string, empty if not valid
   */
  std::string deref_user_tag(const user_tag& tag) const
  {
    if (tag.tag_num < 0 || static_cast<size_t>(tag.tag_num) >= user_tags.size())
      {
	return {};
      }
    else
      {
	return user_tags[tag.tag_num];
      }
  }

  /** Get tag reference from string
   *
   * @returns User tag reference, -1 if not valid
   */
  user_tag_reference get_ref(const std::string& tag) const
  {
    auto it = user_tags_index.find(tag);
    if (it != user_tags_index.end())
      {
	return it->second;
      }
    else
      {
	return -1;
      }
  }

  /** Clear the state */
  void clear()
  {
    user_tags.clear();
    user_tags_index.clear();
  }

  /** Read a set of user tags from the given string region and write the tags
   * into the index and into the given set of tags.
   *
   * @return Whether parsing succeeded
   */
  bool parse(std::set<user_tag>& tags,
	     const char *& start, const char* end,
	     const std::string& package_name);

  /** Add tag to collection
   *
   * @param tag Tag to add
   *
   * @param ref User tag reference of the tag added (or the one present, if
   *            previously there)
   *
   * @return Whether the operation succeeded
   */
  bool add(const std::string& tag, user_tag_reference& ref);

  /** Check if the tag is valid/allowed
   *
   * "Tags" don't contain non-printable characters, tabs, newlines or whitespace
   * in general (including simple spaces)
   */
  static bool check_valid(const std::string& tag);

private:

  /** Collection of tags
   *
   * To speed the program up and save memory, only store one copy of each
   * distinct tag, and keep a reference to this list.
   */
  std::vector<std::string> user_tags;

  /** Stores the reference corresponding to each string */
  std::map<std::string, user_tag_reference> user_tags_index;
};


#endif
