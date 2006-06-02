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
	SMACsdec.h

=============================================================================*/
#if !defined(__SMACsdec_h__)
#define __SMACsdec_h__

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#if	TARGET_API_MAC_OSX && !USE_QTML_HEADERS
#include <Carbon/Carbon.h>
#else
#include <Sound.h>
#endif

#if	TARGET_API_MAC_OSX
#include "CAMutex.h"
#endif

#include "AudioCodec.h"
#include <stdio.h>

//#define	CaptureDataToFile	1

//=============================================================================
//	SMACsdec
//=============================================================================

class SMACsdec
{

//	Construction/Destruction
public:
								SMACsdec(ComponentInstance inSelf);
	virtual						~SMACsdec();

//	Sound Component Methods
public:
	virtual void				SetSource(SoundSource inSourceID, ComponentInstance inSourceComponent);
	virtual void				SetOutput(SoundComponentData* inRequested, SoundComponentData** inActual);
	
	virtual void				PlaySourceBuffer(SoundSource inSourceID, SoundParamBlock* inPB, SInt32 inActions);
	virtual void				StopSource(SInt16 inNumberSources, SoundSource* inSources);
	
	virtual void				GetInfo(SoundSource inSourceID, OSType inSelector, void* outData);
	virtual void				SetInfo(SoundSource inSourceID, OSType inSelector, void* inData);
	
	virtual void				GetSourceData(SoundComponentData** outData);

//	Implementation
protected:
	virtual void				GetCompressionInfo(CompressionInfo& outCompressionInfo) = 0;
	virtual void				GetDecompressionParams(void* outData) = 0;
	virtual ComponentInstance	SetDecompressionParams(const void* inData) = 0;
	
	virtual void				GetCurrentInputPacket(void*& outPacketData, UInt32& outPacketDataSize, UInt32& outPacketDataFrameSize);
	virtual void				ConsumeCurrentInputPacket(UInt32 inPacketDataByteSize, UInt32 inPacketDataFrameSize);

	ComponentInstance			mSelf;
	ComponentInstance			mSourceComponent;
	AudioCodec					mDecoder;
	SoundSource					mSourceID;
	SoundComponentData*			mSourceData;
	SoundComponentData			mOutputData;
	UInt32						mPacketFrameSize;
	float *						mFloatBuffer;
	Byte*						mOutputBuffer;
	Boolean						mHasAltiVec;

#if	TARGET_API_MAC_OSX
	CAMutex*					mThreadStateMutex;
#endif

#if	CaptureDataToFile
	SInt16						mOutputFileRefNum;
#endif

#if USE_DIRECT_ADEC
protected:
	// pure virtual interface to redirect audio codec calls thru derived class
									
	virtual	ComponentResult		DirectAudioCodecVersion(AudioCodec inCodec) 
								= NULL;
								
	virtual	ComponentResult		DirectAudioCodecGetPropertyInfo(AudioCodec inCodec,
									AudioCodecPropertyID	inPropertyID,
									UInt32*					outSize,
									Boolean*				outWritable)
								= NULL;
	
	virtual	ComponentResult		DirectAudioCodecGetProperty(AudioCodec inCodec,
									AudioCodecPropertyID	inPropertyID,
									UInt32*					ioPropertyDataSize,
									void*					outPropertyData)
								= NULL;
	
	virtual	ComponentResult		DirectAudioCodecSetProperty(AudioCodec inCodec,
									AudioCodecPropertyID	inPropertyID,
									UInt32					inPropertyDataSize,
									const void*				inPropertyData)
								= NULL;
	
	virtual	ComponentResult		DirectAudioCodecInitialize(AudioCodec inCodec,
									const AudioStreamBasicDescription*	inInputFormat,
									const AudioStreamBasicDescription*	inOutputFormat,
									const void*							inMagicCookie,
									UInt32								inMagicCookieByteSize)
								= NULL;
								
	virtual	ComponentResult		DirectAudioCodecUninitialize(AudioCodec inCodec)
								= NULL;
	
	virtual	ComponentResult		DirectAudioCodecAppendInputData(AudioCodec inCodec,
									const void*							inInputData,
									UInt32*								ioInputDataByteSize,
									UInt32*								ioNumberPackets,
									const AudioStreamPacketDescription*	inPacketDescription)
								= NULL;
								
	virtual	ComponentResult		DirectAudioCodecProduceOutputPackets(AudioCodec inCodec,
									void*							outOutputData,
									UInt32*							ioOutputDataByteSize,
									UInt32*							ioNumberPackets,
									AudioStreamPacketDescription*	outPacketDescription,
									UInt32*							outStatus) 
								= NULL;
								
	virtual ComponentResult		DirectAudioCodecReset(AudioCodec inCodec)
								= NULL;
								
	virtual ComponentResult		DirectAudioCodecClose(AudioCodec inCodec) 
									{ return noErr; } // not pure virtual, because it's called in our destructor
								
#endif
	
};

#endif
