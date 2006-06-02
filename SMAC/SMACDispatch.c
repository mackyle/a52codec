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
/*
	File:		SMACDispatch.c

	Contains:	Common dispatching for SMAC

	Written by:	Jim Reekes and Kip Olson

	Copyright:	© 1992-1997, 1999-2001 by Apple Computer, Inc., all rights reserved.

*/

#include <CAConditionalMacros.h>
#include <Carbon/Carbon.h>

#if	!TARGET_API_MAC_CARBON
#include <Gestalt.h>
#include <LowMem.h>
#include <MacMemory.h>
#include <Movies.h>
#include <Sound.h>
#include <SoundPriv.h>
#include <LowMemPriv.h>
#include <Traps.h>
#include <ProcessesPriv.h>
#endif

#if	TARGET_API_MAC_CARBON
#define	GLUE(a,b)		a##b
#define	GLUE2(a,b)		GLUE(a,b)
#define	GLUE3(a,b,c)	GLUE2(GLUE2(a,b),c)
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Sound Component Function Prototypes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// unless SoundComponentBaseName is define, none of the dispatching code will be included

#ifdef SoundComponentBaseName

// The SoundComponentBaseName is used to concatenate unique routine names.

#define SoundComponentDispatch GLUE2(SoundComponentBaseName(), Dispatch)
#define __SoundComponentOpen GLUE3(__, SoundComponentBaseName(), Open)
#define __SoundComponentClose GLUE3(__, SoundComponentBaseName(), Close)
#define __SoundComponentCanDo GLUE3(__, SoundComponentBaseName(), CanDo)
#define __SoundComponentVersion GLUE3(__, SoundComponentBaseName(), Version)
#define __SoundComponentRegister GLUE3(__, SoundComponentBaseName(), Register)
#define __SoundComponentInitOutputDevice GLUE3(__, SoundComponentBaseName(), InitOutputDevice)
#define __SoundComponentSetSource GLUE3(__, SoundComponentBaseName(), SetSource)
#define __SoundComponentGetSource GLUE3(__, SoundComponentBaseName(), GetSource)
#define __SoundComponentGetSourceData GLUE3(__, SoundComponentBaseName(), GetSourceData)
#define __SoundComponentSetOutput GLUE3(__, SoundComponentBaseName(), SetOutput)
#define __SoundComponentAddSource GLUE3(__, SoundComponentBaseName(), AddSource)
#define __SoundComponentRemoveSource GLUE3(__, SoundComponentBaseName(), RemoveSource)
#define __SoundComponentGetInfo GLUE3(__, SoundComponentBaseName(), GetInfo)
#define __SoundComponentSetInfo GLUE3(__, SoundComponentBaseName(), SetInfo)
#define __SoundComponentStartSource GLUE3(__, SoundComponentBaseName(), StartSource)
#define __SoundComponentStopSource GLUE3(__, SoundComponentBaseName(), StopSource)
#define __SoundComponentPauseSource GLUE3(__, SoundComponentBaseName(), PauseSource)
#define __SoundComponentPlaySourceBuffer GLUE3(__, SoundComponentBaseName(), PlaySourceBuffer)
#define __SoundComponentAddMChannelSource GLUE3(__, SoundComponentBaseName(), AddMChannelSource)
#define __SoundComponentValidSource GLUE3(__, SoundComponentBaseName(), ValidSource)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// component stuff

static pascal ComponentResult __SoundComponentOpen(void *unused1, ComponentInstance self);
static pascal ComponentResult __SoundComponentClose(SoundComponentGlobalsPtr globals, ComponentInstance self);
static pascal ComponentResult __SoundComponentCanDo(void *unused1, short selector);
#ifdef __SoundComponentVersionRoutine
static pascal ComponentResult __SoundComponentVersion(void *unused1);
#endif
#ifdef __SoundComponentRegisterRoutine
static pascal ComponentResult __SoundComponentRegister(SoundComponentGlobalsPtr globals);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// basic stuff

