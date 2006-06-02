/*	Copyright: 	© Copyright 2004 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	SMACsdec.cpp

=============================================================================*/

#if USE_DIRECT_DRMZ
// this works because all our sound component calls are to our source
#define SoundComponentSetSource			drmzDecomp_SetSource
#define SoundComponentSetOutput			drmzDecomp_SetOutput
#define SoundComponentGetInfo			drmzDecomp_GetInfo
#define SoundComponentSetInfo			drmzDecomp_SetInfo
#define SoundComponentPlaySourceBuffer	drmzDecomp_PlaySourceBuffer
#define SoundComponentStopSource		drmzDecomp_StopSource
#define SoundComponentGetSourceData		drmzDecomp_GetSourceData
#endif

#if USE_DIRECT_ADEC
// remap calls to AudioCodec interface thru our protected class interface
#define AudioCodecVersion				DirectAudioCodecVersion
#define AudioCodecGetPropertyInfo		DirectAudioCodecGetPropertyInfo
#define AudioCodecGetProperty			DirectAudioCodecGetProperty
#define AudioCodecSetProperty			DirectAudioCodecSetProperty
#define AudioCodecInitialize			DirectAudioCodecInitialize
#define AudioCodecUninitialize			DirectAudioCodecUninitialize
#define AudioCodecAppendInputData		DirectAudioCodecAppendInputData
#define AudioCodecProduceOutputPackets	DirectAudioCodecProduceOutputPackets
#define AudioCodecReset					DirectAudioCodecReset
#endif

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#if	!TARGET_API_MAC_OSX || USE_QTML_HEADERS
#include <SndEnv.h>
#else
#include <CoreServices/CoreServices.h>
#endif

#include <string.h>

#include "SMACsdec.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "SMACSCDUtility.h"

#if defined( __GNUC__ )
    #if( __GNUC__ < 3 )
        #define IS_COMPILER_WORKING	0
    #else
        #define IS_COMPILER_WORKING	1
    #endif
#else
    #define IS_COMPILER_WORKING	1
#endif

#if TARGET_MAC_OS && IS_COMPILER_WORKING
#include "PCMBlitterLibDispatch.h"
#endif

// from transform.h
#define CAST( 	      type, expression) ( (type)(expression) )
#define ROUND(e)	CAST(short, e);
#define ROUND_TO_DESK(Src, Dest)							\
{														\
	if(Src>32767)										\
		Dest = 32767;									\
	else if (Src < -32768)								\
		Dest = -32768;									\
	else												\
		Dest = ROUND(Src);								\
}


//=============================================================================
//	SMACsdec
//=============================================================================

SMACsdec::SMACsdec(ComponentInstance inSelf)
:
	mSelf(inSelf),
	mSourceComponent(NULL),
	mDecoder(NULL),
	mSourceID(0),
	mSourceData(NULL),
	mPacketFrameSize(0),
	mFloatBuffer(NULL),
	mOutputBuffer(NULL)
#if	CaptureDataToFile
	,mOutputFileRefNum(-1)
#endif
{
	memset(&mOutputData, 0, sizeof(SoundComponentData));
	mOutputData.format = k16BitNativeEndianFormat;
	mOutputData.sampleSize = 16;

#if TARGET_API_MAC_OSX && TARGET_CPU_PPC	
	mHasAltiVec = SMACSCDUtility::HasAltiVec();
#else
	mHasAltiVec = false;
#endif
	
#if	TARGET_API_MAC_OSX
	mThreadStateMutex = new CAMutex("SMACsdec::mThreadStateMutex");
#endif

#if	CaptureDataToFile
	FSRef theFSRef;
	FSRef theParentRef;
	UniChar	theFileName[] = { 'o', 'u', 't', 'p', 'u', 't', '.', 's', 'd', '2' };
	OSStatus theError = FSPathMakeRef((const UInt8*)"/Volumes/Annex/Users/moorf/output.sd2", &theFSRef, NULL);
	if(theError != fnfErr)
	{
		FSDeleteObject(&theFSRef);
	}
	theError = FSPathMakeRef((const UInt8*)"/Volumes/Annex/Users/moorf", &theParentRef, NULL);
	theError = FSCreateFileUnicode(&theParentRef, 10, theFileName, kFSCatInfoNone, NULL, &theFSRef, NULL);
	theError = FSOpenFork(&theFSRef, 0, NULL, fsRdWrPerm, &mOutputFileRefNum);
	if(theError != 0)
	{
		mOutputFileRefNum = -1;
	}
#endif
}

