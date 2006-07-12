/*
 *  AC3MovieImport.c
 *
 *  Copyright (c) 2006  David Conrad
 *  Copyright (c) 2006  Graham Booker
 *
 *  This file is part of A52Codec
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
 */

/*
 *  This file is based on MatroskaImport.c, part of the 
 *  MatroskaQT, written by David Conrad
 *
 */

/*
 *  MatroskaImport.c is based on OggImport.c, part of the 
 *  XiphQT project, written by Arek Korbik
 *
 */

/*
 *  OggImport.c is based on the original importer code
 *  written by Steve Nicolai.
 *
 */

#include <QuickTime/QuickTime.h>
#include <AudioToolbox/AudioToolbox.h>
#include "versions.h"
#include "debug.h"

#include "a52.h"

#define kTimeScale (48000)

// Component globals
typedef struct {
	ComponentInstance		self;
	long					dataHOffset;
	long					fileSize;
	ComponentInstance		dataHandler;
	SoundDescriptionHandle	audioDesc;
    SInt64              dataStartOffset;
    SInt64              dataEndOffset;
} AC3MovieImportGlobalsRecord, *AC3MovieImportGlobals;

#if TARGET_OS_MAC
#define COMPONENTFUNC static pascal ComponentResult
#else
#define COMPONENTFUNC static ComponentResult
#endif

COMPONENTFUNC AC3MovieImportOpen(AC3MovieImportGlobals globals, ComponentInstance self);
COMPONENTFUNC AC3MovieImportClose(AC3MovieImportGlobals globals, ComponentInstance self);
COMPONENTFUNC AC3MovieImportVersion(AC3MovieImportGlobals globals);

COMPONENTFUNC AC3MovieImportSetOffsetAndLimit64(AC3MovieImportGlobals globals, 
												const wide *offset, const wide *limit);
COMPONENTFUNC AC3MovieImportSetOffsetAndLimit(AC3MovieImportGlobals globals, 
											  unsigned long offset, unsigned long limit);

COMPONENTFUNC AC3MovieImportValidate(AC3MovieImportGlobals globals, 
									 const FSSpec *         theFile,
									 Handle                 theData,
									 Boolean *              valid);
COMPONENTFUNC AC3MovieImportValidateDataRef(AC3MovieImportGlobals globals, 
											Handle                 dataRef,
											OSType                 dataRefType,
											UInt8 *                valid);
	
//COMPONENTFUNC AC3MovieImportIdle(AC3MovieImportGlobals globals,
//                                 long                   inFlags,
//                                 long *                 outFlags);
//COMPONENTFUNC AC3MovieImportSetIdleManager(AC3MovieImportGlobals globals, IdleManager im);
	
COMPONENTFUNC AC3MovieImportFile(AC3MovieImportGlobals globals, const FSSpec *theFile,
								 Movie theMovie, Track targetTrack, Track *usedTrack, TimeValue atTime, 
								 TimeValue *durationAdded, long inFlags, long *outFlags);
COMPONENTFUNC AC3MovieImportDataRef(AC3MovieImportGlobals globals, Handle dataRef,
									OSType dataRefType, Movie theMovie,
									Track targetTrack, Track *usedTrack,
									TimeValue atTime, TimeValue *durationAdded,
									long inFlags, long *outFlags);
//COMPONENTFUNC AC3MovieImportSetNewMovieFlags(AC3MovieImportGlobals globals, long flags);


COMPONENTFUNC AC3MovieImportGetMIMETypeList(AC3MovieImportGlobals globals, 
											QTAtomContainer *retMimeInfo);
COMPONENTFUNC AC3MovieImportGetFileType(AC3MovieImportGlobals globals, OSType *fileType);

//COMPONENTFUNC AC3MovieImportGetLoadState(AC3MovieImportGlobals globals, long *loadState);
COMPONENTFUNC AC3MovieImportGetMaxLoadedTime(AC3MovieImportGlobals globals, TimeValue *time);
//COMPONENTFUNC AC3MovieImportEstimateCompletionTime(AC3MovieImportGlobals globals, TimeRecord *time);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			Component Dispatcher
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define CALLCOMPONENT_BASENAME() 		AC3MovieImport
#define CALLCOMPONENT_GLOBALS() 		AC3MovieImportGlobals storage

#define MOVIEIMPORT_BASENAME() 			CALLCOMPONENT_BASENAME()
#define MOVIEIMPORT_GLOBALS() 			CALLCOMPONENT_GLOBALS()

#define COMPONENT_UPP_SELECT_ROOT()		MovieImport
#define COMPONENT_DISPATCH_FILE			"AC3MovieImportDispatch.h"