#ifdef __SoundComponentInitOutputDeviceRoutine
static pascal ComponentResult __SoundComponentInitOutputDevice(SoundComponentGlobalsPtr globals, long actions);
#endif
#ifdef __SoundComponentSetSourceRoutine
static pascal ComponentResult __SoundComponentSetSource(SoundComponentGlobalsPtr globals, SoundSource sourceID, ComponentInstance source);
#endif
static pascal ComponentResult __SoundComponentGetSource(SoundComponentGlobalsPtr globals, SoundSource sourceID, ComponentInstance *source);
#ifdef __SoundComponentGetSourceDataRoutine
static pascal ComponentResult __SoundComponentGetSourceData(SoundComponentGlobalsPtr globals, SoundComponentDataPtr *sourceData);
#endif
#ifdef __SoundComponentSetOutputRoutine
static pascal ComponentResult __SoundComponentSetOutput(SoundComponentGlobalsPtr globals, SoundComponentDataPtr requested, SoundComponentDataPtr *actual);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// junction methods (i.e. the mixer) must be called at non-interrupt level

#ifdef __SoundComponentAddSourceRoutine
static pascal ComponentResult __SoundComponentAddSource(SoundComponentGlobalsPtr globals, SoundSource *sourceID);
#endif
#ifdef __SoundComponentRemoveSourceRoutine
static pascal ComponentResult __SoundComponentRemoveSource(SoundComponentGlobalsPtr globals, SoundSource sourceID);
#endif
#ifdef __SoundComponentAddMChannelSourceRoutine
static pascal ComponentResult __SoundComponentAddMChannelSource(SoundComponentGlobalsPtr globals, OSType streamID, SoundSource *sourceID);
#endif
#ifdef __SoundComponentValidSourceRoutine
static pascal ComponentResult __SoundComponentValidSource(SoundComponentGlobalsPtr globals, SoundSource sourceID);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// info methods

#ifdef __SoundComponentGetInfoRoutine
static pascal ComponentResult __SoundComponentGetInfo(SoundComponentGlobalsPtr globals, SoundSource sourceID, OSType selector, void *infoPtr);
#endif
#ifdef __SoundComponentSetInfoRoutine
static pascal ComponentResult __SoundComponentSetInfo(SoundComponentGlobalsPtr globals, SoundSource sourceID, OSType selector, void *infoPtr);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// control methods
#ifdef __SoundComponentStartSourceRoutine
static pascal ComponentResult __SoundComponentStartSource(SoundComponentGlobalsPtr globals, short count, SoundSource *sources);
#endif
#ifdef __SoundComponentStopSourceRoutine
static pascal ComponentResult __SoundComponentStopSource(SoundComponentGlobalsPtr globals, short count, SoundSource *sources);
#endif
#ifdef __SoundComponentPauseSourceRoutine
static pascal ComponentResult __SoundComponentPauseSource(SoundComponentGlobalsPtr globals, short count, SoundSource *sources);
#endif
#ifdef __SoundComponentPlaySourceBufferRoutine
static pascal ComponentResult __SoundComponentPlaySourceBuffer(SoundComponentGlobalsPtr globals, SoundSource sourceID, SoundParamBlockPtr pb, long actions);
#endif


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// types
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

// These structs are use in PowerMac builds to cast the
// ComponentParameters passed into our component's entry point.

enum {
	uppSoundComponentDispatchProcInfo = kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(ComponentResult)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ComponentParameters *)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(void *)))
};

#define paramsHeader unsigned char flags; unsigned char paramSize; short what

typedef struct {
	paramsHeader;
	ComponentInstance self;
	void *unused1;
} OpenPB, *OpenPBPtr;

typedef struct {
	paramsHeader;
	ComponentInstance self;
} ClosePB, *ClosePBPtr;

typedef struct {
	paramsHeader;
	long actions;
} InitOutputDevicePB, *InitOutputDevicePBPtr;

typedef struct {
	paramsHeader;
	ComponentInstance source;
	SoundSource sourceID;
} SetSourcePB, *SetSourcePBPtr;

typedef struct {
	paramsHeader;
	ComponentInstance *source;
	SoundSource sourceID;
} GetSourcePB, *GetSourcePBPtr;

