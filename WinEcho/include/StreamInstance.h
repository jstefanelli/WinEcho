#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include "MyMMDevice.h"
#include "RollingBuffer.h"
#include "WinEchoApi.h"

#ifdef __cplusplus

namespace winecho {
	class WINECHO_API StreamInstance {
	protected:
		HANDLE threads[2];
		SYNCHRONIZATION_BARRIER setupBarrier;
		std::shared_ptr<MyMMDevice> captureDevice;
		std::shared_ptr<MyMMDevice> renderDevice;
		winecho::RollingBuffer buffer;
		WAVEFORMATEX* captureFormat;
		WAVEFORMATEX* renderFormat;
		int desiredLatency;
		bool running;

		static bool CheckFormats(WAVEFORMATEX* source, WAVEFORMATEX* target);
		static size_t CalcBufferSize(WAVEFORMATEX* format, UINT32 availableFrames);
		static DWORD __stdcall captureThread(LPVOID params);
		static DWORD __stdcall renderThread(LPVOID params);
		void runCapture();
		void runRender();

	public:
		StreamInstance(std::shared_ptr<MyMMDevice> sourceDevice, std::shared_ptr<MyMMDevice> targetDevice, size_t bufferSize, unsigned int desiredLatencyMs);
		~StreamInstance();

		StreamInstance() = delete;
		StreamInstance(const StreamInstance&) = delete;
		StreamInstance& operator=(const StreamInstance&) = delete;

		inline bool IsRunning() const { return running; }
		bool Start();
		void Stop(bool wait);
	};
}

extern "C" {
#endif

extern WINECHO_API void* genStreamInstance(void* sourceDevice, void* targetDevice, size_t bufferSize, unsigned int desiredLatencyMs);
extern WINECHO_API void releaseStreamInstance(void* streamInstance);

extern WINECHO_API bool streamInstanceIsRunning(void* streamInstance);
extern WINECHO_API bool streamInstanceStart(void* streamInstance);
extern WINECHO_API void streamInstanceStop(void* streamInstance, bool wait);

#ifdef __cplusplus
}
#endif