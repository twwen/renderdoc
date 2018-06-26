/******************************************************************************
 * The MIT License (MIT)
 *
 * * Copyright (c) 2015-2018 Baldur Karlsson
 * Copyright (c) 2014 Crytek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "driver/d3d11/d3d11_device.h"
#include "hooks/hooks.h"

#include "driver/dx/official/d3d9.h"
#include "d3d9_device.h"

typedef int(WINAPI *PFN_BEGIN_EVENT)(DWORD, WCHAR *);
typedef int(WINAPI *PFN_END_EVENT)();
typedef int(WINAPI *PFN_SET_MARKER_EVENT)(DWORD, WCHAR *);
typedef void(WINAPI *PFN_SET_OPTIONS)(DWORD);
typedef DWORD(WINAPI *PFN_GET_OPTIONS)();

typedef IDirect3D9 *(WINAPI *PFN_D3D9_CREATE)(UINT);

class D3D9Hook : LibraryHook
{
public:
  bool CreateHooks(const char *libName)
  {
    PERF_BeginEvent.Initialize("D3DPERF_BeginEvent", "d3d9.dll", PERF_BeginEvent_hook);
    PERF_EndEvent.Initialize("D3DPERF_EndEvent", "d3d9.dll", PERF_EndEvent_hook);
    PERF_SetMarker.Initialize("D3DPERF_SetMarker", "d3d9.dll", PERF_SetMarker_hook);
    PERF_SetOptions.Initialize("D3DPERF_SetOptions", "d3d9.dll", PERF_SetOptions_hook);
    PERF_GetStatus.Initialize("D3DPERF_GetStatus", "d3d9.dll", PERF_GetStatus_hook);

    Create9.Initialize("Direct3DCreate9", "d3d9.dll", Create9_hook);

    return true;
  }

private:
  static D3D9Hook d3d9hooks;

  // D3DPERF api
  Hook<PFN_BEGIN_EVENT> PERF_BeginEvent;
  Hook<PFN_END_EVENT> PERF_EndEvent;
  Hook<PFN_SET_MARKER_EVENT> PERF_SetMarker;
  Hook<PFN_SET_OPTIONS> PERF_SetOptions;
  Hook<PFN_GET_OPTIONS> PERF_GetStatus;
  Hook<PFN_D3D9_CREATE> Create9;

  static int WINAPI PERF_BeginEvent_hook(DWORD col, WCHAR *wszName)
  {
    int ret = WrappedID3D11Device::BeginEvent((uint32_t)col, wszName);

    d3d9hooks.PERF_BeginEvent()(col, wszName);

    return ret;
  }

  static int WINAPI PERF_EndEvent_hook()
  {
    int ret = WrappedID3D11Device::EndEvent();

    d3d9hooks.PERF_EndEvent()();

    return ret;
  }

  static void WINAPI PERF_SetMarker_hook(DWORD col, WCHAR *wszName)
  {
    WrappedID3D11Device::SetMarker((uint32_t)col, wszName);

    d3d9hooks.PERF_SetMarker()(col, wszName);
  }

  static void WINAPI PERF_SetOptions_hook(DWORD dwOptions)
  {
    if(dwOptions & 1)
      RDCLOG("Application requested not to be hooked via D3DPERF_SetOptions: no longer supported.");
  }

  static DWORD WINAPI PERF_GetStatus_hook() { return 1; }
  static IDirect3D9 *WINAPI Create9_hook(UINT SDKVersion)
  {
    RDCLOG("App creating d3d9 %x", SDKVersion);

    IDirect3D9 *realD3D = d3d9hooks.Create9()(SDKVersion);

    return new WrappedD3D9(realD3D);
  }
};

D3D9Hook D3D9Hook::d3d9hooks;