typedef struct {
	paramsHeader;
	SoundComponentDataPtr *sourceData;
} GetSourceDataPB, *GetSourceDataPBPtr;

typedef struct {
	paramsHeader;
	SoundComponentDataPtr *actual;
	SoundComponentDataPtr requested;
} SetOutputPB, *SetOutputPBPtr;

typedef struct {
	paramsHeader;
	SoundSource *sourceID;
} AddSourcePB, *AddSourcePBPtr;

typedef struct {
	paramsHeader;
	SoundSource sourceID;
} RemoveSourcePB, *RemoveSourcePBPtr;

typedef struct {
	paramsHeader;
	void *infoPtr;
	OSType selector;
	SoundSource sourceID;
} GetInfoPB, *GetInfoPBPtr;

typedef struct {
	paramsHeader;
	void *infoPtr;
	OSType selector;
	SoundSource sourceID;
} SetInfoPB, *SetInfoPBPtr;

typedef struct {
	paramsHeader;
	SoundSource *sources;
	short count;
} StartSourcePB, *StartSourcePBPtr;

typedef struct {
	paramsHeader;
	SoundSource *sources;
	short count;
} StopSourcePB, *StopSourcePBPtr;

typedef struct {
	paramsHeader;
	SoundSource *sources;
	short count;
} PauseSourcePB, *PauseSourcePBPtr;

typedef struct {
	paramsHeader;
	long actions;
	SoundParamBlockPtr pb;
	SoundSource sourceID;
} PlaySourcePB, *PlaySourcePBPtr;

typedef struct {
	paramsHeader;
	SoundSource *sourceID;
	OSType streamID;
} AddMChannelSourcePB, *AddMChannelSourcePBPtr;

typedef struct {
	paramsHeader;
	SoundSource sourceID;
} ValidSourcePB, *ValidSourcePBPtr;

typedef struct {
	paramsHeader;
	TimeRecord *theTime;
	void * unused1;
} GetTimePB, *GetTimePBPtr;

typedef struct {
	paramsHeader;
	UnsignedFixed sampleRate;
	wide *sampleCount;
	void * unused1;
} SoundTickPB, *SoundTickPBPtr;

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Sound Component Entry Point
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PowerPC dispatching, entry point is a routine descriptor

#if TARGET_CPU_PPC || TARGET_API_MAC_CARBON

#if	TARGET_API_MAC_CARBON
pascal ComponentResult SoundComponentDispatch(ComponentParameters *p, SoundComponentGlobalsPtr globals);

pascal ComponentResult SoundComponentDispatch(ComponentParameters *p, SoundComponentGlobalsPtr globals)
#else
static pascal ComponentResult SoundComponentProc(ComponentParameters *params, SoundComponentGlobalsPtr globals);

RoutineDescriptor SoundComponentDispatch = BUILD_ROUTINE_DESCRIPTOR(uppSoundComponentDispatchProcInfo, SoundComponentProc);

