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
	AudioCodecComponentGlue.c

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "AudioCodec.h"
#include "ACCodecDispatchTypes.h"

#if	!TARGET_OS_MAC || !TARGET_RT_MAC_MACHO
	#include <MixedMode.h>
	#if MP_SUPPORT
		#include "MPMixedModeSupport.h"
	#endif
#endif

DEFINE_API(ComponentResult)
AudioCodecGetPropertyInfo(	AudioCodec				inCodec,
							AudioCodecPropertyID	inPropertyID,
							UInt32*					outSize,
							Boolean*				outWritable)
{
	AudioCodecGetPropertyInfoGluePB myAudioCodecGetPropertyInfoGluePB;
	myAudioCodecGetPropertyInfoGluePB.componentFlags = 0;
	myAudioCodecGetPropertyInfoGluePB.componentParamSize = 12;
	myAudioCodecGetPropertyInfoGluePB.componentWhat = kAudioCodecGetPropertyInfoSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecGetPropertyInfoGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecGetPropertyInfoGluePB.outWritable = outWritable;
	myAudioCodecGetPropertyInfoGluePB.outSize = outSize;
	myAudioCodecGetPropertyInfoGluePB.inPropertyID = inPropertyID;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecGetPropertyInfoGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecGetPropertyInfoGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecGetPropertyInfoGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecGetProperty(	AudioCodec				inCodec,
						AudioCodecPropertyID	inPropertyID,
						UInt32*					ioPropertyDataSize,
						void*					outPropertyData)
{
	AudioCodecGetPropertyGluePB myAudioCodecGetPropertyGluePB;
	myAudioCodecGetPropertyGluePB.componentFlags = 0;
	myAudioCodecGetPropertyGluePB.componentParamSize = 12;
	myAudioCodecGetPropertyGluePB.componentWhat = kAudioCodecGetPropertySelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecGetPropertyGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecGetPropertyGluePB.outPropertyData = outPropertyData;
	myAudioCodecGetPropertyGluePB.ioPropertyDataSize = ioPropertyDataSize;
	myAudioCodecGetPropertyGluePB.inPropertyID = inPropertyID;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecGetPropertyGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecGetPropertyGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecGetPropertyGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecSetProperty(	AudioCodec				inCodec,
						AudioCodecPropertyID	inPropertyID,
						UInt32					inPropertyDataSize,
						const void*				inPropertyData)
{
	AudioCodecSetPropertyGluePB myAudioCodecSetPropertyGluePB;
	myAudioCodecSetPropertyGluePB.componentFlags = 0;
	myAudioCodecSetPropertyGluePB.componentParamSize = 12;
	myAudioCodecSetPropertyGluePB.componentWhat = kAudioCodecSetPropertySelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecSetPropertyGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecSetPropertyGluePB.inPropertyData = inPropertyData;
	myAudioCodecSetPropertyGluePB.inPropertyDataSize = inPropertyDataSize;
	myAudioCodecSetPropertyGluePB.inPropertyID = inPropertyID;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecSetPropertyGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecSetPropertyGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecSetPropertyGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecInitialize(	AudioCodec							inCodec,
						const AudioStreamBasicDescription*	inInputFormat,
						const AudioStreamBasicDescription*	inOutputFormat,
						const void*							inMagicCookie,
						UInt32								inMagicCookieByteSize)
{
	AudioCodecInitializeGluePB myAudioCodecInitializeGluePB;
	myAudioCodecInitializeGluePB.componentFlags = 0;
	myAudioCodecInitializeGluePB.componentParamSize = 16;
	myAudioCodecInitializeGluePB.componentWhat = kAudioCodecInitializeSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecInitializeGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecInitializeGluePB.inMagicCookieByteSize = inMagicCookieByteSize;
	myAudioCodecInitializeGluePB.inMagicCookie = inMagicCookie;
	myAudioCodecInitializeGluePB.inOutputFormat = inOutputFormat;
	myAudioCodecInitializeGluePB.inInputFormat = inInputFormat;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecInitializeGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecInitializeGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecInitializeGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecUninitialize(AudioCodec inCodec)
{
	AudioCodecUninitializeGluePB myAudioCodecUninitializeGluePB;
	myAudioCodecUninitializeGluePB.componentFlags = 0;
	myAudioCodecUninitializeGluePB.componentParamSize = 0;
	myAudioCodecUninitializeGluePB.componentWhat = kAudioCodecUninitializeSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecUninitializeGluePB.inCodec = inCodec;
	#endif

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecUninitializeGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecUninitializeGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecUninitializeGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecAppendInputData(	AudioCodec							inCodec,
							const void*							inInputData,
							UInt32*								ioInputDataByteSize,
							UInt32*								ioNumberPackets,
							const AudioStreamPacketDescription*	inPacketDescription)
{
	AudioCodecAppendInputDataGluePB myAudioCodecAppendInputDataGluePB;
	myAudioCodecAppendInputDataGluePB.componentFlags = 0;
	myAudioCodecAppendInputDataGluePB.componentParamSize = 16;
	myAudioCodecAppendInputDataGluePB.componentWhat = kAudioCodecAppendInputDataSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecAppendInputDataGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecAppendInputDataGluePB.inPacketDescription = inPacketDescription;
	myAudioCodecAppendInputDataGluePB.ioNumberPackets = ioNumberPackets;
	myAudioCodecAppendInputDataGluePB.ioInputDataByteSize = ioInputDataByteSize;
	myAudioCodecAppendInputDataGluePB.inInputData = inInputData;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecAppendInputDataGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecAppendInputDataGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecAppendInputDataGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecProduceOutputPackets(	AudioCodec						inCodec,
								void*							outOutputData,
								UInt32*							ioOutputDataByteSize,
								UInt32*							ioNumberPackets,
								AudioStreamPacketDescription*	outPacketDescription,
								UInt32*							outStatus)
{
	AudioCodecProduceOutputPacketsGluePB myAudioCodecProduceOutputPacketsGluePB;
	myAudioCodecProduceOutputPacketsGluePB.componentFlags = 0;
	myAudioCodecProduceOutputPacketsGluePB.componentParamSize = 20;
	myAudioCodecProduceOutputPacketsGluePB.componentWhat = kAudioCodecProduceOutputDataSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecProduceOutputPacketsGluePB.inCodec = inCodec;
	#endif
	
	myAudioCodecProduceOutputPacketsGluePB.outStatus = outStatus;
	myAudioCodecProduceOutputPacketsGluePB.outPacketDescription = outPacketDescription;
	myAudioCodecProduceOutputPacketsGluePB.ioNumberPackets = ioNumberPackets;
	myAudioCodecProduceOutputPacketsGluePB.ioOutputDataByteSize = ioOutputDataByteSize;
	myAudioCodecProduceOutputPacketsGluePB.outOutputData = outOutputData;

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecProduceOutputPacketsGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecProduceOutputPacketsGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecProduceOutputPacketsGluePB);
	#endif
}

DEFINE_API(ComponentResult)
AudioCodecReset(AudioCodec inCodec)
{
	AudioCodecResetGluePB myAudioCodecResetGluePB;
	myAudioCodecResetGluePB.componentFlags = 0;
	myAudioCodecResetGluePB.componentParamSize = 0;
	myAudioCodecResetGluePB.componentWhat = kAudioCodecResetSelect;
	
	#if	!TARGET_OS_WIN32
		myAudioCodecResetGluePB.inCodec = inCodec;
	#endif

	#if TARGET_API_MAC_OS8
		return (ComponentResult)CallUniversalProc(CallComponentUPP, 0x000000F0, &myAudioCodecResetGluePB);
	#elif TARGET_API_MAC_OSX
		return (ComponentResult)CallComponentDispatch((ComponentParameters*)&myAudioCodecResetGluePB);
	#else
		return (ComponentResult)CallComponent(inCodec, (ComponentParameters*)&myAudioCodecResetGluePB);
	#endif
}
