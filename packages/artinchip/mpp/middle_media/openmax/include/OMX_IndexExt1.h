/*
 * Copyright (c) 2016 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/** @file OMX_IndexExt.h - OpenMax IL version 1.1.2
 * The OMX_IndexExt header file contains extensions to the definitions
 * for both applications and components .
 */

#ifndef OMX_IndexExt1_h
#define OMX_IndexExt1_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each OMX header shall include all required header files to allow the
 * header to compile without errors.  The includes below are required
 * for this header file to compile successfully
 */
#include <OMX_Index.h>


/** Khronos standard extension indices.

This enum lists the current Khronos extension indices to OpenMAX IL.
*/
typedef enum OMX_INDEXEXTTYPE {
	/*0x7f000000-0x7f000FFF  common*/
	OMX_IndexVendorStreamFrameEnd = 0x7F000001,/**< reference: OMX_PARAM_FRAMEEND */
	OMX_IndexVendorClearBuffer = 0x7F000002,

	/*0x7f001000-0x7f001fff video_render*/
	OMX_IndexVendorVideoRenderInit = 0x7F001000,
	OMX_IndexVendorVideoRenderScreenSize = 0x7F001001, /**< reference: OMX_PARAM_SCREEN_SIZE */

	/*0x7f002000-0x7f002fff audio_render*/
	OMX_IndexVendorAudioRenderInit = 0x7F002000,
	OMX_IndexVendorAudioRenderVolume = 0x7F002001,/**< reference: OMX_PARAM_AUDIO_VOLUME */

	/*0x7f003000-0x7f003fff Demuxer*/
	OMX_IndexVendorDemuxerSkipTrack = 0x7F003000, /**< reference: OMX_PARAM_SKIP_TRACK */

} OMX_INDEXEXTTYPE;



typedef struct OMX_PARAM_FRAMEEND {
    OMX_U32 nSize; /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex; /**< port that this structure applies to */
    OMX_BOOL bFrameEnd;             /**0-clear   1- set */
} OMX_PARAM_FRAMEEND;

typedef struct OMX_PARAM_SCREEN_SIZE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nWidth;
    OMX_S32 nHeight;
} OMX_PARAM_SCREEN_SIZE;


typedef struct OMX_PARAM_AUDIO_VOLUME {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nVolume;
} OMX_PARAM_AUDIO_VOLUME;

typedef struct OMX_PARAM_SKIP_TRACK {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
} OMX_PARAM_SKIP_TRACK;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_IndexExt_h */
/* File EOF */