SMACsdec::~SMACsdec()
{
	if(mDecoder != NULL)
	{
#if USE_DIRECT_ADEC
		DirectAudioCodecClose(mDecoder);
#else
		CloseComponent(mDecoder);
#endif
	}
	
	delete[] mOutputBuffer;
	delete[] mFloatBuffer;

#if	TARGET_API_MAC_OSX
	delete mThreadStateMutex;
	mThreadStateMutex = NULL;
#endif
	
#if	CaptureDataToFile
	if(mOutputFileRefNum != -1)
	{
		FSCloseFork(mOutputFileRefNum);
	}
#endif
}

void	SMACsdec::SetSource(SoundSource inSourceID, ComponentInstance inSourceComponent)
{
	mSourceID = inSourceID;
	mSourceComponent = inSourceComponent;
	mSourceData = NULL;
	
	SoundComponentData* theSourceData;
	ComponentResult theError = SoundComponentSetOutput(mSourceComponent, &mOutputData, &theSourceData);
	ThrowIfError(theError, (CAException)theError, "SMACsdec::SetSource: got an error from SoundComponentSetOutput");
}

void	SMACsdec::SetOutput(SoundComponentData* inRequested, SoundComponentData** outActual)
{
	if((inRequested->format == mOutputData.format) && (inRequested->sampleSize == mOutputData.sampleSize))
	{
		mOutputData = *inRequested;
	}
	else
	{
		*outActual = &mOutputData;
		throw static_cast<ComponentResult>(paramErr);
	}
}

void	SMACsdec::PlaySourceBuffer(SoundSource inSourceID, SoundParamBlock* inPB, SInt32 inActions)
{
	//	get rid of the input data
	mSourceData = NULL;
	mOutputData.sampleCount = 0;

	ComponentResult theError = SoundComponentPlaySourceBuffer(mSourceComponent, inSourceID, inPB, inActions);
	ThrowIfError(theError, (CAException)theError, "SMACsdec::PlaySourceBuffer: got an error from SoundComponentPlaySourceBuffer");
}

void	SMACsdec::StopSource(SInt16 inNumberSources, SoundSource*	inSources)
{
	//	get rid of the input data
	mSourceData = NULL;
	mOutputData.sampleCount = 0;
	
	//	reset the decoder
	if(mDecoder != NULL)
	{
		AudioCodecReset(mDecoder);
	}
	
	ComponentResult theError = SoundComponentStopSource(mSourceComponent, inNumberSources, inSources);
	ThrowIfError(theError, (CAException)theError, "SMACsdec::StopSource: got an error from SoundComponentStopSource");
}

void	SMACsdec::GetInfo(SoundSource inSourceID, OSType inSelector, void* outData)
{
	switch(inSelector)
	{
		case siCompressionFactor:
			GetCompressionInfo(*static_cast<CompressionInfo*>(outData));
			break;
		
		case siDecompressionParams:
			GetDecompressionParams(outData);
			break;
	
		default:
			ThrowIf(mSourceComponent == NULL, siUnknownInfoType, "SMACsdec::GetInfo: no source to pass request to")
			ComponentResult theError = SoundComponentGetInfo(mSourceComponent, inSourceID, inSelector, outData);
			ThrowIfError(theError, (CAException)theError, "SMACsdec::GetInfo: got an error from SoundComponentGetInfo");
			break;
	};
}

