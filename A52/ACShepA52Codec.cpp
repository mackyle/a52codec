/*
	A52 / AC-3 Codec Base Class
	2004, Shepmaster <shepmaster@applesolutions.com>
	
	Uses code from the CoreAudio SDK.
*/

//=============================================================================
//	Includes
//=============================================================================

#include "ACShepA52Codec.h"

//=============================================================================
//	ACShepA52Codec
//=============================================================================

ACShepA52Codec::ACShepA52Codec(UInt32 inInputBufferByteSize) : ACSimpleCodec(inInputBufferByteSize) {
}

ACShepA52Codec::~ACShepA52Codec() {
}



void	ACShepA52Codec::Initialize(const AudioStreamBasicDescription* inInputFormat,
								   const AudioStreamBasicDescription* inOutputFormat,
								   const void* inMagicCookie, UInt32 inMagicCookieByteSize) {

	//	use the given arguments, if necessary
	if(inInputFormat != NULL)
	{
		SetCurrentInputFormat(*inInputFormat);
	}

	if(inOutputFormat != NULL)
	{
		SetCurrentOutputFormat(*inOutputFormat);
	}

	//	make sure the sample rate and number of channels match between the input format and the output format
		
	if( (mInputFormat.mSampleRate != mOutputFormat.mSampleRate) ||
		(mInputFormat.mChannelsPerFrame != mOutputFormat.mChannelsPerFrame))
	{
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}

	
	ACSimpleCodec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
}

void	ACShepA52Codec::Reset() {
	ACSimpleCodec::Reset();
}

void	ACShepA52Codec::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable) {
	switch(inPropertyID)
	{
		case kAudioCodecPropertyMaximumPacketByteSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
		
		case kAudioCodecPropertyRequiresPacketDescription:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;

		case kAudioCodecPropertyHasVariablePacketByteSizes:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
            
		case kAudioCodecPropertyPacketFrameSize:
			outPropertyDataSize = sizeof(UInt32);
			outWritable = false;
			break;
            		
		default:
			ACSimpleCodec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
			break;
			
	};
}

void	ACShepA52Codec::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData) {	
	switch(inPropertyID) {
		case kAudioCodecPropertyManufacturerCFString:
		{
			if (ioPropertyDataSize != sizeof(CFStringRef)) {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Shepmaster Productions"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
		}
		
		case kAudioCodecPropertyMaximumPacketByteSize:
  			
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = 3840; //Stolen from liba52 docs
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
            break;
        case kAudioCodecPropertyRequiresPacketDescription:
  			
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = 0; 
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
            break;
        case kAudioCodecPropertyHasVariablePacketByteSizes:
  			
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = 1;
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
            break;
		case kAudioCodecPropertyPacketFrameSize:
			
			if(ioPropertyDataSize == sizeof(UInt32)) {
                *reinterpret_cast<UInt32*>(outPropertyData) = 6 * 256; // A frame has 6 blocks of 256 samples
            } else {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
			break;
		default:
			ACSimpleCodec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}

void ACShepA52Codec::ReallocateInputBuffer(UInt32 inInputBufferByteSize) {
	ACSimpleCodec::ReallocateInputBuffer(inInputBufferByteSize);
}