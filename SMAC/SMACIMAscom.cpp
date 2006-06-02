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
	SMACIMAscom.cpp
	
=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#include <string.h>

#include "SMACIMAscom.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "SMACSCDUtility.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

enum
{
	kIMAAudioGeneralFormatID				= FOUR_CHAR_CODE('ima4'),
	kIMAAudioGeneralVersion					= 0
};

// NESInt16 == Native-Endian Signed 16-bit Integer
enum
{
	kNESInt16FormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked
};

//=============================================================================
//	SMACIMAscom
//=============================================================================

SMACIMAscom::SMACIMAscom(ComponentInstance inSelf)
:
	SMACscom(inSelf, kIMAAudioGeneralFormatID)
{
	// Defaults
	mSampleRate = 44100;
	mNumChannels = 2;
}

SMACIMAscom::~SMACIMAscom()
{
}

void	SMACIMAscom::GetInfo(SoundSource inSourceID, OSType inSelector, void* outData)
{
	switch(inSelector)
	{
		case siOptionsDialog:
			*static_cast<SInt16*>(outData) = 0; // No options dialog here
			break;
	
		//case siCompressionInputRateList:
		//	break; // use the default list -- IMA does care about the sample rate
	
		//case siCompressionOutputSampleRate:
		//	break; // Not used unless the encoder suppoerts sample rate conversion
	
		case siCompressionMaxPacketSize:
			// Hopefully this is set
			if (mMaxPacketByteSize)
			{
				*((UInt32 *)outData) = mMaxPacketByteSize;
			}
			else
			{
				// Worst case since we don't really know
				*((UInt32 *)outData) = kIMA4BlockBytes * 2; // stereo
			}
			break;
	
		case siCompressionChannels:
			*((short *)outData) = (short)mNumChannels;
			break;
				
		default:
			SMACscom::GetInfo(inSourceID, inSelector, outData);
			break;
	};
}

void	SMACIMAscom::SetInfo(SoundSource inSourceID, OSType inSelector, void* inData)
{
	switch(inSelector)
	{
		case siOptionsDialog:
		// We don't do an Options Dialog for IMA, but if we did, the call to 
		// activate it would go here
			break;
		
		// IMA does not care about the sample rate, but we'll just accept the data so if we're
		// asked what the sample rate is, we can tell whoever it is that asks.
		case siCompressionSampleRate: 
			mSampleRate = (*((UnsignedFixed *)inData)) >> 16;
			break;
	
		// This really is the only setting IMA cares about
		case siCompressionChannels:
			mNumChannels = *(short*)inData;
			break;
	
		// IMA does not do VBR, but if a different codec does, the following is necessary -- 
		// it just can't fail here. If a codec has a discrete VBR mode and a discrete CBR mode,
		// it can get toggled here		
		//case siClientAcceptsVBR:
		//	break;
	
		// This is used to signal the codec that the actual source data is exhausted and
		// it's time to append zeros for encoders that require this
		//case siSourceIsExhausted:
			// in this case it seems to be passed by value -- ugh!
			// mSourceIsExhausted = (Boolean)((UInt32)inData);
			// Now pass this on, so no break!
		default:
			SMACscom::SetInfo(inSourceID, inSelector, inData);
			break;
	};
}

// This is the answer to a QuickTime call to GetInfo for siCompressionFactor
void	SMACIMAscom::GetCompressionInfo(CompressionInfo& outCompressionInfo)
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
// The compression params are or contain the "MagicCookie" used by the codec
// A MagicCookie can just be a "wave chunk" --  a series of QuickTime atoms that
// specify the format and the appropriate configuration
void	SMACIMAscom::GetCompressionParams(void* outData)
{	
	//	We don't have Compression params
	Handle theCompressionParams = NULL;
	//	set the return value
	*static_cast<Handle*>(outData) = theCompressionParams;
}

