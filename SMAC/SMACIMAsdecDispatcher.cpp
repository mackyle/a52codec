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
	SMACIMAsdecDispatcher.cpp

=============================================================================*/

// Theory of Operation
//
// This is largely boilerplate code. The IMA can be replaced by the appropriate
// format name.

//=============================================================================
//	Includes
//=============================================================================
#include <CAConditionalMacros.h>

#include "SMACIMAsdec.h"

//=============================================================================
//	SMACIMAsdecDispatcher
//
//	This is the dispatcher for components implemented by the SMACIMAsdec class.
//=============================================================================

//	the component globals
struct	SMACIMAsdecGlobals
{
	ComponentInstance	sourceComponent;
	SMACIMAsdec*		mThis;
	SMACIMAsdecGlobals() : sourceComponent(NULL), mThis(NULL) {}
};
typedef SMACIMAsdecGlobals*	SMACIMAsdecGlobalsPtr;

//	the basic defines for SoundComponentDispatch.c
// The versions are such that they'll override the standard SoundManager ones
#if DEBUG
#define kSoundComponentVersion					0x70ff0004
#else
#define kSoundComponentVersion					0x00020004
#endif
#define SoundComponentBaseName()				SMACIMAsdec
#define SoundComponentGlobals					SMACIMAsdecGlobals
#define SoundComponentGlobalsPtr				SMACIMAsdecGlobalsPtr


//	the component routines implemented
#define __SoundComponentOpenRoutine				kImplemented
#define __SoundComponentCloseRoutine			kImplemented
#define __SoundComponentSetSourceRoutine		kImplemented
#define __SoundComponentSetOutputRoutine		kImplemented
#define __SoundComponentPlaySourceBufferRoutine	kImplemented
#define __SoundComponentStopSourceRoutine		kImplemented
#define __SoundComponentGetInfoRoutine			kImplemented
#define __SoundComponentSetInfoRoutine			kImplemented
#define __SoundComponentGetSourceDataRoutine	kImplemented

//	include SoundComponentDispatch.c to write the dispatcher
extern "C"
{
#include "SMACDispatch.c"

static pascal ComponentResult __SoundComponentOpen(void* inUnused, ComponentInstance inSelf)
{
	ComponentResult theAnswer = 0;

	try
	{
		//	create the globals
		SMACIMAsdecGlobals* theGlobals = new SMACIMAsdecGlobals();
		
		//	creat the SMACIMAsdec object
		theGlobals->mThis = new SMACIMAsdec(inSelf);
		
		//	tell the Component Manager about our globals
		SetComponentInstanceStorage(inSelf, (Handle)theGlobals);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

static pascal ComponentResult __SoundComponentClose(SoundComponentGlobalsPtr inGlobals, ComponentInstance inSelf)
{
	if(inGlobals != NULL)
	{
		//	close the source component
		if(inGlobals->sourceComponent != NULL)
		{
			CloseComponent(inGlobals->sourceComponent);
		}
		
		//	dispose of the SMACIMAsdec object
		delete inGlobals->mThis;
		
		//	dispose of the globals themselves
		delete inGlobals;
	}
	
	return 0;
}

static pascal ComponentResult __SoundComponentSetSource(SoundComponentGlobalsPtr inGlobals, SoundSource inSourceID, ComponentInstance inSourceComponent)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->sourceComponent = inSourceComponent;
		inGlobals->mThis->SetSource(inSourceID, inSourceComponent);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

static pascal ComponentResult __SoundComponentSetOutput(SoundComponentGlobalsPtr inGlobals, SoundComponentDataPtr inRequested, SoundComponentDataPtr* inActual)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->mThis->SetOutput(inRequested, inActual);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

static pascal ComponentResult __SoundComponentPlaySourceBuffer(SoundComponentGlobalsPtr inGlobals, SoundSource inSourceID, SoundParamBlockPtr inPB, SInt32 inActions)
{
	ComponentResult theAnswer = 0;
	
	try
	{
		inGlobals->mThis->PlaySourceBuffer(inSourceID, inPB, inActions);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

static pascal ComponentResult __SoundComponentStopSource(SoundComponentGlobalsPtr inGlobals, SInt16 inNumberSources, SoundSource* inSources)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->mThis->StopSource(inNumberSources, inSources);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

static pascal ComponentResult __SoundComponentGetInfo(SoundComponentGlobalsPtr inGlobals, SoundSource inSourceID, OSType inSelector, void* outData)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->mThis->GetInfo(inSourceID, inSelector, outData);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}
	
	return theAnswer;
}

static pascal ComponentResult __SoundComponentSetInfo(SoundComponentGlobalsPtr inGlobals, SoundSource inSourceID, OSType inSelector, void* inData)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->mThis->SetInfo(inSourceID, inSelector, inData);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}
    
	return theAnswer;
}

static pascal ComponentResult __SoundComponentGetSourceData(SoundComponentGlobalsPtr inGlobals, SoundComponentData** outData)
{
	ComponentResult theAnswer = 0;

	try
	{
		inGlobals->mThis->GetSourceData(outData);
	}
	catch(UInt32 theError)
	{
		theAnswer = theError;
	}
	catch(...)
	{
		theAnswer = -50;
	}

	return theAnswer;
}

} // extern "C"

