/** \file throttle.h */   // -*-c++-*-

// Copyright (C) 2010 Daniel Burrows
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

#ifndef APTITUDE_UTIL_THROTTLE_H
#define APTITUDE_UTIL_THROTTLE_H

#include <memory>

namespace aptitude
{
  namespace util
  {
    /** \brief Used to ensure that an expensive operation (such as
     *  updating a progress indicator) doesn't run too often.
     *
     *  To test code that uses this class, use the mock that's
     *  available in mocks/.
     */
    class throttle
    {
    public:
      virtual ~throttle() = 0;

      /** \return \b true if the timer has expired. */
      virtual bool update_required() = 0;

      /** \brief Reset the timer that controls when the display is
       *  updated.
       */
      virtual void reset_timer() = 0;
    };

    /** \brief Create a throttle object.
     *
     *  \todo This should take an argument giving the update interval
     *  in seconds.
     */
    std::shared_ptr<throttle> create_throttle();
  }
}

#endif // APTITUDE_UTIL_THROTTLE_H
