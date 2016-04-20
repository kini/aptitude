/** \file search_input.h */  // -*-c++-*-

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

#ifndef APTITUDE_VIEWS_MOCKS_SEARCH_INPUT_H
#define APTITUDE_VIEWS_MOCKS_SEARCH_INPUT_H

// Local includes:
#include <generic/views/search_input.h>

// System includes:
#include <gmock/gmock.h>

namespace aptitude
{
  namespace views
  {
    namespace mocks
    {
      /** \brief Mock implementation of search_input for use in unit
       *  tests.
       *
       *  Provides signals which, by default, are connected by the
       *  connect_() methods.
       */
      class search_input : public views::search_input
      {
      public:
        MOCK_METHOD0(get_search_text, std::wstring());
        MOCK_METHOD1(set_search_text, void(const std::wstring &));
        MOCK_METHOD1(set_error_message, void(const std::wstring &));
        MOCK_METHOD1(set_input_validity, void(bool));
        MOCK_METHOD1(set_find_sensitivity, void(bool));
        MOCK_METHOD1(connect_search_text_changed, sigc::connection(const sigc::slot<void> &));
        MOCK_METHOD1(connect_search, sigc::connection(const sigc::slot<void> &));

        sigc::signal<void> signal_search_text_changed;
        sigc::signal<void> signal;

        search_input()
        {
          using testing::_;
          using testing::Invoke;

	  /* It doesn't work in libsigc++ 2.8 (probably due to the addition of
	     "iterator connect(slot_type&& slot_)" on top of the previous
	     "iterator connect(const slot_type& slot_)" in
	     libsigc++-2.0/2.8.0-1/sigc++/signal.h , so the compiler cannot
	     decide which version to use.

	     These classes seem to be a WIP related with the GTK implementation,
	     and it seems difficult to get it to work when juggling so many
	     blades (Google-Mock not updated for 2 years in Debian, libsigc++,
	     C++11 support still incomplete in many places....) so comment out
	     to make the rest work at the moment.  It doesn't seem terribly
	     important to check that signals and slots work for this class, if
	     it works everywhere else.

          ON_CALL(*this, connect_search_text_changed(_))
            .WillByDefault(Invoke(&signal_search_text_changed,
                                  &sigc::signal<void>::connect));

          ON_CALL(*this, connect_search(_))
            .WillByDefault(Invoke(&signal,
                                  &sigc::signal<void>::connect));
	  */
        }
      };
    }
  }
}

#endif // APTITUDE_VIEWS_MOCKS_SEARCH_INPUT_H
