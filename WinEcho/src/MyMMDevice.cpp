#include "MyMMDevice.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <exception>
#include <stdexcept>
#include "CApiUtils.h"

const IID IID_IAudioClient = __uuidof(IAudioClient);

namespace winecho {
	MyMMDevice::MyMMDevice(IMMDevice* dev) : device(dev) {
		IPropertyStore* prop = nullptr;

		LPWSTR id;
		HRESULT hr = dev->GetId(&id);
		if (FAILED(hr) || id == nullptr) {
			throw new std::runtime_error("IMMDevice::GetId() failed");
		}

		this->id = id;

		hr = dev->OpenPropertyStore(STGM_READ, &prop);

		if (FAILED(hr) || prop == nullptr) {
			throw std::runtime_error("IMMDevice::OpenPropertyStore() failed");
		}

		PROPVARIANT propVariant;

		hr = prop->GetValue(PKEY_DeviceInterface_FriendlyName, &propVariant);
		if (FAILED(hr)) {
			prop->Release();
			throw std::runtime_error("IPropertyStore::GetValue() failed");
		}

		name = std::wstring(propVariant.pwszVal);

		hr = prop->GetValue(PKEY_Device_DeviceDesc, &propVariant);
		if (FAILED(hr)) {
			prop->Release();
			throw std::runtime_error("IPropertyStore::GetValue() failed");
		}

		description = std::wstring(propVariant.pwszVal);

		hr = prop->Release();
		prop = nullptr;
		if (FAILED(hr)) {
			throw std::runtime_error("IPropertyStore::Release() failed");
		}
	}

	MyMMDevice::~MyMMDevice() {
		device->Release();
		device = nullptr;
	}

	std::wstring MyMMDevice::Id() const {
		return id;
	}

	std::wstring MyMMDevice::Name() const {
		return name;
	}

	std::wstring MyMMDevice::Description() const {
		return description;
	}

	IMMDevice* MyMMDevice::Device() const {
		return device;
	}

	std::shared_ptr<MyAudioClient> MyMMDevice::Activate() {
		IAudioClient* client = nullptr;
		HRESULT hr = device->Activate(IID_IAudioClient, CLSCTX_INPROC_SERVER, nullptr, (void**)&client);
		if (SUCCEEDED(hr) && client != nullptr) {
			return std::make_shared<MyAudioClient>(client);
		}
		return nullptr;
	}
}

extern "C" {
	void releaseDeviceInstance(void* device) {
		if (device == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::MyMMDevice>(device);
		delete handle;
	}

	void deviceId(void* device, const wchar_t* idStr, unsigned int* length) {
		if (device == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::MyMMDevice>(device);

		if (idStr == nullptr) {
			*length = (*handle)->Id().length() + 1;
			return;
		}

		auto id = (*handle)->Id();
		if (*length < id.length() + 1) {
			return;
		}

		memcpy_s((void*)idStr, *length * sizeof(wchar_t), id.c_str(), (id.length() + 1) * sizeof(wchar_t));
	}

	void deivceName(void* device, const wchar_t* nameStr, unsigned int* length) {
		if (device == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::MyMMDevice>(device);

		if (nameStr == nullptr) {
			*length = (*handle)->Name().length() + 1;
			return;
		}

		auto name = (*handle)->Name();
		if (*length < name.length() + 1) {
			return;
		}

		memcpy_s((void*)nameStr, *length * sizeof(wchar_t), name.c_str(), (name.length() + 1) * sizeof(wchar_t));
	}

	void deviceDescription(void* device, const wchar_t* descStr, unsigned int* length) {
		if (device == nullptr) {
			return;
		}

		auto* handle = getHandle<winecho::MyMMDevice>(device);

		if (descStr == nullptr) {
			*length = (*handle)->Description().length() + 1;
			return;
		}

		auto desc = (*handle)->Description();
		if (*length < desc.length() + 1) {
			return;
		}

		memcpy_s((void*)descStr, *length * sizeof(wchar_t), desc.c_str(), (desc.length() + 1) * sizeof(wchar_t));
	}

}