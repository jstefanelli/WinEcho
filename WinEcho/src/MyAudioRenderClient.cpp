#include "MyAudioRenderClient.h"
#include <stdexcept>

namespace winecho {
	MyAudioRenderClient::MyAudioRenderClient(IAudioRenderClient* renderClient) : renderClient(renderClient) {
		
	}

	MyAudioRenderClient::~MyAudioRenderClient() {
		renderClient->Release();
		renderClient = nullptr;
	}

	IAudioRenderClient* MyAudioRenderClient::RenderClient() const {
		return renderClient;
	}

	HRESULT MyAudioRenderClient::GetBuffer(UINT32 framesRequested, BYTE** buffer) {
		return renderClient->GetBuffer(framesRequested, buffer);
	}

	HRESULT MyAudioRenderClient::ReleaseBuffer(UINT32 framesReleased, DWORD flags) {
		return renderClient->ReleaseBuffer(framesReleased, flags);
	}
}