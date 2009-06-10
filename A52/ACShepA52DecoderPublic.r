/*
	A52 / AC-3 Decompression Codec
	2004, Shepmaster <shepmaster@applesolutions.com>
	
	Uses code from the liba52 project and the CoreAudio SDK.
*/

#define RES_ID							-17131
#define COMP_TYPE						'adec'
#define COMP_SUBTYPE					'ac-3'
#define COMP_MANUF						'cd3r'
#define VERSION							0x00017071
#define NAME							"AC3"
#define DESCRIPTION						"An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define ENTRY_POINT						"ACShepA52DecoderEntry"

#include "AUResources.r"

#define RES_ID							-17133
#define COMP_TYPE						'adec'
#define COMP_SUBTYPE					0x6D732000
#define COMP_MANUF						'cd3r'
#define VERSION							0x00017071
#define NAME							"AC3"
#define DESCRIPTION						"An AudioCodec that decodes A/52 and AC-3 into linear PCM data"
#define ENTRY_POINT						"ACShepA52DecoderEntry"

#include "AUResources.r"