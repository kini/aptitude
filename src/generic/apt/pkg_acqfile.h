// pkg_acqfile.h                  -*-c++-*-
//
//   Copyright (C) 2002, 2005 Daniel Burrows
//   Copyright 2015 Manuel A. Fernandez Montecelo
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; see the file COPYING.  If not, write to
//   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//   Boston, MA 02110-1301, USA.


#ifndef PKG_ACQFILE_H
#define PKG_ACQFILE_H

#include <apt-pkg/acquire-item.h>

/** \file pkg_acqfile.h
 */

/** Like pkgAcqArchive, but uses generic File objects to download to
 *  the cwd (and copies from file:/ URLs).
 */
bool get_archive(pkgAcquire *Owner, pkgSourceList *Sources,
		 pkgRecords *Recs, pkgCache::VerIterator const &Version,
		 std::string directory, std::string &StoreFilename);

#endif
