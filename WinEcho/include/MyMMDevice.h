#pragma once

#include <limits>
#include <string>
#include <memory>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmdeviceapi.h>
#include "WinEchoApi.h"
#include "MyAudioClient.h"

#ifdef __cplusplus

namespace winecho {
	class WINECHO_API MyMMDevice {
		protected:
			IMMDevice* device;
			std::wstring description;
			std::wstring name;
			std::wstring id;
			
		public:
			MyMMDevice(IMMDevice* device);
			MyMMDevice() = delete;
			MyMMDevice(const MyMMDevice&) = delete;
			MyMMDevice& operator=(const MyMMDevice&) = delete;

			~MyMMDevice();

			std::wstring Id() const;
			std::wstring Name() const;
			std::wstring Description() const;
			IMMDevice* Device() const;
			std::shared_ptr<MyAudioClient> Activate();
	};
}

extern "C" {
#endif
	extern WINECHO_API void releaseDeviceInstance(void* device);

	extern WINECHO_API void deviceId(void* device, const wchar_t* idStr, unsigned int* length);
	extern WINECHO_API void deivceName(void* device, const wchar_t* nameStr, unsigned int* length);
	extern WINECHO_API void deviceDescription(void* device, const wchar_t* descStr, unsigned int* length);

#ifdef __cplusplus
}
#endif