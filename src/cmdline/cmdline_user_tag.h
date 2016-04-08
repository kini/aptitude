// cmdline_user_tag.h
//
//   Copyright (C) 2008 Daniel Burrows
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

#ifndef CMDLINE_USER_TAG_H
#define CMDLINE_USER_TAG_H

/** \file cmdline_user_tag.h
 */

namespace aptitude
{
  namespace cmdline
  {
    /** \brief Add or remove a user tag depending on the value of argv[0]. */
    int cmdline_user_tag(int argc, char *argv[], int quiet, int verbose);
  }
}

#endif // CMDLINE_USER_TAG_H
