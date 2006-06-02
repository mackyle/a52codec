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
	SMACIMAsdec.cpp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#include <string.h>
#include "SMACIMAsdec.h"
#include "CADebugMacros.h"
#include "CAException.h"

enum
{
	kIMAAudioGeneralFormatID				= FOUR_CHAR_CODE('ima4'),
	kIMAAudioGeneralVersion					= 0
};


//=============================================================================
//	SMACIMAsdec
//=============================================================================

SMACIMAsdec::SMACIMAsdec(ComponentInstance inSelf)
:
	SMACsdec(inSelf)
{
	// Defaults
	mSampleRate = 44100;
	mNumChannels = 2;
}

SMACIMAsdec::~SMACIMAsdec()
{
}

void	SMACIMAsdec::GetInfo(SoundSource inSourceID, OSType inSelector, void* outData)
{
    switch(inSelector)
	{
		case siCompressionChannels:
			*((short *)outData) = (short)mNumChannels;
			break;
			
		default:
			SMACsdec::GetInfo(inSourceID, inSelector, outData);
			break;
	};
}

void	SMACIMAsdec::SetInfo(SoundSource inSourceID, OSType inSelector, void* inData)
{
	switch(inSelector)
	{
		// IMA does not care about the sample rate, but we'll just accept the data so if we're
		// asked what the sample rate is, we can tell whoever it is that asks.
		case siCompressionSampleRate:
			mSampleRate = (*((UnsignedFixed *)inData)) >> 16; 
			break;

		// This really is the only setting IMA cares about
		case siCompressionChannels:
			mNumChannels = *(short*)inData;
			mOutputData.numChannels = mNumChannels;
			break;

		default:
			SMACsdec::SetInfo(inSourceID, inSelector, inData);
			break;
	};
}

// This is the answer to a QuickTime call to GetInfo for siCompressionFactor
void	SMACIMAsdec::GetCompressionInfo(CompressionInfo& outCompressionInfo)
{
	outCompressionInfo.recordSize = sizeof(CompressionInfo);
	outCompressionInfo.format = kIMACompression;
	outCompressionInfo.compressionID = fixedCompression;
	outCompressionInfo.samplesPerPacket = kIMA4BlockSamples;
	outCompressionInfo.bytesPerPacket = kIMA4BlockBytes;
	outCompressionInfo.bytesPerSample = kIMA4BytesPerSample;
	outCompressionInfo.bytesPerFrame = kIMA4BlockBytes;
	outCompressionInfo.futureUse1 = 0;
}

// Not used by IMA, but more complex codecs will need to actually return something here.
// The decompression params are or contain the "MagicCookie" used by the codec
// A MagicCookie can just be a "wave chunk" --  a series of QuickTime atoms that
// specify the format and the appropriate configuration
void	SMACIMAsdec::GetDecompressionParams(void* outData)
{
	//	We don't have Decompression params
	Handle theDecompressionParams = NULL;
	//	set the return value
	*static_cast<Handle*>(outData) = theDecompressionParams;
}

// Normally, QuickTime will call this directly. In IMA it doesn't.
// The compression params are or contain the "MagicCookie" used by the codec
// When this is called we can initialize the codec as we will have all the necessary information
// to do so. 
ComponentInstance	SMACIMAsdec::SetDecompressionParams(const void* inData)
{
	AudioStreamBasicDescription theInputFormat = {mSampleRate, kIMAAudioGeneralFormatID, 0, kIMA4BlockBytes, kIMA4BlockSamples, 0, mNumChannels, 0, 0 };

	//	use the format to locate a suitable decoder
	Component theDecoderComponent = FindIMAAudioDecoderComponent(theInputFormat);
	ThrowIf(theDecoderComponent == NULL, badFormat, "SMACIMAsdec::SetDecompressionParams: couldn't find a decoder");

	ComponentInstance theDecoder = InitializeIMAAudioDecoder(theDecoderComponent, theInputFormat);
	ThrowIf(theDecoder == NULL, badFormat, "SMACIMAsdec::SetDecompressionParams: couldn't initialize the decoder");
	
	return theDecoder;
}

