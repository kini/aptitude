// pkg_item.cc
//
// Copyright 1999-2005, 2007-2009 Daniel Burrows
// Copyright 2014-2016 Manuel A. Fernandez Montecelo
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#include "aptitude.h"

#include <cwidget/config/column_definition.h>
#include <cwidget/config/keybindings.h>
#include <cwidget/dialogs.h>
#include <cwidget/fragment.h>
#include <cwidget/generic/util/ssprintf.h>
#include <cwidget/generic/util/transcode.h>
#include <cwidget/toplevel.h>

#include "pkg_columnizer.h"
#include "pkg_item.h"
#include "ui.h"
#include "view_changelog.h"
#include "progress.h"

#include <generic/apt/apt.h>
#include <generic/apt/apt_undo_group.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/dpkg.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>

#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/functors/ptr_fun.h>

#include <iostream>

#include <unistd.h>

using namespace std;
namespace cw = cwidget;
namespace cwidget
{
  using namespace widgets;
}

static void try_delete_essential(wstring s,
				 const pkgCache::PkgIterator pkg,
				 bool purge)
{
  if(s == cw::util::transcode(_(confirm_delete_essential_str)) ||
     s == cw::util::transcode(confirm_delete_essential_str))
    {
      undo_group *grp=new apt_undo_group;

      (*apt_cache_file)->mark_delete(pkg, purge, false, grp);

      if(grp->empty())
	delete grp;
      else
	apt_undos->add_item(grp);
    }
}

static void confirm_delete_essential(const pkgCache::PkgIterator &pkg,
				     bool purge)
{
  eassert((pkg->Flags&pkgCache::Flag::Essential)==pkgCache::Flag::Essential ||
	 (pkg->Flags&pkgCache::Flag::Important)==pkgCache::Flag::Important);

  cw::fragment *f=wrapbox(cw::fragf(_("%s is an essential package!%n%nAre you sure you want to remove it?%nType '%s' if you are."), pkg.FullName(true).c_str(), _(confirm_delete_essential_str)));

  cw::widget_ref w=cw::dialogs::string(f,
				   L"",
				   cw::util::arg(sigc::bind(sigc::ptr_fun(try_delete_essential),
						  pkg, purge)),
				   NULL,
				   NULL,
				   NULL,
				   cw::style_attrs_flip(A_REVERSE));

  w->show_all();

  popup_widget(w);
}

pkg_item::pkg_item(const pkgCache::PkgIterator &_package,
		   sigc::signal2<void, const pkgCache::PkgIterator &, const pkgCache::VerIterator &> *sig)
:package(_package), info_signal(sig)
{
  highlighted_changed.connect(sigc::mem_fun(this, &pkg_item::do_highlighted_changed));
}

void pkg_item::do_highlighted_changed(bool highlighted)
{
  if(highlighted)
    {
      if(info_signal)
	(*info_signal)(package, visible_version());
    }
  else
    {
      if(info_signal)
	(*info_signal)(pkgCache::PkgIterator(),
		       pkgCache::VerIterator(*apt_cache_file));
    }
}

void pkg_item::do_select(undo_group *undo)
{
  pkgCache::PkgIterator& package_to_install = package;

  if (is_virtual(package))
    {
      std::string pkgtext;
      std::vector<std::string> pkgnames;
      for (pkgCache::PrvIterator prv = package.ProvidesList(); !prv.end(); ++prv)
	{
	  string name = prv.OwnerPkg().FullName(true);
	  pkgnames.push_back(name);

	  pkgtext += cwidget::util::ssprintf("* %s\n", name.c_str());
	}

      // messages copied from from cmdline_action.cc
      if (pkgnames.empty())
	{
	  _error->Error(_("\"%s\" exists in the package database, but it is not a real package and no package provides it\n"),
			package.FullName(true).c_str());
	}
      else if (pkgnames.size() == 1)
	{
	  // only one, install this one
	  package_to_install = package.ProvidesList().OwnerPkg();
	}
      else
	{
	  _error->Error(_("\"%s\" is a virtual package provided by:\n\n%s\nYou must choose one to install.\n"),
			package.FullName(true).c_str(), pkgtext.c_str());
	}
    }

  (*apt_cache_file)->mark_install(package_to_install, aptcfg->FindB(PACKAGE "::Auto-Install", true), false, undo);
}

