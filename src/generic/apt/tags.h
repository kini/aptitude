// tags.h                                            -*-c++-*-
//
//   Copyright (C) 2005, 2007, 2010 Daniel Burrows
//   Copyright (C) 2014 Daniel Hartwig
//   Copyright (C) 2016-2017 Manuel A. Fernandez Montecelo
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
//

#ifndef TAGS_H
#define TAGS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include <string>

#include <apt-pkg/pkgcache.h>

/** \brief Interface for Package tags.
 * 
 *  \file tags.h
 */

class OpProgress;

namespace aptitude
{
  namespace apt
  {
    typedef std::string tag;
    typedef std::set<tag> tag_set;

    inline std::string get_fullname(const tag &t)
    {
      return static_cast<std::string>(t);
    }

    /** \brief Initialize the cache of debtags information. */
    void load_tags(OpProgress *progress);

    /** \brief Get the tags for the given package. */
    const tag_set get_tags(const pkgCache::PkgIterator &pkg);

    /** \brief Get the name of the facet corresponding to a tag. */
    std::string get_facet_name(const tag &t);

    /** \brief Get the name of a tag (the full name minus the facet). */
    std::string get_tag_name(const tag &t);

    /** \brief Get the short description of a tag. */
    std::string get_tag_short_description(const tag &t);

    /** \brief Get the long description of a tag. */
    std::string get_tag_long_description(const tag &t);

    // \note This interface could be more efficient if it just used
    // facet names like libept does.  Using tags is a concession to
    // backwards compatibility (it's hard to implement one interface
    // that covers both cases without a lot of cruft).  In any event,
    // this shouldn't be called enough to matter.

    /** \brief Get the short description of the facet corresponding to a tag. */
    std::string get_facet_short_description(const tag &t);

    /** \brief Get the long description of the facet corresponding to a tag. */
    std::string get_facet_long_description(const tag &t);
  }
}

#endif