void	SMACsdec::SetInfo(SoundSource inSourceID, OSType inSelector, void* inData)
{
    switch(inSelector)
	{
		case siDecompressionParams:
			{
			#if	TARGET_API_MAC_OSX
				// Lock the thread if we can as we are going to completely replace the decoder
				bool threadLocked;

				threadLocked = mThreadStateMutex->Lock();
			#endif

				//	process the the new params and produce an initialized
				//	AudioCodec instance
				ComponentInstance theDecoder = SetDecompressionParams(inData);
				ThrowIf(theDecoder == NULL, badFormat, "SMACsdec::SetInfo: siDecompressionParams didn't generate a decoder");

				//	get rid of the input data
				mSourceData = NULL;
				mOutputData.sampleCount = 0;
	
				//	close the old decoder if necessary
				if((mDecoder != NULL) && (theDecoder != mDecoder))
				{
#if USE_DIRECT_ADEC
					DirectAudioCodecClose(mDecoder);
#else
					CloseComponent(mDecoder);
#endif
				}
				
				//	use the new one
				mDecoder = theDecoder;
				
				//	get the number of frames in 1 packet of data
				UInt32 theSize = sizeof(UInt32);
				ComponentResult theError = AudioCodecGetProperty(mDecoder, kAudioCodecPropertyPacketFrameSize, &theSize, &mPacketFrameSize);
				ThrowIfError(theError, (CAException)theError, "SMACsdec::SetInfo: siDecompressionParams got an error from AudioCodecGetProperty");
				
				//	toss the old output buffer
				delete[] mOutputBuffer;
				delete[] mFloatBuffer;
				
				//	allocate enough space for 1 packet of data, since that's
				//	that's all this component will produce per call to GetSourceData
				//	note that this is going to be 16 bit integer
				mOutputBuffer = new Byte[mPacketFrameSize * 2 * mOutputData.numChannels];
                mFloatBuffer = new float[mPacketFrameSize * mOutputData.numChannels];
				
			#if	TARGET_API_MAC_OSX
				// If we need to, unlock the thread
				if(threadLocked)
				{
					mThreadStateMutex->Unlock();
				}
			#endif
			}
			break;

		default:
			ThrowIf(mSourceComponent == NULL, siUnknownInfoType, "SMACsdec::SetInfo: no source to pass request to")
			ComponentResult theError = SoundComponentSetInfo(mSourceComponent, inSourceID, inSelector, inData);
			throw theError;
			break;
	};
}