#include <CoreServices/Components.k.h>
#include <QuickTime/QuickTimeComponents.k.h>
#include <QuickTime/ComponentDispatchHelper.c>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			Component Routines
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportOpen(AC3MovieImportGlobals globals, ComponentInstance self)
{
	OSErr err;
	
	printf("allocing\n");
// Allocate memory for our globals, and inform the component manager that we've done so
	globals = (AC3MovieImportGlobals)NewPtrClear(sizeof(AC3MovieImportGlobalsRecord));
	if ((err = MemError())) goto bail;
	
	globals->self = self;
	
	SetComponentInstanceStorage(self, (Handle)globals);
	
bail:
		return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportClose(AC3MovieImportGlobals globals, ComponentInstance self)
{
#pragma unused(self)
	
	printf("closing\n");
// Make sure to dealocate our storage
	if (globals)
		DisposePtr((Ptr)globals);
	
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportVersion(AC3MovieImportGlobals globals)
{
	dprintf("-- Version() called\n");
	return kAC3Movie_eat__Version;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportSetOffsetAndLimit64(AC3MovieImportGlobals globals, 
												const wide *offset, const wide *limit)
{
	dprintf("-- SetOffsetAndLimit64(%ld%ld, %ld%ld) called\n", 
			offset->hi, offset->lo, limit->hi, limit->lo);
	globals->dataStartOffset = WideToSInt64(*offset);
	globals->dataEndOffset  = WideToSInt64(*limit);
	
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportSetOffsetAndLimit(AC3MovieImportGlobals globals, 
											  unsigned long offset, unsigned long limit)
{
	dprintf("-- SetOffsetAndLimit(%ld, %ld) called\n", offset, limit);
	globals->dataStartOffset = S64SetU(offset);	
	globals->dataEndOffset = S64SetU(limit);
	
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportValidate(AC3MovieImportGlobals   globals, 
									 const FSSpec *             theFile,
									 Handle                     theData,
									 Boolean *                  valid)
{
	ComponentResult err = noErr;
	UInt8 extvalid = 0;
	
	dprintf("-- Validate() called\n");
	if (theFile == NULL)
	{
		Handle	dataHandle = NewHandle(sizeof(HandleDataRefRecord));
		if (dataHandle != NULL)
		{
			(*(HandleDataRefRecord **)dataHandle)->dataHndl = theData;
			err = MovieImportValidateDataRef(globals->self,
											 dataHandle,
											 HandleDataHandlerSubType,
											 &extvalid);
			DisposeHandle(dataHandle);
		}
	}
	else
	{
		AliasHandle alias = NULL;
		FSRef theFileFSRef;
		
		err = FSpMakeFSRef(theFile, &theFileFSRef);
		if (err == noErr)
			err = FSNewAliasMinimal(&theFileFSRef, &alias);
		
		if (err == noErr)
		{
			err = MovieImportValidateDataRef(globals->self,
											 (Handle)alias,
											 rAliasType,
											 &extvalid);
			
			DisposeHandle((Handle)alias);
		}
	}
	
	if (extvalid > 0)
		*valid = true;
	else
		*valid = false;
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportValidateDataRef(AC3MovieImportGlobals    globals, 
											Handle                      dataRef,
											OSType                      dataRefType,
											UInt8 *                     valid)
{
#pragma unused(globals)
	
	OSErr err = noErr;
	DataHandler dataHandler = 0;
	unsigned char header[7];
	
	*valid = 0;
	
// OpenADataHandler is a 'convenience' function which finds, opens, and sets up a data handler
// component in a single call. It's internal implementation uses the sequence of GetDataHandler,
// OpenAComponent and SetDataRef calls. 
	err = OpenADataHandler(dataRef, dataRefType, NULL, 0, NULL, kDataHCanRead, &dataHandler);
	if (err || NULL == dataHandler) goto bail;
	
	err = DataHScheduleData(dataHandler, (Ptr)&header, 0, sizeof(header), 0, NULL, NULL);
	if (err) goto bail;
	
// If your image format has a "magic number" at the start, this is where you should look for it.
	
// Electric Image files don't have a magic number, but they do have a version code, which should
// be a big-endian two-byte integer "5".  So we'll check that and the number of frames which should
// be at least 1.
	int flags;
	int sample_rate;
	int bit_rate;
	int frame_size = a52_syncinfo(header, &flags, &sample_rate, &bit_rate);
	if (frame_size > 0) {
		*valid = 128;
	} 	
	
bail:
		if (dataHandler)
			CloseComponent(dataHandler);
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportFile(AC3MovieImportGlobals globals, const FSSpec *theFile,
								 Movie theMovie, Track targetTrack, Track *usedTrack,
								 TimeValue atTime, TimeValue *durationAdded,
								 long inFlags, long *outFlags)
{
	ComponentResult err = noErr;
	AliasHandle alias = NULL;
	FSRef theFileFSRef;
	
	dprintf("-- File() called\n");
	
	*outFlags = 0;
	
	err = FSpMakeFSRef(theFile, &theFileFSRef);
	if (err == noErr)
		err = FSNewAliasMinimal(&theFileFSRef, &alias);
	
	if (err == noErr)
	{
		err = MovieImportDataRef(globals->self,
								 (Handle)alias,
								 rAliasType,
								 theMovie,
								 targetTrack,
								 usedTrack,
								 atTime,
								 durationAdded,
								 inFlags,
								 outFlags);
		
		if (!(*outFlags & movieImportResultNeedIdles))
			DisposeHandle((Handle) alias);
		/*        else
			globals->aliasHandle = (Handle)alias;*/
	}
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportDataRef(AC3MovieImportGlobals globals, Handle dataRef,
									OSType dataRefType, Movie theMovie,
									Track targetTrack, Track *usedTrack,
									TimeValue atTime, TimeValue *durationAdded,
									long inFlags, long *outFlags)
{
#pragma unused(globals,targetTrack)
	
	OSErr err = noErr;
	Track audioTrack = NULL;
	Media audioMedia;
	ComponentInstance dataHandler = 0;
	unsigned char header[7];
	long fileOffset = 0, fileSize;
	
	*outFlags = 0;
	
// movieImportMustUseTrack indicates that we must use an existing track, because we
// don't support this and always create a new track return paramErr.
	if (inFlags & movieImportMustUseTrack)
		return paramErr;
	
// Retrieve the best data handler component to use with the given data reference,
// the kDataHCanRead flag specifies that you intend to use the data handler component to read data.
// Then open the returned component using standard Component Manager calls.
	err = OpenAComponent(GetDataHandler(dataRef, dataRefType, kDataHCanRead), &dataHandler);
	if (err) goto bail;
	
// Provide a data reference to the data handler.
// Once you have assigned a data reference to the data handler, you may start reading and/or writing
// movie data from that data reference.
	err = DataHSetDataRef(dataHandler, dataRef);
	if (err) goto bail;
	
// Open a read path to the current data reference. You need to do this before your component can read
// data using a data handler component.
	err = DataHOpenForRead(dataHandler);
	if (err) goto bail;
	
// Get the size, in bytes, of the current data reference, this is
// functionally equivalent to the File Manager's GetEOF function.
	err = DataHGetFileSize(dataHandler, &fileSize);
	if (err) goto bail;
	
	while (fileOffset < fileSize) {
		err = DataHScheduleData(dataHandler, (Ptr)header, fileOffset, sizeof(header), 0, NULL, NULL);
		if (err) goto bail;
		
		int flags;
		int sample_rate;
		int bit_rate;
		int frame_size = a52_syncinfo(header, &flags, &sample_rate, &bit_rate);
		if(frame_size == 0)
		{
			fileOffset++;
			continue;
		}
		if (audioTrack == NULL) {
		// Create a new video track
			audioTrack = NewMovieTrack(theMovie,					// Specify the Movie
									   0,							// A fixed denoting width of the track, in pixels.
									   0,							// A fixed denoting height of the track, in pixels
									   kFullVolume);					// Specifies the volume setting of the track
			
		// Create a media for the track. The media refers to the actual data samples used by the track. 
			audioMedia = NewTrackMedia(audioTrack,		// Specifies the track for this operation
									   SoundMediaType,	// Type of media to create
									   kTimeScale,		// Set the time scale, this defines the media's time coordinate system
									   dataRef,			// Specifies the data reference
									   dataRefType);	// Specifies the type of data reference
			if ((err = GetMoviesError())) goto bail;
			
			int channels = 0;
			int layout = 0;
			switch(flags & A52_CHANNEL_MASK)
			{
				case A52_MONO:
				case A52_CHANNEL1:
				case A52_CHANNEL2:
					channels = 1;
					layout = kAudioChannelLayoutTag_Mono;
					break;
				case A52_CHANNEL:
				case A52_STEREO:
				case A52_DOLBY:
					channels = 2;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_DVD_4;
					else
						layout = kAudioChannelLayoutTag_Stereo;
					break;
				case A52_3F:
					channels = 3;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_DVD_10;
					else
						layout = kAudioChannelLayoutTag_ITU_3_0;
					break;
				case A52_2F1R:
					channels = 3;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_DVD_5;
					else
						layout = kAudioChannelLayoutTag_ITU_2_1;
					break;
				case A52_3F1R:
					channels = 4;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_DVD_11;
					else
						layout = kAudioChannelLayoutTag_ITU_3_1;
					break;
				case A52_2F2R:
					channels = 4;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_DVD_6;
					else
						layout = kAudioChannelLayoutTag_ITU_2_2;
					break;
				case A52_3F2R:
					channels = 5;
					if(flags & A52_LFE)
						layout = kAudioChannelLayoutTag_ITU_3_2_1;
					else
						layout = kAudioChannelLayoutTag_ITU_3_2;
					break;
			}
			
			AudioStreamBasicDescription absd;
			memset(&absd, 0, sizeof(absd));
			absd.mFormatID = kAudioFormatAC3;
			absd.mSampleRate = 48000;
			if(flags & A52_LFE)
				channels++;
			absd.mChannelsPerFrame = channels;
			
			AudioChannelLayout acl;
			acl.mChannelLayoutTag = layout;
			acl.mChannelBitmap = 0;
			acl.mNumberChannelDescriptions = 0;
			
			err = QTSoundDescriptionCreate(&absd, &acl, sizeof(acl), NULL, 0, kQTSoundDescriptionKind_Movie_Version2, &(globals->audioDesc));
			if(err) goto bail;
			
		// Enable the track.
			SetTrackEnabled(audioTrack, true);
		}
		
	// Add the sample -  AddMediaSampleReference does not add sample data to the file or device that contains a media.
	// Rather, it defines references to sample data contained elswhere. Note that one reference may refer to more
	// than one sample--all the samples described by a reference must be the same size. This function does not update the movie data
	// as part of the add operation therefore you do not have to call BeginMediaEdits before calling AddMediaSampleReference.
		err = AddMediaSampleReference(audioMedia,		// Specifies the media for this operation
									  fileOffset,		// Specifies the offset into the data file
									  frame_size,		// Specifies the number of bytes of sample data to be identified by the reference
									  6 * 256,	// Specifies the duration of each sample in the reference
									  (SampleDescriptionHandle)globals->audioDesc,	 // Handle to a sample description
									  1,				// Specifies the number of samples contained in the reference
									  0,				// Contains flags that control the operation	
									  NULL);			// Returns the time where the reference was inserted, NULL to ignore
		if (err) goto bail;
		
	// Increment to the next frame
		fileOffset += frame_size;
	}
	
// Insert the added media into the track
// The InsertMediaIntoTrack function inserts a reference to a media segment into a track. Specify the segment in the media
// by providing a starting time and duration and specify the point in the destination track by providing a time in the track
	err = InsertMediaIntoTrack(audioTrack,					// Specifies the track
							   0,						// Time value specifying where the segment is to be inserted in the movie's time scale
														// -1 to add the media data to the end of the track
							   0,							// Time value specifying the starting point of the segment in the media's time scale
							   GetMediaDuration(audioMedia),	// time value specifying the duration of the media's segment in the media's time scale
							   fixed1);						// Specifies the media's rate, 1.0 indicates natural playback rate - value should be a positive, nonzero 0x010000
	if (err) goto bail;
	
// Report the duration added
	*durationAdded = GetTrackDuration(audioTrack) - atTime;
	
bail:
		if (audioTrack) {
			if (err) {
				DisposeMovieTrack(audioTrack);
				audioTrack = NULL;
			} else {
		// Set the outFlags to reflect what was done.
				*outFlags |= movieImportCreateTrack;
			}
		}
	
// Remember to close what you open.
	if (dataHandler)
		CloseComponent(dataHandler);
	
// Return the track identifier of the track that received the imported data in the usedTrack pointer. Your component
// needs to set this parameter only if you operate on a single track or if you create a new track. If you modify more
// than one track, leave the field referred to by this parameter unchanged. 
	if (usedTrack) *usedTrack = audioTrack;
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportGetFileType(AC3MovieImportGlobals globals, OSType *fileType)
{
	dprintf("-- GetFileType() called\n");
	*fileType = kAC3MovieFormat;
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportGetMIMETypeList(AC3MovieImportGlobals globals, 
											QTAtomContainer *retMimeInfo)
{
	dprintf("-- GetMIMETypeList() called\n");
	return GetComponentResource((Component)globals->self, FOUR_CHAR_CODE('mime'), 
								kImporterResID, (Handle *)retMimeInfo);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENTFUNC AC3MovieImportGetMaxLoadedTime(AC3MovieImportGlobals globals, TimeValue *time)
{
	/*	dprintf("-- GetMaxLoadedTime() called: %8ld (at: %ld)\n", globals->timeLoaded, TickCount());
	
	*time = globals->timeLoaded;*/
	
	return noErr;
}

pascal ComponentResult AC3MovieImportIdle(AC3MovieImportGlobals globals, long inFlags, long *outFlags)
{
	return noErr;
}