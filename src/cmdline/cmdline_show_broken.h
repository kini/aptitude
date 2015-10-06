// cmdline_show_broken.h                  -*-c++-*-
//
//   Copyright 2004 Daniel Burrows
//   Copyright 2015 Manuel A. Fernandez Montecelo

#ifndef CMDLINE_SHOW_BROKEN_H
#define CMDLINE_SHOW_BROKEN_H

#include <apt-pkg/pkgcache.h>


/** \file cmdline_show_broken.h
 */

/** Shows a list of all broken packages together with their dependencies.
 *  Similar to and based on the equivalent routine in apt-get.
 *
 *  @return \b false if some packages are broken.
 */
bool show_broken();


/** Shows broken dependencies for a single package
 *
 * @pkg The package
 */
void show_broken_deps(const pkgCache::PkgIterator& pkg);


#endif // CMDLINE_SHOW_BROKEN
