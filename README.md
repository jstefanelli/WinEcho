# WinEcho
A 0-dependency C++/C windows library for handling WASAPI loopback audio devices

## Overview

WinEcho is a C++ windows library (With an extra C interface) that allows playback of WASAPI loopback render devices on other render devices.

## Usage

First, get a list of devices using the `winecho::MyMMDeviceEnumerator` class. This class accepts a `EDataFlow` parameter, but the library is set up to handle only `EDataFlow::eRender` devices.

When you find your **capture device** (recording device) and **render device** (playback device), you can generate a `winecho::StreamInstance` class.

The parameters are:

- Capture device
- Render devce
- Buffer size (bigger buffer sizes help with cracks in the playback, but add significant latency)
- Desired latency (in milliseconds. This is only a hint and is not guaranteed)

The `winecho::StreamInstance::Start()` method spawns the threads that handle capture and render.

The `winecho::StreamInstance::Stop(bool wait)` methods stops the threads (and with the `wait` parameter you choose wether to wait for the threads to finish).

### Error Handling

Errors from the `winecho::StreamInstance` class can be queried using the `winecho::StreamInstance::GetError()` method.

## TODO

Error handling is basic and only for the `winecho::StreamInstance` class, better handling may be needed

The library only supports devices with the same audio configuration (bit rate, channel number, sample size) in the same stream.

The C interface only implements `winecho::MyMMDeviceEnumerator`, `winecho::MyMMDevice` and `winecho::StreamInstance`.
