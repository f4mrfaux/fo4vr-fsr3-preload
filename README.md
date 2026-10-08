# FSR3Preload for Fallout 4 VR

A tiny F4SEVR plugin that makes FSR upscaling work in Fallout 4 VR on AMD (and any non-DLSS) GPUs when
[Fallout4 Upscaler VR](https://www.nexusmods.com/fallout4/mods/73715) 1.0.3 is paired with
[Upscaler Base Plugin](https://www.nexusmods.com/site/mods/502) 1.2.0.

Without it, selecting FSR crashes the game at start-up. With it, FSR 3 upscaling initialises and runs.

## The problem

- Fallout4 Upscaler VR was last updated in August 2023. On load it pre-loads a fixed list of dependency DLLs from
  `Data\UpscalerBasePlugin\` (the FSR 2 era names: `ffx_fsr2_api_x64.dll`, `ffx_fsr2_api_dx12_x64.dll`, ...).
- Upscaler Base Plugin 1.2.0 (December 2024) was reworked for the Skyrim VR upscaler. Its FSR path now needs
  `ffx_backend_dx11_x64.dll`, `ffx_fsr3upscaler_x64.dll` and `ffx_fsr3_x64.dll`, and asks Windows for them by bare name.
- Nothing pre-loads those three, and `Data\UpscalerBasePlugin\` is not on the DLL search path, so the loads fail.
  `Fallout4Upscaler.log` shows:

  ```
  [info] Unable to load ffx_backend_dx11_x64.dll
  [info] Unable to load ffx_fsr3_x64.dll
  [info] FSR3Interface Inited!
  [info] Creating UpscaleMethod_FSR3_DX11
  ```

  The plugin carries on regardless and calls a null function pointer. Buffout reports
  `EXCEPTION_ACCESS_VIOLATION ... Tried to execute memory at 0x000000000000` with `PDPerfPlugin.dll` on the stack.
- DLSS is unaffected because `nvngx_dlss.dll` is on the old pre-load list, which is why this only bites FSR users.

The Skyrim VR upscaler does not have this problem: its loader
([`src/XSEPlugin.cpp`](https://github.com/PureDark/Skyrim-Upscaler/blob/VR/src/XSEPlugin.cpp), VR branch) pre-loads exactly
these three DLLs by full path before `PDPerfPlugin.dll`. The Fallout 4 VR plugin never received that change.

## The fix

`FSR3Preload.dll` does the missing step. When F4SEVR loads it, it loads the three FSR 3 DLLs by full path from
`Data\UpscalerBasePlugin\`. When the base plugin later asks for them by name, Windows returns the already-loaded modules.

It patches nothing, hooks nothing, and touches no game memory. Plugin load order does not matter, because the base plugin
only asks for the DLLs later, when the D3D11 device is set up.

## Requirements

- Fallout 4 VR 1.2.72 with F4SEVR (uses the classic `F4SEPlugin_Query` / `F4SEPlugin_Load` interface; tested on 0.6.21)
- Fallout4 Upscaler VR 1.0.3 and its own requirements
- Upscaler Base Plugin 1.2.0, with these files in `Data\UpscalerBasePlugin\`:
  `PDPerfPlugin.dll`, `ffx_backend_dx11_x64.dll`, `ffx_fsr3upscaler_x64.dll`, `ffx_fsr3_x64.dll`

This repository does not redistribute the upscaler, the base plugin or the FidelityFX DLLs.

## Install

1. Put `FSR3Preload.dll` in `Data\F4SE\Plugins\` (as a mod in Mod Organizer 2 or Vortex).
2. In `Data\F4SE\Plugins\Fallout4Upscaler.ini` set:

   ```ini
   mEnableUpscaler = true
   mUpscaleType = 1
   mEnableFixedFoveatedRendering = false
   ```

   `mUpscaleType = 1` is labelled "FSR2" in the ini; with base plugin 1.2.0 it selects the FSR 3 DX11 upscaler.
   Fixed foveated rendering in this plugin uses NVIDIA Variable Rate Shading and does nothing on other GPUs.
3. Launch through F4SEVR.

### GingasVR "Fallout VR Essentials Overhaul" users

Copy the list's `DLSS - Increases Performance - Nvidia GPU` mod to a new mod, add `FSR3Preload.dll` to its
`F4SE\Plugins\` folder, make the ini changes above, enable it, and leave the DLSS and DLAA options disabled.

## Verify

`Documents\My Games\Fallout4VR\F4SE\FSR3Preload.log` should read:

```
FSR3Preload v1.0.0
Loaded ffx_backend_dx11_x64.dll at 0x...
Loaded ffx_fsr3upscaler_x64.dll at 0x...
Loaded ffx_fsr3_x64.dll at 0x...
```

`Fallout4Upscaler.log` should no longer contain the two "Unable to load" lines above and should contain
`ffxFsr3ContextCreate Success!`. (`Unable to load ffx_backend_dx12_x64.dll` is expected and harmless.)

A `FAILED ... (error 126)` line in `FSR3Preload.log` means that DLL is missing from `Data\UpscalerBasePlugin\`.

## Tested

One machine so far:

| | |
|---|---|
| GPU | AMD Radeon RX 7900 XTX |
| Headset | Valve Steam Frame (SteamVR 2.18.2 beta) |
| Game | Fallout 4 VR 1.2.72, F4SEVR 0.6.21 |
| Mod list | Fallout VR Essentials Overhaul 20.1 (HD profile) |
| Result | FSR 3 at Quality (3542x1771 rendered, 5288x2644 output across both eyes); no crash |

Not yet tested: other GPUs (Intel, older AMD), other headsets, other mod lists. Reports welcome.

## Build

From an *x64 Native Tools Command Prompt for VS 2022*:

```bat
build.bat
```

This produces `build\FSR3Preload.dll` (static CRT; imports only `kernel32`, `shell32`, `ole32`) and `build\harness.exe`.

`test\harness.c` reproduces the mechanism outside the game. Create a folder containing `harness.exe`,
`Data\F4SE\Plugins\FSR3Preload.dll` and `Data\UpscalerBasePlugin\` with the three FSR 3 DLLs, then run from that folder:

```bat
harness.exe none
harness.exe Data\F4SE\Plugins\FSR3Preload.dll
```

The first run fails to find the DLLs by bare name (the bug); the second finds all three (the fix).

## Credits

- PureDark for Fallout4 Upscaler VR, Upscaler Base Plugin and the open Skyrim VR upscaler source that shows the intended load sequence.
- AMD for FidelityFX Super Resolution.

## Licence

MIT. See [LICENSE](LICENSE).