static pascal ComponentResult SoundComponentProc(ComponentParameters *p, SoundComponentGlobalsPtr globals)
#endif
{
	ComponentResult				result = badComponentSelector;

	switch (p->what)	// standard component selectors
	{
		#ifdef __SoundComponentRegisterRoutine
		case kComponentRegisterSelect:
			result = __SoundComponentRegister(globals);
			break;
		#endif

		case kComponentVersionSelect:
		#ifdef __SoundComponentVersionRoutine
			result = __SoundComponentVersion(0);
		#else
			result = kSoundComponentVersion;
		#endif
			break;

		case kComponentCanDoSelect:
			result = __SoundComponentCanDo(0, *((short *) &p->params[0]));
			break;

		#ifdef __SoundComponentCloseRoutine
		case kComponentCloseSelect:
			result = __SoundComponentClose(globals, ((ClosePBPtr)p)->self);
			break;
		#endif

		#ifdef __SoundComponentOpenRoutine
		case kComponentOpenSelect:
			result = __SoundComponentOpen(globals, ((OpenPBPtr)p)->self);
			break;
		#endif

		// selectors that cannot be delegated

		#ifdef __SoundComponentInitOutputDeviceRoutine
		case kSoundComponentInitOutputDeviceSelect:
			result = __SoundComponentInitOutputDevice(globals, ((InitOutputDevicePBPtr)p)->actions);
			break;
		#endif

		#ifdef __SoundComponentSetSourceRoutine
		case kSoundComponentSetSourceSelect:
			result = __SoundComponentSetSource(globals, ((SetSourcePBPtr)p)->sourceID, ((SetSourcePBPtr)p)->source);
			break;
		#endif

		case kSoundComponentGetSourceSelect:
			result = __SoundComponentGetSource(globals, ((GetSourcePBPtr)p)->sourceID, ((GetSourcePBPtr)p)->source);
			break;

		#ifdef __SoundComponentGetSourceDataRoutine
		case kSoundComponentGetSourceDataSelect:
			result = __SoundComponentGetSourceData(globals, ((GetSourceDataPBPtr)p)->sourceData);
			break;
		#endif

		#ifdef __SoundComponentSetOutputRoutine
		case kSoundComponentSetOutputSelect:
			result = __SoundComponentSetOutput(globals, ((SetOutputPBPtr)p)->requested, ((SetOutputPBPtr)p)->actual);
			break;
		#endif

		// selectors that can be delegated

		#ifdef __SoundComponentAddSourceRoutine
		case kSoundComponentAddSourceSelect:
			result = __SoundComponentAddSource(globals, ((AddSourcePBPtr)p)->sourceID);
			break;
		#endif

		#ifdef __SoundComponentRemoveSourceRoutine
		case kSoundComponentRemoveSourceSelect:
			result = __SoundComponentRemoveSource(globals, ((RemoveSourcePBPtr)p)->sourceID);
			break;
		#endif

		#ifdef __SoundComponentGetInfoRoutine
		case kSoundComponentGetInfoSelect:
#if	SMInstrumentation_On
			if(((GetInfoPBPtr)p)->selector == '1984')
			{
				SMInstrumentationFlushTrace(globals->instrumentationTrace);
			}
#endif
			result = __SoundComponentGetInfo(globals, ((GetInfoPBPtr)p)->sourceID, ((GetInfoPBPtr)p)->selector, ((GetInfoPBPtr)p)->infoPtr);
			break;
		#endif

		#ifdef __SoundComponentSetInfoRoutine
		case kSoundComponentSetInfoSelect:
			result = __SoundComponentSetInfo(globals, ((SetInfoPBPtr)p)->sourceID, ((SetInfoPBPtr)p)->selector, ((SetInfoPBPtr)p)->infoPtr);
			break;
		#endif

		#ifdef __SoundComponentStartSourceRoutine
		case kSoundComponentStartSourceSelect:
			result = __SoundComponentStartSource(globals, ((StartSourcePBPtr)p)->count, ((StartSourcePBPtr)p)->sources);
			break;
		#endif

		#ifdef __SoundComponentStopSourceRoutine
		case kSoundComponentStopSourceSelect:
			result = __SoundComponentStopSource(globals, ((StopSourcePBPtr)p)->count, ((StopSourcePBPtr)p)->sources);
			break;
		#endif

		#ifdef __SoundComponentPauseSourceRoutine
		case kSoundComponentPauseSourceSelect:
			result = __SoundComponentPauseSource(globals, ((PauseSourcePBPtr)p)->count, ((PauseSourcePBPtr)p)->sources);
			break;
		#endif

		#ifdef __SoundComponentPlaySourceBufferRoutine
		case kSoundComponentPlaySourceBufferSelect:
			result = __SoundComponentPlaySourceBuffer(globals, ((PlaySourcePBPtr)p)->sourceID, ((PlaySourcePBPtr)p)->pb, ((PlaySourcePBPtr)p)->actions);
			break;
		#endif

		#ifdef __SoundComponentAddMChannelSourceRoutine
		case kSoundComponentAddMChannelSourceSelect:
			result = __SoundComponentAddMChannelSource(globals, ((AddMChannelSourcePBPtr)p)->streamID, ((AddMChannelSourcePBPtr)p)->sourceID);
			break;
		#endif

		#ifdef __SoundComponentValidSourceRoutine
		case kSoundComponentValidSourceSelect:
			result = __SoundComponentValidSource(globals, ((ValidSourcePBPtr)p)->sourceID);
			break;
		#endif

		default:
			if (p->what > kDelegatedSoundComponentSelectors)
				result = DelegateComponentCall(p, globals->sourceComponent);
			break;
	}

	return (result);
}

