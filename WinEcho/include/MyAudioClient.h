#pragma once

#include <memory>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#include "MyAudioCaptureClient.h"
#include "MyAudioRenderClient.h"
#include "WinEchoApi.h"

namespace winecho {
	class WINECHO_API MyAudioClient {
		protected:
			IAudioClient* client;
			WAVEFORMATEX* mixFormat;

		public:
			MyAudioClient() = delete;
			MyAudioClient(const MyAudioClient&) = delete;
			MyAudioClient& operator=(const MyAudioClient&) = delete;

			MyAudioClient(IAudioClient* client);
			~MyAudioClient();

			IAudioClient* Client() const;
			WAVEFORMATEX* MixFormat() const;

			HRESULT Initialize(REFERENCE_TIME referenceTime, bool loopback, bool eventBased);
			UINT32 GetBufferSize();
			UINT32 GetCurrentPadding();

			std::shared_ptr<MyAudioCaptureClient> GetCaptureClientService();
			std::shared_ptr<MyAudioRenderClient> GetRenderClientService();
			HRESULT Start();
			HRESULT Stop();
			HRESULT SetEventHandle(HANDLE eventHandle);

			static constexpr REFERENCE_TIME REFTIMES_PER_SEC = 10000000;
			static constexpr REFERENCE_TIME REFTIMES_PER_MILLISEC = 10000;
	};
}