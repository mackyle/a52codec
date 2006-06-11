/*
 *  AC3MovieImport.r
 *
 *    Information bit definitions for the 'thng' and other OggImport
 *    resources.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
 *  Copyright (c) 2006  David Conrad
 *  Copyright (c) 2006  Graham Booker
 *
 *  This file is based off of OggImport.r from XiphQT, the Xiph QuickTime 
 *  Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  
 *  AC3MovieImport is distributed with the same conditions as outlined above.
 *
 */


#define TARGET_REZ_CARBON_MACHO 1

#define thng_RezTemplateVersion 2

#define cfrg_RezTemplateVersion 1

#if TARGET_REZ_CARBON_MACHO

    #if defined(ppc_YES)
        // PPC architecture
        #define TARGET_REZ_MAC_PPC 1
    #else
        #define TARGET_REZ_MAC_PPC 0
    #endif

    #if defined(i386_YES)
        // x86 architecture
        #define TARGET_REZ_MAC_X86 1
    #else
        #define TARGET_REZ_MAC_X86 0
    #endif

    #define TARGET_REZ_WIN32 0
#else
    // Must be building on Windows
    #define TARGET_REZ_WIN32 1
#endif

#if TARGET_REZ_CARBON_MACHO
    #include <Carbon/Carbon.r>
    #include <QuickTime/QuickTime.r>
	#undef __CARBON_R__
	#undef __CORESERVICES_R__
	#undef __CARBONCORE_R__
	#undef __COMPONENTS_R__
#else
    #include "ConditionalMacros.r"
    #include "MacTypes.r"
    #include "Components.r"
    #include "QuickTimeComponents.r"
    #include "CodeFragments.r"
	#undef __COMPONENTS_R__
#endif

#include "versions.h"

#define kImporterComponentType 'eat '


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AC3 Importer

#define kImporterFlags canMovieImportFiles | canMovieImportValidateFile | \
		canMovieImportPartial | canMovieImportInPlace | hasMovieImportMIMEList | \
		canMovieImportDataReferences | canMovieImportValidateDataReferences | \
        cmpThreadSafe


resource 'thng' (kImporterResID, AC3MovieImporterName, purgeable) {
    kImporterComponentType, 
    kAC3MovieFormat, 
    'vide', 
    0, 0, 0, 0,
    'STR ', kImporterNameStringResID,
    'STR ', kImporterInfoStringResID,
    0, 0,		// no icon
    kAC3Movie_eat__Version,
    componentDoAutoVersion|componentHasMultiplePlatforms, 0,
    {
#if TARGET_OS_MAC	       // COMPONENT PLATFORM INFORMATION ----------------------
    #if TARGET_REZ_CARBON_MACHO
        #if !(TARGET_REZ_MAC_PPC || TARGET_REZ_MAC_X86)
        	#error "Platform architecture not defined, TARGET_REZ_MAC_PPC and/or TARGET_REZ_MAC_X86 must be defined!"
        #endif
        #if TARGET_REZ_MAC_PPC
            kImporterFlags, 
            'dlle',								// Code Resource type - Entry point found by symbol name 'dlle' resource
            kImporterResID,								// ID of 'dlle' resource
            platformPowerPCNativeEntryPoint,	// PPC
        #endif
        #if TARGET_REZ_MAC_X86
			kImporterFlags,
			'dlle',
            kImporterResID,
            platformIA32NativeEntryPoint,		// INTEL
        #endif
	#elif TARGET_REZ_CARBON_CFM
        #error "Obsolete, new components should not be built for this configuration."
		kImporterFlags,				// Component Flags
		'cfrg',								// Special Case: data-fork based code fragment
		kImporterResID,	 							/* Code ID usage for CFM components:
												0 (kCFragResourceID) - This means the first member in the code fragment;
													Should only be used when building a single component per file. When doing so
													using kCFragResourceID simplifies things because a custom 'cfrg' resource is not required
												n - This value must match the special 'cpnt' qualifier 1 in the custom 'cfrg' resource */
		platformPowerPCNativeEntryPoint,	// Platform Type (response from gestaltComponentPlatform or failing that, gestaltSysArchitecture)
	#elif TARGET_REZ_MAC_PPC
        #error "Obsolete, new components should not be built for this configuration."
		kImporterFlags, 
		'eat ',								// Code Type
		kImporterResID,								// Code ID
		platformPowerPC,
	#elif TARGET_REZ_MAC_68K
    	#error "Obsolete, new components should not be built for this configuration."
		kImporterFlags,
		'eat ',
		kImporterResID,
		platform68k,
	#else
		#error "At least one TARGET_REZ_XXX_XXX platform must be defined."
	#endif
#endif
#if TARGET_OS_WIN32
    kImporterFlags, 
    'dlle',
    kImporterResID,
    platformWin32,
#endif
    },
    'thnr', kImporterResID
};

