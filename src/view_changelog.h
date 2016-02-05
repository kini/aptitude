// view_changelog.h
//
//   Copyright 2004 Daniel Burrows
//   Copyright (C) 2015-2016 Manuel A. Fernandez Montecelo

#ifndef VIEW_CHANGELOG_H
#define VIEW_CHANGELOG_H

#include <apt-pkg/pkgcache.h>

/** \brief  A utility function to view a package's changelog.  Inserts widgets
 *  into the main UI as appropriate.
 * 
 *  \file view_changelog.h
 */

void view_changelog(pkgCache::VerIterator ver);

#endif
