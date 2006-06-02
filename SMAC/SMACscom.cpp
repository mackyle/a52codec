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
	SMACscom.cpp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#include <string.h>

#include "SMACscom.h"
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

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

//=============================================================================
//	SMACscom
//=============================================================================

SMACscom::SMACscom(ComponentInstance inSelf, UInt32 inFormatID)
:
	mSelf(inSelf),
	mSourceComponent(NULL),
	mEncoder(NULL),
	mSourceID(0),
	mSourceData(NULL),
	mPacketFrameSize(0),
	mMaxPacketByteSize(0),
	mOutputBuffer(NULL)
{
	//	fill out the output ExtendedSoundComponentData
	memset(&mOutputData, 0, sizeof(ExtendedSoundComponentData));
	mOutputData.desc.flags = kExtendedSoundData;
	mOutputData.desc.format = inFormatID;
	mOutputData.desc.sampleSize = 16;
	mOutputData.recordSize = sizeof(ExtendedSoundComponentData);
	mOutputData.extendedFlags = kExtendedSoundBufferSizeValid;
    mSourceIsExhausted = false;
#if TARGET_API_MAC_OSX && TARGET_CPU_PPC	
	mHasAltiVec = SMACSCDUtility::HasAltiVec();
#else
	mHasAltiVec = false;
#endif
}

SMACscom::~SMACscom()
{
	if(mEncoder != NULL)
	{
		CloseComponent(mEncoder);
	}
	
	delete[] mOutputBuffer;
}

void	SMACscom::SetSource(SoundSource inSourceID, ComponentInstance inSourceComponent)
{
	mSourceID = inSourceID;
	mSourceComponent = inSourceComponent;
	mSourceData = NULL;
}

void	SMACscom::SetOutput(SoundComponentData* inRequested, SoundComponentData** outActual)
{
	if(inRequested->format == mOutputData.desc.format)
	{
		memset(&mOutputData, 0, sizeof(ExtendedSoundComponentData));
		mOutputData.desc = *inRequested;
		mOutputData.recordSize = sizeof(ExtendedSoundComponentData);
		mOutputData.extendedFlags = kExtendedSoundBufferSizeValid;
	}
	else
	{
		*outActual = reinterpret_cast<SoundComponentData*>(&mOutputData);
		throw static_cast<ComponentResult>(paramErr);
	}
}

void	SMACscom::PlaySourceBuffer(SoundSource inSourceID, SoundParamBlock* inPB, SInt32 inActions)
{
	//	get rid of the input data
	mSourceData = NULL;
	mOutputData.desc.sampleCount = 0;
	mOutputData.bufferSize = 0;
	mOutputData.frameCount = 0;
	mOutputData.commonFrameSize = 0;
	
	ComponentResult theError = SoundComponentPlaySourceBuffer(mSourceComponent, inSourceID, inPB, inActions);
	ThrowIfError(theError, (CAException)theError, "SMACscom::PlaySourceBuffer: got an error from SoundComponentPlaySourceBuffer");
}

void	SMACscom::StopSource(SInt16 inNumberSources, SoundSource*	inSources)
{
	//	get rid of the input data
	mSourceData = NULL;
	mOutputData.desc.sampleCount = 0;
	mOutputData.bufferSize = 0;
	mOutputData.frameCount = 0;
	mOutputData.commonFrameSize = 0;
	
	//	reset the encoder
	if(mEncoder != NULL)
	{
		AudioCodecReset(mEncoder);
	}
	
	ComponentResult theError = SoundComponentStopSource(mSourceComponent, inNumberSources, inSources);
	ThrowIfError(theError, (CAException)theError, "SMACscom::StopSource: got an error from SoundComponentStopSource");
}

void	SMACscom::GetInfo(SoundSource inSourceID, OSType inSelector, void* outData)
{
	switch(inSelector)
	{
		case siCompressionFactor:
			GetCompressionInfo(*static_cast<CompressionInfo*>(outData));
			break;
		
		case siCompressionParams:
			GetCompressionParams(outData);
			break;

		default:
			ThrowIf(mSourceComponent == NULL, siUnknownInfoType, "SMACscom::GetInfo: no source to pass request to")
			ComponentResult theError = SoundComponentGetInfo(mSourceComponent, inSourceID, inSelector, outData);
			ThrowIfError(theError, (CAException)theError, "SMACscom::GetInfo: got an error from SoundComponentGetInfo");
			break;
	};
}