// Component Alias
resource 'thga' (kImporterResID, AC3MovieImporterName, purgeable) {
    kImporterComponentType,                    // Type
    'AC3 ',                             // Subtype - this must be in uppercase. It will match an ".ac3" suffix case-insensitively.
    'vide',                             // media type supported
    kImporterFlags | movieImportSubTypeIsFileExtension,	// The subtype is a file name suffix
    0,                                  // Component Flags Mask
    0,                                  // Code Type
    0,                                  // Code ID
    'STR ',                             // Name Type
    kImporterNameStringResID,           // Name ID
    'STR ',                             // Info Type
    kImporterInfoStringResID,           // Info ID 
    0,                                  // Icon Type 
    0,                                  // Icon ID
                // TARGET COMPONENT ---------------
    kImporterComponentType,                    // Type
    kAC3MovieFormat,                    // SubType
    'vide',                             // Manufaturer
    kImporterFlags,                     // Component Flags
    0,                                  // Component Flags Mask
    'thnr', kImporterResID, 0
};

resource 'thnr' (kImporterResID, AC3MovieImporterName, purgeable) {
    {
        'mime', 1, 0, 'mime', kImporterResID, cmpResourceNoFlags,
        'mcfg', 1, 0, 'mcfg', kImporterResID, cmpResourceNoFlags,
    };
};


#if TARGET_REZ_CARBON_MACHO /* || TARGET_REZ_WIN32 */
resource 'dlle' (kImporterResID, AC3MovieImporterName) {
    "AC3MovieImportComponentDispatch"
};
#endif

// name and info string are shared by the compressor and decompressor
resource 'STR ' (kImporterNameStringResID, AC3MovieImporterName, purgeable) {
    "AC3 Importer"
};

resource 'STR ' (kImporterInfoStringResID, AC3MovieImporterName, purgeable) {
    "AC3 " "0.1" " (See http://trac.cod3r.com/a52codec)."
};


/* 
    This is an example of how to build an atom container resource to hold mime types.
    This component's GetMIMETypeList implementation simply loads this resource and returns it.
    Please note that atoms of the same type MUST be grouped together within an atom container.
*/
resource 'mime' (kImporterResID, AC3MovieImporterName, purgeable) {
    {
        kMimeInfoMimeTypeTag,      1, "audio/x-ac3";
        kMimeInfoMimeTypeTag,      2, "audio/ac3";
        kMimeInfoFileExtensionTag, 1, "ac3";
        kMimeInfoFileExtensionTag, 2, "ac3";
        kMimeInfoDescriptionTag,   1, "AC3 Audio";
        kMimeInfoDescriptionTag,   2, "AC3 Audio";
    };
};

resource 'mcfg' (kImporterResID, AC3MovieImporterName, purgeable) {
    kVersionDoesntMatter,
    {
        kQTMediaConfigVideoGroupID,
        kQTMediaConfigBinaryFile | kQTMediaConfigCanUseApp,
        kAC3MovieFormat,
        'TVOD',	/* we don't have a creator code for our files, hijack QT player */
        kImporterComponentType, 
        kAC3MovieFormat, 
        'moov',
        0, 0,
        'AC3 ',
        kQTMediaInfoNetGroup,
        
        /* no synonyms */
        {
        },
        
        {
        "AC3 video file",
        "ac3",
        "QuickTime Player",
        "AC3 file importer",
        "",
        },
        
        /* mime types array */
        {
            "audio/x-ac3";
            "audio/ac3";
        };
//    };
//    {
    };
};


