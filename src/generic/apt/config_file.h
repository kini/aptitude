//
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
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


/** @file
 *
 * Helpers to interact with the config file.
 *
 */

#ifndef APTITUDE_GENERIC_APT_CONFIG_FILE_H
#define APTITUDE_GENERIC_APT_CONFIG_FILE_H

#include <string>


namespace aptitude {
namespace apt {


/** Class to help to manage the config file
 *
 * The implementation is not optimal, but it should be better than the old
 * implementation in apt.cc, if nothing else because the different parts are
 * more clearly separated and at least it attempts to address many problems of
 * the old implementation: avoids saving empty files, avoids rewriting if the
 * new config is the same as the old, etc (see bugs #504152, #502617, #442937,
 * #407284; some of them with duplicates).
 *
 * Some of this functionality can perhaps be moved from this to
 * generic/util/util.h if it's useful for other parts, but not many parts of
 * aptitude need to write files, and in any case it would need an extensive
 * review of all related code which is not the focus at the moment (this part
 * was doing everything on its own and apart from other parts of aptitude
 * anyway).
 *
 * This relies on boost::filesystem, when the C++ Filesystem TS becomes
 * available and stable in the compilers it should use that.
 */
class ConfigFile
{
 public:
  /** Attempt to write this content to the configuration file
   *
   * It will avoid writing the content in several situations, like if it's empty
   * (in that case it will attempt to remove the existing file), or avoids to
   * write if the existing file has the same contents (helps in the case of
   * read-only filesystems).
   *
   * @param content Content to write
   *
   * @return Whether the operation was successful
   */
  static bool write(const std::string& content);

 private:
  /** Get directory where the config is saved
   *
   * @return The requested value
   */
  static std::string get_config_dir();

  /** Ensure that the directory exists, or attempt to create it otherwise
   *
   * @param dirpath Path of the directory
   *
   * @return Whether the directory exists
   */
  static bool ensure_dir(const std::string& dirpath);

  /** Get content of the given file
   *
   * @param filepath Path of the file
   *
   * @param file_size_limit Avoid reading if the file exceeds this size
   *
   * @return Contents of the file
   */
  static std::string get_file_content(const std::string& filepath, size_t file_size_limit);

  /** Write given content to the the given file
   *
   * @param filepath Path of the file
   *
   * @param content Content to write
   *
   * @return Contents of the file
   */
  static bool write_file(const std::string& filepath, const std::string& content);

  /** Remove file
   *
   * @param filepath Path of the file
   *
   * @return Whether the operation was successful
   */
  static bool remove_file(const std::string& filepath);
};


}
}

#endif
