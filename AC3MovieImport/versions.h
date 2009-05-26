/*
 *  versions.h
 *
 *    The current version of the AC3MovieImport component.
 *
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This file is part of A52Codec, based off of the same file in XiphQT.
 *
 *  A52Codec is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  A52Codec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with A52Codec; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: versions.h 10322 2006-02-26 10:40:18Z dconrad1 $
 *
 */


#if !defined(__versions_h__)
#define __versions_h__

#ifdef DEBUG
#define kAC3Movie_eat__Version		(0x00FF0100)
#else
#define kAC3Movie_eat__Version		(0x00070005)
#endif /* DEBUG */


#define kAC3MovieBundleID   "com.cod3r.ac3movieimport"

#define kImporterResID                  4000
#define kImporterNameStringResID        4000
#define kImporterInfoStringResID        4001

#define kMovieComponentManufacturer     'cod3'	
#define kAC3MovieFormat                 'ac-3' 


#ifdef _DEBUG
#define AC3MovieImporterName        "AC3 Importer"
#else
#define AC3MovieImporterName        "AC3 Importer"
#endif

#endif /* __versions_h__ */