void pkg_item::select(undo_group *undo)
{
  if(aptcfg->FindB(PACKAGE "::UI::New-Package-Commands", true))
    do_select(undo);
  else if(!(*apt_cache_file)[package].Delete())
    do_select(undo);
  else
    do_hold(undo);
}

void pkg_item::do_hold(undo_group *undo)
  // Sets an explicit hold state.
{
  (*apt_cache_file)->mark_keep(package, is_auto_installed(package), true, undo);
}

void pkg_item::hold(undo_group *undo)
  // Sets an /explicit/ hold state.  May be useful for, eg, saying that the
  // current package version (which is the newest) should be kept even if/when
  // a newer version becomes available.
{
  if(aptcfg->FindB(PACKAGE "::UI::New-Package-Commands", true))
    do_hold(undo);
  else
    // Toggle the held state.
    (*apt_cache_file)->mark_keep(package,
				 is_auto_installed(package),
				 (*apt_cache_file)->get_ext_state(package).selection_state!=pkgCache::State::Hold,
				 undo);
}

void pkg_item::keep(undo_group *undo)
{
  // copied to pkg_ver_item::keep()

  // Keep, don't hold, the package.
  (*apt_cache_file)->mark_keep(package,
			       is_auto_installed(package),
			       false,
			       undo);
}

void pkg_item::do_remove(undo_group *undo)
{
  if(((package->Flags&pkgCache::Flag::Essential)==pkgCache::Flag::Essential ||
      (package->Flags&pkgCache::Flag::Important)==pkgCache::Flag::Important) &&
     (*apt_cache_file)[package].Status != 2)
    confirm_delete_essential(package, false);
  else
    (*apt_cache_file)->mark_delete(package, false, false, undo);
}

void pkg_item::remove(undo_group *undo)
{
  if(aptcfg->FindB(PACKAGE "::UI::New-Package-Commands", true))
    do_remove(undo);
  else if(!(*apt_cache_file)[package].Install() && !((*apt_cache_file)[package].iFlags&pkgDepCache::ReInstall))
    do_remove(undo);
  else
    {
      if((*apt_cache_file)[package].iFlags&pkgDepCache::ReInstall)
	(*apt_cache_file)->mark_keep(package, false, false, undo);
      else
	(*apt_cache_file)->mark_keep(package, false, (*apt_cache_file)[package].Status==1, undo);
    }
}

// No "do_purge" because purge was always idempotent.
void pkg_item::purge(undo_group *undo)
{
  if(((package->Flags&pkgCache::Flag::Essential)==pkgCache::Flag::Essential ||
      (package->Flags&pkgCache::Flag::Important)==pkgCache::Flag::Important) &&
     (*apt_cache_file)[package].Status != 2)
    confirm_delete_essential(package, false);
  else
    (*apt_cache_file)->mark_delete(package, true, false, undo);
}

void pkg_item::reinstall(undo_group *undo)
{
  if(!package.CurrentVer().end())
    (*apt_cache_file)->mark_install(package,
				    aptcfg->FindB(PACKAGE "::Auto-Install", true),
				    true,
				    undo);
}

void pkg_item::forbid_upgrade(undo_group *undo)
{
  pkgCache::VerIterator curver=package.CurrentVer();
  pkgCache::VerIterator candver=(*apt_cache_file)[package].CandidateVerIter(*apt_cache_file);

  if(!curver.end() && !candver.end() && curver!=candver)
    (*apt_cache_file)->forbid_upgrade(package, candver.VerStr(), undo);
}

