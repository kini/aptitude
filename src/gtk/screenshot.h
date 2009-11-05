/** \file screenshot.h */    // -*-c++-*-


// Copyright (C) 2009 Daniel Burrows
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
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#ifndef GTK_SCREENSHOT_H
#define GTK_SCREENSHOT_H

// GTK+ classes and functions relating to screenshots.

#include <gtkmm/image.h>

#include <boost/shared_ptr.hpp>

#include <generic/apt/screenshot.h>

namespace gui
{
  class cached_screenshot;

  class screenshot_image : public Gtk::Image
  {
    // These are used to generate a new screenshot object when the
    // image is exposed.
    std::string package_name;
    aptitude::screenshot_type type;

    sigc::connection screenshot_prepared_connection;
    sigc::connection screenshot_failed_connection;
    sigc::connection screenshot_updated_connection;
    sigc::connection screenshot_ready_connection;

    boost::shared_ptr<cached_screenshot> screenshot;
    // Set to true when the download succeeds; used to decide whether
    // cancel_download() should blank the image.
    bool download_complete;

    std::string describe() const;

    /** \brief If there is an active screenshot download, detach from
     *	it.
     */
    void disconnect();

    /** \brief If there is not an active screenshot download, start
     *	one.
     */
    void connect();

    /** \brief Invoked when the screenshot object's pixbuf is created.
     */
    void prepared();

    /** \brief Invoked when the screenshot is entirely loaded.
     */

    void success();

  public:
    /** \brief Create a screenshot image widget.
     *
     *  \param _package_name  The name of the package whose screenshot
     *                        should be downloaded.
     *  \param _type          The type of screenshot to download.
     */
    screenshot_image(const std::string &_package_name,
		     aptitude::screenshot_type _type);

    /** \brief Invoke to start a download, even if the widget is not
     *	visible.
     */
    void start_download();

    /** \brief Invoke to cancel any pending download.
     *
     *  If the screenshot was already downloaded, this has no effect.
     */
    void cancel_download();
  };
}

#endif // GTK_SCREENSHOT_H