// Normally, QuickTime will call this directly. In IMA it doesn't.
// The compression params are or contain the "MagicCookie" used by the codec
// When this is called we can initialize the codec as we will have all the necessary information
// to do so. 
ComponentInstance	SMACIMAscom::SetCompressionParams(const void* inData)
{
	// Parse the incoming data, store in the compression params
	//	figure out the general format to encode
	AudioStreamBasicDescription theOutputFormat = {mSampleRate, kIMAAudioGeneralFormatID, 0, kIMA4BlockBytes, kIMA4BlockSamples, 0, mNumChannels, 0, 0 };
	//	use the format to locate a suitable encoder
	Component theEncoderComponent = FindIMAAudioEncoderComponent(theOutputFormat);
	ThrowIf(theEncoderComponent == NULL, badFormat, "SMACIMAscom::SetCompressionParams: couldn't find an encoder");
	
	//	initialize the encoder with the translation we want it to perform
	ComponentInstance theEncoder = InitializeIMAAudioEncoder(theEncoderComponent, theOutputFormat);
	ThrowIf(theEncoder == NULL, badFormat, "SMACIMAscom::SetCompressionParams: couldn't initialize the encoder");

	return theEncoder;
}

Component	SMACIMAscom::FindIMAAudioEncoderComponent(const AudioStreamBasicDescription& inFormat)
{
	ComponentDescription	theEncoderDescription = { kAudioEncoderComponentType, inFormat.mFormatID, 0, 0, 0 };
	
	// If a component has sub types, ala MPEG-4, you'll need to loop here and look at the sub-type
	Component theEncoderComponent = FindNextComponent(NULL, &theEncoderDescription);

	return theEncoderComponent;
}

// Note: The encoder deals with SInt16s while the decoder deals with CoreAudio floats.
// This is primarily to illustrate how to deal with either format.
static AudioStreamBasicDescription*	FindSInt16Format(const AudioStreamBasicDescription* inFormatList, UInt32 inNumberFormats)
{
	AudioStreamBasicDescription* theAnswer = NULL;
	
	// Encoders can have multiple input formats
	while((inNumberFormats > 0) && (theAnswer == NULL))
	{
		if((inFormatList->mFormatID == kAudioFormatLinearPCM) && ((inFormatList->mFormatFlags & kNESInt16FormatFlags) == kNESInt16FormatFlags) && (inFormatList->mBitsPerChannel == 16))
		{
			theAnswer = const_cast<AudioStreamBasicDescription*>(inFormatList);
		}
		
		++inFormatList;
		--inNumberFormats;
	}
	
	return theAnswer;
}

ComponentInstance	SMACIMAscom::InitializeIMAAudioEncoder(Component inEncoderComponent, const AudioStreamBasicDescription& inFormat)
{
	UInt32 theSize;
	ComponentInstance theEncoder = OpenComponent(inEncoderComponent);
	ThrowIf(theEncoder == NULL, badComponentInstance, "SMACIMAscom::InitializeIMAAudioEncoder: couldn't open the component");
	
	//	first, give the encoder the info we have
	theSize = sizeof(AudioStreamBasicDescription);
	ComponentResult theError = AudioCodecSetProperty(theEncoder, kAudioCodecPropertyCurrentOutputFormat, theSize, &inFormat);
	ThrowIfError(theError, (CAException)theError, "SMACIMAscom::InitializeIMAAudioEncoder: got an error setting the output format");
	
	//	now find out what it can take as input
	theError = AudioCodecGetPropertyInfo(theEncoder, kAudioCodecPropertySupportedInputFormats, &theSize, NULL);
	ThrowIfError(theError, (CAException)theError, "SMACIMAscom::InitializeIMAAudioEncoder: got an error getting the available input format list size");
	
	UInt32 theNumberAvailableInputFormats = theSize / sizeof(AudioStreamBasicDescription);
	AudioStreamBasicDescription* theAvailableInputFormats = new AudioStreamBasicDescription[theNumberAvailableInputFormats];
	
	try
	{
		theSize = theNumberAvailableInputFormats * sizeof(AudioStreamBasicDescription);
		theError = AudioCodecGetProperty(theEncoder, kAudioCodecPropertySupportedInputFormats, &theSize, theAvailableInputFormats);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::InitializeIMAAudioEncoder: got an error getting the available input formats");
	
		//	find an acceptable input format
		AudioStreamBasicDescription* theInputFormat = FindSInt16Format(theAvailableInputFormats, theNumberAvailableInputFormats);
		ThrowIf(theInputFormat == NULL, badFormat, "SMACIMAscom::InitializeIMAAudioEncoder: couldn't find an acceptable input format");
	
		// finish filling out the input format
		theInputFormat->mSampleRate = inFormat.mSampleRate;
		theInputFormat->mChannelsPerFrame = inFormat.mChannelsPerFrame;
		theInputFormat->mBytesPerFrame = 2 * inFormat.mChannelsPerFrame;
		theInputFormat->mFormatID = kAudioFormatLinearPCM;
		theInputFormat->mFormatFlags = kNESInt16FormatFlags;
		theInputFormat->mBytesPerPacket = 2 * inFormat.mChannelsPerFrame;
		theInputFormat->mFramesPerPacket = 1;
		theInputFormat->mBitsPerChannel = 16;

		//	tell the encoder about it, but set the sample rate and the number of channels
		theSize = sizeof(AudioStreamBasicDescription);
		theError = AudioCodecSetProperty(theEncoder, kAudioCodecPropertyCurrentInputFormat, theSize, theInputFormat);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::InitializeIMAAudioEncoder: got an error setting the input format");
		
		delete[] theAvailableInputFormats;
		theAvailableInputFormats = NULL;
	}
	catch(...)
	{
		delete[] theAvailableInputFormats;
		throw;
	}
	
	//	finally initialize the encoder    
    theError = AudioCodecInitialize(theEncoder, NULL, NULL, NULL, 0);
	ThrowIfError(theError, (CAException)theError, "SMACIMAscom::InitializeIMAAudioEncoder: got an error initializing the encoder");
	
	return theEncoder;
}

