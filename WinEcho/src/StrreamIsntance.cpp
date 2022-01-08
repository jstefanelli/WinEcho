#include "StreamInstance.h"
#include "CApiUtils.h"

namespace winecho {
	StreamInstance::StreamInstance(std::shared_ptr<MyMMDevice> sourceDevice, std::shared_ptr<MyMMDevice> targetDevice, size_t bufferSize, unsigned int desiredLatencyMs) : buffer(bufferSize)
	{
		desiredLatency = desiredLatencyMs;
		captureDevice = sourceDevice;
		renderDevice = targetDevice;
		InitializeSynchronizationBarrier(&setupBarrier, 2, -1);
		error_mutex = CreateMutex(NULL, FALSE, NULL);
		running = false;
	}

	StreamInstance::~StreamInstance()
	{
		if (running) {
			Stop(true);
		}
		CloseHandle(error_mutex);
	}

	DWORD __stdcall StreamInstance::captureThread(LPVOID params) {
		auto* id = ((StreamInstance*)params);

		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr)) {
			id->running = false;
			id->pushError(WINECHO_STREAMINSTANCE_ERROR_COM_INIT_FAILURE);
			return 1;
		}

		id->runCapture();

		CoUninitialize();
		return 0;
	}

	DWORD __stdcall StreamInstance::renderThread(LPVOID params) {
		auto* id = ((StreamInstance*)params);

		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr)) {
			id->running = false;
			id->pushError(WINECHO_STREAMINSTANCE_ERROR_COM_INIT_FAILURE);
			return 1;
		}

		id->runRender();

		CoUninitialize();
		return 0;
	}

	bool StreamInstance::Start()
	{
		if(running)
			return true;
		
		if (captureDevice == nullptr) {
			pushError(WINECHO_STREAMINSTANCE_ERROR_NO_CAPTURE_DEVICE);
			return false;
		}

		if (renderDevice == nullptr) {
			pushError(WINECHO_STREAMINSTANCE_ERROR_NO_RENDER_DEVICE);
			return false;
		}

		running = true;
		threads[0] = CreateThread(NULL, NULL, captureThread, (LPVOID)this, NULL, NULL);
		threads[1] = CreateThread(NULL, NULL, renderThread, (LPVOID)this, NULL, NULL);
		return true;
	}

	void StreamInstance::Stop(bool wait)
	{
		if (!running)
			return;

		running = false;
		DeleteSynchronizationBarrier(&setupBarrier);

		if (wait) {
			WaitForMultipleObjects(2, threads, true, INFINITE);
		}

	}

	size_t StreamInstance::CalcBufferSize(WAVEFORMATEX* format, UINT32 frameAmount) {
		return (size_t)format->nChannels * ((size_t)format->wBitsPerSample / 8) * (size_t)frameAmount;
	}

	bool StreamInstance::CheckFormats(WAVEFORMATEX* source, WAVEFORMATEX* target) {
		return source->nChannels == target->nChannels &&
			source->nSamplesPerSec == target->nSamplesPerSec &&
			source->wBitsPerSample == target->wBitsPerSample &&
			source->nBlockAlign == target->nBlockAlign;
	}

	void StreamInstance::runCapture() {
		std::shared_ptr<winecho::MyMMDevice> device = captureDevice;

		auto sourceClient = device->Activate();
		if (sourceClient == nullptr) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_ACTIVATION_FAILED);
			return;
		}

		captureFormat = sourceClient->MixFormat();

		if (!running) {
			return;
		}

		EnterSynchronizationBarrier(&setupBarrier, NULL);
		if (!CheckFormats(captureFormat, renderFormat)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_FORMAT_MATCH_FAILED);
			return;
		}

		HRESULT hr = sourceClient->Initialize(desiredLatency * MyAudioClient::REFTIMES_PER_MILLISEC, true, true);
		if (FAILED(hr)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_INIT_FAILED);
			return;
		}

		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		if (eventHandle == NULL) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_EVENT_FAILED);
			CoUninitialize();
			return;
		}

		hr = sourceClient->SetEventHandle(eventHandle);
		if (FAILED(hr)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_SETEVENT_FAILED);
			CoUninitialize();
			return;
		}

		UINT32 sourceBufferSize = sourceClient->GetBufferSize();

		auto sourceCapture = sourceClient->GetCaptureClientService();
		if (sourceCapture == nullptr) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETCLIENT_FAILED);
			return;
		}

		if (!running) {
			return;
		}

		hr = sourceClient->Start();
		if (FAILED(hr)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_CLIENT_START_FAILED);
			return;
		}

		BYTE* sourceBuffer = nullptr;
		UINT32 sourceFramesAvailable = 0;
		DWORD sourceFlags = 0;

		while (running) {
			DWORD waitRes;
			waitRes = WaitForSingleObject(eventHandle, 50);

			UINT32 nextSourcePacketSize;

			hr = sourceCapture->GetNextPacketSize(&nextSourcePacketSize);
			if (FAILED(hr)) {
				running = false;
				pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETPACKETSIZE_FAILED);
				return;
			}

			if (nextSourcePacketSize > 0) {
				hr = sourceCapture->GetBuffer(&sourceFramesAvailable, &sourceBuffer, &sourceFlags);
				if (FAILED(hr)) {
					running = false;
					pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETBUFFER_FAILED);
					return;
				}

				size_t bufferSize = CalcBufferSize(captureFormat, sourceFramesAvailable);

				if (sourceFlags & AUDCLNT_BUFFERFLAGS_SILENT) {
					while (!buffer.WriteValue(127, bufferSize, 10)) {
						if (!running)
							break;
					}
				}
				else {
					while (!buffer.Write(sourceBuffer, bufferSize, 10)) {
						if (!running)
							break;
					}
				}

				hr = sourceCapture->ReleaseBuffer(sourceFramesAvailable);
				if (FAILED(hr)) {
					running = false;
					pushError(WINECHO_STREAMINSTANCE_ERROR_CAPTURE_RELEASEBUFFER_FAILED);
					return;
				}
			}
		}

		hr = sourceClient->Stop();
	}

	void StreamInstance::runRender() {
		std::shared_ptr<winecho::MyMMDevice> device = renderDevice;

		auto targetClient = device->Activate();
		if (targetClient == nullptr) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_NO_RENDER_DEVICE);
			return;
		}

		renderFormat = targetClient->MixFormat();

		if (!running) {
			return;
		}

		EnterSynchronizationBarrier(&setupBarrier, NULL);
		if (!CheckFormats(captureFormat, renderFormat)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_FORMAT_MATCH_FAILED);
			return;
		}

		HRESULT hr = targetClient->Initialize(desiredLatency * MyAudioClient::REFTIMES_PER_MILLISEC, false, true);
		if (FAILED(hr)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_INIT_FAILED);
			return;
		}

		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		if (eventHandle == NULL) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_EVENT_FAILED);
			return;
		}

		hr = targetClient->SetEventHandle(eventHandle);
		if (FAILED(hr)) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_SETEVENT_FAILED);
			return;
		}

		UINT32 targetBufferSize = targetClient->GetBufferSize();

		auto targetRender = targetClient->GetRenderClientService();
		if (targetRender == nullptr) {
			running = false;
			pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_GETCLIENT_FAILED);
			return;
		}


		bool targetStarted = false;
		BYTE* targetBuffer = nullptr;

		if (!running) {
			return;
		}

		if (!targetStarted) {
			hr = targetClient->Start();
			if (FAILED(hr)) {
				running = false;
				pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_CLIENT_START_FAILED);
				return;
			}
			targetStarted = true;
		}

		while (running) {
			DWORD waitRes;
			do {
				waitRes = WaitForSingleObject(eventHandle, INFINITE);
				if (!running)
					break;
			} while (waitRes != WAIT_OBJECT_0);

			UINT32 availableFrames = targetBufferSize - targetClient->GetCurrentPadding();

			size_t bufferSize = CalcBufferSize(renderFormat, availableFrames);

			if (availableFrames > 0) {
				hr = targetRender->GetBuffer(availableFrames, &targetBuffer);
				if (FAILED(hr) || targetBuffer == nullptr) {
					running = false;
					pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_GETBUFFER_FAILED);
					return;
				}

				while (!buffer.Read(targetBuffer, bufferSize, 10)) {
					if (!running) {
						break;
					}
				}

				hr = targetRender->ReleaseBuffer(availableFrames, 0);
				if (FAILED(hr)) {
					running = false;
					pushError(WINECHO_STREAMINSTANCE_ERROR_RENDER_RELEASEBUFFER_FAILED);
					return;
				}
			}
		}

		if (targetStarted) {
			hr = targetClient->Stop();
		}
	}

	void StreamInstance::pushError(DWORD error) {
		if (error == WINECHO_STREAMINSTANCE_OK)
			return;

		WaitForSingleObject(error_mutex, INFINITE);

		errors.push(error);

		ReleaseMutex(error_mutex);
	}

	DWORD StreamInstance::GetError() {
		WaitForSingleObject(error_mutex, INFINITE);

		DWORD error;
		if (errors.size() == 0) {
			error = WINECHO_STREAMINSTANCE_OK;
		}
		else {
			error = errors.top();
			errors.pop();
		}
		ReleaseMutex(error_mutex);

		return error;
	}
}


