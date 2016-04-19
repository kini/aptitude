/** \file mainwindow.cc */

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

#include "mainwindow.h"

#include "areas.h"

#include <loggers.h>

#include <gtk/toplevel/model.h>
#include <gtk/toplevel/view.h>

#include <gtkmm.h>
#include <libglademm.h>

using aptitude::Loggers;

namespace gui
{
  namespace
  {
    class window : public Gtk::Window
    {
      Gtk::Bin *main_bin;
      logging::LoggerPtr logger;

    public:
      window(BaseObjectType *cobject, const Glib::RefPtr<Gnome::Glade::Xml> &glade)
        : Gtk::Window(cobject)
      {
        // ... maybe use an alignment to ensure we know where it is?
        glade->get_widget("main_bin", main_bin);
        logger = Loggers::getAptitudeGtkMainWindow();
      }

      /** \brief Attach a view to this main window.
       *
       *  Must be invoked exactly once.
       */
      void set_view(const std::shared_ptr<toplevel::view> &view)
      {
        if(main_bin->get_child() != NULL)
          LOG_ERROR(logger, "Two views added to the main window, discarding the second one.");
        else
          {
            LOG_TRACE(logger, "Adding the view " << view << " to the main window.");
            main_bin->add(*view->get_widget());
          }
      }
    };

    // The implementation of the main window's interface.
    class main_window_impl : public main_window
    {
      Gtk::Window *w;
      std::shared_ptr<areas> all_areas;

    public:
      main_window_impl(window *_w,
                       const std::shared_ptr<areas> &_all_areas)
        : w(_w),
          all_areas(_all_areas)
      {
      }

      Gtk::Window *get_window() { return w; }
      std::shared_ptr<areas> get_areas() { return all_areas; }
    };
  }

  main_window::~main_window()
  {
  }

  std::shared_ptr<main_window>
  create_mainwindow(const Glib::RefPtr<Gnome::Glade::Xml> &glade,
                    const std::shared_ptr<toplevel::view> &view,
                    const std::shared_ptr<areas> &all_areas)
  {
    window *w;
    glade->get_widget_derived("main_window_2", w);
    w->set_view(view);

    return std::make_shared<main_window_impl>(w, all_areas);
  }
}
