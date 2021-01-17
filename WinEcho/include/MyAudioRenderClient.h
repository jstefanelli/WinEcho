#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "WinEchoApi.h"

namespace winecho {
	class WINECHO_API MyAudioRenderClient {
		protected:
			IAudioRenderClient* renderClient;
		public:
			MyAudioRenderClient() = delete;
			MyAudioRenderClient(const MyAudioRenderClient&) = delete;
			MyAudioRenderClient& operator=(const MyAudioRenderClient&) = delete;

			MyAudioRenderClient(IAudioRenderClient* renderClient);
			~MyAudioRenderClient();

			IAudioRenderClient* RenderClient() const;

			HRESULT GetBuffer(UINT32 framesRequested, BYTE** buffer);
			HRESULT ReleaseBuffer(UINT32 framesRequested, DWORD flags);

	};
}