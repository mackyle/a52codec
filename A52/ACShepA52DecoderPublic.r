/*
	A52 / AC-3 Decompression Codec
	2004, Shepmaster <shepmaster@applesolutions.com>
	
	Uses code from the liba52 project and the CoreAudio SDK.
*/

//#define kPrimaryResourceID				-17130
#define RES_ID							-17131
#define COMP_TYPE						'adec'
#define COMP_SUBTYPE					'ac-3'
#define COMP_MANUF						'shep'
#define VERSION							0x00010000
#define NAME							"AC3"
#define DESCRIPTION						"An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define ENTRY_POINT						"ACShepA52DecoderEntry"

#define kPrimaryResourceID               -17131
#define kComponentType                   'adec'
#define kComponentSubtype                'ac-3'
#define kComponentManufacturer           'shep'
#define	kComponentFlags                  0
#define kComponentVersion                0x00010000
#define kComponentName                   "AC3"
#define kComponentInfo                   "An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define kComponentEntryPoint             "ACShepA52DecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

#include "XCAResources.r"

#define RES_ID							-17132
#define COMP_TYPE						'adec'
#define COMP_SUBTYPE					0x6D732000
#define COMP_MANUF						'shep'
#define VERSION							0x00010000
#define NAME							"AC3"
#define DESCRIPTION						"An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define ENTRY_POINT						"ACShepA52DecoderEntry"

#define kPrimaryResourceID               -17132
#define kComponentType                   'adec'
#define kComponentSubtype                0x6D732000
#define kComponentManufacturer           'shep'
#define	kComponentFlags                  0
#define kComponentVersion                0x00010000
#define kComponentName                   "AC3"
#define kComponentInfo                   "An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define kComponentEntryPoint             "ACShepA52DecoderEntry"
#define	kComponentPublicResourceMapType	 0
#define kComponentIsThreadSafe           1

#include "XCAResources.r"
