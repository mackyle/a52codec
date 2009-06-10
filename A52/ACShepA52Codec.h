/*
	A52 / AC-3 Codec Base Class
	2004, Shepmaster <shepmaster@applesolutions.com>
	
	Uses code from the CoreAudio SDK.
*/

#if !defined(__ACShepA52Codec_h__)
#define __ACShepA52Codec_h__

//=============================================================================
//	Includes
//=============================================================================

#include "ACSimpleCodec.h"
#include <vector>

//=============================================================================
//	ACShepA52Codec
//
//	This class encapsulates the common implementation of an A/52 codec.
//  Basically we throw in a buffer...
//=============================================================================

class ACShepA52Codec : public ACSimpleCodec {

//	Construction/Destruction
public:
						ACShepA52Codec(UInt32 inInputBufferByteSize, OSType theSubType);
	virtual				~ACShepA52Codec();

//	Data Handling
public:
	virtual void		Initialize(const AudioStreamBasicDescription* inInputFormat, const AudioStreamBasicDescription* inOutputFormat, const void* inMagicCookie, UInt32 inMagicCookieByteSize);
	virtual void		Reset();

	virtual void		GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData);
	virtual void		GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, Boolean& outWritable);
	virtual	void		ReallocateInputBuffer(UInt32 inInputBufferByteSize);


//	Implementation
protected:
};

#endif