Component	SMACIMAsdec::FindIMAAudioDecoderComponent(const AudioStreamBasicDescription& inFormat)
{
	Component				theDecoderComponent = NULL;
	ComponentDescription	theDecoderDescription = { kAudioDecoderComponentType, inFormat.mFormatID, 0, 0, 0 };

	// If a component has sub types, ala MPEG-4, you'll need to loop here and look at the sub-type	
	theDecoderComponent = FindNextComponent(NULL, &theDecoderDescription);

	return theDecoderComponent;
}

// Note: The decoder deals with CoreAudio floats while the encoder deals with SInt16s.
// This is primarily to illustrate how to deal with either format.
static AudioStreamBasicDescription*	FindNEFloatFormat(const AudioStreamBasicDescription* inFormatList, UInt32 inNumberFormats)
{
	AudioStreamBasicDescription* theAnswer = NULL;
	
	// Decoders can have multiple output formats
	while((inNumberFormats > 0) && (theAnswer == NULL))
	{
		if((inFormatList->mFormatID == kAudioFormatLinearPCM) && ((inFormatList->mFormatFlags & kAudioFormatFlagsNativeFloatPacked) == kAudioFormatFlagsNativeFloatPacked) && (inFormatList->mBitsPerChannel == 32))
		{
			theAnswer = const_cast<AudioStreamBasicDescription*>(inFormatList);
		}
		
		++inFormatList;
		--inNumberFormats;
	}
	
	return theAnswer;
}

ComponentInstance	SMACIMAsdec::InitializeIMAAudioDecoder(Component inDecoderComponent, const AudioStreamBasicDescription& inFormat)
{
	UInt32 theSize;
	ComponentInstance theDecoder = OpenComponent(inDecoderComponent);
	ThrowIf(theDecoder == NULL, badComponentInstance, "SMACIMAsdec::InitializeIMAAudioDecoder: couldn't open the component");
	
	//	first, give the decoder the info we have
	theSize = sizeof(AudioStreamBasicDescription);
	ComponentResult theError = AudioCodecSetProperty(theDecoder, kAudioCodecPropertyCurrentInputFormat, theSize, &inFormat);
	ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::InitializeIMAAudioDecoder: got an error setting the input format");
		
	//	now find out what it can output
	theError = AudioCodecGetPropertyInfo(theDecoder, kAudioCodecPropertySupportedOutputFormats, &theSize, NULL);
	ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::InitializeIMAAudioDecoder: got an error getting the available output format list size");
	
	UInt32 theNumberAvailableOutputFormats = theSize / sizeof(AudioStreamBasicDescription);
	AudioStreamBasicDescription* theAvailableOutputFormats = new AudioStreamBasicDescription[theNumberAvailableOutputFormats];
	
	try
	{
		theSize = theNumberAvailableOutputFormats * sizeof(AudioStreamBasicDescription);
		theError = AudioCodecGetProperty(theDecoder, kAudioCodecPropertySupportedOutputFormats, &theSize, theAvailableOutputFormats);
		ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::InitializeIMAAudioDecoder: got an error getting the available output formats");
	
		//	find an acceptable output format
		AudioStreamBasicDescription* theOutputFormat = FindNEFloatFormat(theAvailableOutputFormats, theNumberAvailableOutputFormats);
		ThrowIf(theOutputFormat == NULL, badFormat, "SMACIMAsdec::InitializeIMAAudioDecoder: couldn't find an acceptable output format");
		
		// finish filling out the output format
		theOutputFormat->mSampleRate = inFormat.mSampleRate;
		theOutputFormat->mChannelsPerFrame = inFormat.mChannelsPerFrame;
		theOutputFormat->mBytesPerFrame = 4 * inFormat.mChannelsPerFrame;
		theOutputFormat->mFormatID = kAudioFormatLinearPCM;
		theOutputFormat->mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
		theOutputFormat->mBytesPerPacket = 4 * inFormat.mChannelsPerFrame;
		theOutputFormat->mFramesPerPacket = 1;
		theOutputFormat->mBitsPerChannel = 32;

		//	tell the decoder about it
		theSize = sizeof(AudioStreamBasicDescription);
		theError = AudioCodecSetProperty(theDecoder, kAudioCodecPropertyCurrentOutputFormat, theSize, theOutputFormat);
		ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::InitializeIMAAudioDecoder: got an error setting the output format");
		
		delete[] theAvailableOutputFormats;
		theAvailableOutputFormats = NULL;
	}
	catch(...)
	{
		delete[] theAvailableOutputFormats;
		throw;
	}
	
	//	finally initialize the decoder
	theError = AudioCodecInitialize(theDecoder, NULL, NULL, NULL, 0);
	ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::InitializeIMAAudioDecoder: got an error initializing the decoder");
	
	return theDecoder;
}

