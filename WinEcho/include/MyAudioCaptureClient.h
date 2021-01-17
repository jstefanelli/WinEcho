#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "WinEchoApi.h"

namespace winecho {
	class WINECHO_API MyAudioCaptureClient {
		protected:
			IAudioCaptureClient* captureClient;
		public:
			MyAudioCaptureClient() = delete;
			MyAudioCaptureClient(const MyAudioCaptureClient&) = delete;
			MyAudioCaptureClient& operator=(const MyAudioCaptureClient&) = delete;

			MyAudioCaptureClient(IAudioCaptureClient* client);
			~MyAudioCaptureClient();

			IAudioCaptureClient* CaptureClient() const;

			HRESULT GetBuffer(UINT32* framesRequestedAndLoaded, BYTE** buffer, DWORD* flags);
			HRESULT ReleaseBuffer(UINT32 framesRequested);
			HRESULT GetNextPacketSize(UINT32* nextPacketSize);
	};
}