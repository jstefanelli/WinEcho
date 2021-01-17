#include "StreamInstance.h"
#include "CApiUtils.h"

namespace winecho {
	StreamInstance::StreamInstance(std::shared_ptr<MyMMDevice> sourceDevice, std::shared_ptr<MyMMDevice> targetDevice, size_t bufferSize, unsigned int desiredLatencyMs) : buffer(bufferSize)
	{
		desiredLatency = desiredLatencyMs;
		captureDevice = sourceDevice;
		renderDevice = targetDevice;
		InitializeSynchronizationBarrier(&setupBarrier, 2, -1);
		running = false;
	}

	StreamInstance::~StreamInstance()
	{
		if (running) {
			Stop(true);
		}
	}

	DWORD __stdcall StreamInstance::captureThread(LPVOID params) {
		auto* id = ((StreamInstance*)params);

		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr)) {
			id->running = false;
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
		
		if(captureDevice == nullptr || renderDevice == nullptr)
			return false;

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
			return;
		}

		captureFormat = sourceClient->MixFormat();

		if (!running) {
			return;
		}

		EnterSynchronizationBarrier(&setupBarrier, NULL);
		if (!CheckFormats(captureFormat, renderFormat)) {
			running = false;
			return;
		}

		HRESULT hr = sourceClient->Initialize(desiredLatency * MyAudioClient::REFTIMES_PER_MILLISEC, true, true);
		if (FAILED(hr)) {
			running = false;
			return;
		}

		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		if (eventHandle == NULL) {
			running = false;
			CoUninitialize();
			return;
		}

		hr = sourceClient->SetEventHandle(eventHandle);
		if (FAILED(hr)) {
			running = false;
			CoUninitialize();
			return;
		}

		UINT32 sourceBufferSize = sourceClient->GetBufferSize();

		auto sourceCapture = sourceClient->GetCaptureClientService();
		if (sourceCapture == nullptr) {
			running = false;
			return;
		}

		if (!running) {
			return;
		}

		hr = sourceClient->Start();
		if (FAILED(hr)) {
			running = false;
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
				return;
			}

			if (nextSourcePacketSize > 0) {
				hr = sourceCapture->GetBuffer(&sourceFramesAvailable, &sourceBuffer, &sourceFlags);
				if (FAILED(hr)) {
					running = false;
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
			return;
		}

		renderFormat = targetClient->MixFormat();

		if (!running) {
			return;
		}

		EnterSynchronizationBarrier(&setupBarrier, NULL);
		if (!CheckFormats(captureFormat, renderFormat)) {
			running = false;
			return;
		}

		HRESULT hr = targetClient->Initialize(desiredLatency * MyAudioClient::REFTIMES_PER_MILLISEC, false, true);
		if (FAILED(hr)) {
			running = false;
			return;
		}

		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		if (eventHandle == NULL) {
			running = false;
			return;
		}

		hr = targetClient->SetEventHandle(eventHandle);
		if (FAILED(hr)) {
			running = false;
			return;
		}

		UINT32 targetBufferSize = targetClient->GetBufferSize();

		auto targetRender = targetClient->GetRenderClientService();
		if (targetRender == nullptr) {
			running = false;
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
					return;
				}
			}
		}

		if (targetStarted) {
			hr = targetClient->Stop();
		}
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
}