void	SMACsdec::GetSourceData(SoundComponentData** outData)
{
	//	initialize the return value
	*outData = NULL;
	
	//	stuff as many packets as we can into the decoder
	UInt32 theError = 0;
	void*	theInputData = NULL;
	UInt32	theInputDataByteSize = 0;
	UInt32	theInputDataFrameSize = 0;
#if	TARGET_API_MAC_OSX
	bool threadLocked;
#endif

#if !(TARGET_MAC_OS && IS_COMPILER_WORKING)
	UInt32 totalFloats;
#endif

#if	TARGET_API_MAC_OSX
	// Lock the thread. The last thing we want is to have our state change in the middle of
	// a decode -- it will typically cause a crash
	threadLocked = mThreadStateMutex->Lock();
#endif
	//	prime things with the current packet
	GetCurrentInputPacket(theInputData, theInputDataByteSize, theInputDataFrameSize);
	// Trying to mimic Decompressor.c in this case
    if((theInputData == NULL) || (theInputDataFrameSize == 0 && theInputDataByteSize == 0) )
    {
    	// silence with non-zero duration.
 		mOutputData.buffer = NULL;
		*outData = mSourceData;
    }
    else
    {
		//	loop and stuff the decoder with packets
		bool isDone = false;
		while((theInputData != NULL) && (theInputDataByteSize != 0) && (!isDone))
		{
			//	give the current packet to the decoder
			UInt32	theNumberInputPackets = 1;
			UInt32	theNumberInputBytesConsumed = theInputDataByteSize;
			AudioStreamPacketDescription theInputPacketDescription = { 0, 0, theInputDataByteSize };
			theError = AudioCodecAppendInputData(mDecoder, theInputData, &theNumberInputBytesConsumed, &theNumberInputPackets, &theInputPacketDescription);
			//printf("mDecoder == %lu, Appended %ld bytes in %ld packets\n", (UInt32)(&mDecoder), theNumberInputBytesConsumed, theNumberInputPackets);
			ThrowIfError(theError, (CAException)theError, "SMACsdec::GetSourceData: got an error from AudioCodecAppendInputData");
			
			//	we know that the decoder is full if it ever says that it didn't take any data from the input buffer
			if( (theNumberInputBytesConsumed > 0) && (theNumberInputBytesConsumed == theInputDataByteSize) )
			{
				//	consume the data the decoder used
				ConsumeCurrentInputPacket(theInputDataByteSize, 0);
				
				//	get the next packet
				GetCurrentInputPacket(theInputData, theInputDataByteSize, theInputDataFrameSize);
			}
			else if (theNumberInputBytesConsumed > 0)
			{
				ConsumeCurrentInputPacket(theNumberInputBytesConsumed, 0);
				isDone = true;
			}
			else
			{
				isDone = true;
			}
		}
		//	the decoder is as full of data as we can make it at this time,
		//	so pull out 1 packet of output data
		UInt32 theOutputDataByteSize = mPacketFrameSize * sizeof(float) * mOutputData.numChannels;
		UInt32 theNumberOutputFrames = mPacketFrameSize;
		UInt32 theDecoderStatus = kAudioCodecProduceOutputPacketFailure;
		theError = AudioCodecProduceOutputPackets(mDecoder, mFloatBuffer, &theOutputDataByteSize, &theNumberOutputFrames, NULL, &theDecoderStatus);
		//printf ("        theNumberOutputFrames == %ld\n",theNumberOutputFrames);
		ThrowIfError(theError, (CAException)theError, "SMACsdec::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
		//	set up the return values if we got a packet back from the decoder
		if(theNumberOutputFrames)
		{
			//theNumberOutputFrames = theOutputDataByteSize / mOutputData.numChannels / sizeof(float);
		//	ThrowIfError(theNumberOutputFrames != theOutputDataByteSize / mOutputData.numChannels / sizeof(float), "SMACsdec::GetSourceData: got an error from AudioCodecProduceOutputPackets");

#if TARGET_MAC_OS && IS_COMPILER_WORKING
			Float32ToNativeInt16(mHasAltiVec, mFloatBuffer, ((short *)mOutputBuffer), theOutputDataByteSize >> 2);
#else
			totalFloats = theOutputDataByteSize >> 2;
			for (unsigned int i = 0; i < totalFloats; i++)
			{
				((short *)mOutputBuffer)[i] = (SInt16)(32768.0 * mFloatBuffer[i]);
			}
#endif
			mOutputData.buffer = mOutputBuffer;
			if (mOutputData.numChannels == 2) // It's either 1 or 2 here in SMAC land
			{
				mOutputData.sampleCount = (theOutputDataByteSize >> 3); // These really are frames. It's a QuickTime/SoundManager ism
			}
			else
			{
				mOutputData.sampleCount = (theOutputDataByteSize >> 2);
			}
			*outData = &mOutputData;
			
	#if	CaptureDataToFile
			if(mOutputFileRefNum != -1)
			{
				UInt32 theActualBytesWritten = 0;
				FSWriteFork(mOutputFileRefNum, fsAtMark, 0, theOutputDataByteSize, mOutputBuffer, &theActualBytesWritten);
			}
	#endif
		}
		else
		{
			mOutputData.buffer = NULL;
			mOutputData.sampleCount = 0;
			*outData = &mOutputData;
		}
    }
#if	TARGET_API_MAC_OSX
	if(threadLocked)
	{
		mThreadStateMutex->Unlock();
	}
#endif
}

void	SMACsdec::GetCurrentInputPacket(void*& outPacketData, UInt32& outPacketDataByteSize, UInt32& outPacketDataFrameSize)
{
	//	This method is called to get a pointer to the next packet of input data.
	//	The only failure cases for the returned values are as follows:
	//		- outPacketData is NULL
	//		- outPacketData is not NULL, but outPacketDataByteSize and outPacketDataFrameSize
	//			are both equal to zero
	//	Note that this method does not consume the data, it merely returns points to it. Use
	//	ConsumeCurrentInputPacket to mark input data as consumed.

	//	set up the return values
	outPacketData = NULL;
	outPacketDataByteSize = 0;
	outPacketDataFrameSize = 0;
	
	//	check to see if we need to get more data from the source component
	if(SMACSCDUtility::NeedMoreSourceData(mSourceData))
	{
		if(mSourceData != NULL)
		{
			mSourceData->sampleCount = 0;
		}
		//	we do, so get it from the source component
		UInt32 theError = SoundComponentGetSourceData(mSourceComponent, &mSourceData);
		ThrowIfError(theError, (CAException)theError, "SMACsdec::GetCurrentInputPacket: got an error from SoundComponentGetSourceData");
        // We pass this on as QuickTime will not be happy if we don't.
        // Note that this is not necessarily what the sample rate is in the original file.
        mOutputData.sampleRate = mSourceData->sampleRate;
    }
	
	//	if we have some data from the source component, dole out the first packet
	if(!SMACSCDUtility::NeedMoreSourceData(mSourceData))
	{
		if(SMACSCDUtility::IsSoundComponentData(mSourceData))
		{
			//	this is a normal SoundComponentData with no packetization info,
			//	so return the whole buffer
			outPacketData = mSourceData->buffer;
			outPacketDataByteSize = mSourceData->numChannels * (mSourceData->sampleSize >> 3) * mSourceData->sampleCount;
			outPacketDataFrameSize = mSourceData->sampleCount;
		}
		else
		{
			//	this is an ExtendedSoundComponentData
			ExtendedSoundComponentData* theExtendedSourceData = reinterpret_cast<ExtendedSoundComponentData*>(mSourceData);
			
			//	it may or may not have packetization info in it
			if(static_cast<UInt32>(theExtendedSourceData->recordSize) < sizeof(ExtendedSoundComponentData))
			{
				//	it's an old ExtendedSoundComponentData which doesn't have the packetization
				//	info, so return the whole buffer
				outPacketData = theExtendedSourceData->desc.buffer;
				if(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid)
				{
					outPacketDataByteSize = theExtendedSourceData->bufferSize;
				}
				if(!(theExtendedSourceData->extendedFlags & kExtendedSoundSampleCountNotValid))
				{
					outPacketDataFrameSize = theExtendedSourceData->desc.sampleCount;
				}
			}
			else
			{
				//	it's a new ExtendedSoundComponentData which may have the packetization info
				if(theExtendedSourceData->extendedFlags & kExtendedSoundCommonFrameSizeValid)
				{
					//	all the packets are the same size, so return the current one
					//	note that we don't know how many frames are in this buffer
					//	also note that the buffer size field has to be valid
					ThrowIf(!(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid), -50, "SMACsdec::GetCurrentInputPacket: need kExtendedSoundBufferSizeValid set for common packet sizes to work");
					outPacketData = theExtendedSourceData->desc.buffer;
					outPacketDataByteSize = theExtendedSourceData->commonFrameSize;
				}
				else if(theExtendedSourceData->extendedFlags & kExtendedSoundFrameSizesValid)
				{
					//	the packets are sized according to theExtendedSourceData->frameSizesArray
					//	note that we don't know how many frames are in this buffer
					ThrowIf(!(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid), -50, "SMACsdec::GetCurrentInputPacket: need kExtendedSoundBufferSizeValid set for arbitrary packet sizes to work");
					outPacketData = theExtendedSourceData->desc.buffer;
					outPacketDataByteSize = *(theExtendedSourceData->frameSizesArray);
				}
				else
				{
					//	there isn't any packetization info, so return the whole buffer
					outPacketData = theExtendedSourceData->desc.buffer;
					if(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid)
					{
						outPacketDataByteSize = theExtendedSourceData->bufferSize;
					}
					if(!(theExtendedSourceData->extendedFlags & kExtendedSoundSampleCountNotValid))
					{
						outPacketDataFrameSize = theExtendedSourceData->desc.sampleCount;
					}
				}
			}
		}
	}
}

void	SMACsdec::ConsumeCurrentInputPacket(UInt32 inPacketDataByteSize, UInt32 inPacketDataFrameSize)
{
	//	This method is called in order to consume the current packet of input data.
	//	Note that this routine enforces all the expectations as far as packetization
	//	is concerned. This routine will throw exceptions if things are even slightly
	//	amiss.

	//	can only consume data if we have it
	if(mSourceData != NULL)
	{
		if(SMACSCDUtility::IsSoundComponentData(mSourceData))
		{
			//	this is a normal SoundComponentData with no packetization info,
			//	so consume the amount indicated, making sure that the two notions
			//	of the amount to consume are the same
			ThrowIf(inPacketDataByteSize != (inPacketDataFrameSize * mSourceData->numChannels * (mSourceData->sampleSize >> 3)), -50, "SMACsdec::ConsumeCurrentInputPacket: concept of how much data to consume doesn't match for the normal case");
			mSourceData->buffer += inPacketDataByteSize;
			mSourceData->sampleCount -= inPacketDataFrameSize;
		}
		else
		{
			//	this is an ExtendedSoundComponentData
			ExtendedSoundComponentData* theExtendedSourceData = reinterpret_cast<ExtendedSoundComponentData*>(mSourceData);
			
			//	it may or may not have packetization info in it
			if(static_cast<UInt32>(theExtendedSourceData->recordSize) < sizeof(ExtendedSoundComponentData))
			{
				//	it's an old ExtendedSoundComponentData which doesn't have the packetization
				//	so consume the amount indicated
				theExtendedSourceData->desc.buffer += inPacketDataByteSize;
				if(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid)
				{
					theExtendedSourceData->bufferSize -= inPacketDataByteSize;
				}
				if(!(theExtendedSourceData->extendedFlags & kExtendedSoundSampleCountNotValid))
				{
					theExtendedSourceData->desc.sampleCount -= inPacketDataFrameSize;
				}
			}
			else
			{
				//	it's a new ExtendedSoundComponentData which may have the packetization info
				if(theExtendedSourceData->extendedFlags & kExtendedSoundCommonFrameSizeValid)
				{
					//	all the packets are the same size, so check to be sure that we are
					//	consuming the correct amount, noting that we have no clue how many
					//	frames of data this is
					ThrowIf(inPacketDataByteSize != static_cast<UInt32>(theExtendedSourceData->commonFrameSize), -50, "SMACsdec::ConsumeCurrentInputPacket: asked to consume something different from the common packet size");
					ThrowIf(!(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid), -50, "SMACsdec::ConsumeCurrentInputPacket: need kExtendedSoundBufferSizeValid set for common packet sizes to work");
					theExtendedSourceData->desc.buffer += inPacketDataByteSize;
					theExtendedSourceData->bufferSize -= inPacketDataByteSize;
				}
				else if(theExtendedSourceData->extendedFlags & kExtendedSoundFrameSizesValid)
				{
					//	the packets are sized according to theExtendedSourceData->frameSizesArray,
					//	so check to be sure we are consuming the correct amount of data, noting that
					//	we have no clue how many frames of data this is
					ThrowIf(inPacketDataByteSize != static_cast<UInt32>(*(theExtendedSourceData->frameSizesArray)), -50, "SMACsdec::ConsumeCurrentInputPacket: asked to consume something different from the first packet size in theExtendedSourceData->frameSizesArray");
					ThrowIf(!(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid), -50, "SMACsdec::ConsumeCurrentInputPacket: need kExtendedSoundBufferSizeValid set for arbitrary packet sizes to work");
					theExtendedSourceData->desc.buffer += inPacketDataByteSize;
					theExtendedSourceData->frameSizesArray += 1;
					theExtendedSourceData->bufferSize -= inPacketDataByteSize;
				}
				else
				{
					//	there isn't any packetization info, so consume the amount indicated
					theExtendedSourceData->desc.buffer += inPacketDataByteSize;
					if(theExtendedSourceData->extendedFlags & kExtendedSoundBufferSizeValid)
					{
						theExtendedSourceData->bufferSize -= inPacketDataByteSize;
					}
					if(!(theExtendedSourceData->extendedFlags & kExtendedSoundSampleCountNotValid))
					{
						theExtendedSourceData->desc.sampleCount -= inPacketDataFrameSize;
					}
				}
			}
		}
	}
}
