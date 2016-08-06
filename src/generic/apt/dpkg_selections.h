//
// Copyright 2015-2016 Manuel A. Fernandez Montecelo
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


/** @file
 *
 * Helpers to interact with dpkg selections (mark packages to install, hold,
 * etc.).
 *
 * \todo At the moment (Sep 2015) writing selections is only implemented in
 * "cmdline/apt-mark.cc" for the apt-mark command.  If this is implemented in
 * libapt-pkg in the future, it should probably use that.
 *
 */

#ifndef APTITUDE_GENERIC_APT_DPKG_SELECTIONS_H
#define APTITUDE_GENERIC_APT_DPKG_SELECTIONS_H

#include <apt-pkg/pkgcache.h>

#include <string>


namespace aptitude {
namespace apt {
namespace dpkg {


/** Class to help to manage the selections and interface with dpkg to enable
 *  them
 */
class DpkgSelections
{
 public:
  /** Constructor */
  DpkgSelections();
  /** Destructor */
  ~DpkgSelections();

  /** Add package selection
   *
   * @param package_name Package name to add
   *
   * @param package_arch Package arch to add
   *
   * @param state Selected state
   */
  inline void add(const std::string& package_name, const std::string& package_arch, pkgCache::State::PkgSelectedState state)
  {
    std::string state_str = to_string(state);
    if (! state_str.empty())
      {
	selections += package_name + ":" + package_arch;
	selections += " ";
	selections += state_str;
	selections += "\n";
      }
  }

  /** Clear selections so far */
  void clear();

  /** Save the selections (enable changes in dpkg database)
   *
   * @return Whether the operation was successful
   */
  bool save_selections();

  /** Translate package selection state to string
   *
   * @param state Selected state
   *
   * @return Value as string
   */
  static inline std::string to_string(pkgCache::State::PkgSelectedState state)
  {
    std::string dpkg_dselect_action;
    switch (state)
      {
      case pkgCache::State::Unknown:   dpkg_dselect_action = "unknown";   break;
      case pkgCache::State::Purge:     dpkg_dselect_action = "purge";     break;
      case pkgCache::State::DeInstall: dpkg_dselect_action = "deinstall"; break;
      case pkgCache::State::Install:   dpkg_dselect_action = "install";   break;
      case pkgCache::State::Hold:      dpkg_dselect_action = "hold";      break;
      default:                         dpkg_dselect_action = "";          break;
      }

    return dpkg_dselect_action;
  }

 private:
  /** Storage for the selections */
  std::string selections;


  /** Helper to actually call dpkg and set selections
   *
   * @param selections Selections to set
   *
   * @return Whether the operation was successful
   */
  static bool save_to_dpkg(const std::string& selections);
};


}
}
}

#endif
