/*	Copyright � 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
#if !defined(__ACCodecDispatch_h__)
#define __ACCodecDispatch_h__

//=============================================================================
//	Includes
//=============================================================================

#include "ACCodec.h"

//=============================================================================
//	ACCodecDispatch
//
//	A function template that implements a component dispatch routine for an
//	AudioCodec in terms of an instance of a subclass of ACCodec.
//	It is best used when called from the primary codec dispatcher like so:
//	
//	extern "C"
//	ComponentResult	FooCodecDispatch(ComponentParameters* inParameters, FooCodec* inThis)
//	{
//		return	ACCodecDispatch(inParameters, inThis);
//	}
//
//	The reason this exists is to better encapuslate all the necessary code
//	to do the dispatching without having to figure out what the C++ mangled
//	name for the entry point to put in the exported symbols file.
//=============================================================================

#if TARGET_OS_MAC
	#if __LP64__
		// comp instance, parameters in forward order
		#define PARAM(_typ, _name, _index, _nparams) \
			_typ _name = *(_typ *)&inParameters->params[_index + 1];
	#else
		// parameters in reverse order, then comp instance
		#define PARAM(_typ, _name, _index, _nparams) \
			_typ _name = *(_typ *)&inParameters->params[_nparams - 1 - _index];
	#endif
#elif TARGET_OS_WIN32
		// (no comp instance), parameters in forward order
		#define PARAM(_typ, _name, _index, _nparams) \
			_typ _name = *(_typ *)&inParameters->params[_index];
#endif

template <class CodecClass>
ComponentResult ACCodecDispatch(ComponentParameters* inParameters, CodecClass* inThis)
{
	ComponentResult	theError = kAudioCodecNoError;
	
	try
	{
		switch (inParameters->what)
		{
			//	these selectors don't use the object pointer
			
			case kComponentOpenSelect:
				{
					Component codec = (Component)inParameters->params[0];
					ComponentDescription cd;
					
					GetComponentInfo(codec, &cd, NULL, NULL, NULL);
					CodecClass*	theCodec = new CodecClass(cd.componentSubType);
					SetComponentInstanceStorage((ComponentInstance)codec, (Handle)theCodec);
				}
				break;
	
			case kComponentCloseSelect:
				delete inThis;
				break;
			
			case kComponentCanDoSelect:
				switch (*((SInt16*)&inParameters->params[1]))
				{
					case kComponentOpenSelect:
					case kComponentCloseSelect:
					case kComponentCanDoSelect:
					case kComponentRegisterSelect:
					case kComponentVersionSelect:
					case kAudioCodecGetPropertyInfoSelect:
					case kAudioCodecGetPropertySelect:
					case kAudioCodecSetPropertySelect:
					case kAudioCodecInitializeSelect:
					case kAudioCodecAppendInputDataSelect:
					case kAudioCodecProduceOutputDataSelect:
					case kAudioCodecResetSelect:
						theError = 1;
						break;
				};
				break;
				
			default:
				//	these selectors use the object pointer
				if(inThis != NULL)
				{
					switch (inParameters->what)
					{
						case kComponentRegisterSelect:
							if(!inThis->Register())
							{
								theError = componentDontRegister;
							}
							break;
				
						case kComponentVersionSelect:
							theError = inThis->GetVersion();
							break;
						
						case kAudioCodecGetPropertyInfoSelect:
							{
								PARAM(AudioCodecPropertyID, inPropertyID, 0, 3);
								PARAM(UInt32 *, outSize, 1, 3);
								PARAM(Boolean *, outWritable, 2, 3);
								
								UInt32 theSize = 0;
								Boolean isWritable = false;
								
								inThis->GetPropertyInfo(inPropertyID, theSize, isWritable);
								if(outSize != NULL)
								{
									*outSize = theSize;
								}
								if(outWritable != NULL)
								{
									*outWritable = isWritable ? 1 : 0;
								}
							}
							break;
				
						case kAudioCodecGetPropertySelect:
							{
								PARAM(AudioCodecPropertyID, inPropertyID, 0, 3);
								PARAM(UInt32 *, ioPropertyDataSize, 1, 3);
								PARAM(void *, outPropertyData, 2, 3);
								
								if((ioPropertyDataSize != NULL) && (outPropertyData != NULL))
								{
									inThis->GetProperty(inPropertyID, *ioPropertyDataSize, outPropertyData);
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecSetPropertySelect:
							{
								PARAM(AudioCodecPropertyID, inPropertyID, 0, 3);
								PARAM(UInt32, inPropertyDataSize, 1, 3);
								PARAM(const void *, inPropertyData, 2, 3);
								
								if(inPropertyData != NULL)
								{
									inThis->SetProperty(inPropertyID, inPropertyDataSize, inPropertyData);
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecInitializeSelect:
							{
								PARAM(const AudioStreamBasicDescription *, inInputFormat, 0, 4);
								PARAM(const AudioStreamBasicDescription *, inOutputFormat, 1, 4);
								PARAM(const void *, inMagicCookie, 2, 4);
								PARAM(UInt32, inMagicCookieByteSize, 3, 4);
								
								inThis->Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
							}
							break;
				
						case kAudioCodecUninitializeSelect:
							{
								inThis->Uninitialize();
							}
							break;
				
						case kAudioCodecAppendInputDataSelect:
							{
								PARAM(const void *, inInputData, 0, 4);
								PARAM(UInt32 *, ioInputDataByteSize, 1, 4);
								PARAM(UInt32 *, ioNumberPackets, 2, 4);
								PARAM(const AudioStreamPacketDescription *, inPacketDescription, 3, 4);
								
								if((inInputData != NULL) && (ioInputDataByteSize != NULL))
								{
									if(ioNumberPackets != NULL)
									{
										inThis->AppendInputData(inInputData, *ioInputDataByteSize, *ioNumberPackets, inPacketDescription);
									}
									else
									{
										UInt32 theNumberPackets = 0;
										inThis->AppendInputData(inInputData, *ioInputDataByteSize, theNumberPackets, inPacketDescription);
									}
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecProduceOutputDataSelect:
							{
								PARAM(void *, outOutputData, 0, 5);
								PARAM(UInt32 *, ioOutputDataByteSize, 1, 5);
								PARAM(UInt32 *, ioNumberPackets, 2, 5);
								PARAM(AudioStreamPacketDescription *, outPacketDescription, 3, 5);
								PARAM(UInt32 *, outStatus, 4, 5);
								
								if((outOutputData != NULL) && (ioOutputDataByteSize != NULL) && (ioNumberPackets != NULL) && (outStatus != NULL))
								{
									*outStatus = inThis->ProduceOutputPackets(outOutputData, *ioOutputDataByteSize, *ioNumberPackets, outPacketDescription);
									if(kAudioCodecProduceOutputPacketFailure == *outStatus)
									{
										theError = paramErr;
									}
								}
								else
								{
									theError = paramErr;
								}
							}
							break;
				
						case kAudioCodecResetSelect:
							{
								inThis->Reset();
							}
							break;
				
						default:
							theError = badComponentSelector;
							break;
					};
				}
				else
				{
					theError = paramErr;
				}
				break;
		};
	}
	catch(ComponentResult inErrorCode)
	{
		theError = inErrorCode;
	}
	catch(...)
	{
		theError = kAudioCodecUnspecifiedError;
	}
	
	return theError;
}

#endif