void pkg_item::set_auto(bool value, undo_group *undo)
{
  // it is faster to check first
  bool current_value = is_auto_installed((*apt_cache_file)[package]);

  if (value != current_value)
    {
      (*apt_cache_file)->mark_auto_installed(package, value, undo);
    }
}

void pkg_item::show_information()
{
  show_info_screen(package, visible_version());
}

void pkg_item::show_changelog()
{
   view_changelog(visible_version());
}

cw::style pkg_item::get_highlight_style()
{
  return cw::treeitem::get_normal_style() + pkg_style(package, true);
}

cw::style pkg_item::get_normal_style()
{
  return cw::treeitem::get_normal_style() + pkg_style(package, false);
}

cw::style pkg_item::pkg_style(pkgCache::PkgIterator package, bool highlighted)
{
#define MAYBE_HIGHLIGHTED(x) (highlighted ? (x "Highlighted") : (x))

  if(package.VersionList().end())
    {
      bool present_now=false, present_inst=false;

      for(pkgCache::PrvIterator i=package.ProvidesList(); !i.end(); i++)
	{
	  if(!i.OwnerPkg().CurrentVer().end())
	    present_now=true;
	  if(!(*apt_cache_file)[i.OwnerPkg()].InstVerIter(*apt_cache_file).end())
	    present_inst=true;


	  if(present_now && present_inst)
	    break;
	}

      if(present_now && present_inst)
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgIsInstalled"));
      else if(present_now)
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToRemove"));
      else if(present_inst)
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToInstall"));
      else
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgNotInstalled"));
    }
  else
    {
      pkgDepCache::StateCache &state=(*apt_cache_file)[package];

      if(!state.InstBroken() &&
	 (state.NewInstall() || (state.iFlags&pkgDepCache::ReInstall)))
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToInstall"));
      else if(state.Status!=2 && // Not being upgraded
	      (*apt_cache_file)->get_ext_state(package).selection_state==pkgCache::State::Hold // Flagged for hold
	      && !state.InstBroken()) // Not currently broken.
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToHold"));
      else if(state.Delete())
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToRemove"));
      else if(state.InstBroken())
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgBroken"));
      else if(state.Upgrade())
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToUpgrade"));
      else if(state.Downgrade())
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgToDowngrade"));
      else if(package.CurrentVer().end())
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgNotInstalled"));
      else
	return cw::get_style(MAYBE_HIGHLIGHTED("PkgIsInstalled"));
    }

#undef MAYBE_HIGHLIGHTED
}

void pkg_item::paint(cw::tree *win, int y, bool hierarchical, const cw::style &st)
{
  int basex=hierarchical?2*get_depth():0;
  int width, height;

  win->getmaxyx(height, width);
  pkg_columnizer::setup_columns();

  cw::config::empty_column_parameters p;
  wstring disp=pkg_columnizer(package, visible_version(), pkg_columnizer::get_columns(), basex).layout_columns(width, p);
  win->mvaddnstr(y, 0, disp.c_str(), width);
}