extern "C" {

	void* genStreamInstance(void* sourceDevice, void* targetDevice, size_t bufferSize, unsigned int desiredLatencyMs)
	{
		if (sourceDevice == nullptr || targetDevice == nullptr) {
			return nullptr;
		}

		auto source = *getHandle<winecho::MyMMDevice>(sourceDevice);
		auto target = *getHandle<winecho::MyMMDevice>(targetDevice);

		return genHandle<winecho::StreamInstance>(new winecho::StreamInstance(std::move(source), std::move(target), bufferSize, desiredLatencyMs));
	}

	void releaseStreamInstance(void* streamInstance)
	{
		if (streamInstance == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::StreamInstance>(streamInstance);
		delete handle;
	}

	bool streamInstanceIsRunning(void* streamInstance)
	{
		if (streamInstance == nullptr)
			return false;

		auto* handle = getHandle<winecho::StreamInstance>(streamInstance);
		return (*handle)->IsRunning();
	}
	
	bool streamInstanceStart(void* streamInstance)
	{
		if (streamInstance == nullptr) {
			return false;
		}

		auto* handle = getHandle<winecho::StreamInstance>(streamInstance);
		return (*handle)->Start();
	}

	void streamInstanceStop(void* streamInstance, bool wait)
	{
		if (streamInstance == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::StreamInstance>(streamInstance);
		return (*handle)->Stop(wait);
	}

	DWORD streamInstanceGetError(void* streamInstance) {
		if(streamInstance == nullptr) {
			return -1;
		}

		auto* handle = getHandle<winecho::StreamInstance>(streamInstance);
		return (*handle)->GetError();
	}
}