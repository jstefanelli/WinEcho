#include "MyAudioClient.h"
#include <stdexcept>

const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

namespace winecho {
	MyAudioClient::MyAudioClient(IAudioClient* client) : client(client) {
		HRESULT hr = client->GetMixFormat(&mixFormat);
		if (FAILED(hr) || mixFormat == nullptr) {
			throw std::runtime_error("IAudioClient::GetMixFormat() failed");
		}
	}

	IAudioClient* MyAudioClient::Client() const {
		return client;
	}

	WAVEFORMATEX* MyAudioClient::MixFormat() const {
		return mixFormat;
	}

	MyAudioClient::~MyAudioClient() {
		CoTaskMemFree(mixFormat);
		mixFormat = nullptr;

		client->Release();
		client = nullptr;
	}


	HRESULT MyAudioClient::Initialize(REFERENCE_TIME referenceTime, bool loopback, bool eventBased) {
		return client->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			(loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0) | (eventBased ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0), 
			referenceTime, 
			0, 
			mixFormat, 
			nullptr);
	}


	UINT32 MyAudioClient::GetBufferSize() {
		UINT32 val = 0;
		HRESULT hr = client->GetBufferSize(&val);
		if (FAILED(hr)) {
			throw std::runtime_error("IAudioClient::GetBufferSize() failed");
		}
		return val;
	}

	UINT32 MyAudioClient::GetCurrentPadding() {
		UINT32 val = 0;
		HRESULT hr = client->GetCurrentPadding(&val);
		if (FAILED(hr)) {
			throw std::runtime_error("IAudioClient::GetCurrentPadding() failed");
		}
		return val;
	}

	std::shared_ptr<MyAudioCaptureClient> MyAudioClient::GetCaptureClientService() {
		IAudioCaptureClient* captureClient = nullptr;
		HRESULT hr = client->GetService(IID_IAudioCaptureClient, (void**)&captureClient);
		if (SUCCEEDED(hr) && captureClient != nullptr) {
			return std::move(std::make_shared<MyAudioCaptureClient>(captureClient));
		}
		return nullptr;
	}

	std::shared_ptr<MyAudioRenderClient> MyAudioClient::GetRenderClientService() {
		IAudioRenderClient* renderClient = nullptr;
		HRESULT hr = client->GetService(IID_IAudioRenderClient, (void**)&renderClient);
		if (SUCCEEDED(hr) && renderClient != nullptr) {
			return std::move(std::make_shared<MyAudioRenderClient>(renderClient));
		}
		return nullptr;
	}

	HRESULT MyAudioClient::Start() {
		return client->Start();	
	}

	HRESULT MyAudioClient::Stop() {
		return client->Stop();
	}

	HRESULT MyAudioClient::SetEventHandle(HANDLE eventHandle) {
		return client->SetEventHandle(eventHandle);
	}

}