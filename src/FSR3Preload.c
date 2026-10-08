// FSR3Preload - F4SEVR plugin
//
// Fallout4Upscaler.dll (2023) pre-loads only FSR2-era DLLs from Data\UpscalerBasePlugin.
// PDPerfPlugin.dll 1.2.0 (Dec 2024) asks for the FSR3 DLLs by bare name, which Windows cannot
// find there, and then calls into a null pointer. PureDark's Skyrim VR upscaler pre-loads them
// by full path (src/XSEPlugin.cpp); this plugin does the same for Fallout 4 VR.
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>

typedef struct { UINT32 infoVersion; const char* name; UINT32 version; } PluginInfo;

static HMODULE g_self = NULL;
static FILE*   g_log  = NULL;

static const wchar_t* kDlls[] = {
    L"ffx_backend_dx11_x64.dll",
    L"ffx_fsr3upscaler_x64.dll",
    L"ffx_fsr3_x64.dll",
};

static void OpenLog(void)
{
    PWSTR docs = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &docs))) {
        wchar_t path[MAX_PATH * 2];
        swprintf(path, MAX_PATH * 2, L"%ls\\My Games\\Fallout4VR\\F4SE\\FSR3Preload.log", docs);
        g_log = _wfopen(path, L"w");
        CoTaskMemFree(docs);
    }
}

static void Log(const wchar_t* fmt, ...)
{
    if (!g_log) return;
    va_list a; va_start(a, fmt); vfwprintf(g_log, fmt, a); va_end(a);
    fputwc(L'\n', g_log); fflush(g_log);
}

// ...\Data\F4SE\Plugins\FSR3Preload.dll -> ...\Data\UpscalerBasePlugin\<name>
static BOOL LibraryPath(const wchar_t* name, wchar_t* out, size_t cch)
{
    wchar_t self[MAX_PATH * 2];
    if (!GetModuleFileNameW(g_self, self, MAX_PATH * 2)) return FALSE;
    for (int up = 0; up < 3; ++up) {
        wchar_t* slash = wcsrchr(self, L'\\');
        if (!slash) return FALSE;
        *slash = 0;
    }
    return swprintf(out, cch, L"%ls\\UpscalerBasePlugin\\%ls", self, name) > 0;
}

static void Preload(void)
{
    OpenLog();
    Log(L"FSR3Preload v1.0.0");
    for (size_t i = 0; i < sizeof(kDlls) / sizeof(kDlls[0]); ++i) {
        wchar_t path[MAX_PATH * 2];
        if (!LibraryPath(kDlls[i], path, MAX_PATH * 2)) { Log(L"FAILED to build path for %ls", kDlls[i]); continue; }
        HMODULE h = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (h) Log(L"Loaded %ls at 0x%p", kDlls[i], (void*)h);
        else   Log(L"FAILED %ls (error %lu) from %ls", kDlls[i], GetLastError(), path);
    }
    if (g_log) { fclose(g_log); g_log = NULL; }
}

__declspec(dllexport) BOOL F4SEPlugin_Query(const void* f4se, PluginInfo* info)
{
    (void)f4se;
    info->infoVersion = 1;
    info->name = "FSR3Preload";
    info->version = 1;
    return TRUE;
}

__declspec(dllexport) BOOL F4SEPlugin_Load(const void* f4se)
{
    (void)f4se;
    Preload();
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) { g_self = inst; DisableThreadLibraryCalls(inst); }
    return TRUE;
}