// IMA doesn't really need to use an Extended Sound Description, but any VBR format
// needs to.  So for purposes of illustration the encoder is using extended sound descriptions
void	SMACIMAscom::SetOutput(SoundComponentData* inRequested, SoundComponentData** outActual)
{
	SMACscom::SetOutput(inRequested, outActual); // Yes, do this first!
	if(inRequested->format == mOutputData.desc.format)
	{
		mOutputData.desc.numChannels = mNumChannels;
		// Set any flags that need to be set -- IMA doesn't need to set any
	}
}

// Overrides the method in SMACscom. Necessary due to the way IMA works. This will not be necessary
// for many codecs.
void	SMACIMAscom::GetSourceData(SoundComponentData** outData)
{
	ComponentResult theError = 0;
	UInt32	theNumberOutputPackets = 0;
	UInt32	theEncoderStatus = 0;
	UInt32	theActualOutputDataByteSize = 0;
	UInt32  maxPacketSize = 0;
	*outData = NULL;
	
	//	make sure we have some source data to start with
	if(SMACSCDUtility::NeedMoreSourceData(mSourceData))
	{
		theError = SoundComponentGetSourceData(mSourceComponent, &mSourceData);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from SoundComponentGetSourceData");
	}
	
	// On more complex encoders this will often be done in the SetCompressionParams call
	if (!mEncoder)
	{
		mNumChannels = mOutputData.desc.numChannels = mSourceData->numChannels;
		mSampleRate = (mSourceData->sampleRate) >> 16;
		CreateIMAEncoder();
		UInt32 theSize = sizeof(UInt32);
		ComponentResult theError = AudioCodecGetProperty(mEncoder, kAudioCodecPropertyPacketFrameSize, &theSize, &mPacketFrameSize);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: siCompressionParams got an error from AudioCodecGetProperty while getting the packet frame size");
		
		//	get the maximum number of bytes in 1 packet of data
		theSize = sizeof(UInt32);
		theError = AudioCodecGetProperty(mEncoder, kAudioCodecPropertyMaximumPacketByteSize, &theSize, &mMaxPacketByteSize);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: siCompressionParams got an error from AudioCodecGetProperty while getting the maximum packet byte size");
	}

	//	spin until we produce a packet of data or we run completely out of source data
	UInt32	theFramesProduced = 0;
	UInt32	theOutputDataByteSize = 0;
	while(SMACSCDUtility::HasData(mSourceData) && (theFramesProduced < mPacketFrameSize))
	{
		//	stuff as much input data into the encoder as we can
		UInt32 theInputDataByteSize = SMACSCDUtility::GetDataByteSize(mSourceData);
		maxPacketSize = min(kIMA4BlockSamples * mNumChannels, theInputDataByteSize/sizeof(SInt16));

		theInputDataByteSize = maxPacketSize * 2; // this is all we have converted
		UInt32 theNumberOfPackets = theInputDataByteSize >> mNumChannels; // works only for 1 or 2 channels
		theError = AudioCodecAppendInputData(mEncoder, mSourceData->buffer, &theInputDataByteSize, &theNumberOfPackets, NULL);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from AudioCodecAppendInputData");
		
		//	update the source data with the amount of data consumed
		SMACSCDUtility::ConsumedData(mSourceData, theInputDataByteSize);
		
		//	see if we can get a packet of output data
		theActualOutputDataByteSize = mMaxPacketByteSize;
		theNumberOutputPackets = 1;
		theEncoderStatus = kAudioCodecProduceOutputPacketFailure;
		theError = AudioCodecProduceOutputPackets(mEncoder, mOutputBuffer, &theActualOutputDataByteSize, &theNumberOutputPackets, NULL, &theEncoderStatus);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
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
				ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from SoundComponentGetSourceData");
			}
		}
	}
	
	// Now if we need to append zeros, we would do it here
	// A common way to signal the codec it should do it is to call AppendInputData with
	// 0 bytes to append
	// IMA never needs to do this
	// The following is actual working code from AAC. It won't do anything in IMA since
	// mSourceIsExhausted is always false, but it won't break anything either
	/*
	if(theFramesProduced < mPacketFrameSize)
	{
		// Tell the encoder to pad with 0s
		if (mSourceIsExhausted)
		{
			UInt32 theInputDataByteSize = SMACSCDUtility::GetDataByteSize(mSourceData); // better be 0
			UInt32 theNumberOfPackets = 0;
			maxPacketSize = min(kIMA4BlockSamples * mNumChannels, theInputDataByteSize/sizeof(SInt16));
			theInputDataByteSize = maxPacketSize * 2; // this is all we have converted
	
			theError = AudioCodecAppendInputData(mEncoder, mSourceData->buffer, &theInputDataByteSize, &theNumberOfPackets, NULL);
			ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from AudioCodecAppendInputData");
		}
		
		//	we ran out of input data, but we still haven't produced enough output data
		//	so we have to try one last time to pull on the encoder in case it has
		//	some left overs that it can give us
		theActualOutputDataByteSize = mMaxPacketByteSize;
		theNumberOutputPackets = 1;
		theEncoderStatus = kAudioCodecProduceOutputPacketFailure;
		theError = AudioCodecProduceOutputPackets(mEncoder, mOutputBuffer, &theActualOutputDataByteSize, &theNumberOutputPackets, NULL, &theEncoderStatus);
		ThrowIfError(theError, (CAException)theError, "SMACIMAscom::GetSourceData: got an error from AudioCodecProduceOutputPackets");
		
		if(theNumberOutputPackets == 1)
		{
			//	we produced a full packet of frames, so we're done
			theFramesProduced = mPacketFrameSize;
			theOutputDataByteSize += theActualOutputDataByteSize;
		}
	}
	*/
	
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

// Not necessary for more complex encoders as SetCompressionParams() will be called directly by QuickTime
void	SMACIMAscom::CreateIMAEncoder()
{
	mEncoder = SetCompressionParams(NULL);
	UInt32 theSize = sizeof(UInt32);
	ComponentResult theError = AudioCodecGetProperty(mEncoder, kAudioCodecPropertyPacketFrameSize, &theSize, &mPacketFrameSize);
	ThrowIfError(theError, (CAException)theError, "SMACIMAscom::CreateIMAEncoder: got an error from AudioCodecGetProperty");
	
	//	toss the old output buffer
	delete[] mOutputBuffer;
	
	//	allocate enough space for 1 packet of data, since that's
	//	that's all this component will produce per call to GetSourceData
	//	note that this is going to be 16 bit integer
	mOutputBuffer = new Byte[mPacketFrameSize * sizeof(SInt16) * mOutputData.desc.numChannels];
}
