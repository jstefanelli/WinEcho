#include "MyAudioCaptureClient.h"
#include <stdexcept>

namespace winecho {
	MyAudioCaptureClient::MyAudioCaptureClient(IAudioCaptureClient* client) : captureClient(client) {
		
	}

	MyAudioCaptureClient::~MyAudioCaptureClient() {
		captureClient->Release();
		captureClient = nullptr;
	}

	IAudioCaptureClient* MyAudioCaptureClient::CaptureClient() const {
		return captureClient;
	}

	HRESULT MyAudioCaptureClient::GetBuffer(UINT32* framesRequestedAnLoaded, BYTE** buffer, DWORD* flags) {
		return captureClient->GetBuffer(buffer, framesRequestedAnLoaded, flags, nullptr, nullptr);
	}

	HRESULT MyAudioCaptureClient::ReleaseBuffer(UINT32 framesReleased) {
		return captureClient->ReleaseBuffer(framesReleased);
	}

	HRESULT MyAudioCaptureClient::GetNextPacketSize(UINT32* nextPacketSize) {
		return captureClient->GetNextPacketSize(nextPacketSize);
	}
}