void	SMACscom::SetInfo(SoundSource inSourceID, OSType inSelector, void* inData)
{
    switch(inSelector)
    {
        case siCompressionParams:
            {
                //	process the the new params and produce an initialized
                //	AudioCodec instance
                ComponentInstance theEncoder = SetCompressionParams(inData);
                ThrowIf(theEncoder == NULL, badFormat, "SMACscom::SetInfo: siCompressionParams didn't generate an encoder");

                //	get rid of the input data
                mSourceData = NULL;
                mOutputData.desc.sampleCount = 0;
                mOutputData.bufferSize = 0;
                mOutputData.frameCount = 0;
                mOutputData.commonFrameSize = 0;

                //	close the old encoder if necessary
                if((mEncoder != NULL) && (theEncoder != mEncoder))
                {
                    CloseComponent(mEncoder);
                }
                
                //	use the new one
                mEncoder = theEncoder;
                
                //	get the number of frames in 1 packet of data
                UInt32 theSize = sizeof(UInt32);
                ComponentResult theError = AudioCodecGetProperty(mEncoder, kAudioCodecPropertyPacketFrameSize, &theSize, &mPacketFrameSize);
                ThrowIfError(theError, (CAException)theError, "SMACscom::SetInfo: siCompressionParams got an error from AudioCodecGetProperty while getting the packet frame size");
                
                //	get the maximum number of bytes in 1 packet of data
                theSize = sizeof(UInt32);
                theError = AudioCodecGetProperty(mEncoder, kAudioCodecPropertyMaximumPacketByteSize, &theSize, &mMaxPacketByteSize);
                ThrowIfError(theError, (CAException)theError, "SMACscom::SetInfo: siCompressionParams got an error from AudioCodecGetProperty while getting the maximum packet byte size");
                
                //	toss the old output buffer
                delete[] mOutputBuffer;
                
                //	allocate enough space for 1 packet of data, since that's
                //	that's all this component will produce per call to GetSourceData
                mOutputBuffer = new Byte[mMaxPacketByteSize];
            }
            break;
        
        case siSourceIsExhausted:
			// in this case it seems to be passed by value -- ugh!
            mSourceIsExhausted = (Boolean)((UInt32)inData);
            // Now pass this on, so no break!
        default:
            ThrowIf(mSourceComponent == NULL, siUnknownInfoType, "SMACscom::SetInfo: no source to pass request to")
            ComponentResult theError = SoundComponentSetInfo(mSourceComponent, inSourceID, inSelector, inData);
            ThrowIfError(theError, (CAException)theError, "SMACscom::SetInfo: got an error from SoundComponentSetInfo");
            break;
    };
}

