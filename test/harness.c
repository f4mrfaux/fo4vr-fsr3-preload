// Test harness: mimics PDPerfPlugin asking for the FSR3 DLLs by bare name.
// usage: harness.exe <path to FSR3Preload.dll or "none">
#include <windows.h>
#include <stdio.h>
typedef struct { UINT32 infoVersion; const char* name; UINT32 version; } PluginInfo;
typedef BOOL (*QueryFn)(const void*, PluginInfo*);
typedef BOOL (*LoadFn)(const void*);
int wmain(int argc, wchar_t** argv)
{
    if (argc > 1 && wcscmp(argv[1], L"none") != 0) {
        HMODULE p = LoadLibraryW(argv[1]);
        if (!p) { printf("could not load preloader (error %lu)\n", GetLastError()); return 2; }
        QueryFn q = (QueryFn)GetProcAddress(p, "F4SEPlugin_Query");
        LoadFn  l = (LoadFn)GetProcAddress(p, "F4SEPlugin_Load");
        if (!q || !l) { printf("exports missing\n"); return 2; }
        PluginInfo info = {0};
        printf("Query=%d name=%s infoVersion=%u; ", q(NULL, &info), info.name, info.infoVersion);
        printf("Load=%d\n", l(NULL));
    }
    const char* names[] = { "ffx_backend_dx11_x64.dll", "ffx_fsr3_x64.dll", "ffx_fsr3upscaler_x64.dll" };
    int ok = 0;
    for (int i = 0; i < 3; ++i) {
        HMODULE h = LoadLibraryA(names[i]);   // bare name, like PDPerfPlugin
        printf("  bare-name LoadLibrary(%s): %s\n", names[i], h ? "FOUND" : "not found");
        ok += h != NULL;
    }
    return ok == 3 ? 0 : 1;
}
