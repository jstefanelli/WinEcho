#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include "MyMMDevice.h"
#include "RollingBuffer.h"
#include "WinEchoApi.h"
#include <stack>

#define WINECHO_STREAMINSTANCE_OK 0
#define WINECHO_STREAMINSTANCE_ERROR_COM_INIT_FAILURE 1
#define WINECHO_STREAMINSTANCE_ERROR_NO_CAPTURE_DEVICE 2
#define WINECHO_STREAMINSTANCE_ERROR_NO_RENDER_DEVICE 3
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_ACTIVATION_FAILED 4
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_ACTIVATION_FAILED 5
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_FORMAT_MATCH_FAILED 6
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_FORMAT_MATCH_FAILED 7
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_INIT_FAILED 8
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_INIT_FAILED 9
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_EVENT_FAILED 10
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_EVENT_FAILED 11
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_SETEVENT_FAILED 12
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_SETEVENT_FAILED 13
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETCLIENT_FAILED 14
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_GETCLIENT_FAILED 15
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_CLIENT_START_FAILED 16
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_CLIENT_START_FAILED 17
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETPACKETSIZE_FAILED 18
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_GETBUFFER_FAILED 19
#define WINECHO_STREAMINSTANCE_ERROR_CAPTURE_RELEASEBUFFER_FAILED 20
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_GETBUFFER_FAILED 21
#define WINECHO_STREAMINSTANCE_ERROR_RENDER_RELEASEBUFFER_FAILED 22

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
		std::stack<DWORD> errors;
		HANDLE error_mutex;

		static bool CheckFormats(WAVEFORMATEX* source, WAVEFORMATEX* target);
		static size_t CalcBufferSize(WAVEFORMATEX* format, UINT32 availableFrames);
		static DWORD __stdcall captureThread(LPVOID params);
		static DWORD __stdcall renderThread(LPVOID params);
		void runCapture();
		void runRender();
		void pushError(DWORD error);

	public:
		StreamInstance(std::shared_ptr<MyMMDevice> sourceDevice, std::shared_ptr<MyMMDevice> targetDevice, size_t bufferSize, unsigned int desiredLatencyMs);
		~StreamInstance();

		StreamInstance() = delete;
		StreamInstance(const StreamInstance&) = delete;
		StreamInstance& operator=(const StreamInstance&) = delete;

		inline bool IsRunning() const { return running; }
		bool Start();
		void Stop(bool wait);
		DWORD GetError();
	};
}

extern "C" {
#endif

extern WINECHO_API void* genStreamInstance(void* sourceDevice, void* targetDevice, size_t bufferSize, unsigned int desiredLatencyMs);
extern WINECHO_API void releaseStreamInstance(void* streamInstance);

extern WINECHO_API bool streamInstanceIsRunning(void* streamInstance);
extern WINECHO_API bool streamInstanceStart(void* streamInstance);
extern WINECHO_API void streamInstanceStop(void* streamInstance, bool wait);
extern WINECHO_API DWORD streamInstanceGetError(void* streamInstance);

#ifdef __cplusplus
}
#endif