#else /* !TARGET_CPU_PPC */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Mac 68k and non Mac OS dispatching

pascal ComponentResult SoundComponentDispatch(ComponentParameters *params, SoundComponentGlobalsPtr globals);

pascal ComponentResult SoundComponentDispatch(ComponentParameters *params, SoundComponentGlobalsPtr globals)
{
	ComponentFunctionUPP		theRtn;
	ComponentResult				result;
	short						selector = params->what;
	Boolean						checkCanDo = false;

#define kDelegateSoundComponentCall	((ComponentFunctionUPP) -1)

SelectorSwitch:
	if (selector < 0)
		switch (selector - kComponentRegisterSelect)	// standard component selectors
		{
			case kComponentRegisterSelect - kComponentRegisterSelect:
				#ifdef __SoundComponentRegisterRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentRegister;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			case kComponentVersionSelect - kComponentRegisterSelect:
				#ifdef __SoundComponentVersionRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentVersion;
				#else
					return (kSoundComponentVersion);
				#endif
				break;

			case kComponentCanDoSelect - kComponentRegisterSelect:
				#ifdef __SoundComponentCanDoRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentCanDo;
				#else
					selector = *((short *) &params->params[0]);
					checkCanDo = true;
					goto SelectorSwitch;
				#endif
				break;

			case kComponentCloseSelect - kComponentRegisterSelect:
				#ifdef __SoundComponentCloseRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentClose;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			case kComponentOpenSelect - kComponentRegisterSelect:
				#ifdef __SoundComponentOpenRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentOpen;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			default:
				theRtn = (ComponentFunctionUPP)nil;
				break;
		}
	else if (selector < kDelegatedSoundComponentSelectors)			// selectors that cannot be delegated
		switch (selector)
		{
			case kSoundComponentInitOutputDeviceSelect:
				#ifdef __SoundComponentInitOutputDeviceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentInitOutputDevice;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			case kSoundComponentSetSourceSelect:
				#ifdef __SoundComponentSetSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentSetSource;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			case kSoundComponentGetSourceSelect:
				#ifdef __SoundComponentGetSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentGetSource;
				#else
					theRtn = (ComponentFunctionUPP) __SoundComponentGetSource;
				#endif
				break;

			case kSoundComponentGetSourceDataSelect:
				#ifdef __SoundComponentGetSourceDataRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentGetSourceData;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			case kSoundComponentSetOutputSelect:
				#ifdef __SoundComponentSetOutputRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentSetOutput;
				#else
					theRtn = (ComponentFunctionUPP)nil;
				#endif
				break;

			default:
				theRtn = (ComponentFunctionUPP)nil;
				break;
		}
	else													// selectors that can be delegated
		switch (selector)
		{
			case kSoundComponentAddSourceSelect:
				#ifdef __SoundComponentAddSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentAddSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentRemoveSourceSelect:
				#ifdef __SoundComponentRemoveSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentRemoveSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentGetInfoSelect:
				#ifdef __SoundComponentGetInfoRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentGetInfo;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentSetInfoSelect:
				#ifdef __SoundComponentSetInfoRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentSetInfo;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentStartSourceSelect:
				#ifdef __SoundComponentStartSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentStartSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentStopSourceSelect:
				#ifdef __SoundComponentStopSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentStopSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentPauseSourceSelect:
				#ifdef __SoundComponentPauseSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentPauseSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentPlaySourceBufferSelect:
				#ifdef __SoundComponentPlaySourceBufferRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentPlaySourceBuffer;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentAddMChannelSourceSelect:
				#ifdef __SoundComponentAddMChannelSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentAddMChannelSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			case kSoundComponentValidSourceSelect:
				#ifdef __SoundComponentValidSourceRoutine
					theRtn = (ComponentFunctionUPP) __SoundComponentValidSource;
				#else
					theRtn = kDelegateSoundComponentCall;
				#endif
				break;

			default:
				theRtn = kDelegateSoundComponentCall;
				break;
		}

	if (checkCanDo)
		result = ((theRtn == nil) || (theRtn == kDelegateSoundComponentCall)) ? 0 : 1;
	else
	{
		if (theRtn == nil)
			result = badComponentSelector;
		else if (theRtn == kDelegateSoundComponentCall)
			result = DelegateComponentCall(params, globals->sourceComponent);
		else
			result = CallComponentFunctionWithStorage((Handle) globals, params, theRtn);
	}
	return (result);
}

