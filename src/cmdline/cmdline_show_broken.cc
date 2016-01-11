// cmdline_show_broken.cc
//
//   Copyright 2004 Daniel Burrows
//   Copyright 2015 Manuel A. Fernandez Montecelo

#include "cmdline_show_broken.h"

#include "cmdline_common.h"
#include "aptitude.h"
#include "generic/apt/apt.h"

#include <cwidget/generic/util/ssprintf.h>

#include <cstdio>
#include <cstring>

using namespace std;


/** Helper function for other functions in this file showing information about
 * broken packages, to print an explanation about the installability of the
 * package provided.
 *
 * For example, a broken package might be so because it conflicts with a package
 * that remains installed, or depends on a version of a package not to be
 * installed, or depends on a package but the user requested it to be removed,
 * etc.
 *
 * @pkg The package to consider
 */
void print_installation_explanation(const pkgCache::PkgIterator& pkg)
{
  pkgCache::VerIterator ver = (*apt_cache_file)[pkg].InstVerIter(*apt_cache_file);

  if (!ver.end()) // ok, it's installable.
    {
      if ((*apt_cache_file)[pkg].Install())
	printf(_("but %s is to be installed"),
	       ver.VerStr());
      else if ((*apt_cache_file)[pkg].Upgradable())
	printf(_("but %s is installed and it is kept back"),
	       pkg.CurrentVer().VerStr());
      else
	printf(_("but %s is installed"),
	       pkg.CurrentVer().VerStr());
    }
  else
    {
      pkgCache::VerIterator cand = (*apt_cache_file)[pkg].CandidateVerIter(*apt_cache_file);
      if (cand.end())
	printf(_("but it is not installable"));
      else
	printf(_("but it is not going to be installed"));
    }
}

void show_broken_deps(const pkgCache::PkgIterator& pkg)
{
  string pkg_name_segment = string(" ") + pkg.FullName(true) + " :";
  const unsigned int indent = pkg_name_segment.length();
  cout << pkg_name_segment;

  pkgCache::VerIterator ver = (*apt_cache_file)[pkg].InstVerIter(*apt_cache_file);
  if (ver.end() == true)
    {
      cout << endl;
      return;
    }

  bool is_first_dep = true;

  for (pkgCache::DepIterator dep = ver.DependsList(); !dep.end(); ++dep)
    {
      pkgCache::DepIterator first = dep, prev = dep;

      while (dep->CompareOp & pkgCache::Dep::Or)
	++dep;

      // Yep, it's broken.
      if (dep.IsCritical() &&
	 !((*apt_cache_file)[dep]&pkgDepCache::DepGInstall))
	{		  
	  bool is_first_of_or = true;
	  // Iterate over the OR group, print out the information.

	  do
	    {
	      if (!is_first_dep)
		for (unsigned int i = 0; i < indent; ++i)
		  printf(" ");

	      is_first_dep = false;

	      size_t indent_dep = strlen(dep.DepType()) + 3;
	      if (!is_first_of_or)
		for (size_t i = 0; i < indent_dep; ++i)
		  printf(" ");
	      else
		printf(" %s: ", first.DepType());

	      is_first_of_or = false;

              cout << first.TargetPkg().FullName(true);

	      if (first.TargetVer())
		printf(" (%s %s)", first.CompType(), first.TargetVer());

	      pkgCache::PkgIterator target = first.TargetPkg();
	      // Don't skip real packages which are provided.
	      if (!target.VersionList().end())
		{
		  printf(" ");
		  print_installation_explanation(target);
		}
	      else
		{
		  if (target.ProvidesList().end())
		    {
		      printf(_(" which is a virtual package and is not provided by any available package\n"));
		    }
		  else
		    {
		      printf(_(" which is a virtual package, provided by:\n"));

		      for (pkgCache::PrvIterator prv = target.ProvidesList(); !prv.end(); ++prv)
			{
			  pkgCache::PkgIterator prv_pkg = prv.OwnerPkg();
			  pkgCache::VerIterator prv_ver = prv.OwnerVer();
			  if (!prv_pkg.end() && !prv_ver.end())
			    {
			      // version of the package providing the virtual
			      // package
			      std::string prv_pkg_ver_str = prv_ver.VerStr() ? (string(" (") + prv_ver.VerStr() + ")") : "";

			      // versioned provides, show only if exist and
			      // doesn't match (if it's a special version for
			      // the virtual package)
			      std::string prv_versioned_str = "";
			      if (prv_ver.VerStr() &&
				  prv.ProvideVersion() &&
				  string(prv_ver.VerStr()) != string(prv.ProvideVersion()))
				{
				  prv_versioned_str = cwidget::util::ssprintf(_(" provides %s=%s"), target.FullName(true).c_str(), prv.ProvideVersion());
				}

			      for (size_t i = 0; i < (indent + indent_dep); ++i)
				printf(" ");
			      printf("- %s%s%s, ", prv_pkg.FullName(true).c_str(), prv_pkg_ver_str.c_str(), prv_versioned_str.c_str());
			      print_installation_explanation(prv_pkg);
			      printf("\n");
			    }
			}
		    }
		}

	      if (first != dep)
		printf(_(" or"));

	      printf("\n");

	      prev = first;
	      ++first;
	    } while (prev != dep);
	}
    }
}

bool show_broken()
{
  pkgvector broken;
  for (pkgCache::PkgIterator i = (*apt_cache_file)->PkgBegin(); !i.end(); ++i)
    {
      if ((*apt_cache_file)[i].InstBroken())
	broken.push_back(i);
    }

  if (!broken.empty())
    {
      printf(_("The following packages have unmet dependencies:\n"));

      for (pkgvector::const_iterator pkg = broken.begin(); pkg != broken.end(); ++pkg)
	show_broken_deps(*pkg);
      return false;
    }

  return true;
}