void	SMACscom::GetSourceData(SoundComponentData** outData)
{
	ComponentResult theError = 0;
	UInt32	theNumberOutputPackets = 0;
	UInt32	theEncoderStatus = 0;
	UInt32	theActualOutputDataByteSize = 0;
	UInt32  maxPacketSize = 0;
#if !TARGET_MAC_OS || !IS_COMPILER_WORKING
	float * tempFloatPtr;
	unsigned long tempULong = 0;
	float tempFloat;
#endif	
	*outData = NULL;
	
	//	make sure we have some source data to start with
	if(SMACSCDUtility::NeedMoreSourceData(mSourceData))
	{
		theError = SoundComponentGetSourceData(mSourceComponent, &mSourceData);
		ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from SoundComponentGetSourceData");
	}
	
	//	spin until we produce a packet of data or we run completely out of source data
	UInt32	theFramesProduced = 0;
	UInt32	theOutputDataByteSize = 0;
	while(SMACSCDUtility::HasData(mSourceData) && (theFramesProduced < mPacketFrameSize))
	{
		//	stuff as much input data into the encoder as we can
		UInt32 theInputDataByteSize = SMACSCDUtility::GetDataByteSize(mSourceData);
		UInt32 theNumberOfPackets = 0;
		maxPacketSize = min(kMaxInputSamples, theInputDataByteSize/sizeof(SInt16));
#if TARGET_MAC_OS && IS_COMPILER_WORKING
		NativeInt16ToFloat32( mHasAltiVec, ((short *)(mSourceData->buffer)), mFloatBuffer, maxPacketSize, 16);
#else
		for (UInt32 i = 0; i < maxPacketSize; i++)
		{
			tempFloat = (float)(((short *)(mSourceData->buffer))[i]);
			tempFloatPtr = &tempFloat;
			if (tempFloat != 0.0 && tempFloat != -0.0)
			{
				tempULong = *((unsigned long *)tempFloatPtr);
				tempULong -= 0x07800000;
				tempFloatPtr = (float *)(&tempULong);
			}
			mFloatBuffer[i] = *tempFloatPtr;
		}
#endif
		theInputDataByteSize = maxPacketSize * 4; // this is all we have converted
		//theInputDataByteSize *= 2;
		theError = AudioCodecAppendInputData(mEncoder, mFloatBuffer, &theInputDataByteSize, &theNumberOfPackets, NULL);
		ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from AudioCodecAppendInputData");
		
		//	update the source data with the amount of data consumed
		SMACSCDUtility::ConsumedData(mSourceData, theInputDataByteSize >> 1);
		
		//	see if we can get a packet of output data
		theActualOutputDataByteSize = mMaxPacketByteSize;
		theNumberOutputPackets = 1;
		theEncoderStatus = kAudioCodecProduceOutputPacketFailure;
		theError = AudioCodecProduceOutputPackets(mEncoder, mOutputBuffer, &theActualOutputDataByteSize, &theNumberOutputPackets, NULL, &theEncoderStatus);
		ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
		if(theNumberOutputPackets == 1)
		{
			//	we produced a full packet of frames, so we're done
			theFramesProduced = mPacketFrameSize;
			theOutputDataByteSize += theActualOutputDataByteSize;
			
		}
		else
		{
			//	we didn't get the data, so get more input data if we have to
			if(SMACSCDUtility::NeedMoreSourceData(mSourceData))
			{
				theError = SoundComponentGetSourceData(mSourceComponent, &mSourceData);
				ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from SoundComponentGetSourceData");
			}
		}
	}
	
	if(theFramesProduced < mPacketFrameSize)
	{
		// Tell the encoder to pad with 0s -- this will change once the API is added
        if (mSourceIsExhausted)
        {
            UInt32 theInputDataByteSize = SMACSCDUtility::GetDataByteSize(mSourceData); // better be 0
            UInt32 theNumberOfPackets = 0;
			maxPacketSize = min(kMaxInputSamples, theInputDataByteSize/sizeof(SInt16));
#if TARGET_MAC_OS && IS_COMPILER_WORKING
			NativeInt16ToFloat32( mHasAltiVec, ((short *)(mSourceData->buffer)), mFloatBuffer, maxPacketSize, 16);
#else
			for (UInt32 i = 0; i < maxPacketSize; i++)
			{
				tempFloat = (float)(((short *)(mSourceData->buffer))[i]);
				tempFloatPtr = &tempFloat;
				if (tempFloat != 0.0 && tempFloat != -0.0)
				{
					tempULong = *((unsigned long *)tempFloatPtr);
					tempULong -= 0x07800000;
					tempFloatPtr = (float *)(&tempULong);
				}
				mFloatBuffer[i] = *tempFloatPtr;
			}
#endif
			theInputDataByteSize = maxPacketSize * 4; // this is all we have converted
			//theInputDataByteSize *= 2;
			theError = AudioCodecAppendInputData(mEncoder, mFloatBuffer, &theInputDataByteSize, &theNumberOfPackets, NULL);
            ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from AudioCodecAppendInputData");
		}
        
		//	we ran out of input data, but we still haven't produced enough output data
		//	so we have to try one last time to pull on the encoder in case it has
		//	some left overs that it can give us
		theActualOutputDataByteSize = mMaxPacketByteSize;
		theNumberOutputPackets = 1;
		theEncoderStatus = kAudioCodecProduceOutputPacketFailure;
		theError = AudioCodecProduceOutputPackets(mEncoder, mOutputBuffer, &theActualOutputDataByteSize, &theNumberOutputPackets, NULL, &theEncoderStatus);
		ThrowIfError(theError, (CAException)theError, "SMACscom::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
		if(theNumberOutputPackets == 1)
		{
			//	we produced a full packet of frames, so we're done
			theFramesProduced = mPacketFrameSize;
			theOutputDataByteSize += theActualOutputDataByteSize;
		}
	}
	
	//	set up the return values if any data was produced
	if(theFramesProduced > 0)
	{
		mOutputData.desc.buffer = mOutputBuffer;
		mOutputData.desc.sampleCount = theFramesProduced;
		mOutputData.bufferSize = theOutputDataByteSize;
		mOutputData.frameCount = 1;
		mOutputData.commonFrameSize = theOutputDataByteSize;
        mOutputData.desc.numChannels = mSourceData->numChannels;
		*outData = reinterpret_cast<SoundComponentData*>(&mOutputData);
	}
}