#endif /* !TARGET_CPU_PPC */


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifndef __SoundComponentGetSourceRoutine
static pascal ComponentResult __SoundComponentGetSource(SoundComponentGlobalsPtr globals, SoundSource sourceID, ComponentInstance *source)
{
#pragma unused (sourceID)

	*source = globals->sourceComponent;
	return (noErr);
}
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifndef __SoundComponentCanDoRoutine
static pascal ComponentResult __SoundComponentCanDo(void *unused1, short selector)
{
#pragma unused (unused1)

	ComponentResult		result;

	switch (selector)
	{
		#ifdef __SoundComponentRegisterRoutine
		case kComponentRegisterSelect:
			result = true;
			break;
		#endif

		case kComponentVersionSelect:
			result = true;
			break;

		case kComponentCanDoSelect:
			result = true;
			break;

		#ifdef __SoundComponentCloseRoutine
		case kComponentCloseSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentOpenRoutine
		case kComponentOpenSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentInitOutputDeviceRoutine
		case kSoundComponentInitOutputDeviceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentSetSourceRoutine
		case kSoundComponentSetSourceSelect:
			result = true;
			break;
		#endif

		case kSoundComponentGetSourceSelect:
			result = true;
			break;

		#ifdef __SoundComponentGetSourceDataRoutine
		case kSoundComponentGetSourceDataSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentSetOutputRoutine
		case kSoundComponentSetOutputSelect:
			result = true;
			break;
		#endif

		// selectors that can be delegated

		#ifdef __SoundComponentAddSourceRoutine
		case kSoundComponentAddSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentRemoveSourceRoutine
		case kSoundComponentRemoveSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentGetInfoRoutine
		case kSoundComponentGetInfoSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentSetInfoRoutine
		case kSoundComponentSetInfoSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentStartSourceRoutine
		case kSoundComponentStartSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentStopSourceRoutine
		case kSoundComponentStopSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentPauseSourceRoutine
		case kSoundComponentPauseSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentPlaySourceBufferRoutine
		case kSoundComponentPlaySourceBufferSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentAddMChannelSourceRoutine
		case kSoundComponentAddMChannelSourceSelect:
			result = true;
			break;
		#endif

		#ifdef __SoundComponentValidSourceRoutine
		case kSoundComponentValidSourceSelect:
			result = true;
			break;
		#endif

		default:
			result = false;
			break;
	}

	return (result);
}
#endif

#endif /* SoundComponentBaseName */


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// end of common code for sound component dispatching
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc utility routines used by sound components
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if	TARGET_API_MAC_CARBON

#ifdef SoundComponentBaseName
#ifndef	PrimeSource
#define	PrimeSource						GLUE3(__, SoundComponentBaseName(), PrimeSource)
#endif
#endif
#endif

static OSErr NewSoundComponentLocalStorage(long size, Handle *storageHandle, Ptr *storage);
static OSErr NewSoundComponentGlobalStorage(long size, Handle *storageHandle, Ptr *storage, Boolean *inAppHeap);
static void  DisposeSoundComponentStorage(Handle storageHandle);