bool pkg_item::dispatch_key(const cw::config::key &k, cw::tree *owner)
{
  if(bindings->key_matches(k, "Versions"))
    show_ver_screen(package);
  else if(bindings->key_matches(k, "Dependencies"))
    {
      if(!visible_version().end())
	show_dep_screen(package, visible_version());
    }
  else if(bindings->key_matches(k, "ReverseDependencies"))
    show_dep_screen(package, visible_version(), true);
  else if(bindings->key_matches(k, "InfoScreen"))
    show_information();
  else if(bindings->key_matches(k, "Changelog") &&
	  !visible_version().end())
    show_changelog();
  else if(bindings->key_matches(k, "InstallSingle"))
    {
      if((*apt_cache_file)[package].CandidateVerIter(*apt_cache_file).end())
	return true;

      undo_group *grp=new apt_undo_group;
      (*apt_cache_file)->mark_single_install(package, grp);
      if(!grp->empty())
	apt_undos->add_item(grp);
      else
	delete grp;
    }
  else if (bindings->key_matches(k, "CollapseTree"))
    {
      // hack to convert a Left key stroke to moving to the parent (several
      // requests for this, like #241945 and #415449; with suggestions that it
      // could be done by moving to Parent --doing this at the moment-- or
      // folding the Parent subtree directly).
      //
      // unfortunately this cannot be done in a cleaner way than injecting from
      // this tree item to the owner, setting "Left" as a keybinding for
      // "Parent" is interpreted in other elements as well, thus disrupting
      // unrelated things, because it's not specific to tree items.
      //
      // NOTE: get("") needs to be passed as uppercase, see #818046
      cw::config::keybinding keybindings_Parent = cw::config::global_bindings.get("PARENT");
      return ((keybindings_Parent.size() > 0) && owner && owner->dispatch_key(keybindings_Parent[0]));
    }
  else
    return pkg_tree_node::dispatch_key(k, owner);

  return true;
}

void pkg_item::dispatch_mouse(short id, int x, mmask_t bstate, cw::tree *owner)
{
  if(bstate & (BUTTON1_DOUBLE_CLICKED | BUTTON2_DOUBLE_CLICKED |
	       BUTTON3_DOUBLE_CLICKED | BUTTON4_DOUBLE_CLICKED |
	       BUTTON1_TRIPLE_CLICKED | BUTTON2_TRIPLE_CLICKED |
	       BUTTON3_TRIPLE_CLICKED | BUTTON4_TRIPLE_CLICKED))
    show_information();
}

bool pkg_item::matches(const string &s) const
{
  cw::util::ref_ptr<aptitude::matching::pattern> p =
    aptitude::matching::parse(s);

  cw::util::ref_ptr<aptitude::matching::search_cache> info =
    aptitude::matching::search_cache::create();

  return aptitude::matching::get_match(p, package, visible_version(),
				       info,
				       *apt_cache_file,
				       *apt_package_records).valid();
}

pkgCache::VerIterator pkg_item::visible_version(const pkgCache::PkgIterator &pkg)
{
  pkgDepCache::StateCache &state=(*apt_cache_file)[pkg];

  pkgCache::VerIterator candver=state.CandidateVerIter(*apt_cache_file);

  if(!candver.end())
    return candver;
  else if(pkg.CurrentVer().end() && state.Install())
    return state.InstVerIter(*apt_cache_file);
  // Return the to-be-installed version
  else
    return pkg.CurrentVer();
}

const wchar_t *pkg_item::tag()
{
  // FIXME: ew
  static wstring pkgname;

  pkgname=cw::util::transcode(package.Name(), "ASCII");

  return pkgname.c_str();
}

const wchar_t *pkg_item::label()
{
  // FIXME: ew
  static wstring pkgname;

  pkgname=cw::util::transcode(package.Name(), "ASCII");

  return pkgname.c_str();
}

const pkgCache::PkgIterator &pkg_item::get_package() const
{
  return package;
}

pkgCache::VerIterator pkg_item::visible_version() const
{
  return visible_version(package);
}

//////////////////////// Menu redirections: /////////////////////////////

bool pkg_item::package_forbid_enabled()
{
  return true;
}

bool pkg_item::package_forbid()
{
  undo_group *grp=new apt_undo_group;

  forbid_upgrade(grp);

  if(!grp->empty())
    apt_undos->add_item(grp);
  else
    delete grp;

  package_states_changed();

  return true;
}

bool pkg_item::package_changelog_enabled()
{
  return true;
}

bool pkg_item::package_changelog()
{
  show_changelog();
  return true;
}

bool pkg_item::package_information_enabled()
{
  return true;
}

bool pkg_item::package_information()
{
  show_information();
  return true;
}
