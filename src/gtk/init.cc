/** \file init.cc */

// Copyright (C) 2010 Daniel Burrows
// Copyright 2008-2009 Obey Arthur Liu
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

#include "init.h"

#include "areas.h"
#include "globals.h"
#include "mainwindow.h"
#include "post_event.h"
#include "resolver.h"

#include <apt-pkg/error.h>

#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>

#include <generic/apt/config_signal.h>

#include <generic/util/dynamic_set_transform.h>
#include <generic/util/dynamic_set_union.h>

#include <gtk/toplevel/model.h>
#include <gtk/toplevel/tabs_notebook.h>
#include <gtk/toplevel/view.h>

#include <memory>

using aptitude::util::dynamic_set;
using aptitude::util::dynamic_set_transform;
using aptitude::util::dynamic_set_union;
using aptitude::util::enumerator;
using boost::lambda::_1;
using boost::lambda::constructor;
using gui::toplevel::area_info;
using gui::toplevel::area_list;
using gui::toplevel::create_area_list;
using gui::toplevel::tab_display_info;
using gui::toplevel::tab_info;
using gui::toplevel::view;

// Contains startup code for the GTK+ GUI.  Would be called "main",
// but that leads to ugly namespace conflicts.

namespace gui
{
  namespace
  {
    void init_style(void)
    {
      Gtk::RC::parse_string (
                             "style \"tiny-button-style\""
                             "{"
                             "  GtkWidget::focus-padding = 0"
                             "  xthickness = 0"
                             "  ythickness = 0"
                             "}"
                             "widget \"*.notebook_close_button\" style \"tiny-button-style\"");
    }

    std::shared_ptr<view> create_main_view(const std::shared_ptr<area_list> &areas)
    {
      std::shared_ptr<dynamic_set_union<std::shared_ptr<tab_info> > > all_tabs_set =
        dynamic_set_union<std::shared_ptr<tab_info> >::create();

      for(std::shared_ptr<enumerator<std::shared_ptr<area_info> > > e = areas->get_areas();
          e->advance(); )
        all_tabs_set->insert_set(e->get_current()->get_tabs());


      typedef dynamic_set_transform<std::shared_ptr<tab_info>,
        std::shared_ptr<tab_display_info> > upcast_set;
      std::shared_ptr<dynamic_set<std::shared_ptr<tab_display_info> > > all_tabs_view_set =
        upcast_set::create(all_tabs_set,
                           constructor<std::shared_ptr<tab_display_info> >());

      // Use a notebook containing all the tabs in all areas for the
      // time being.
      //
      // Eventually a better main view will be used.
      return create_tabs_notebook(all_tabs_view_set);
    }

    void do_apt_init()
    {
      // \todo Display progress somehow (maybe using the notification
      // framework?)
      OpProgress p;
      bool operation_needs_lock = true;
      apt_init(&p, true, operation_needs_lock, NULL);

      // \todo Support update-on-startup once updating is plumbed
      // through.
      //
      //if(getuid() == 0 && aptcfg->FindB(PACKAGE "::Update-On-Startup", true))
      //do_update();
    }
  }

  bool init(int argc, char **argv)
  {
    // Don't crash if a subprocess breaks a pipe.
    signal(SIGPIPE, SIG_IGN);

    // GTK+ provides a perfectly good routine, gtk_init_check(), to
    // initialize GTK+ *and report whether the initialization
    // succeeded*.  gtkmm doesn't wrap it.  But initializing GTK+
    // twice won't hurt, so we do that.
    if(!gtk_init_check(&argc, &argv))
      return false;

    Glib::init();
    // If we don't check thread_supported() first, thread_init()
    // aborts with an error on some architectures. (see Debian bug
    // #555120)
    if(!Glib::thread_supported())
      Glib::thread_init();

    globals::init_post_event();

    // We have to create the main loop object before loading the Glade
    // file, or Glade goes and pouts in a corner and refuses to do
    // anything.
    globals::init_main_loop(argc, argv);

    Glib::RefPtr<Gnome::Glade::Xml> glade = globals::load_glade(argv[0]);

    if(!glade)
      {
	_error->Error(_("Unable to load the user interface definition file %s/aptitude.glade."),
		      PKGDATADIR);

	return false;
      }

    // Set up the style for GTK+ widgets.
    init_style();

    // Set up the resolver-triggering signals.
    init_resolver();

    // Postpone apt_init until we enter the main loop, so we get a GUI
    // progress bar.
    Glib::signal_idle().connect(sigc::bind_return(sigc::ptr_fun(&do_apt_init),
						  false));

    std::shared_ptr<areas> all_areas = create_areas();

    std::shared_ptr<view> main_win_view = create_main_view(all_areas->get_areas());
    std::shared_ptr<main_window> main = create_mainwindow(glade, main_win_view, all_areas);
    main->get_window()->signal_unmap().connect(sigc::ptr_fun(&globals::main_quit));
    main->get_window()->show();

    // Run the main loop, exiting when main_win is closed:
    globals::run_main_loop();

    return true;
  }
}