#if	!TARGET_API_MAC_CARBON
static OSErr GetAppStorageWithRespect(Handle *storageHandle, long size);
static OSErr GetSystemStorageWithRespect(Handle *storageHandle, long size);
static OSErr HandleVMCFMCommon(CFragInitBlockPtr initInfo);
OSErr HandleVMCFMInitialize(CFragInitBlockPtr initInfo);
void HandleVMCFMTerminate(void);
static OSErr SetClosureMemory(CFragContextID contextID, CFragClosureID closureID, Boolean holdMemory);
#endif

#if	TARGET_API_MAC_CARBON

static inline OSErr NewSoundComponentGlobalStorage(long size, Handle *storageHandle, Ptr *storage, Boolean *inAppHeap)
{
	Handle		h = NewHandle(size);
	if (h != NULL) {
		*storageHandle = h;
		*storage = *h;
		*inAppHeap = true;
	}
	return MemError();
}

static inline OSErr NewSoundComponentLocalStorage(long size, Handle *storageHandle, Ptr *storage)
{
	Boolean	ignore;
	return NewSoundComponentGlobalStorage(size, storageHandle, storage, &ignore);
}

static inline void DisposeSoundComponentStorage(Handle storageHandle)
{
	DisposeHandle(storageHandle);
}

#else	/* !TARGET_API_MAC_CARBON */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// routines for Power Mac builds, which are simply in-lines for 68k
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static long PurgeAppSpaceContiguous(void)
{
	long	totalPurgeSpace;
	long	contigPurgeSpace;

	PurgeSpace(&totalPurgeSpace, &contigPurgeSpace);
	return (contigPurgeSpace);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static unsigned long PurgeSpaceSystemContiguous(void)
{
	long	totalPurgeSpace;
	long	contigPurgeSpace;
#if	!TARGET_API_MAC_CARBON
	THz		curZone;

	curZone = GetZone();
	SetZone(SystemZone());
#endif
	PurgeSpace(&totalPurgeSpace, &contigPurgeSpace);
#if	!TARGET_API_MAC_CARBON
	SetZone(curZone);
#endif
	return (contigPurgeSpace);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Try to get space in application's heap and making sure we allocate the memory
// allowing enough space in the heap for others.
// Hold memory to make it safe for VM during interrupts.

static OSErr GetAppStorageWithRespect(Handle *storageHandle, long size)
{
	Handle			h;
	OSErr			result;

	ReserveMem(size);					// create a low heap object
	h = NewHandle(size);
	if (h != nil)
	{
		result = noErr;
		if (PurgeAppSpaceContiguous() < kMinAppHeapFreeSpace)
		{
			DisposeHandle(h);
			result = memFullErr;
		}
	}
	else	// NewHandle() failed
		result = MemError();

	if (result == noErr)
	{
		HLock(h);
#if !TARGET_OS_WIN32 && !TARGET_API_MAC_CARBON
		//	no harm in always holding the memory 
		//	even when VM is off
		result = HoldMemory(*h, size);
		if (result != noErr)
		{
			DisposeHandle(h);
			h = nil;
		}
		
		//	hold the master pointer
		result = HoldMemory(h, 4);
		if(result != noErr)
		{
			UnholdMemory(*h, size);
			DisposeHandle(h);
			h = nil;
		}
#endif
		*storageHandle = h;
	}
	return (result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Try to get space in system heap and making sure we allocate the memory
// allowing enough space in the heap for others.

static OSErr GetSystemStorageWithRespect(Handle *storageHandle, long size)
{
	Handle			h;
	Handle			tempH;
	unsigned long	neededContigSpace;
	unsigned long	hiSysRoom;
	unsigned long	lowTempRoom;
	OSErr			result;

#if	TARGET_API_MAC_CARBON
	h = NewHandle(size);
#else
	ReserveMemSys(size);
	h = NewHandleSys(size);
#endif
	if (h != nil)
	{
		result = noErr;
#if TARGET_API_MAC_CARBON || !TARGET_OS_MAC
		neededContigSpace = 32000;
#else
		neededContigSpace = GetExpandMemMinSysHeapFreeSpace();
#endif

#if TARGET_OS_MAC && !TARGET_API_MAC_CARBON
#ifdef INITVERSION
		if ( GetToolTrapAddress(_Unimplemented) != GetToolTrapAddress(_OSDispatch) )
#else
		if ( GetExpandMemProcessMgrExists() )
#endif
		{
			if (SysZoneFloatSizes(nil, &lowTempRoom) != noErr)
				lowTempRoom = 0;
			if (lowTempRoom < neededContigSpace)
			{
				if (SysZoneFloatSizes(&hiSysRoom, nil) != noErr)
					hiSysRoom = 0;
				if ((hiSysRoom + lowTempRoom) < neededContigSpace)
				{
					DisposeHandle(h);
					result = memFullErr;
				}
			}
		}

		else	// we don't know if we can grow the system heap, so let's guess
#endif // TARGET_OS_MAC && !TARGET_API_MAC_CARBON
		{
			if (PurgeSpaceSystemContiguous() < neededContigSpace)
			// that didn't work, try to force the system heap to compact and grow
			// by allocated the reserve amount and if that fails, then give up
			{
#if	TARGET_API_MAC_CARBON
				tempH = NewHandle(neededContigSpace);
#else
				tempH = NewHandleSys(neededContigSpace);
#endif
				if (tempH != nil)
					DisposeHandle(tempH);
				else
				{
					DisposeHandle(h);
					result = memFullErr;
				}
			}
		}
	}
	else	// NewHandleSys() failed
		result = MemError();

	if (result == noErr)
	{
		HLock(h);

#if !TARGET_OS_WIN32 && !TARGET_API_MAC_CARBON
		//	no harm in always holding the memory 
		//	even when VM is off
		result = HoldMemory(*h, size);
		if (result != noErr)
		{
			DisposeHandle(h);
			h = nil;
		}
		
		//	hold the master pointer
		result = HoldMemory(h, 4);
		if(result != noErr)
		{
			UnholdMemory(*h, size);
			DisposeHandle(h);
			h = nil;
		}
#endif

		*storageHandle = h;
	}
	return (result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Try to allocate local component storage, usually in the application's heap
// because the component is only being used by the application. But if we cannot
// find available memory in the application's heap, then we'll try to find
// it in the system.

// returns a locked and deferenced handle

static OSErr NewSoundComponentLocalStorage(long size, Handle *storageHandle, Ptr *storage)
{
	Handle		h;
	OSErr		result;

// check for expiration of development releases on Macs only

	result = GetSystemStorageWithRespect(&h, size);		// try for system space

	if (result != noErr)								// do we have storage?
		result = GetAppStorageWithRespect(&h, size);	// try for app storage

	FailIf(result != noErr, Failure);
	*storageHandle = h;
	*storage = StripAddress(*h);
	return (noErr);

Failure:
	return (result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Try to allocate global component storage, usually in the system heap so that
// the component can be shared by any application. If the system heap cannot be
// used, then use the application's heap. If we do use the app heap, then return
// this fact to the caller so it will know that it is using local storage. On entry
// the caller must set the inAppHeap flag properly, and it will be return with a
// setting that reflects whether or not the storage is global.

// returns a locked and deferenced handle

static OSErr NewSoundComponentGlobalStorage(long size, Handle *storageHandle, Ptr *storage, Boolean *inAppHeap)
{
	Handle		h;
	OSErr		result;

	result = memFullErr;								// assume we have no storage
	*inAppHeap = false;
	result = GetSystemStorageWithRespect(&h, size);

	if (result != noErr)								// do we have storage?
	{
		result = GetAppStorageWithRespect(&h, size);
		if (result == noErr)
			*inAppHeap = true;
	}

	FailIf(result != noErr, Failure);
	*storageHandle = h;
	*storage = StripAddress(*h);
	return (noErr);

Failure:
	return (result);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	define to be a real routine so we can unhold memory
static void DisposeSoundComponentStorage(Handle storageHandle)
{
#if !TARGET_OS_WIN32 && !TARGET_API_MAC_CARBON
	UnholdMemory(*storageHandle, InlineGetHandleSize(storageHandle));
	UnholdMemory(storageHandle, 4);
#endif

	DisposeHandle(storageHandle);
}

#endif	/* !TARGET_API_MAC_CARBON */