// Overrides the method in SMACsdec. Necessary due to the way IMA works. This will not be necessary
// for many codecs.
void	SMACIMAsdec::GetSourceData(SoundComponentData** outData)
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
		// On more complex decoders this isn't always necessary
		if (!mDecoder)
		{
			mNumChannels = mOutputData.numChannels = mSourceData->numChannels;
			mSampleRate = (mSourceData->sampleRate) >> 16;
			CreateIMADecoder();
		}
		
		//	loop and stuff the decoder with packets
		bool isDone = false;
		while((theInputData != NULL) && (theInputDataByteSize != 0) && (!isDone))
		{
			//	give the current packet to the decoder
			UInt32	theNumberInputPackets = 1;
			UInt32	theNumberInputBytesConsumed = theInputDataByteSize;
			AudioStreamPacketDescription theInputPacketDescription = { 0, 0, theInputDataByteSize };
			theError = AudioCodecAppendInputData(mDecoder, theInputData, &theNumberInputBytesConsumed, &theNumberInputPackets, &theInputPacketDescription);
			ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::GetSourceData: got an error from AudioCodecAppendInputData");
			
			//	we know that the decoder is full if it ever says that it didn't take any data from the input buffer
			if(theNumberInputBytesConsumed > 0 && theNumberInputBytesConsumed == theInputDataByteSize)
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
		ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
		//	set up the return values if we got a packet back from the decoder
		if(theNumberOutputFrames)
		{
// gcc 2.95 doesn't work right in this case, so we guard against it
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
				mOutputData.sampleCount = (theOutputDataByteSize >> 3); // Yes, these really are frames.
			}
			else
			{
				mOutputData.sampleCount = (theOutputDataByteSize >> 2); // mono
			}
			*outData = &mOutputData;
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

// Not necessary for more complex codecs as SetDecompressionParams() will be called directly by QuickTime
void	SMACIMAsdec::CreateIMADecoder()
{
	mDecoder = SetDecompressionParams(NULL);
	UInt32 theSize = sizeof(UInt32);
	ComponentResult theError = AudioCodecGetProperty(mDecoder, kAudioCodecPropertyPacketFrameSize, &theSize, &mPacketFrameSize);
	ThrowIfError(theError, (CAException)theError, "SMACIMAsdec::CreateIMADecoder: got an error from AudioCodecGetProperty");
	
	//	toss the old output buffer
	delete[] mOutputBuffer;
	delete[] mFloatBuffer;
	
	//	allocate enough space for 1 packet of data, since that's
	//	that's all this component will produce per call to GetSourceData
	//	note that this is going to be 16 bit integer so we need to allocate a separate
	//	buffer to deal with the floating point data as well
	mOutputBuffer = new Byte[mPacketFrameSize * sizeof(SInt16) * mOutputData.numChannels];
	mFloatBuffer = new float[mPacketFrameSize * mOutputData.numChannels];
}
