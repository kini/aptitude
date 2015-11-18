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


#include "config_file.h"

#include "aptitude.h"

#include "generic/util/util.h"

#include <apt-pkg/error.h>

#include <boost/filesystem.hpp>

#include <fstream>
#include <sstream>
#include <streambuf>

#include <error.h>
#include <sys/stat.h>


using std::string;
namespace fs = boost::filesystem;


namespace aptitude {
namespace apt {


bool ConfigFile::write(const std::string& content)
{
  string cfg_dir_path = get_config_dir();
  string cfg_file_path = cfg_dir_path + "/config";

  if (content.empty())
    {
      // if new config is empty and old file exists, should be removed,
      // independently of its contents
      bool result_ok = remove_file(cfg_file_path);
      if (!result_ok)
	{
	  _error->Error(_("Could not remove old configuration file: %s"), cfg_file_path.c_str());
	  return false;
	}

      // if config is empty, directory might be empty, attempt to remove (but if
      // fails, no problem)
      try
	{
	  fs::remove(cfg_dir_path);
	}
      catch (...) { }

      // if we arrive here, nothing remaining to do, old files were removed if
      // existed, and new ones not written
      return true;
    }
  else
    {
      // if new config is NOT empty and old file exists, check whether the
      // content is already there

      if (fs::is_regular_file(cfg_file_path))
	{
	  // read old content
	  std::string old_content;
	  try
	    {
	      size_t file_size_limit = 1024*1024;
	      old_content = get_file_content(cfg_file_path, file_size_limit);
	    }
	  catch (...)
	    {
	      _error->Error(_("Could not get the content of the old configuration file: %s"), cfg_file_path.c_str());
	      return false;
	    }

	  // is new content same as old?
	  if (content == old_content)
	    {
	      // if content is the same, no need to write
	      return true;
	    }
	  else
	    {
	      // if old and new content are different, write new content and
	      // return operation result
	      bool result = write_file(cfg_file_path, content);
	      return result;
	    }
	}
      else if (fs::exists(cfg_file_path))
	{
	  _error->Error(_("Old configuration file is not a regular file: %s"), cfg_file_path.c_str());
	  return false;
	}
      else
	{
	  // old config file does not exist, create new

	  // ensure that directory exists
	  bool dir_ok = ensure_dir(cfg_dir_path);
	  if (! dir_ok)
	    {
	      _error->Error(_("Directory does not exist and could not create it: %s"), cfg_dir_path.c_str());
	      return false;
	    }

	  // write content and return operation result
	  bool result = write_file(cfg_file_path, content);
	  return result;
	}
    }
}


std::string ConfigFile::get_config_dir()
{
  // get home directory
  std::string home_dir;
  const char* envHOME = getenv("HOME");
  if (!strempty(envHOME))
    {
      home_dir = envHOME;
    }
  else
    {
      home_dir = get_homedir();
    }

  // aptitude configuration dir
  std::string cfg_dir = home_dir + "/.aptitude";

  return cfg_dir;
}


bool ConfigFile::ensure_dir(const std::string& dirpath)
{
  if (mkdir(dirpath.c_str(), 0700)<0 && errno != EEXIST)
    {
      _error->Errno("mkdir", "%s", dirpath.c_str());
      return false;
    }
  else
    {
      return true;
    }
}


std::string ConfigFile::get_file_content(const std::string& filepath, size_t file_size_limit)
{
  std::string content;

  try
    {
      std::ifstream file(filepath);

      file.seekg(0, std::ios::end);
      size_t file_size = file.tellg();
      if (file_size > file_size_limit)
	{
	  _error->Error(_("File is unexpectedly large (> %zu bytes): %s"),
			file_size_limit, filepath.c_str());
	  throw std::runtime_error("");
	}
      content.reserve(file_size);
      file.seekg(0, std::ios::beg);

      content.assign(std::istreambuf_iterator<char>(file),
		     std::istreambuf_iterator<char>());

      return content;
    }
  catch (...)
    {
      _error->Error(_("Error reading from old configuration file: %s"), filepath.c_str());
      throw std::runtime_error("");
    }
}


bool ConfigFile::write_file(const std::string& filepath, const std::string& content)
{
  try
    {
      // perhaps should use generic/util/temp.* or be implemented in a better
      // way, but this is better than what was before (see #764046)
      std::string filepath_pseudorand = filepath + std::to_string(getpid());

      std::ofstream file(filepath_pseudorand);
      if (!file)
	{
	  throw std::runtime_error("");
	}
      else
	{
	  file << content;
	  file.close();
	}

      fs::rename(filepath_pseudorand, filepath);

      return true;
    }
  catch (...)
    {
      _error->Error(_("Error writing to file: %s"), filepath.c_str());
      return false;
    }
}


bool ConfigFile::remove_file(const std::string& filepath)
{
  try
    {
      if ( ! fs::exists(filepath))
	{
	  return true;
	}
      else
	{
	  if (fs::is_regular_file(filepath))
	    {
	      bool result_ok = fs::remove(filepath);
	      if (result_ok)
		{
		  return true;
		}
	      else
		{
		  _error->Error(_("Could not remove file: %s"), filepath.c_str());
		  return false;
		}
	    }
	  else
	    {
	      _error->Error(_("Will not remove file, it is not a regular file: %s"), filepath.c_str());
	      return false;
	    }
	}
    }
  catch (...)
    {
      _error->Error(_("Error removing file: %s"), filepath.c_str());
      return false;
    }
}

}
}
