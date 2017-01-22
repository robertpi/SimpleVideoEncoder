/****************************** Module Header ******************************\
* Module Name:  PictureWriter.cpp
* Project:      CSUniversalAppImageToVideo
* Copyright (c) Microsoft Corporation.
*
* This class implement encoding images to video. We set the duration to one
* second per image default.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/licenses.aspx#MPL
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/


#include "pch.h"

#include "PictureWriter.h"
#include <stdexcept>
#include "comdef.h"

#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "comsuppw.lib")

using namespace Microsoft::WRL;
using namespace Windows::Foundation;

const unsigned int RATE_NUM = 10;
const unsigned int RATE_DENOM = 1;
const unsigned int BITRATE = 3000000;
const unsigned int ASPECT_NUM = 1;
const unsigned int ASPECT_DENOM = 1;
const unsigned long  BPP_IN = 32;
const long long DURATION = 500000 * (long long)RATE_DENOM / (long long)RATE_NUM; // In hundred-nanoseconds
const unsigned int ONE_SECOND = RATE_NUM / RATE_DENOM;
const unsigned int FRAME_NUM = 10 * ONE_SECOND;

#define CHK(statement)	{									\
	HRESULT _hr = (statement);								\
	if (FAILED(_hr))										\
	{														\
		_com_error err(_hr);								\
		printf("%i: %s", _hr, err.ErrorMessage());			\
		throw std::domain_error((char*)err.ErrorMessage()); \
	};														\
}
#define EXPORT extern "C" __declspec(dllexport)

struct Video {
	LONGLONG hnsSampleTime;
	unsigned int width;
	unsigned int height;
	IMFSinkWriter *spSinkWriter;
	DWORD streamIndex;
};

EXPORT Video *NewVideoEncoder(LPCWSTR file, unsigned int width, unsigned int height)
{
	CHK(MFStartup(MF_VERSION));

	struct Video *video = (Video *)malloc(sizeof(struct Video));
	video->width = width;
	video->height = height;
	video->hnsSampleTime = 0;

	IMFByteStream *spByteStream;
	CHK(MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, file, &spByteStream));
	
	// Create the Sink Writer
	IMFAttributes *spAttr;
	CHK(MFCreateAttributes(&spAttr, 10));
	CHK(spAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, true));

	CHK(MFCreateSinkWriterFromURL(L".mp4", spByteStream, spAttr, &video->spSinkWriter));

	// Setup the output media type   

	IMFMediaType *spTypeOut;
	CHK(MFCreateMediaType(&spTypeOut));
	CHK(spTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHK(spTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
	CHK(spTypeOut->SetUINT32(MF_MT_AVG_BITRATE, BITRATE));
	CHK(spTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	CHK(MFSetAttributeSize(spTypeOut, MF_MT_FRAME_SIZE, width, height));
	CHK(MFSetAttributeRatio(spTypeOut, MF_MT_FRAME_RATE, RATE_NUM, RATE_DENOM));
	CHK(MFSetAttributeRatio(spTypeOut, MF_MT_PIXEL_ASPECT_RATIO, ASPECT_NUM, ASPECT_DENOM));

	CHK(video->spSinkWriter->AddStream(spTypeOut, &video->streamIndex));

	// Setup the input media type   

	IMFMediaType *spTypeIn;
	CHK(MFCreateMediaType(&spTypeIn));
	CHK(spTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHK(spTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
	CHK(spTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	CHK(MFSetAttributeSize(spTypeIn, MF_MT_FRAME_SIZE, width, height));
	CHK(MFSetAttributeRatio(spTypeIn, MF_MT_FRAME_RATE, RATE_NUM, RATE_DENOM));
	CHK(MFSetAttributeRatio(spTypeIn, MF_MT_PIXEL_ASPECT_RATIO, ASPECT_NUM, ASPECT_DENOM));

	CHK(video->spSinkWriter->SetInputMediaType(video->streamIndex, spTypeIn, nullptr));

	CHK(video->spSinkWriter->BeginWriting());

	return video;
}

EXPORT void AddFrame(struct Video *video, const byte *buffer, unsigned long bufferLength,  int imageWidth, int imageHeight, int imageWidth2)
{

	// Create a media sample   
	IMFSample *spSample;
	CHK(MFCreateSample(&spSample));
	CHK(spSample->SetSampleDuration(DURATION));
	CHK(spSample->SetSampleTime(0));
	video->hnsSampleTime += DURATION;

	// Add a media buffer
	IMFMediaBuffer *spBuffer;
	CHK(MFCreateMemoryBuffer(bufferLength, &spBuffer));
	CHK(spBuffer->SetCurrentLength(bufferLength));
	CHK(spSample->AddBuffer(spBuffer));

	// Copy the picture into the buffer
	unsigned char *pbBuffer = nullptr;
	CHK(spBuffer->Lock(&pbBuffer, nullptr, nullptr));
	//CHK(MFCopyImage(pbBuffer + 4 * video->width * (video->height - imageHeight),
	//	4 * video->width, buffer, -4 * imageWidth2, 4 * imageWidth2, imageHeight));
	memcpy(pbBuffer, buffer, video->height * imageWidth2 * 4);
	CHK(spBuffer->Unlock());

	// Write the media sample   
	CHK(video->spSinkWriter->WriteSample(video->streamIndex, spSample));
}

EXPORT void Finalize(struct Video *video)
{
	CHK(video->spSinkWriter->Finalize());
	delete video;
}