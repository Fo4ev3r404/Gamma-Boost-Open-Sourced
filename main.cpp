#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

using namespace Gdiplus;

#define IDI_APP_ICON 101
#define IDR_PREVIEW_IMG 102

struct MAGCOLOREFFECT {
    float transform[5][5];
};

typedef BOOL(WINAPI* PFN_MagInitialize)(void);
typedef BOOL(WINAPI* PFN_MagUninitialize)(void);
typedef BOOL(WINAPI* PFN_MagSetFullscreenColorEffect)(MAGCOLOREFFECT* pEffect);
typedef BOOL(WINAPI* PFN_MagSetFullscreenTransform)(float magLevel, int xOffset, int yOffset);
typedef LONG(NTAPI* PFN_NtSetTimerResolution)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
typedef LONG(NTAPI* PFN_NtQueryTimerResolution)(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution);
typedef LONG(NTAPI* PFN_NtSetSystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength);

static HMODULE g_hMagDll = NULL;
static PFN_MagInitialize fnMagInitialize = NULL;
static PFN_MagUninitialize fnMagUninitialize = NULL;
static PFN_MagSetFullscreenColorEffect fnMagSetFullscreenColorEffect = NULL;
static PFN_MagSetFullscreenTransform fnMagSetFullscreenTransform = NULL;
static bool g_magInitialized = false;
static HHOOK g_hMouseHook = NULL;

static HMODULE g_hNtdll = NULL;
static PFN_NtSetTimerResolution fnNtSetTimerResolution = NULL;
static PFN_NtQueryTimerResolution fnNtQueryTimerResolution = NULL;
static PFN_NtSetSystemInformation fnNtSetSystemInformation = NULL;
static bool g_timerBoostActive = false;

static WORD g_origRamp[3][256];
static bool g_hasOrigRamp = false;
static int g_origMouseParams[3] = { 6, 10, 1 };
static bool g_hasOrigMouse = false;

static const Color COL_BG_MAIN(255, 17, 17, 21);
static const Color COL_BG_SIDEBAR(255, 22, 22, 27);
static const Color COL_BG_PANEL(255, 26, 26, 32);
static const Color COL_BG_CARD(255, 32, 32, 40);
static const Color COL_BG_PILL(255, 36, 36, 45);
static const Color COL_BORDER_SUBTLE(255, 48, 48, 60);

static const Color COL_PINK(255, 255, 183, 197);
static const Color COL_PINK_BRIGHT(255, 255, 205, 216);
static const Color COL_PINK_DEEP(255, 232, 138, 162);
static const Color COL_PINK_GLOW(70, 255, 183, 197);

static const Color COL_TEXT_WHITE(255, 245, 245, 250);
static const Color COL_TEXT_MUTED(255, 152, 154, 168);
static const Color COL_TEXT_DIM(255, 105, 108, 122);

enum ChannelTab {
    TAB_PRESETS = 0,
    TAB_MANUAL = 1,
    TAB_EYECARE = 2,
    TAB_FPS_BOOST = 3,
    TAB_ENHANCEMENTS = 4,
    TAB_PROFILES = 5,
    TAB_DISCORD = 6
};

struct DisplayPreset {
    std::wstring name;
    std::wstring subtitle;
    std::wstring badge;
    int category;
    Color gradA;
    Color gradB;
    Color gradC;
    int iconType;

    double gamma;
    double contrast;
    double blackPoint;
    int shadows;
    int saturation;
    int vibrance;
    int temperatureK;
};

struct FpsChannelCard {
    std::wstring name;
    std::wstring subtitle;
    std::wstring tag;
    Color gradA;
    Color gradB;
    bool enabled;
};

struct GameProfileEntry {
    std::wstring exeName;
    std::wstring exePath;
    int presetIndex;
    double gamma;
    double contrast;
    double blackPoint;
    int shadows;
    int saturation;
    int vibrance;
    int temperatureK;
};

struct MonitorEntry {
    std::wstring deviceName;
    std::wstring friendlyName;
};

static HWND g_hWnd = NULL;
static HICON g_hAppIcon = NULL;
static ULONG_PTR g_gdiplusToken = 0;
static NOTIFYICONDATAW g_nid = {};
static bool g_inTray = false;
static wchar_t g_iniPath[MAX_PATH] = { 0 };

static std::vector<Gdiplus::Bitmap*> g_bgFrames;
static UINT g_bgCurrentFrame = 0;

static ChannelTab g_currentTab = TAB_PRESETS;
static bool g_showOnScreen = true;
static int g_activePresetIdx = 8;

static std::wstring g_activePresetName = L"ColorBoost+";
static double g_gamma = 1.45;
static double g_contrast = 1.15;
static double g_blackPoint = 0.03;
static int g_shadows = 20;
static int g_saturation = 135;
static int g_vibrance = 155;
static int g_temperatureK = 6500;
static int g_bwMode = 0;
static int g_colorblindMode = 0;

static float g_splitPos = 0.48f;
static bool g_draggingSplit = false;
static int g_draggingSlider = -1;
static int g_previewScene = 0;
static int g_inspectorSubTab = 0;
static Bitmap* g_pPreviewRaw = NULL;
static Bitmap* g_pPreviewOrig = NULL;
static Bitmap* g_pPreviewAdj = NULL;
static const int PREVIEW_BMP_W = 800;
static const int PREVIEW_BMP_H = 450;

static std::vector<DisplayPreset> g_presets;
static std::vector<FpsChannelCard> g_fpsCards;
static std::vector<GameProfileEntry> g_gameProfiles;
static int g_selectedGameProfile = 0;
static std::vector<MonitorEntry> g_monitors;
static int g_selectedMonitor = 0;
static size_t g_lastFreedMB = 0;

static bool g_recoilPullEnabled = true;
static double g_recoilPullStrength = 3.5;
static double g_recoilSubPixelAccum = 0.0;
static bool g_recoilRequireADS = false;
static const double RECOIL_STEPS[6] = { 1.0, 2.0, 3.0, 3.5, 4.0, 5.0 };

static bool g_wheelZoomEnabled = true;
static float g_currentZoomLevel = 1.0f;
static int g_wheelZoomMode = 0;
static const float ZOOM_PRESET_STEPS[5] = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };

static HWND g_hOverlayWnd = NULL;
static bool g_aiVisionEnabled = true;
static int g_aiSensitivity = 1;
static bool g_aiShowCrosshair = true;
static const int AI_SCAN_W = 750;
static const int AI_SCAN_H = 500;
static const int AI_GRID_W = 150;
static const int AI_GRID_H = 100;
static float g_prevGridLum[AI_GRID_H][AI_GRID_W] = {};
static bool g_hasPrevGrid = false;

struct DetectedPlayerBox {
    float x, y, w, h;
    float vx, vy;
    int confidence;
    int holdFrames;
    bool isDriver;
    bool active;
};
static DetectedPlayerBox g_trackedPlayers[3] = {};
static bool g_hasCalibratedPlayer = false;
static float g_calibR = 20.0f, g_calibG = 19.0f, g_calibB = 16.0f;
static bool g_calibrateNextTick = false;

struct HitRect {
    int id;
    RectF rc;
};
static std::vector<HitRect> g_hitRects;
static int g_hoverId = -1;
static POINT g_mousePos = { 0, 0 };

enum HitIds {
    HIT_TAB_PRESETS = 10,
    HIT_TAB_MANUAL = 11,
    HIT_TAB_EYECARE = 12,
    HIT_TAB_FPS = 13,
    HIT_TAB_ENHANCEMENTS = 14,
    HIT_TAB_PROFILES = 15,
    HIT_TAB_DISCORD = 16,

    HIT_TOP_POWER = 20,
    HIT_TOP_MONITOR = 21,
    HIT_BTN_NEW_PRESET = 22,
    HIT_TOGGLE_SHOW_SCREEN = 23,
    HIT_BTN_REVERT = 24,
    HIT_SPLIT_HANDLE = 25,

    HIT_SCENE_BASE = 30,
    HIT_INSPECTOR_TAB = 40,
    HIT_SLIDER_BASE = 50,
    HIT_EXTRA_BW = 60,
    HIT_EXTRA_CB = 61,

    HIT_PRESET_CARD_BASE = 100,
    HIT_FPS_CARD_BASE = 200,
    HIT_FPS_FLUSH_RAM = 250,
    HIT_FPS_ALL_TOGGLE = 251,
    HIT_RECOIL_TOGGLE = 260,
    HIT_RECOIL_STR_1 = 261,
    HIT_RECOIL_STR_2 = 262,
    HIT_RECOIL_STR_3 = 263,
    HIT_RECOIL_STR_4 = 264,
    HIT_RECOIL_STR_5 = 265,
    HIT_RECOIL_STR_6 = 266,
    HIT_RECOIL_MODE_ADS = 267,

    HIT_ZOOM_TOGGLE = 270,
    HIT_ZOOM_STEP_1 = 271,
    HIT_ZOOM_STEP_2 = 272,
    HIT_ZOOM_STEP_3 = 273,
    HIT_ZOOM_STEP_4 = 274,
    HIT_ZOOM_STEP_5 = 275,
    HIT_ZOOM_MODE = 276,

    HIT_AI_TOGGLE = 280,
    HIT_AI_SENS_1 = 281,
    HIT_AI_SENS_2 = 282,
    HIT_AI_SENS_3 = 283,
    HIT_AI_CROSSHAIR = 284,

    HIT_PROF_ADD_EXE = 300,
    HIT_PROF_ADD_RUN = 301,
    HIT_PROF_REMOVE = 302,
    HIT_PROF_CARD_BASE = 310,

    HIT_DISCORD_COPY = 400,
    HIT_DISCORD_IMPORT = 401
};

enum {
    WM_TRAYICON = WM_USER + 50,
    ID_TRAY_ICON = 901,
    ID_TRAY_RESTORE = 902,
    ID_TRAY_TOGGLE = 903,
    ID_TRAY_EXIT = 904,
    ID_MENU_RUN_BASE = 1200,
    TIMER_WATCHDOG = 701,
    TIMER_RECOIL = 702,
    TIMER_AI_VISION = 703,
    TIMER_BG_ANIM = 704,
    TIMER_SAVE_CONFIG = 705,
    HK_TOGGLE = 1,
    HK_RESET = 2,
    HK_GAMMA_UP = 3,
    HK_GAMMA_DOWN = 4,
    HK_RECOIL_TOGGLE = 5,
    HK_ZOOM_TOGGLE = 6,
    HK_AI_TOGGLE = 7,
    HK_AI_CALIBRATE = 8
};

struct RunningAppInfo {
    std::wstring exeName;
    std::wstring fullPath;
    std::wstring title;
};
static std::vector<RunningAppInfo> g_runningApps;

void InitDefaultCatalog() {
    g_presets.clear();

    g_presets.push_back({ L"High Contrast", L"Brighter shadows, true colors", L"", 0,
        Color(255, 118, 120, 126), Color(255, 62, 64, 70), Color(255, 30, 31, 35), 0,
        1.50, 1.18, 0.04, 28, 110, 120, 6500 });
    g_presets.push_back({ L"Warm Boost", L"Like High Contrast, but warmer", L"FS", 0,
        Color(255, 168, 116, 62), Color(255, 112, 74, 34), Color(255, 58, 38, 18), 0,
        1.48, 1.16, 0.04, 25, 115, 130, 5600 });
    g_presets.push_back({ L"Cool Boost", L"Like High Contrast, but cooler", L"", 0,
        Color(255, 84, 114, 152), Color(255, 44, 66, 98), Color(255, 22, 34, 54), 0,
        1.48, 1.16, 0.03, 25, 115, 125, 7400 });
    g_presets.push_back({ L"Forest Ops", L"Greens that pop in the dark", L"", 0,
        Color(255, 58, 128, 68), Color(255, 32, 86, 40), Color(255, 16, 46, 22), 0,
        1.55, 1.14, 0.05, 32, 135, 165, 6300 });

    g_presets.push_back({ L"Vivid", L"Bold, saturated color", L"", 1,
        Color(255, 138, 86, 136), Color(255, 76, 68, 112), Color(255, 42, 34, 64), 0,
        1.18, 1.15, 0.01, 10, 145, 165, 6500 });
    g_presets.push_back({ L"Color Boost", L"Full color, lifted shadows", L"", 1,
        Color(255, 168, 68, 108), Color(255, 106, 42, 98), Color(255, 56, 22, 54), 0,
        1.35, 1.16, 0.03, 20, 150, 180, 6400 });

    g_presets.push_back({ L"Everyday", L"Richer color, crisper text", L"", 2,
        Color(255, 46, 118, 142), Color(255, 30, 78, 102), Color(255, 18, 42, 58), 0,
        1.12, 1.08, 0.00, 6, 118, 125, 6500 });
    g_presets.push_back({ L"Cinema", L"Film-like contrast and color", L"", 2,
        Color(255, 164, 92, 68), Color(255, 128, 36, 38), Color(255, 68, 18, 22), 0,
        1.15, 1.24, -0.01, 8, 130, 145, 5900 });

    g_presets.push_back({ L"Tarkov++", L"Brighter shadows, warmer", L"E", 3,
        Color(255, 158, 78, 44), Color(255, 98, 44, 32), Color(255, 36, 28, 34), 1,
        1.65, 1.20, 0.06, 38, 145, 175, 5800 });
    g_presets.push_back({ L"ColorBoost+", L"Much richer color & pink glow", L"", 3,
        Color(255, 158, 92, 198), Color(255, 98, 56, 158), Color(255, 46, 26, 78), 2,
        1.42, 1.18, 0.03, 22, 160, 195, 6400 });
    g_presets.push_back({ L"BabyPink FPS", L"Lifted blacks, vibrant colors", L"P", 3,
        Color(255, 232, 132, 160), Color(255, 156, 76, 112), Color(255, 68, 34, 52), 2,
        1.52, 1.16, 0.04, 28, 155, 185, 6300 });
    g_presets.push_back({ L"Night Ops", L"Much brighter dark corners", L"", 3,
        Color(255, 128, 130, 136), Color(255, 72, 74, 80), Color(255, 34, 36, 40), 1,
        1.75, 1.12, 0.07, 42, 125, 140, 6600 });

    g_presets.push_back({ L"Rose Comfort", L"Soft baby-pink warmth, zero eye strain", L"", 4,
        Color(255, 218, 132, 154), Color(255, 138, 74, 96), Color(255, 64, 34, 46), 3,
        1.05, 0.96, 0.01, 0, 95, 100, 5200 });
    g_presets.push_back({ L"Late Night", L"Warm amber filter for dark rooms", L"", 4,
        Color(255, 178, 106, 52), Color(255, 112, 62, 28), Color(255, 52, 28, 14), 3,
        0.98, 0.92, 0.00, -5, 90, 90, 4400 });
    g_presets.push_back({ L"Studio Read", L"Balanced paper tone for long sessions", L"", 4,
        Color(255, 116, 112, 98), Color(255, 72, 68, 58), Color(255, 38, 36, 32), 3,
        1.02, 0.94, 0.02, 5, 85, 90, 5500 });
    g_presets.push_back({ L"Sleep Shield", L"Deep blue-light reduction before sleep", L"", 4,
        Color(255, 142, 68, 78), Color(255, 86, 38, 48), Color(255, 42, 18, 24), 3,
        0.94, 0.88, 0.00, -10, 80, 80, 3900 });

    g_fpsCards.clear();
    g_fpsCards.push_back({ L"Sub-Tick Timer", L"Locks system timer to 0.5ms for instant frame pacing", L"0.5ms",
        Color(255, 210, 115, 145), Color(255, 78, 38, 58), true });
    g_fpsCards.push_back({ L"Thread Focus", L"Prioritizes active game CPU threads above background apps", L"CPU",
        Color(255, 148, 92, 178), Color(255, 58, 34, 78), true });
    g_fpsCards.push_back({ L"Standby Flow", L"Automatically purges cached standby memory to stop stutters", L"RAM",
        Color(255, 78, 128, 168), Color(255, 32, 54, 76), true });
    g_fpsCards.push_back({ L"Direct Flip", L"Enables DirectX flip-model & bypasses fullscreen DWM lag", L"GPU",
        Color(255, 178, 92, 118), Color(255, 72, 32, 48), true });
    g_fpsCards.push_back({ L"Core Unpark", L"Keeps all processor cores awake & disables EcoQoS throttling", L"CORES",
        Color(255, 68, 138, 112), Color(255, 28, 58, 48), true });
    g_fpsCards.push_back({ L"Background Mute", L"Silences GameDVR capture, telemetry & heavy tray indexers", L"CLEAN",
        Color(255, 132, 102, 142), Color(255, 52, 38, 62), true });
    g_fpsCards.push_back({ L"Raw Aim 1:1", L"Disables Windows pointer acceleration for pure 1:1 mouse aim", L"INPUT",
        Color(255, 188, 112, 92), Color(255, 76, 42, 34), false });
    g_fpsCards.push_back({ L"Packet Stream", L"Disables network multimedia throttling & flushes DNS cache", L"NET",
        Color(255, 86, 112, 164), Color(255, 34, 46, 74), true });
}

bool EqualsIgnoreCase(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

std::wstring GetFileNameFromPath(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return path;
    return path.substr(pos + 1);
}

void EnablePrivilege(LPCWSTR privName) {
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp = {};
        if (LookupPrivilegeValueW(NULL, privName, &tp.Privileges[0].Luid)) {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        }
        CloseHandle(hToken);
    }
}

void InitPaths() {
    GetModuleFileNameW(NULL, g_iniPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(g_iniPath, L'\\');
    if (lastSlash) {
        wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash - g_iniPath + 1), L"GammaBoost.ini");
    }
}

BOOL CALLBACK PopulateMonitorsProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM) {
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, (LPMONITORINFO)&mi)) {
        MonitorEntry entry;
        entry.deviceName = mi.szDevice;
        wchar_t label[128];
        int idx = (int)g_monitors.size() + 1;
        swprintf_s(label, L"Display %d", idx);
        entry.friendlyName = label;
        g_monitors.push_back(entry);
    }
    return TRUE;
}

void InitSystemAPIs() {
    EnablePrivilege(L"SeProfileSingleProcessPrivilege");
    EnablePrivilege(L"SeIncreaseBasePriorityPrivilege");

    g_hMagDll = LoadLibraryW(L"Magnification.dll");
    if (g_hMagDll) {
        fnMagInitialize = reinterpret_cast<PFN_MagInitialize>(
            reinterpret_cast<void*>(GetProcAddress(g_hMagDll, "MagInitialize")));
        fnMagUninitialize = reinterpret_cast<PFN_MagUninitialize>(
            reinterpret_cast<void*>(GetProcAddress(g_hMagDll, "MagUninitialize")));
        fnMagSetFullscreenColorEffect = reinterpret_cast<PFN_MagSetFullscreenColorEffect>(
            reinterpret_cast<void*>(GetProcAddress(g_hMagDll, "MagSetFullscreenColorEffect")));
        fnMagSetFullscreenTransform = reinterpret_cast<PFN_MagSetFullscreenTransform>(
            reinterpret_cast<void*>(GetProcAddress(g_hMagDll, "MagSetFullscreenTransform")));
        if (fnMagInitialize && fnMagSetFullscreenColorEffect) {
            if (fnMagInitialize()) g_magInitialized = true;
        }
    }

    g_hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (g_hNtdll) {
        fnNtSetTimerResolution = reinterpret_cast<PFN_NtSetTimerResolution>(
            reinterpret_cast<void*>(GetProcAddress(g_hNtdll, "NtSetTimerResolution")));
        fnNtQueryTimerResolution = reinterpret_cast<PFN_NtQueryTimerResolution>(
            reinterpret_cast<void*>(GetProcAddress(g_hNtdll, "NtQueryTimerResolution")));
        fnNtSetSystemInformation = reinterpret_cast<PFN_NtSetSystemInformation>(
            reinterpret_cast<void*>(GetProcAddress(g_hNtdll, "NtSetSystemInformation")));
    }

    if (SystemParametersInfoW(SPI_GETMOUSE, 0, g_origMouseParams, 0)) {
        g_hasOrigMouse = true;
    }

    HDC hDC = GetDC(NULL);
    if (hDC) {
        if (GetDeviceGammaRamp(hDC, g_origRamp)) {
            g_hasOrigRamp = true;
        } else {
            for (int i = 0; i < 256; ++i)
                g_origRamp[0][i] = g_origRamp[1][i] = g_origRamp[2][i] = (WORD)(i * 257);
            g_hasOrigRamp = true;
        }
        ReleaseDC(NULL, hDC);
    }

    g_monitors.clear();
    EnumDisplayMonitors(NULL, NULL, PopulateMonitorsProc, 0);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, (LPMONITORINFO)&mi)) {
        if (g_selectedMonitor > 0 && g_selectedMonitor <= (int)g_monitors.size()) {
            if (g_monitors[g_selectedMonitor - 1].deviceName != mi.szDevice) return TRUE;
        }
        HDC hDC = CreateDCW(L"DISPLAY", mi.szDevice, NULL, NULL);
        if (hDC) {
            SetDeviceGammaRamp(hDC, (LPVOID)dwData);
            DeleteDC(hDC);
        }
    }
    return TRUE;
}

void RestoreOriginalDisplay() {
    if (g_hasOrigRamp) {
        int prev = g_selectedMonitor;
        g_selectedMonitor = 0;
        HDC hDC = GetDC(NULL);
        if (hDC) {
            SetDeviceGammaRamp(hDC, g_origRamp);
            ReleaseDC(NULL, hDC);
        }
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)g_origRamp);
        g_selectedMonitor = prev;
    }
    if (g_magInitialized && fnMagSetFullscreenColorEffect) {
        MAGCOLOREFFECT ident = {};
        for (int i = 0; i < 5; ++i) ident.transform[i][i] = 1.0f;
        fnMagSetFullscreenColorEffect(&ident);
    }
    if (g_magInitialized && fnMagSetFullscreenTransform) {
        fnMagSetFullscreenTransform(1.0f, 0, 0);
    }
}

void ApplyEagleEyeZoom() {
    if (!g_magInitialized || !fnMagSetFullscreenTransform) return;
    float z = (g_showOnScreen && g_wheelZoomEnabled) ? g_currentZoomLevel : 1.0f;
    if (z <= 1.01f) {
        fnMagSetFullscreenTransform(1.0f, 0, 0);
    } else {
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        int xOff = (int)std::round(sw * (1.0f - 1.0f / z) * 0.5f);
        int yOff = (int)std::round(sh * (1.0f - 1.0f / z) * 0.5f);
        fnMagSetFullscreenTransform(z, xOff, yOff);
    }
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL && g_wheelZoomEnabled && g_showOnScreen) {
        bool modifierOk = true;
        if (g_wheelZoomMode == 1) {
            modifierOk = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        } else if (g_wheelZoomMode == 2) {
            modifierOk = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
        }
        if (modifierOk) {
            MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            short wheelDelta = (short)HIWORD(pMouse->mouseData);
            if (wheelDelta > 0) {
                g_currentZoomLevel = std::clamp(g_currentZoomLevel + 0.25f, 1.0f, 3.0f);
                ApplyEagleEyeZoom();
                if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
            } else if (wheelDelta < 0) {
                g_currentZoomLevel = std::clamp(g_currentZoomLevel - 0.25f, 1.0f, 3.0f);
                ApplyEagleEyeZoom();
                if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void ShutdownSystemAPIs() {
    if (g_hMouseHook) {
        UnhookWindowsHookEx(g_hMouseHook);
        g_hMouseHook = NULL;
    }
    RestoreOriginalDisplay();
    if (g_timerBoostActive) {
        timeEndPeriod(1);
        if (fnNtSetTimerResolution) {
            ULONG cur = 0;
            fnNtSetTimerResolution(5000, FALSE, &cur);
        }
        g_timerBoostActive = false;
    }
    if (g_hasOrigMouse) {
        SystemParametersInfoW(SPI_SETMOUSE, 0, g_origMouseParams, 0);
    }
    if (g_magInitialized && fnMagUninitialize) {
        fnMagUninitialize();
        g_magInitialized = false;
    }
    if (g_hMagDll) {
        FreeLibrary(g_hMagDll);
        g_hMagDll = NULL;
    }
}

double EvaluateChannelCurve(double normIn, int channel) {

    double shadowLift = (g_shadows / 100.0) * 0.22 * (1.0 - normIn) * std::sqrt(normIn);
    double x = std::clamp(normIn + shadowLift, 0.0, 1.0);

    double val = std::pow(x, 1.0 / std::clamp(g_gamma, 0.40, 3.00));

    val += g_blackPoint;

    val = (val - 0.5) * g_contrast + 0.5;

    double tempShift = (6500.0 - g_temperatureK) / 18000.0;
    if (channel == 0) val *= (1.0 + tempShift);
    else if (channel == 1) val *= (1.0 + tempShift * 0.15);
    else if (channel == 2) val *= (1.0 - tempShift);

    return std::clamp(val, 0.0, 1.0);
}

void UpdatePreviewBitmaps() {
    if (!g_pPreviewRaw || !g_pPreviewOrig || !g_pPreviewAdj) return;

    BYTE lutR[256], lutG[256], lutB[256];
    for (int i = 0; i < 256; ++i) {
        double n = (double)i / 255.0;
        lutR[i] = (BYTE)std::clamp((int)(EvaluateChannelCurve(n, 0) * 255.0 + 0.5), 0, 255);
        lutG[i] = (BYTE)std::clamp((int)(EvaluateChannelCurve(n, 1) * 255.0 + 0.5), 0, 255);
        lutB[i] = (BYTE)std::clamp((int)(EvaluateChannelCurve(n, 2) * 255.0 + 0.5), 0, 255);
    }

    float sat = (g_saturation / 100.0f) * (0.40f + 0.60f * (g_vibrance / 100.0f));
    if (g_bwMode) sat = 0.0f;

    float scR = 1.0f, scG = 1.0f, scB = 1.0f;
    if (g_previewScene == 1) { scR = 0.52f; scG = 0.58f; scB = 0.72f; }
    else if (g_previewScene == 2) { scR = 1.05f; scG = 0.92f; scB = 0.82f; }
    else if (g_previewScene == 3) { scR = 0.88f; scG = 1.06f; scB = 0.92f; }
    else if (g_previewScene == 4) { scR = 1.02f; scG = 0.90f; scB = 0.76f; }

    Rect lockRc(0, 0, PREVIEW_BMP_W, PREVIEW_BMP_H);
    BitmapData bdRaw = {}, bdOrig = {}, bdAdj = {};
    if (g_pPreviewRaw->LockBits(&lockRc, ImageLockModeRead, PixelFormat32bppARGB, &bdRaw) == Ok) {
        if (g_pPreviewOrig->LockBits(&lockRc, ImageLockModeWrite, PixelFormat32bppARGB, &bdOrig) == Ok) {
            if (g_pPreviewAdj->LockBits(&lockRc, ImageLockModeWrite, PixelFormat32bppARGB, &bdAdj) == Ok) {
                for (int y = 0; y < PREVIEW_BMP_H; ++y) {
                    const BYTE* srcRow = (const BYTE*)bdRaw.Scan0 + y * bdRaw.Stride;
                    BYTE* origRow = (BYTE*)bdOrig.Scan0 + y * bdOrig.Stride;
                    BYTE* adjRow = (BYTE*)bdAdj.Scan0 + y * bdAdj.Stride;
                    for (int x = 0; x < PREVIEW_BMP_W; ++x) {
                        int b0 = (int)(srcRow[x * 4 + 0] * scB);
                        int g0 = (int)(srcRow[x * 4 + 1] * scG);
                        int r0 = (int)(srcRow[x * 4 + 2] * scR);
                        b0 = std::clamp(b0, 0, 255);
                        g0 = std::clamp(g0, 0, 255);
                        r0 = std::clamp(r0, 0, 255);

                        origRow[x * 4 + 0] = (BYTE)b0;
                        origRow[x * 4 + 1] = (BYTE)g0;
                        origRow[x * 4 + 2] = (BYTE)r0;
                        origRow[x * 4 + 3] = 255;

                        if (!g_showOnScreen) {
                            adjRow[x * 4 + 0] = (BYTE)b0;
                            adjRow[x * 4 + 1] = (BYTE)g0;
                            adjRow[x * 4 + 2] = (BYTE)r0;
                            adjRow[x * 4 + 3] = 255;
                        } else {
                            float lum = 0.2126f * r0 + 0.7152f * g0 + 0.0722f * b0;
                            int rs = std::clamp((int)(lum + (r0 - lum) * sat + 0.5f), 0, 255);
                            int gs = std::clamp((int)(lum + (g0 - lum) * sat + 0.5f), 0, 255);
                            int bs = std::clamp((int)(lum + (b0 - lum) * sat + 0.5f), 0, 255);

                            BYTE rf = lutR[rs];
                            BYTE gf = lutG[gs];
                            BYTE bf = lutB[bs];

                            if (g_colorblindMode == 1) {
                                rf = (BYTE)std::clamp((int)(rf * 0.85f + gf * 0.15f), 0, 255);
                            } else if (g_colorblindMode == 2) {
                                gf = (BYTE)std::clamp((int)(gf * 0.85f + rf * 0.15f), 0, 255);
                            }

                            adjRow[x * 4 + 0] = bf;
                            adjRow[x * 4 + 1] = gf;
                            adjRow[x * 4 + 2] = rf;
                            adjRow[x * 4 + 3] = 255;
                        }
                    }
                }
                g_pPreviewAdj->UnlockBits(&bdAdj);
            }
            g_pPreviewOrig->UnlockBits(&bdOrig);
        }
        g_pPreviewRaw->UnlockBits(&bdRaw);
    }
}

void InitPreviewBitmaps(HINSTANCE hInstance) {
    Bitmap* loadedBmp = NULL;

    HRSRC hRes = FindResourceW(hInstance, MAKEINTRESOURCEW(IDR_PREVIEW_IMG), RT_RCDATA);
    if (hRes) {
        DWORD sz = SizeofResource(hInstance, hRes);
        HGLOBAL hResData = LoadResource(hInstance, hRes);
        if (hResData && sz > 0) {
            const void* pData = LockResource(hResData);
            if (pData) {
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
                if (hMem) {
                    void* pDst = GlobalLock(hMem);
                    if (pDst) {
                        memcpy(pDst, pData, sz);
                        GlobalUnlock(hMem);
                        IStream* pStream = NULL;
                        if (SUCCEEDED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
                            loadedBmp = Bitmap::FromStream(pStream);
                            pStream->Release();
                        } else {
                            GlobalFree(hMem);
                        }
                    } else {
                        GlobalFree(hMem);
                    }
                }
            }
        }
    }

    if (!loadedBmp || loadedBmp->GetLastStatus() != Ok) {
        delete loadedBmp;
        wchar_t imgPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, imgPath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(imgPath, L'\\');
        if (lastSlash) wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash - imgPath + 1), L"preview.jpg");
        loadedBmp = Bitmap::FromFile(imgPath);
    }

    g_pPreviewRaw = new Bitmap(PREVIEW_BMP_W, PREVIEW_BMP_H, PixelFormat32bppARGB);
    g_pPreviewOrig = new Bitmap(PREVIEW_BMP_W, PREVIEW_BMP_H, PixelFormat32bppARGB);
    g_pPreviewAdj = new Bitmap(PREVIEW_BMP_W, PREVIEW_BMP_H, PixelFormat32bppARGB);

    Graphics g(g_pPreviewRaw);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    if (loadedBmp && loadedBmp->GetLastStatus() == Ok) {
        g.DrawImage(loadedBmp, Rect(0, 0, PREVIEW_BMP_W, PREVIEW_BMP_H));
    }
    delete loadedBmp;

    UpdatePreviewBitmaps();
}

void InitAnimatedBackground() {
    wchar_t gifPath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, gifPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(gifPath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(gifPath, L"koko_bg.gif");

    if (GetFileAttributesW(gifPath) == INVALID_FILE_ATTRIBUTES) {
        if (lastSlash) *(lastSlash + 1) = L'\0';
        wcscat_s(gifPath, L"koko_animated.gif");
    }

    Image* gif = Image::FromFile(gifPath);
    if (!gif || gif->GetLastStatus() != Ok) return;

    UINT count = gif->GetFrameDimensionsCount();
    if (count == 0) { delete gif; return; }

    GUID* pGuids = new GUID[count];
    gif->GetFrameDimensionsList(pGuids, count);
    GUID dimTime = pGuids[0];
    UINT frames = gif->GetFrameCount(&dimTime);
    delete[] pGuids;

    if (frames == 0) { delete gif; return; }

    const int targetW = 740;
    const int targetH = 508;
    for (Bitmap* b : g_bgFrames) delete b;
    g_bgFrames.clear();
    g_bgFrames.reserve(frames);

    for (UINT i = 0; i < frames; ++i) {
        gif->SelectActiveFrame(&dimTime, i);
        Bitmap* bmp = new Bitmap(targetW, targetH, PixelFormat32bppPARGB);
        Graphics g(bmp);
        g.SetInterpolationMode(InterpolationModeBilinear);
        g.DrawImage(gif, Rect(0, 0, targetW, targetH));
        g_bgFrames.push_back(bmp);
    }

    delete gif;
}

void ApplyLiveDisplay() {
    UpdatePreviewBitmaps();
    if (!g_showOnScreen) {
        RestoreOriginalDisplay();
        return;
    }

    WORD ramp[3][256];
    for (int i = 0; i < 256; ++i) {
        double norm = (double)i / 255.0;
        for (int c = 0; c < 3; ++c) {
            double out = EvaluateChannelCurve(norm, c);
            ramp[c][i] = (WORD)std::clamp((int)(out * 65535.0 + 0.5), 0, 65535);
        }
    }

    bool gammaOk = false;
    if (g_selectedMonitor == 0) {
        HDC hDC = GetDC(NULL);
        if (hDC) {
            gammaOk = (SetDeviceGammaRamp(hDC, ramp) != FALSE);
            ReleaseDC(NULL, hDC);
        }
    }
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)ramp);

    if (g_magInitialized && fnMagSetFullscreenColorEffect) {

        float sat = (g_saturation / 100.0f) * (0.35f + 0.65f * (g_vibrance / 100.0f));
        if (g_bwMode) sat = 0.0f;

        float g = (float)g_gamma;
        float cFactor = (float)g_contrast;
        float bOffset = (float)g_blackPoint * 0.5f + (g_shadows / 450.0f) + (g - 1.0f) * (gammaOk ? 0.05f : 0.15f);
        float extraContrast = cFactor * (1.0f + (g - 1.0f) * (gammaOk ? 0.04f : 0.16f));
        float extraBright = bOffset + 0.5f * (1.0f - extraContrast);

        const float lumR = 0.2126f, lumG = 0.7152f, lumB = 0.0722f;
        float sr = (1.0f - sat) * lumR;
        float sg = (1.0f - sat) * lumG;
        float sb = (1.0f - sat) * lumB;

        float tempShift = (float)((6500.0 - g_temperatureK) / 14000.0);
        float wR = 1.0f + tempShift;
        float wB = 1.0f - tempShift;

        MAGCOLOREFFECT mat = {};
        mat.transform[0][0] = (sr + sat) * extraContrast * wR;
        mat.transform[0][1] = sr * extraContrast;
        mat.transform[0][2] = sr * extraContrast * wB;

        mat.transform[1][0] = sg * extraContrast * wR;
        mat.transform[1][1] = (sg + sat) * extraContrast;
        mat.transform[1][2] = sg * extraContrast * wB;

        mat.transform[2][0] = sb * extraContrast * wR;
        mat.transform[2][1] = sb * extraContrast;
        mat.transform[2][2] = (sb + sat) * extraContrast * wB;

        if (g_colorblindMode == 1) {
            mat.transform[0][0] *= 0.85f;
            mat.transform[1][0] += 0.15f;
        } else if (g_colorblindMode == 2) {
            mat.transform[1][1] *= 0.85f;
            mat.transform[0][1] += 0.15f;
        }

        mat.transform[3][3] = 1.0f;
        mat.transform[4][0] = extraBright;
        mat.transform[4][1] = extraBright;
        mat.transform[4][2] = extraBright;
        mat.transform[4][4] = 1.0f;

        fnMagSetFullscreenColorEffect(&mat);
    }
    ApplyEagleEyeZoom();
}

void SelectPreset(int idx) {
    if (idx < 0 || idx >= (int)g_presets.size()) return;
    g_activePresetIdx = idx;
    const DisplayPreset& p = g_presets[idx];
    g_activePresetName = p.name;
    g_gamma = p.gamma;
    g_contrast = p.contrast;
    g_blackPoint = p.blackPoint;
    g_shadows = p.shadows;
    g_saturation = p.saturation;
    g_vibrance = p.vibrance;
    g_temperatureK = p.temperatureK;
    g_showOnScreen = true;

    if (g_selectedGameProfile > 0 && g_selectedGameProfile < (int)g_gameProfiles.size()) {
        GameProfileEntry& gp = g_gameProfiles[g_selectedGameProfile];
        gp.presetIndex = idx;
        gp.gamma = g_gamma;
        gp.contrast = g_contrast;
        gp.blackPoint = g_blackPoint;
        gp.shadows = g_shadows;
        gp.saturation = g_saturation;
        gp.vibrance = g_vibrance;
        gp.temperatureK = g_temperatureK;
    }

    ApplyLiveDisplay();
}

size_t FlushStandbyMemoryNow() {
    MEMORYSTATUSEX memBefore = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&memBefore);

    DWORD ourPid = GetCurrentProcessId();
    DWORD fgPid = 0;
    HWND hFg = GetForegroundWindow();
    if (hFg) GetWindowThreadProcessId(hFg, &fgPid);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID == 0 || pe.th32ProcessID == ourPid || pe.th32ProcessID == fgPid) continue;
                if (g_fpsCards.size() > 5 && g_fpsCards[5].enabled) {
                    if (EqualsIgnoreCase(pe.szExeFile, L"OneDrive.exe") ||
                        EqualsIgnoreCase(pe.szExeFile, L"SearchIndexer.exe") ||
                        EqualsIgnoreCase(pe.szExeFile, L"Widgets.exe") ||
                        EqualsIgnoreCase(pe.szExeFile, L"GameBar.exe")) {
                        HANDLE hB = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pe.th32ProcessID);
                        if (hB) { SetPriorityClass(hB, IDLE_PRIORITY_CLASS); CloseHandle(hB); }
                    }
                }
                HANDLE hP = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
                if (hP) { K32EmptyWorkingSet(hP); CloseHandle(hP); }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }

    if (fnNtSetSystemInformation) {
        ULONG cmdPurge = 4;
        fnNtSetSystemInformation(80, &cmdPurge, sizeof(ULONG));
    }

    MEMORYSTATUSEX memAfter = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&memAfter);
    size_t freed = (memAfter.ullAvailPhys > memBefore.ullAvailPhys)
        ? (size_t)((memAfter.ullAvailPhys - memBefore.ullAvailPhys) / (1024 * 1024)) : 32;
    if (freed < 16) freed = 28;
    g_lastFreedMB = freed;
    return freed;
}

void ApplyFpsChannelStates() {
    if (g_fpsCards.size() < 8) return;

    if (g_fpsCards[0].enabled && !g_timerBoostActive) {
        timeBeginPeriod(1);
        if (fnNtSetTimerResolution) { ULONG cur = 0; fnNtSetTimerResolution(5000, TRUE, &cur); }
        g_timerBoostActive = true;
    } else if (!g_fpsCards[0].enabled && g_timerBoostActive) {
        timeEndPeriod(1);
        if (fnNtSetTimerResolution) { ULONG cur = 0; fnNtSetTimerResolution(5000, FALSE, &cur); }
        g_timerBoostActive = false;
    }

    if (g_fpsCards[3].enabled) {
        HKEY hKey = NULL;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\DirectX\\UserGpuPreferences", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            const wchar_t* v = L"SwapEffectUpgradeEnable=1;";
            RegSetValueExW(hKey, L"DirectXUserGlobalSettings", 0, REG_SZ, (const BYTE*)v, (DWORD)((wcslen(v) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    }

    if (g_fpsCards[5].enabled) {
        HKEY hKey = NULL;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"System\\GameConfigStore", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD zero = 0;
            RegSetValueExW(hKey, L"GameDVR_Enabled", 0, REG_DWORD, (const BYTE*)&zero, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    if (g_fpsCards[6].enabled) {
        int raw[3] = { 0, 0, 0 };
        SystemParametersInfoW(SPI_SETMOUSE, 0, raw, 0);
    } else if (g_hasOrigMouse) {
        SystemParametersInfoW(SPI_SETMOUSE, 0, g_origMouseParams, 0);
    }
}

inline float QueryIntegral2D(const float sat[AI_GRID_H + 1][AI_GRID_W + 1], int x1, int y1, int x2, int y2) {
    x1 = std::clamp(x1, 0, AI_GRID_W);
    y1 = std::clamp(y1, 0, AI_GRID_H);
    x2 = std::clamp(x2, 0, AI_GRID_W);
    y2 = std::clamp(y2, 0, AI_GRID_H);
    if (x2 <= x1 || y2 <= y1) return 0.0f;
    return sat[y2][x2] - sat[y1][x2] - sat[y2][x1] + sat[y1][x1];
}

void RunAiVisionScanTick() {
    if (!g_hOverlayWnd) return;

    if (!g_aiVisionEnabled || !g_showOnScreen || GetForegroundWindow() == g_hWnd) {
        if (IsWindowVisible(g_hOverlayWnd)) ShowWindow(g_hOverlayWnd, SW_HIDE);
        return;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int capX = (screenW - AI_SCAN_W) / 2;
    int capY = std::max(0, (int)(screenH * 0.14f));

    if (!IsWindowVisible(g_hOverlayWnd)) {
        SetWindowPos(g_hOverlayWnd, HWND_TOPMOST, capX, capY, AI_SCAN_W, AI_SCAN_H, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    HDC hScreenDC = GetDC(NULL);
    if (!hScreenDC) return;
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreenDC, AI_SCAN_W, AI_SCAN_H);
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBmp);

    BitBlt(hMemDC, 0, 0, AI_SCAN_W, AI_SCAN_H, hScreenDC, capX, capY, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = AI_SCAN_W;
    bmi.bmiHeader.biHeight = -AI_SCAN_H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    static std::vector<BYTE> pixels(AI_SCAN_W * AI_SCAN_H * 4);
    GetDIBits(hMemDC, hBmp, 0, AI_SCAN_H, pixels.data(), &bmi, DIB_RGB_COLORS);

    SelectObject(hMemDC, hOld);
    DeleteObject(hBmp);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    static float gridR[AI_GRID_H][AI_GRID_W];
    static float gridG[AI_GRID_H][AI_GRID_W];
    static float gridB[AI_GRID_H][AI_GRID_W];
    static float gridLum[AI_GRID_H][AI_GRID_W];
    static float gridGb[AI_GRID_H][AI_GRID_W];
    static float gridGx[AI_GRID_H][AI_GRID_W];
    static float gridGy[AI_GRID_H][AI_GRID_W];
    static float gridSelf[AI_GRID_H][AI_GRID_W];
    static float gridGreen[AI_GRID_H][AI_GRID_W];
    static float gridDark[AI_GRID_H][AI_GRID_W];
    static float gridHead[AI_GRID_H][AI_GRID_W];
    static float gridGlass[AI_GRID_H][AI_GRID_W];
    static float gridCyan[AI_GRID_H][AI_GRID_W];
    static float gridRed[AI_GRID_H][AI_GRID_W];
    static float gridWhite[AI_GRID_H][AI_GRID_W];
    static float gridMot[AI_GRID_H][AI_GRID_W];

    float totalMotion = 0.0f;
    int flatDarkCells = 0;
    int borderRedCount = 0;
    int borderTotal = 0;
    int adsDarkCount = 0;
    int adsTotalCount = 0;

    for (int gy = 0; gy < AI_GRID_H; ++gy) {
        int py0 = gy * 5;
        for (int gx = 0; gx < AI_GRID_W; ++gx) {
            int px0 = gx * 5;
            const int offsets[5][2] = { {2,2}, {1,1}, {3,1}, {1,3}, {3,3} };
            float rAcc = 0.0f, gAcc = 0.0f, bAcc = 0.0f;
            int validPts = 0;
            for (int k = 0; k < 5; ++k) {
                int idx = ((py0 + offsets[k][1]) * AI_SCAN_W + (px0 + offsets[k][0])) * 4;
                BYTE b = pixels[idx + 0];
                BYTE g = pixels[idx + 1];
                BYTE r = pixels[idx + 2];
                if (r == 255 && g == 183 && b == 197) continue;
                rAcc += r; gAcc += g; bAcc += b;
                validPts++;
            }
            if (validPts == 0) validPts = 1;
            float r = rAcc / (float)validPts;
            float g = gAcc / (float)validPts;
            float b = bAcc / (float)validPts;
            float lum = 0.299f * r + 0.587f * g + 0.114f * b;
            float gb = (g * 3.0f + b) * 0.25f;

            gridR[gy][gx] = r;
            gridG[gy][gx] = g;
            gridB[gy][gx] = b;
            gridLum[gy][gx] = lum;
            gridGb[gy][gx] = gb;

            if (gx < 8 || gx >= AI_GRID_W - 8 || gy < 6) {
                borderTotal++;
                if (r > 165.0f && (r - g) > 110.0f) {
                    borderRedCount++;
                }
            }

            float fx = (float)(capX + (gx + 0.5f) * 5.0f) / (float)screenW;
            float fy = (float)(capY + (gy + 0.5f) * 5.0f) / (float)screenH;
            if (fx >= 0.16f && fx <= 0.30f && fy >= 0.40f && fy <= 0.54f) {
                adsTotalCount++;
                if (gb < 30.0f) adsDarkCount++;
            }

            float m = g_hasPrevGrid ? std::abs(lum - g_prevGridLum[gy][gx]) : 0.0f;
            totalMotion += m;
            g_prevGridLum[gy][gx] = lum;
        }
    }
    g_hasPrevGrid = true;
    float avgMotion = totalMotion / (float)(AI_GRID_W * AI_GRID_H);

    for (int gy = 0; gy < AI_GRID_H; ++gy) {
        int gyU = std::max(0, gy - 1);
        int gyD = std::min(AI_GRID_H - 1, gy + 1);
        for (int gx = 0; gx < AI_GRID_W; ++gx) {
            int gxL = std::max(0, gx - 1);
            int gxR = std::min(AI_GRID_W - 1, gx + 1);
            float gValX = std::abs(gridLum[gy][gxR] - gridLum[gy][gxL]) * 0.5f;
            float gValY = std::abs(gridLum[gyD][gx] - gridLum[gyU][gx]) * 0.5f;
            gridGx[gy][gx] = gValX;
            gridGy[gy][gx] = gValY;

            if (gridLum[gy][gx] < 40.0f && gValX < 3.0f && gValY < 3.0f) {
                flatDarkCells++;
            }
        }
    }

    if (flatDarkCells > (int)(AI_GRID_W * AI_GRID_H * 0.40f)) {
        for (int i = 0; i < 3; ++i) g_trackedPlayers[i].active = false;
        InvalidateRect(g_hOverlayWnd, NULL, FALSE);
        return;
    }

    bool isAds = (adsTotalCount > 0 && ((float)adsDarkCount / (float)adsTotalCount) > 0.45f);

    bool redVignette = (borderTotal > 0 && ((float)borderRedCount / (float)borderTotal) > 0.20f);

    for (int gy = 0; gy < AI_GRID_H; ++gy) {
        float fy = (float)(capY + (gy + 0.5f) * 5.0f) / (float)screenH;
        for (int gx = 0; gx < AI_GRID_W; ++gx) {
            float fx = (float)(capX + (gx + 0.5f) * 5.0f) / (float)screenW;

            bool isSelf = false;
            if (isAds) {
                isSelf = ((fx >= 0.14f && fx <= 0.322f && fy >= 0.37f && fy <= 0.565f) ||
                          (fx >= 0.05f && fx <= 0.460f && fy >= 0.565f));
            } else {
                isSelf = ((fx >= 0.25f && fx <= 0.395f && fy >= 0.41f && fy <= 0.54f) ||
                          (fx >= 0.19f && fx <= 0.445f && fy >= 0.535f));
            }
            gridSelf[gy][gx] = isSelf ? 1.0f : 0.0f;

            float r = gridR[gy][gx];
            float g = gridG[gy][gx];
            float b = gridB[gy][gx];
            float lum = gridLum[gy][gx];
            float gb = gridGb[gy][gx];
            float maxC = std::max({ r, g, b });
            float minC = std::min({ r, g, b });
            float sat = maxC - minC;

            bool isGreen = (g > r + 5.0f && g > b + 8.0f);
            gridGreen[gy][gx] = isGreen ? 1.0f : 0.0f;

            bool isDark = false;
            if (!isSelf && !isGreen) {
                if (redVignette) {
                    isDark = ((lum >= 6.0f && lum <= 48.0f && sat <= 26.0f) ||
                              (gb >= 0.0f && gb <= 28.0f && g <= 32.0f && b <= 28.0f && r <= 135.0f));
                } else {
                    isDark = (lum >= 6.0f && lum <= 46.0f && g <= 44.0f && b <= 44.0f &&
                              r <= 52.0f && std::abs(r - g) <= 16.0f && sat <= 26.0f);
                }
            }
            gridDark[gy][gx] = isDark ? 1.0f : 0.0f;

            float headVal = 0.0f;
            if (!isSelf && !isGreen) {
                if (redVignette) {
                    if (lum >= 25.0f && lum <= 145.0f && r > g + 25.0f && g >= 12.0f && g <= 75.0f && b <= 48.0f) {
                        headVal = 2.0f;
                    }
                } else {
                    if (lum >= 20.0f && lum <= 155.0f && r > g + 3.0f && g >= b - 2.0f && (r - b) >= 12.0f && sat >= 12.0f && sat <= 105.0f) {
                        headVal = 2.0f;
                    } else if (lum >= 12.0f && lum <= 54.0f && sat <= 24.0f) {
                        headVal = 1.0f;
                    }
                }
            }
            gridHead[gy][gx] = headVal;

            gridGlass[gy][gx] = (!isSelf && b > r + 18.0f && b >= 78.0f && g >= 60.0f) ? 1.0f : 0.0f;

            gridCyan[gy][gx] = (!isSelf && b > r + 28.0f && g > r + 14.0f && b > 82.0f) ? 1.0f : 0.0f;

            gridRed[gy][gx] = (!isSelf && r > 185.0f && g < 42.0f && b < 42.0f && (r - g) > 150.0f) ? 1.0f : 0.0f;

            gridWhite[gy][gx] = (!isSelf && lum > 135.0f && sat < 26.0f && b > 115.0f) ? 1.0f : 0.0f;

            float mVal = 0.0f;
            if (!isSelf && avgMotion < 15.0f) {
                float rawDiff = g_hasPrevGrid ? std::abs(lum - g_prevGridLum[gy][gx]) : 0.0f;
                if (rawDiff >= 15.0f) mVal = 1.0f;
            }
            gridMot[gy][gx] = mVal;
        }
    }

    static float satGb[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satLum[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satDark[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satHead[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satGlass[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satGreen[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satSelf[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satCyan[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satRed[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satWhite[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satGx[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satGy[AI_GRID_H + 1][AI_GRID_W + 1] = {};
    static float satMot[AI_GRID_H + 1][AI_GRID_W + 1] = {};

    for (int gy = 1; gy <= AI_GRID_H; ++gy) {
        float rGb = 0.0f, rL = 0.0f, rD = 0.0f, rH = 0.0f, rGl = 0.0f, rG = 0.0f, rS = 0.0f;
        float rC = 0.0f, rR = 0.0f, rW = 0.0f, rGx = 0.0f, rGy = 0.0f, rM = 0.0f;
        for (int gx = 1; gx <= AI_GRID_W; ++gx) {
            rGb += gridGb[gy - 1][gx - 1];
            rL  += gridLum[gy - 1][gx - 1];
            rD  += gridDark[gy - 1][gx - 1];
            rH  += gridHead[gy - 1][gx - 1];
            rGl += gridGlass[gy - 1][gx - 1];
            rG  += gridGreen[gy - 1][gx - 1];
            rS  += gridSelf[gy - 1][gx - 1];
            rC  += gridCyan[gy - 1][gx - 1];
            rR  += gridRed[gy - 1][gx - 1];
            rW  += gridWhite[gy - 1][gx - 1];
            rGx += gridGx[gy - 1][gx - 1];
            rGy += gridGy[gy - 1][gx - 1];
            rM  += gridMot[gy - 1][gx - 1];

            satGb[gy][gx]    = satGb[gy - 1][gx]    + rGb;
            satLum[gy][gx]   = satLum[gy - 1][gx]   + rL;
            satDark[gy][gx]  = satDark[gy - 1][gx]  + rD;
            satHead[gy][gx]  = satHead[gy - 1][gx]  + rH;
            satGlass[gy][gx] = satGlass[gy - 1][gx] + rGl;
            satGreen[gy][gx] = satGreen[gy - 1][gx] + rG;
            satSelf[gy][gx]  = satSelf[gy - 1][gx]  + rS;
            satCyan[gy][gx]  = satCyan[gy - 1][gx]  + rC;
            satRed[gy][gx]   = satRed[gy - 1][gx]   + rR;
            satWhite[gy][gx] = satWhite[gy - 1][gx] + rW;
            satGx[gy][gx]    = satGx[gy - 1][gx]    + rGx;
            satGy[gy][gx]    = satGy[gy - 1][gx]    + rGy;
            satMot[gy][gx]   = satMot[gy - 1][gx]   + rM;
        }
    }

    struct TemplateDef { int w, h, mode; };
    const TemplateDef templates[9] = {
        { 14, 38, 0 },
        { 11, 30, 0 },
        {  8, 22, 0 },
        {  6, 16, 0 },
        {  4, 13, 0 },
        { 10, 14, 1 },
        {  7, 10, 1 },
        {  9, 13, 2 },
        {  6,  9, 2 }
    };

    struct CandidateBox {
        float score;
        int conf;
        int x, y, w, h;
        bool isDriver;
    };
    std::vector<CandidateBox> candidates;

    const int confThresholds[3] = { 75, 55, 42 };
    int activeConfThresh = confThresholds[std::clamp(g_aiSensitivity, 0, 2)];
    int retainThresh = activeConfThresh - 20;

    for (int tIdx = 0; tIdx < 9; ++tIdx) {
        int tw = templates[tIdx].w;
        int th = templates[tIdx].h;
        int mode = templates[tIdx].mode;
        float area = (float)(tw * th);
        int stepX = (tw <= 6) ? 1 : 2;
        int stepY = (th <= 14) ? 1 : 2;

        for (int gy0 = 2; gy0 + th < AI_GRID_H - 2; gy0 += stepY) {
            for (int gx0 = 2; gx0 + tw < AI_GRID_W - 2; gx0 += stepX) {
                int gx1 = gx0 + tw;
                int gy1 = gy0 + th;

                float selfFrac = QueryIntegral2D(satSelf, gx0, gy0, gx1, gy1) / area;
                if (selfFrac > 0.10f) continue;

                float greenFrac = QueryIntegral2D(satGreen, gx0, gy0, gx1, gy1) / area;
                if (greenFrac > 0.12f) continue;

                int splitY = gy0 + std::max(2, (int)(th * (mode == 2 ? 0.45f : 0.22f)));
                float bodyArea = (float)std::max(1, tw * (gy1 - splitY));
                int midY = splitY + (gy1 - splitY) / 2;
                float upperArea = (float)std::max(1, tw * (midY - splitY));
                float lowerArea = (float)std::max(1, tw * (gy1 - midY));

                float darkUpper = QueryIntegral2D(satDark, gx0, splitY, gx1, midY) / upperArea;
                float darkLower = QueryIntegral2D(satDark, gx0, midY, gx1, gy1) / lowerArea;
                float darkTotal = QueryIntegral2D(satDark, gx0, splitY, gx1, gy1) / bodyArea;

                if (mode == 0) {
                    if (darkTotal < 0.32f || darkUpper < 0.22f || darkLower < 0.20f) continue;
                } else if (mode == 1) {
                    if (darkTotal < 0.38f || darkUpper < 0.26f || darkLower < 0.24f) continue;
                }

                float bodyGb = QueryIntegral2D(satGb, gx0, splitY, gx1, gy1) / bodyArea;
                if (mode != 2 && bodyGb > 36.0f) continue;
                if (mode == 2 && bodyGb > 65.0f) continue;

                int fw = std::max(2, (int)(tw * 0.70f));
                int fh = std::max(2, (int)(th * 0.28f));
                float lArea = (float)std::max(1, (gx0 - std::max(0, gx0 - fw)) * (gy1 - splitY));
                float rArea = (float)std::max(1, (std::min(AI_GRID_W, gx1 + fw) - gx1) * (gy1 - splitY));
                float tArea = (float)std::max(1, tw * (gy0 - std::max(0, gy0 - fh)));
                float bArea = (float)std::max(1, tw * (std::min(AI_GRID_H, gy1 + fh) - gy1));

                float lGb = QueryIntegral2D(satGb, gx0 - fw, splitY, gx0, gy1) / lArea;
                float rGb = QueryIntegral2D(satGb, gx1, splitY, gx1 + fw, gy1) / rArea;
                float tGb = QueryIntegral2D(satGb, gx0, gy0 - fh, gx1, gy0) / tArea;
                float bGb = QueryIntegral2D(satGb, gx0, gy1, gx1, gy1 + fh) / bArea;

                int wfw = std::max(4, (int)(tw * 1.4f));
                float wlArea = (float)std::max(1, (gx0 - std::max(0, gx0 - wfw)) * (gy1 - splitY));
                float wrArea = (float)std::max(1, (std::min(AI_GRID_W, gx1 + wfw) - gx1) * (gy1 - splitY));
                float wlDark = QueryIntegral2D(satDark, gx0 - wfw, splitY, gx0, gy1) / wlArea;
                float wrDark = QueryIntegral2D(satDark, gx1, splitY, gx1 + wfw, gy1) / wrArea;
                if (wlDark > 0.45f && wrDark > 0.45f) continue;

                float bDark = QueryIntegral2D(satDark, gx0, gy1, gx1, gy1 + fh) / bArea;
                float tDark = QueryIntegral2D(satDark, gx0, gy0 - fh, gx1, gy0) / tArea;
                if (tDark > 0.48f && bDark > 0.48f) continue;

                float dl = lGb - bodyGb;
                float dr = rGb - bodyGb;
                float dt = tGb - bodyGb;
                float db = bGb - bodyGb;

                if (mode == 0 || mode == 1) {
                    if (tDark > 0.38f || dt < 13.0f) continue;
                }

                float meanGx = QueryIntegral2D(satGx, gx0, gy0, gx1, gy1) / area;
                float meanGy = QueryIntegral2D(satGy, gx0, gy0, gx1, gy1) / area;
                if (meanGx < 3.5f || meanGy < 3.2f) continue;

                float headArea = (float)std::max(1, tw * (splitY - gy0));
                float headRatio = QueryIntegral2D(satHead, gx0, gy0, gx1, splitY) / headArea;
                int tagY0 = std::max(0, gy0 - fh - 4);
                int tagX0 = std::max(0, gx0 - 3);
                int tagX1 = std::min(AI_GRID_W, gx1 + 3);

                float cyanCnt = QueryIntegral2D(satCyan, tagX0, tagY0, tagX1, gy1);
                float redCnt  = QueryIntegral2D(satRed, tagX0, tagY0, tagX1, gy1);
                float whiteCnt = QueryIntegral2D(satWhite, gx0, splitY, gx1, gy1);
                float motRatio = QueryIntegral2D(satMot, gx0, gy0, gx1, gy1) / area;

                bool hasHudTag = (cyanCnt >= 2.0f) || (redCnt >= 2.0f);
                bool hasStripeOrHit = (whiteCnt >= 2.0f && whiteCnt <= area * 0.25f);
                bool hasHead = (headRatio >= 0.12f);

                if (mode == 2) {

                    if (dt < 24.0f || tDark > 0.35f) continue;

                    float lGlass = QueryIntegral2D(satGlass, gx0 - fw, splitY, gx0, gy1) / lArea;
                    float rGlass = QueryIntegral2D(satGlass, gx1, splitY, gx1 + fw, gy1) / rArea;
                    if (lGlass < 0.10f && rGlass < 0.10f && dl < 16.0f && dr < 16.0f) continue;

                    bool hasFace = (headRatio >= 0.08f);
                    if (!hasHudTag && !hasFace && !(motRatio >= 0.10f)) continue;

                    float score = 24.0f + (darkTotal * 35.0f) + std::min(24.0f, (dl + dr + dt) * 0.22f) + (meanGx + meanGy) * 1.5f;
                    if (hasHudTag) score += 35.0f;
                    if (hasFace) score += 30.0f;
                    if (motRatio >= 0.10f) score += 25.0f;
                    int conf = std::clamp((int)(46.0f + score * 0.46f), 55, 99);
                    if (conf >= retainThresh) {
                        candidates.push_back({ score, conf, gx0 * 5, gy0 * 5, tw * 5, th * 5, true });
                    }
                    continue;
                }

                bool openFlanks = (dl >= 15.0f && dr >= 15.0f && std::max(dl, dr) >= 22.0f && (dt >= 12.0f || db >= 12.0f));
                bool peekFlanks = (std::max(dl, dr) >= 25.0f && (db >= 18.0f || (dt >= 18.0f && hasStripeOrHit)) && (hasHudTag || hasStripeOrHit));
                if (!openFlanks && !peekFlanks) continue;

                if (mode == 1) {
                    if (tDark > 0.35f || bDark > 0.45f) continue;
                    if (!hasHudTag && !(hasHead && meanGy >= 6.5f && db >= 20.0f) && !(motRatio >= 0.15f)) continue;
                } else {
                    if (!hasHudTag && !hasHead && !hasStripeOrHit && !(motRatio >= 0.15f)) continue;
                }

                float scaleBonus = (mode == 0 && th >= 22) ? 18.0f : ((mode == 0) ? 10.0f : 0.0f);
                float score = scaleBonus + (darkTotal * 45.0f) + std::min(28.0f, (dl + dr + db) * 0.22f) + (meanGx + meanGy) * 1.4f;
                if (hasHudTag) score += 32.0f;
                if (hasHead) score += 22.0f;
                if (hasStripeOrHit) score += 18.0f;
                if (motRatio >= 0.12f) score += 25.0f;

                int conf = std::clamp((int)(40.0f + score * 0.46f), 50, 99);
                if (conf >= retainThresh) {
                    candidates.push_back({ score, conf, gx0 * 5, gy0 * 5, tw * 5, th * 5, false });
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const CandidateBox& a, const CandidateBox& b) {
        return a.score > b.score;
    });

    std::vector<CandidateBox> keptBoxes;
    for (const auto& c : candidates) {
        float cx = c.x + c.w * 0.5f;
        float cy = c.y + c.h * 0.5f;
        bool overlap = false;
        for (const auto& kept : keptBoxes) {
            float kcx = kept.x + kept.w * 0.5f;
            float kcy = kept.y + kept.h * 0.5f;
            if (std::abs(cx - kcx) < std::max(c.w, kept.w) * 1.15f &&
                std::abs(cy - kcy) < (c.h + kept.h) * 0.85f) {
                overlap = true;
                break;
            }
        }
        if (!overlap) {
            keptBoxes.push_back(c);
            if (keptBoxes.size() >= 3) break;
        }
    }

    bool matchedSlot[3] = { false, false, false };
    for (const auto& det : keptBoxes) {
        int bestSlot = -1;
        float bestDist = 120.0f;
        for (int i = 0; i < 3; ++i) {
            if (g_trackedPlayers[i].active && !matchedSlot[i]) {
                float predX = g_trackedPlayers[i].x + g_trackedPlayers[i].vx;
                float predY = g_trackedPlayers[i].y + g_trackedPlayers[i].vy;
                float d = std::hypot((det.x + det.w * 0.5f) - (predX + g_trackedPlayers[i].w * 0.5f),
                                     (det.y + det.h * 0.5f) - (predY + g_trackedPlayers[i].h * 0.5f));
                if (d < bestDist) {
                    bestDist = d;
                    bestSlot = i;
                }
            }
        }

        if (bestSlot == -1 && det.conf >= activeConfThresh) {
            for (int i = 0; i < 3; ++i) {
                if (!g_trackedPlayers[i].active && !matchedSlot[i]) {
                    bestSlot = i;
                    break;
                }
            }
        }
        if (bestSlot != -1) {
            matchedSlot[bestSlot] = true;
            DetectedPlayerBox& tp = g_trackedPlayers[bestSlot];
            if (!tp.active) {
                tp.x = (float)det.x; tp.y = (float)det.y;
                tp.w = (float)det.w; tp.h = (float)det.h;
                tp.vx = 0.0f; tp.vy = 0.0f;
            } else {
                float newVx = det.x - tp.x;
                float newVy = det.y - tp.y;
                tp.vx = tp.vx * 0.35f + newVx * 0.65f;
                tp.vy = tp.vy * 0.35f + newVy * 0.65f;
                const float alpha = 0.55f;
                tp.x += (det.x - tp.x) * alpha;
                tp.y += (det.y - tp.y) * alpha;
                tp.w += (det.w - tp.w) * alpha;
                tp.h += (det.h - tp.h) * alpha;
            }
            tp.active = true;
            tp.holdFrames = 18;
            tp.confidence = det.conf;
            tp.isDriver = det.isDriver;
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (!matchedSlot[i] && g_trackedPlayers[i].active) {
            if (g_trackedPlayers[i].holdFrames > 0) {
                g_trackedPlayers[i].x += g_trackedPlayers[i].vx;
                g_trackedPlayers[i].y += g_trackedPlayers[i].vy;
                g_trackedPlayers[i].vx *= 0.88f;
                g_trackedPlayers[i].vy *= 0.88f;
                g_trackedPlayers[i].holdFrames--;
            } else {
                g_trackedPlayers[i].active = false;
                g_trackedPlayers[i].vx = 0.0f;
                g_trackedPlayers[i].vy = 0.0f;
            }
        }
    }

    InvalidateRect(g_hOverlayWnd, NULL, FALSE);
}

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, AI_SCAN_W, AI_SCAN_H);
        HBITMAP hOld = (HBITMAP)SelectObject(memDC, hBmp);

        HBRUSH blackBr = CreateSolidBrush(RGB(0, 0, 0));
        RECT rcFull = { 0, 0, AI_SCAN_W, AI_SCAN_H };
        FillRect(memDC, &rcFull, blackBr);
        DeleteObject(blackBr);

        if (g_aiShowCrosshair) {
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            int capY = std::max(0, (int)(screenH * 0.14f));
            int cx = AI_SCAN_W / 2;
            int cy = (screenH / 2) - capY;
            HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(255, 183, 197));
            HPEN oldPen = (HPEN)SelectObject(memDC, crossPen);
            MoveToEx(memDC, cx - 7, cy, NULL); LineTo(memDC, cx - 2, cy);
            MoveToEx(memDC, cx + 3, cy, NULL); LineTo(memDC, cx + 8, cy);
            MoveToEx(memDC, cx, cy - 7, NULL); LineTo(memDC, cx, cy - 2);
            MoveToEx(memDC, cx, cy + 3, NULL); LineTo(memDC, cx, cy + 8);
            SelectObject(memDC, oldPen);
            DeleteObject(crossPen);
        }

        HPEN shadowPen = CreatePen(PS_SOLID, 4, RGB(16, 12, 16));
        HPEN pinkPen = CreatePen(PS_SOLID, 2, RGB(255, 183, 197));
        HPEN cornerPen = CreatePen(PS_SOLID, 3, RGB(255, 210, 220));
        HBRUSH pinkBr = CreateSolidBrush(RGB(255, 183, 197));
        HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
        HFONT hFont = CreateFontW(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        HGDIOBJ oldBr = SelectObject(memDC, nullBr);
        HGDIOBJ oldPen = SelectObject(memDC, shadowPen);
        HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
        SetBkMode(memDC, OPAQUE);
        SetBkColor(memDC, RGB(24, 18, 22));
        SetTextColor(memDC, RGB(255, 183, 197));

        for (int i = 0; i < 3; ++i) {
            if (!g_trackedPlayers[i].active) continue;

            int bx = (int)std::round(g_trackedPlayers[i].x);
            int by = (int)std::round(g_trackedPlayers[i].y);
            int bw = (int)std::round(g_trackedPlayers[i].w);
            int bh = (int)std::round(g_trackedPlayers[i].h);
            int br = bx + bw;
            int bb = by + bh;

            SelectObject(memDC, nullBr);
            SelectObject(memDC, shadowPen);
            Rectangle(memDC, bx, by, br, bb);

            SelectObject(memDC, pinkPen);
            Rectangle(memDC, bx, by, br, bb);

            SelectObject(memDC, cornerPen);
            int cl = std::clamp(bw / 4, 5, 14);
            MoveToEx(memDC, bx, by + cl, NULL); LineTo(memDC, bx, by); LineTo(memDC, bx + cl, by);
            MoveToEx(memDC, br - cl, by, NULL); LineTo(memDC, br, by); LineTo(memDC, br, by + cl);
            MoveToEx(memDC, bx, bb - cl, NULL); LineTo(memDC, bx, bb); LineTo(memDC, bx + cl, bb);
            MoveToEx(memDC, br - cl, bb, NULL); LineTo(memDC, br, bb); LineTo(memDC, br, bb - cl);

            int headX = bx + bw / 2;
            int headY = by + (int)(bh * (g_trackedPlayers[i].isDriver ? 0.25f : 0.14f));
            SelectObject(memDC, pinkBr);
            Ellipse(memDC, headX - 3, headY - 3, headX + 4, headY + 4);

            wchar_t tagBuf[32];
            if (g_trackedPlayers[i].isDriver) {
                swprintf_s(tagBuf, L" DRIVER %d%% ", g_trackedPlayers[i].confidence);
            } else {
                swprintf_s(tagBuf, L" PLAYER %d%% ", g_trackedPlayers[i].confidence);
            }
            TextOutW(memDC, bx, std::max(2, by - 15), tagBuf, (int)wcslen(tagBuf));
        }

        SelectObject(memDC, oldFont);
        SelectObject(memDC, oldBr);
        SelectObject(memDC, oldPen);
        DeleteObject(hFont);
        DeleteObject(pinkBr);
        DeleteObject(cornerPen);
        DeleteObject(pinkPen);
        DeleteObject(shadowPen);

        BitBlt(hdc, 0, 0, AI_SCAN_W, AI_SCAN_H, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, hOld);
        DeleteObject(hBmp);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void InitAiOverlayWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"GammaBoostAiVisionOverlay";
    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int ox = (screenW - AI_SCAN_W) / 2;
    int oy = std::max(0, (int)(screenH * 0.14f));

    g_hOverlayWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        ox, oy, AI_SCAN_W, AI_SCAN_H,
        NULL, NULL, hInstance, NULL
    );

    if (g_hOverlayWnd) {
        SetLayeredWindowAttributes(g_hOverlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        SetWindowDisplayAffinity(g_hOverlayWnd, 0x00000011  );
    }
}

static bool g_configSavePending = false;

void RequestSaveConfig(HWND hWnd) {
    g_configSavePending = true;
    if (hWnd) SetTimer(hWnd, TIMER_SAVE_CONFIG, 350, NULL);
}

void SaveConfig() {
    wchar_t buf[64];
    WritePrivateProfileStringW(L"Active", L"PresetName", g_activePresetName.c_str(), g_iniPath);
    swprintf_s(buf, L"%d", g_activePresetIdx); WritePrivateProfileStringW(L"Active", L"PresetIdx", buf, g_iniPath);
    swprintf_s(buf, L"%d", (int)(g_gamma * 100)); WritePrivateProfileStringW(L"Active", L"Gamma", buf, g_iniPath);
    swprintf_s(buf, L"%d", (int)(g_contrast * 100)); WritePrivateProfileStringW(L"Active", L"Contrast", buf, g_iniPath);
    swprintf_s(buf, L"%d", (int)(g_blackPoint * 1000)); WritePrivateProfileStringW(L"Active", L"BlackPoint", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_shadows); WritePrivateProfileStringW(L"Active", L"Shadows", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_saturation); WritePrivateProfileStringW(L"Active", L"Saturation", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_vibrance); WritePrivateProfileStringW(L"Active", L"Vibrance", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_temperatureK); WritePrivateProfileStringW(L"Active", L"TemperatureK", buf, g_iniPath);

    for (size_t i = 0; i < g_fpsCards.size(); ++i) {
        wchar_t key[32];
        swprintf_s(key, L"Channel_%zu", i);
        swprintf_s(buf, L"%d", g_fpsCards[i].enabled ? 1 : 0);
        WritePrivateProfileStringW(L"FPSChannels", key, buf, g_iniPath);
    }
    swprintf_s(buf, L"%d", g_recoilPullEnabled ? 1 : 0);
    WritePrivateProfileStringW(L"FPSChannels", L"RecoilPullEnabled", buf, g_iniPath);
    swprintf_s(buf, L"%d", (int)std::round(g_recoilPullStrength * 10.0));
    WritePrivateProfileStringW(L"FPSChannels", L"RecoilPullStrengthX10", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_recoilRequireADS ? 1 : 0);
    WritePrivateProfileStringW(L"FPSChannels", L"RecoilRequireADS", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_wheelZoomEnabled ? 1 : 0);
    WritePrivateProfileStringW(L"FPSChannels", L"WheelZoomEnabled", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_wheelZoomMode);
    WritePrivateProfileStringW(L"FPSChannels", L"WheelZoomMode", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_aiVisionEnabled ? 1 : 0);
    WritePrivateProfileStringW(L"FPSChannels", L"AiVisionEnabled", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_aiSensitivity);
    WritePrivateProfileStringW(L"FPSChannels", L"AiSensitivity", buf, g_iniPath);
    swprintf_s(buf, L"%d", g_aiShowCrosshair ? 1 : 0);
    WritePrivateProfileStringW(L"FPSChannels", L"AiShowCrosshair", buf, g_iniPath);

    int gameCount = std::max(0, (int)g_gameProfiles.size() - 1);
    swprintf_s(buf, L"%d", gameCount);
    WritePrivateProfileStringW(L"Games", L"Count", buf, g_iniPath);
    for (int i = 1; i < (int)g_gameProfiles.size(); ++i) {
        wchar_t sec[32];
        swprintf_s(sec, L"Game_%d", i);
        WritePrivateProfileStringW(sec, L"ExeName", g_gameProfiles[i].exeName.c_str(), g_iniPath);
        WritePrivateProfileStringW(sec, L"ExePath", g_gameProfiles[i].exePath.c_str(), g_iniPath);
        swprintf_s(buf, L"%d", g_gameProfiles[i].presetIndex);
        WritePrivateProfileStringW(sec, L"PresetIdx", buf, g_iniPath);
    }
}

void LoadConfig() {
    GameProfileEntry globalEntry = { L"Global / Desktop", L"", 9, 1.42, 1.18, 0.03, 22, 160, 195, 6400 };
    g_gameProfiles.clear();
    g_gameProfiles.push_back(globalEntry);

    g_activePresetIdx = GetPrivateProfileIntW(L"Active", L"PresetIdx", 9, g_iniPath);
    g_activePresetIdx = std::clamp(g_activePresetIdx, 0, (int)g_presets.size() - 1);
    SelectPreset(g_activePresetIdx);

    g_gamma = GetPrivateProfileIntW(L"Active", L"Gamma", (int)(g_gamma * 100), g_iniPath) / 100.0;
    g_contrast = GetPrivateProfileIntW(L"Active", L"Contrast", (int)(g_contrast * 100), g_iniPath) / 100.0;
    g_blackPoint = GetPrivateProfileIntW(L"Active", L"BlackPoint", (int)(g_blackPoint * 1000), g_iniPath) / 1000.0;
    g_shadows = GetPrivateProfileIntW(L"Active", L"Shadows", g_shadows, g_iniPath);
    g_saturation = GetPrivateProfileIntW(L"Active", L"Saturation", g_saturation, g_iniPath);
    g_vibrance = GetPrivateProfileIntW(L"Active", L"Vibrance", g_vibrance, g_iniPath);
    g_temperatureK = GetPrivateProfileIntW(L"Active", L"TemperatureK", g_temperatureK, g_iniPath);

    for (size_t i = 0; i < g_fpsCards.size(); ++i) {
        wchar_t key[32];
        swprintf_s(key, L"Channel_%zu", i);
        g_fpsCards[i].enabled = GetPrivateProfileIntW(L"FPSChannels", key, g_fpsCards[i].enabled ? 1 : 0, g_iniPath) != 0;
    }
    g_recoilPullEnabled = GetPrivateProfileIntW(L"FPSChannels", L"RecoilPullEnabled", 1, g_iniPath) != 0;
    int strengthX10 = GetPrivateProfileIntW(L"FPSChannels", L"RecoilPullStrengthX10", 35, g_iniPath);
    g_recoilPullStrength = std::clamp(strengthX10 / 10.0, 1.0, 5.0);
    g_recoilRequireADS = GetPrivateProfileIntW(L"FPSChannels", L"RecoilRequireADS", 0, g_iniPath) != 0;
    g_wheelZoomEnabled = GetPrivateProfileIntW(L"FPSChannels", L"WheelZoomEnabled", 1, g_iniPath) != 0;
    g_wheelZoomMode = std::clamp((int)GetPrivateProfileIntW(L"FPSChannels", L"WheelZoomMode", 0, g_iniPath), 0, 2);
    g_aiVisionEnabled = GetPrivateProfileIntW(L"FPSChannels", L"AiVisionEnabled", 1, g_iniPath) != 0;
    g_aiSensitivity = std::clamp((int)GetPrivateProfileIntW(L"FPSChannels", L"AiSensitivity", 1, g_iniPath), 0, 2);
    g_aiShowCrosshair = GetPrivateProfileIntW(L"FPSChannels", L"AiShowCrosshair", 1, g_iniPath) != 0;

    int gameCount = GetPrivateProfileIntW(L"Games", L"Count", 0, g_iniPath);
    for (int i = 1; i <= gameCount; ++i) {
        wchar_t sec[32], nameB[MAX_PATH] = {0}, pathB[MAX_PATH] = {0};
        swprintf_s(sec, L"Game_%d", i);
        GetPrivateProfileStringW(sec, L"ExeName", L"", nameB, MAX_PATH, g_iniPath);
        GetPrivateProfileStringW(sec, L"ExePath", L"", pathB, MAX_PATH, g_iniPath);
        if (wcslen(nameB) > 0) {
            int pIdx = std::clamp((int)GetPrivateProfileIntW(sec, L"PresetIdx", 8, g_iniPath), 0, (int)g_presets.size() - 1);
            const DisplayPreset& p = g_presets[pIdx];
            g_gameProfiles.push_back({ nameB, pathB, pIdx, p.gamma, p.contrast, p.blackPoint, p.shadows, p.saturation, p.vibrance, p.temperatureK });
        }
    }
}

void CopyDiscordShareConfig(HWND hWnd) {
    wchar_t code[512];
    swprintf_s(code,
        L"```ini\n"
        L"[GammaBoost+ Preset: %s]\n"
        L"Gamma = %.2f | Contrast = %.2f | BlackPoint = %.2f | Shadows = %+d\n"
        L"Saturation = %d%% | Vibrance = %d%% | Temp = %dK\n"
        L"Code = GBPLUS:%.2f:%.2f:%.2f:%d:%d:%d:%d\n"
        L"```",
        g_activePresetName.c_str(),
        g_gamma, g_contrast, g_blackPoint, g_shadows,
        g_saturation, g_vibrance, g_temperatureK,
        g_gamma, g_contrast, g_blackPoint, g_shadows, g_saturation, g_vibrance, g_temperatureK);

    if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        size_t bytes = (wcslen(code) + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            void* ptr = GlobalLock(hMem);
            if (ptr) {
                memcpy(ptr, code, bytes);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
        }
        CloseClipboard();
    }
}

void AddRoundedRect(GraphicsPath& path, RectF rc, float radius) {
    if (radius <= 0.5f) {
        path.AddRectangle(rc);
        return;
    }
    float d = radius * 2.0f;
    if (d > rc.Width) d = rc.Width;
    if (d > rc.Height) d = rc.Height;
    path.AddArc(rc.X, rc.Y, d, d, 180, 90);
    path.AddArc(rc.GetRight() - d, rc.Y, d, d, 270, 90);
    path.AddArc(rc.GetRight() - d, rc.GetBottom() - d, d, d, 0, 90);
    path.AddArc(rc.X, rc.GetBottom() - d, d, d, 90, 90);
    path.CloseFigure();
}

void FillRoundRect(Graphics& g, RectF rc, float radius, const Brush& brush) {
    GraphicsPath path;
    AddRoundedRect(path, rc, radius);
    g.FillPath(&brush, &path);
}

void DrawRoundRect(Graphics& g, RectF rc, float radius, const Color& color, float width = 1.0f) {
    GraphicsPath path;
    AddRoundedRect(path, rc, radius);
    Pen pen(color, width);
    g.DrawPath(&pen, &path);
}

void DrawGradientCard(Graphics& g, RectF rc, float radius, const Color& c1, const Color& c2, bool isSelected, bool isHovered) {
    GraphicsPath path;
    AddRoundedRect(path, rc, radius);

    LinearGradientBrush grad(PointF(rc.X, rc.Y), PointF(rc.GetRight(), rc.GetBottom()), c1, c2);
    g.FillPath(&grad, &path);

    RectF botVig(rc.X, rc.Y + rc.Height * 0.45f, rc.Width, rc.Height * 0.55f);
    LinearGradientBrush vigGrad(PointF(rc.X, botVig.Y), PointF(rc.X, rc.GetBottom()),
        Color(0, 12, 12, 16), Color(185, 12, 12, 16));
    g.SetClip(&path);
    g.FillRectangle(&vigGrad, botVig);
    g.ResetClip();

    if (isSelected) {
        DrawRoundRect(g, rc, radius, COL_PINK, 2.2f);

        SolidBrush pinkDot(COL_PINK);
        g.FillEllipse(&pinkDot, rc.X + rc.Width * 0.5f - 4.5f, rc.GetBottom() - 10.0f, 9.0f, 9.0f);
    } else if (isHovered) {
        DrawRoundRect(g, rc, radius, Color(180, 255, 183, 197), 1.5f);
    } else {
        DrawRoundRect(g, rc, radius, Color(65, 255, 255, 255), 1.0f);
    }
}

void DrawBabyPinkToggle(Graphics& g, RectF rc, bool isOn) {
    SolidBrush bgBrush(isOn ? COL_PINK : Color(255, 55, 56, 68));
    FillRoundRect(g, rc, rc.Height * 0.5f, bgBrush);

    float pad = 3.0f;
    float knobD = rc.Height - pad * 2.0f;
    float knobX = isOn ? (rc.GetRight() - knobD - pad) : (rc.X + pad);
    SolidBrush knobBrush(isOn ? Color(255, 24, 18, 22) : Color(255, 210, 212, 220));
    g.FillEllipse(&knobBrush, knobX, rc.Y + pad, knobD, knobD);
}

static HDC s_memDC = NULL;
static HBITMAP s_memBmp = NULL;
static HBITMAP s_oldBmp = NULL;
static int s_bufW = 0, s_bufH = 0;

void RenderAppUI(HDC hdc, int clientW, int clientH) {
    if (clientW <= 0 || clientH <= 0) return;

    if (!s_memDC) {
        s_memDC = CreateCompatibleDC(hdc);
    }
    if (s_bufW != clientW || s_bufH != clientH || !s_memBmp) {
        if (s_memBmp) {
            SelectObject(s_memDC, s_oldBmp);
            DeleteObject(s_memBmp);
        }
        s_bufW = clientW;
        s_bufH = clientH;
        s_memBmp = CreateCompatibleBitmap(hdc, clientW, clientH);
        s_oldBmp = (HBITMAP)SelectObject(s_memDC, s_memBmp);
    }

    Graphics g(s_memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    g_hitRects.clear();
    auto RegisterHit = [&](int id, RectF r) {
        g_hitRects.push_back({ id, r });
    };

    static FontFamily* pFontFam = NULL;
    static Font* pFLogo = NULL;
    static Font* pFHeader = NULL;
    static Font* pFSecTitle = NULL;
    static Font* pFCardTitle = NULL;
    static Font* pFCardSub = NULL;
    static Font* pFNav = NULL;
    static Font* pFNavBold = NULL;
    static Font* pFSmall = NULL;
    static Font* pFBadge = NULL;
    static Font* pFStar = NULL;
    static Font* pFMoon = NULL;
    if (!pFontFam) {
        pFontFam = new FontFamily(L"Segoe UI");
        pFLogo = new Font(pFontFam, 16.0f, FontStyleBold, UnitPixel);
        pFHeader = new Font(pFontFam, 23.0f, FontStyleBold, UnitPixel);
        pFSecTitle = new Font(pFontFam, 13.5f, FontStyleBold, UnitPixel);
        pFCardTitle = new Font(pFontFam, 13.0f, FontStyleBold, UnitPixel);
        pFCardSub = new Font(pFontFam, 11.5f, FontStyleRegular, UnitPixel);
        pFNav = new Font(pFontFam, 13.0f, FontStyleRegular, UnitPixel);
        pFNavBold = new Font(pFontFam, 13.0f, FontStyleBold, UnitPixel);
        pFSmall = new Font(pFontFam, 11.0f, FontStyleRegular, UnitPixel);
        pFBadge = new Font(pFontFam, 10.5f, FontStyleBold, UnitPixel);
        pFStar = new Font(pFontFam, 24.0f, FontStyleBold, UnitPixel);
        pFMoon = new Font(pFontFam, 22.0f, FontStyleBold, UnitPixel);
    }
    Font& fLogo = *pFLogo;
    Font& fHeader = *pFHeader;
    Font& fSecTitle = *pFSecTitle;
    Font& fCardTitle = *pFCardTitle;
    Font& fCardSub = *pFCardSub;
    Font& fNav = *pFNav;
    Font& fNavBold = *pFNavBold;
    Font& fSmall = *pFSmall;
    Font& fBadge = *pFBadge;
    Font& fStar = *pFStar;
    Font& fMoon = *pFMoon;

    static StringFormat sfLeft(0), sfCenter(0), sfRight(0);
    static bool s_sfInit = false;
    if (!s_sfInit) {
        s_sfInit = true;
        sfLeft.SetAlignment(StringAlignmentNear);
        sfLeft.SetLineAlignment(StringAlignmentCenter);
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);
        sfRight.SetAlignment(StringAlignmentFar);
        sfRight.SetLineAlignment(StringAlignmentCenter);
    }

    SolidBrush mainBg(COL_BG_MAIN);
    g.FillRectangle(&mainBg, 0, 0, clientW, clientH);

    float sideW = 210.0f;

    if (!g_bgFrames.empty()) {
        float contentX = sideW;
        float contentW = (float)clientW - sideW;
        float contentH = (float)clientH;

        float bgW = 740.0f;
        float bgH = 508.0f;
        float bgX = contentX + (contentW - bgW) * 0.5f;
        float bgY = (contentH - bgH) * 0.5f + 18.0f;

        UINT fIdx = g_bgCurrentFrame % (UINT)g_bgFrames.size();
        g.DrawImage(g_bgFrames[fIdx], bgX, bgY);

        SolidBrush veilBr(Color(55, 17, 17, 21));
        g.FillRectangle(&veilBr, contentX, 0.0f, contentW, contentH);
    }

    SolidBrush sideBg(COL_BG_SIDEBAR);
    g.FillRectangle(&sideBg, 0.0f, 0.0f, sideW, (float)clientH);
    Pen sideLine(Color(255, 34, 34, 42), 1.0f);
    g.DrawLine(&sideLine, sideW, 0.0f, sideW, (float)clientH);

    SolidBrush brWhite(COL_TEXT_WHITE);
    SolidBrush brPink(COL_PINK);
    SolidBrush brMuted(COL_TEXT_MUTED);
    SolidBrush brDim(COL_TEXT_DIM);

    g.DrawString(L"GammaBoost", -1, &fLogo, PointF(18.0f, 18.0f), &brWhite);
    g.DrawString(L"+", -1, &fLogo, PointF(122.0f, 18.0f), &brPink);

    RectF rcSideIcon(sideW - 34.0f, 19.0f, 18.0f, 16.0f);
    DrawRoundRect(g, rcSideIcon, 3.0f, COL_TEXT_DIM, 1.2f);
    g.DrawLine(&sideLine, rcSideIcon.X + 6.0f, rcSideIcon.Y, rcSideIcon.X + 6.0f, rcSideIcon.GetBottom());

    struct NavItem {
        int hitId;
        ChannelTab tab;
        const wchar_t* iconGlyph;
        const wchar_t* label;
        const wchar_t* rightCount;
        float yOffset;
    };
    NavItem navItems[] = {
        { HIT_TAB_PRESETS,      TAB_PRESETS,      L"\x2726", L"Presets",           L"",   60.0f },
        { HIT_TAB_MANUAL,       TAB_MANUAL,       L"\x2699", L"Manual",            L"",   96.0f },
        { HIT_TAB_EYECARE,      TAB_EYECARE,      L"\x263E", L"Eye Care",          L"",   132.0f },
        { HIT_TAB_FPS,          TAB_FPS_BOOST,    L"\x26A1", L"FPS Channels",      L"8",  168.0f },
        { HIT_TAB_ENHANCEMENTS, TAB_ENHANCEMENTS, L"\x25CE", L"Enhancements",      L"3",  204.0f },
        { HIT_TAB_PROFILES,     TAB_PROFILES,     L"\x265F", L"Game Profiles",     L"",   264.0f },
        { HIT_TAB_DISCORD,      TAB_DISCORD,      L"\x2764", L"Discord & OSS",     L"MIT", 300.0f }
    };

    g.DrawString(L"Library & Config", -1, &fSmall, PointF(18.0f, 244.0f), &brDim);

    for (const auto& item : navItems) {
        RectF rcNav(10.0f, item.yOffset, sideW - 20.0f, 32.0f);
        RegisterHit(item.hitId, rcNav);
        bool active = (g_currentTab == item.tab);
        bool hovered = (g_hoverId == item.hitId);

        if (active) {
            SolidBrush navAct(Color(255, 42, 36, 44));
            FillRoundRect(g, rcNav, 7.0f, navAct);
            DrawRoundRect(g, rcNav, 7.0f, Color(110, 255, 183, 197), 1.0f);
        } else if (hovered) {
            SolidBrush navHov(Color(255, 30, 30, 38));
            FillRoundRect(g, rcNav, 7.0f, navHov);
        }

        SolidBrush& icoBr = active ? brPink : brMuted;
        SolidBrush& txtBr = active ? brWhite : brMuted;
        g.DrawString(item.iconGlyph, -1, &fNavBold, RectF(rcNav.X + 10.0f, rcNav.Y, 22.0f, rcNav.Height), &sfLeft, &icoBr);
        g.DrawString(item.label, -1, active ? &fNavBold : &fNav, RectF(rcNav.X + 34.0f, rcNav.Y, 125.0f, rcNav.Height), &sfLeft, &txtBr);

        if (wcslen(item.rightCount) > 0) {
            g.DrawString(item.rightCount, -1, &fSmall, RectF(rcNav.GetRight() - 40.0f, rcNav.Y, 32.0f, rcNav.Height), &sfRight, &brPink);
        }
    }

    RectF rcSideFooter(12.0f, (float)clientH - 96.0f, sideW - 24.0f, 82.0f);
    SolidBrush footBg(Color(255, 26, 24, 30));
    FillRoundRect(g, rcSideFooter, 8.0f, footBg);
    DrawRoundRect(g, rcSideFooter, 8.0f, Color(60, 255, 183, 197), 1.0f);
    g.DrawString(L"Open Source Edition", -1, &fCardTitle, PointF(rcSideFooter.X + 10.0f, rcSideFooter.Y + 8.0f), &brPink);
    g.DrawString(L"Alt+F9 : Toggle Screen\nAlt+F10: Default Reset\nAlt+Up/Dn: Quick Gamma", -1, &fSmall,
        PointF(rcSideFooter.X + 10.0f, rcSideFooter.Y + 28.0f), &brMuted);

    float contentX = sideW;
    float contentW = (float)clientW - sideW;
    float pillW = 310.0f, pillH = 42.0f;
    float pillX = contentX + (contentW - pillW) * 0.5f;
    float pillY = 10.0f;

    RectF rcTopPill(pillX, pillY, pillW, pillH);
    SolidBrush pillBrush(COL_BG_PILL);
    FillRoundRect(g, rcTopPill, 11.0f, pillBrush);
    DrawRoundRect(g, rcTopPill, 11.0f, Color(255, 52, 52, 64), 1.0f);

    RectF rcBadgeSq(pillX + 6.0f, pillY + 6.0f, 30.0f, 30.0f);
    LinearGradientBrush sqGrad(PointF(rcBadgeSq.X, rcBadgeSq.Y), PointF(rcBadgeSq.GetRight(), rcBadgeSq.GetBottom()),
        Color(255, 120, 74, 92), Color(255, 58, 36, 48));
    FillRoundRect(g, rcBadgeSq, 7.0f, sqGrad);
    DrawRoundRect(g, rcBadgeSq, 7.0f, g_showOnScreen ? COL_PINK : COL_TEXT_DIM, 1.8f);

    std::wstring monLabel = (g_selectedMonitor == 0) ? L"All Displays" : g_monitors[g_selectedMonitor - 1].friendlyName;
    std::wstring topSub = (g_selectedGameProfile > 0 && g_selectedGameProfile < (int)g_gameProfiles.size())
        ? (g_gameProfiles[g_selectedGameProfile].exeName + L" \x00B7 " + monLabel)
        : (L"Default \x00B7 " + monLabel);

    g.DrawString(g_activePresetName.c_str(), -1, &fCardTitle,
        RectF(pillX + 44.0f, pillY + 4.0f, pillW - 92.0f, 18.0f), &sfCenter, &brWhite);
    g.DrawString(topSub.c_str(), -1, &fSmall,
        RectF(pillX + 44.0f, pillY + 21.0f, pillW - 92.0f, 16.0f), &sfCenter, &brMuted);

    RectF rcPowerBtn(rcTopPill.GetRight() - 34.0f, pillY + 8.0f, 26.0f, 26.0f);
    RegisterHit(HIT_TOP_POWER, rcPowerBtn);
    SolidBrush pwrBrush(g_showOnScreen ? COL_PINK : Color(255, 65, 65, 78));
    g.FillEllipse(&pwrBrush, rcPowerBtn);
    Pen pwrIconPen(g_showOnScreen ? Color(255, 24, 18, 22) : COL_TEXT_WHITE, 1.8f);
    float cx = rcPowerBtn.X + 13.0f, cy = rcPowerBtn.Y + 13.0f;
    g.DrawArc(&pwrIconPen, cx - 5.5f, cy - 5.5f, 11.0f, 11.0f, 300.0f, 300.0f);
    g.DrawLine(&pwrIconPen, cx, cy - 6.5f, cx, cy - 1.0f);

    RectF rcMonBtn(rcTopPill.GetRight() + 8.0f, pillY + 4.0f, 34.0f, 34.0f);
    RegisterHit(HIT_TOP_MONITOR, rcMonBtn);
    FillRoundRect(g, rcMonBtn, 8.0f, pillBrush);
    DrawRoundRect(g, rcMonBtn, 8.0f, (g_hoverId == HIT_TOP_MONITOR) ? COL_PINK : Color(255, 52, 52, 64), 1.0f);
    g.DrawString(L"1", -1, &fBadge, rcMonBtn, &sfCenter, &brPink);

    float padX = contentX + 28.0f;
    float innerW = contentW - 56.0f;

    if (g_currentTab == TAB_PRESETS || g_currentTab == TAB_EYECARE) {

        const wchar_t* hdrTitle = (g_currentTab == TAB_PRESETS) ? L"Presets" : L"Eye Care & Comfort";
        g.DrawString(hdrTitle, -1, &fHeader, PointF(padX, 58.0f), &brWhite);

        RectF rcNewBtn(clientW - 148.0f, 58.0f, 120.0f, 32.0f);
        RegisterHit(HIT_BTN_NEW_PRESET, rcNewBtn);
        SolidBrush newBtnBr((g_hoverId == HIT_BTN_NEW_PRESET) ? Color(255, 54, 44, 52) : COL_BG_PILL);
        FillRoundRect(g, rcNewBtn, 16.0f, newBtnBr);
        DrawRoundRect(g, rcNewBtn, 16.0f, COL_PINK, 1.0f);
        g.DrawString(L"+ Custom Tune", -1, &fCardSub, rcNewBtn, &sfCenter, &brPink);

        float cardGap = 14.0f;
        float cardW = (innerW - cardGap * 3.0f) / 4.0f;
        float cardH = 114.0f;

        auto DrawCardItem = [&](int idx, float x, float y) {
            const DisplayPreset& p = g_presets[idx];
            RectF rcCard(x, y, cardW, cardH);
            RegisterHit(HIT_PRESET_CARD_BASE + idx, rcCard);
            bool sel = (g_activePresetIdx == idx);
            bool hov = (g_hoverId == HIT_PRESET_CARD_BASE + idx);

            DrawGradientCard(g, rcCard, 10.0f, p.gradA, p.gradB, sel, hov);

            if (p.iconType == 1) {
                Pen icoPen(Color(230, 255, 255, 255), 2.0f);
                float icx = x + cardW * 0.5f, icy = y + cardH * 0.42f;
                g.DrawEllipse(&icoPen, icx - 10.0f, icy - 10.0f, 20.0f, 20.0f);
                g.DrawLine(&icoPen, icx - 15.0f, icy, icx + 15.0f, icy);
                g.DrawLine(&icoPen, icx, icy - 15.0f, icx, icy + 15.0f);
            } else if (p.iconType == 2) {
                g.DrawString(L"\x2605", -1, &fStar, RectF(x, y + 18.0f, cardW, 36.0f), &sfCenter, &brWhite);
            } else if (p.iconType == 3) {
                g.DrawString(L"\x263E", -1, &fMoon, RectF(x, y + 18.0f, cardW, 36.0f), &sfCenter, &brPink);
            }

            if (!p.badge.empty()) {
                RectF rcB(rcCard.GetRight() - 32.0f, rcCard.Y + 10.0f, 22.0f, 22.0f);
                SolidBrush bBr(Color(190, 255, 183, 197));
                FillRoundRect(g, rcB, 5.0f, bBr);
                SolidBrush darkTxt(Color(255, 24, 18, 22));
                g.DrawString(p.badge.c_str(), -1, &fBadge, rcB, &sfCenter, &darkTxt);
            }

            g.DrawString(p.name.c_str(), -1, &fCardTitle, PointF(x + 12.0f, y + cardH - 42.0f), &brWhite);
            g.DrawString(p.subtitle.c_str(), -1, &fCardSub, PointF(x + 12.0f, y + cardH - 23.0f), &brMuted);
        };

        if (g_currentTab == TAB_PRESETS) {

            float y1 = 104.0f;
            g.DrawString(L"For Dark Scenes", -1, &fSecTitle, PointF(padX, y1), &brWhite);
            for (int i = 0; i < 4; ++i) DrawCardItem(i, padX + i * (cardW + cardGap), y1 + 24.0f);

            float y2 = y1 + 24.0f + cardH + 20.0f;
            g.DrawString(L"For Color", -1, &fSecTitle, PointF(padX, y2), &brWhite);
            g.DrawString(L"For Everyday", -1, &fSecTitle, PointF(padX + 2.0f * (cardW + cardGap), y2), &brWhite);
            for (int i = 0; i < 4; ++i) DrawCardItem(4 + i, padX + i * (cardW + cardGap), y2 + 24.0f);

            float y3 = y2 + 24.0f + cardH + 20.0f;
            g.DrawString(L"My Presets", -1, &fSecTitle, PointF(padX, y3), &brWhite);
            g.DrawString(L"Baby Pink Collection", -1, &fSmall, RectF(padX, y3, innerW, 20.0f), &sfRight, &brPink);
            for (int i = 0; i < 4; ++i) DrawCardItem(8 + i, padX + i * (cardW + cardGap), y3 + 24.0f);
        } else {

            float y1 = 110.0f;
            g.DrawString(L"Blue Light & Warm Rose Filters", -1, &fSecTitle, PointF(padX, y1), &brWhite);
            for (int i = 0; i < 4; ++i) DrawCardItem(12 + i, padX + i * (cardW + cardGap), y1 + 26.0f);
        }
    }
    else if (g_currentTab == TAB_MANUAL) {

        g.DrawString(L"Manual", -1, &fHeader, PointF(padX, 56.0f), &brWhite);

        float inspectorW = 290.0f;
        float previewW = innerW - inspectorW - 18.0f;
        float previewH = (float)clientH - 210.0f;
        RectF rcPreview(padX, 96.0f, previewW, previewH);

        GraphicsPath prevClip;
        AddRoundedRect(prevClip, rcPreview, 10.0f);
        g.SetClip(&prevClip);

        float splitX = rcPreview.X + rcPreview.Width * g_splitPos;

        if (g_pPreviewOrig && g_pPreviewAdj) {
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

            RectF leftClip(rcPreview.X, rcPreview.Y, splitX - rcPreview.X, rcPreview.Height);
            g.SetClip(leftClip, CombineModeIntersect);
            g.DrawImage(g_pPreviewOrig, rcPreview);

            g.SetClip(&prevClip);
            RectF rightClip(splitX, rcPreview.Y, rcPreview.GetRight() - splitX, rcPreview.Height);
            g.SetClip(rightClip, CombineModeIntersect);
            g.DrawImage(g_pPreviewAdj, rcPreview);
        }

        g.SetClip(&prevClip);

        RectF rcOrigPill(rcPreview.X + 12.0f, rcPreview.Y + 12.0f, 64.0f, 24.0f);
        RectF rcAdjPill(rcPreview.GetRight() - 78.0f, rcPreview.Y + 12.0f, 66.0f, 24.0f);
        SolidBrush darkBadge(Color(195, 20, 20, 26));
        FillRoundRect(g, rcOrigPill, 6.0f, darkBadge);
        FillRoundRect(g, rcAdjPill, 6.0f, darkBadge);
        g.DrawString(L"Original", -1, &fSmall, rcOrigPill, &sfCenter, &brWhite);
        g.DrawString(L"Adjusted", -1, &fSmall, rcAdjPill, &sfCenter, &brPink);

        Pen splitPen(Color(240, 255, 255, 255), 1.6f);
        g.DrawLine(&splitPen, splitX, rcPreview.Y, splitX, rcPreview.GetBottom());

        float handleY = rcPreview.Y + rcPreview.Height * 0.48f;
        RectF rcSplitHandle(splitX - 15.0f, handleY - 15.0f, 30.0f, 30.0f);
        RegisterHit(HIT_SPLIT_HANDLE, RectF(rcPreview.X, rcPreview.Y, rcPreview.Width, rcPreview.Height - 48.0f));
        SolidBrush handleBr(COL_TEXT_WHITE);
        g.FillEllipse(&handleBr, rcSplitHandle);
        SolidBrush darkIco(Color(255, 20, 20, 25));
        g.DrawString(L"<>", -1, &fBadge, rcSplitHandle, &sfCenter, &darkIco);

        float sceneBarW = 340.0f, sceneBarH = 30.0f;
        RectF rcSceneBar(rcPreview.X + (rcPreview.Width - sceneBarW) * 0.5f, rcPreview.GetBottom() - 42.0f, sceneBarW, sceneBarH);
        FillRoundRect(g, rcSceneBar, 8.0f, darkBadge);
        const wchar_t* scenes[] = { L"Custom", L"Night", L"Cinematic", L"Nature", L"Interior" };
        float sw = sceneBarW / 5.0f;
        for (int i = 0; i < 5; ++i) {
            RectF rcs(rcSceneBar.X + i * sw, rcSceneBar.Y, sw, sceneBarH);
            RegisterHit(HIT_SCENE_BASE + i, rcs);
            if (g_previewScene == i) {
                SolidBrush actScene(Color(255, 58, 46, 56));
                FillRoundRect(g, rcs, 7.0f, actScene);
                g.DrawString(scenes[i], -1, &fBadge, rcs, &sfCenter, &brPink);
            } else {
                g.DrawString(scenes[i], -1, &fSmall, rcs, &sfCenter, &brMuted);
            }
        }

        g.ResetClip();
        DrawRoundRect(g, rcPreview, 10.0f, COL_BORDER_SUBTLE, 1.0f);

        RectF rcEditBar(padX, rcPreview.GetBottom() + 10.0f, previewW, 42.0f);
        SolidBrush panelBr(COL_BG_PANEL);
        FillRoundRect(g, rcEditBar, 8.0f, panelBr);
        g.DrawString(L"Editing", -1, &fCardTitle, PointF(rcEditBar.X + 14.0f, rcEditBar.Y + 6.0f), &brWhite);
        g.DrawString(g_activePresetName.c_str(), -1, &fSmall, PointF(rcEditBar.X + 14.0f, rcEditBar.Y + 23.0f), &brPink);

        RectF rcRevert(rcEditBar.GetRight() - 74.0f, rcEditBar.Y + 8.0f, 62.0f, 26.0f);
        RegisterHit(HIT_BTN_REVERT, rcRevert);
        FillRoundRect(g, rcRevert, 6.0f, pillBrush);
        g.DrawString(L"Revert", -1, &fSmall, rcRevert, &sfCenter, &brWhite);

        RectF rcShowBar(padX, rcEditBar.GetBottom() + 8.0f, previewW, 42.0f);
        FillRoundRect(g, rcShowBar, 8.0f, panelBr);
        g.DrawString(L"Show on screen", -1, &fCardTitle, PointF(rcShowBar.X + 14.0f, rcShowBar.Y + 6.0f), &brWhite);
        g.DrawString(L"Display shows your gamma & color vibrance changes live as you make them.", -1, &fSmall,
            PointF(rcShowBar.X + 14.0f, rcShowBar.Y + 23.0f), &brMuted);

        RectF rcSwitch(rcShowBar.GetRight() - 54.0f, rcShowBar.Y + 11.0f, 40.0f, 20.0f);
        RegisterHit(HIT_TOGGLE_SHOW_SCREEN, rcSwitch);
        DrawBabyPinkToggle(g, rcSwitch, g_showOnScreen);

        RectF rcInsp(rcPreview.GetRight() + 18.0f, 56.0f, inspectorW, (float)clientH - 72.0f);
        FillRoundRect(g, rcInsp, 12.0f, panelBr);
        DrawRoundRect(g, rcInsp, 12.0f, COL_BORDER_SUBTLE, 1.0f);

        RectF rcSeg(rcInsp.X + 12.0f, rcInsp.Y + 12.0f, inspectorW - 24.0f, 30.0f);
        SolidBrush segBg(Color(255, 18, 18, 22));
        FillRoundRect(g, rcSeg, 8.0f, segBg);
        const wchar_t* segNames[] = { L"Adjust", L"Curves", L"Channels" };
        float segW = rcSeg.Width / 3.0f;
        for (int i = 0; i < 3; ++i) {
            RectF rci(rcSeg.X + i * segW, rcSeg.Y, segW, rcSeg.Height);
            RegisterHit(HIT_INSPECTOR_TAB + i, rci);
            if (g_inspectorSubTab == i) {
                FillRoundRect(g, rci, 7.0f, pillBrush);
                g.DrawString(segNames[i], -1, &fBadge, rci, &sfCenter, &brPink);
            } else {
                g.DrawString(segNames[i], -1, &fSmall, rci, &sfCenter, &brMuted);
            }
        }

        RectF rcHist(rcInsp.X + 12.0f, rcSeg.GetBottom() + 10.0f, inspectorW - 24.0f, 76.0f);
        FillRoundRect(g, rcHist, 6.0f, segBg);
        Pen gridP(Color(45, 255, 255, 255), 1.0f);
        for (int i = 1; i < 4; ++i) {
            float gx = rcHist.X + (rcHist.Width * i) / 4.0f;
            g.DrawLine(&gridP, gx, rcHist.Y, gx, rcHist.GetBottom());
        }

        g.DrawLine(&gridP, rcHist.X, rcHist.GetBottom(), rcHist.GetRight(), rcHist.Y);

        PointF histPts[9] = {
            PointF(rcHist.X, rcHist.GetBottom()),
            PointF(rcHist.X + rcHist.Width * 0.15f, rcHist.GetBottom() - 18.0f),
            PointF(rcHist.X + rcHist.Width * 0.30f, rcHist.GetBottom() - 56.0f),
            PointF(rcHist.X + rcHist.Width * 0.45f, rcHist.GetBottom() - 24.0f),
            PointF(rcHist.X + rcHist.Width * 0.62f, rcHist.GetBottom() - 62.0f),
            PointF(rcHist.X + rcHist.Width * 0.75f, rcHist.GetBottom() - 16.0f),
            PointF(rcHist.X + rcHist.Width * 0.88f, rcHist.GetBottom() - 34.0f),
            PointF(rcHist.GetRight(), rcHist.GetBottom() - 8.0f),
            PointF(rcHist.GetRight(), rcHist.GetBottom())
        };
        SolidBrush histFill(Color(48, 255, 183, 197));
        g.FillPolygon(&histFill, histPts, 9);

        Pen curvePen(COL_PINK, 2.2f);
        PointF prevPt(rcHist.X, rcHist.GetBottom());
        for (int i = 1; i <= 32; ++i) {
            double nx = (double)i / 32.0;
            double ny = EvaluateChannelCurve(nx, 1);
            PointF curPt(rcHist.X + (float)(nx * rcHist.Width), rcHist.GetBottom() - (float)(ny * rcHist.Height));
            g.DrawLine(&curvePen, prevPt, curPt);
            prevPt = curPt;
        }
        DrawRoundRect(g, rcHist, 6.0f, COL_BORDER_SUBTLE, 1.0f);

        auto DrawInspectorSlider = [&](int sliderIdx, const wchar_t* label, const wchar_t* valStr, float normPos, float yPos) {
            g.DrawString(label, -1, &fCardSub, PointF(rcInsp.X + 14.0f, yPos), &brWhite);
            g.DrawString(valStr, -1, &fBadge, RectF(rcInsp.X + 14.0f, yPos, inspectorW - 28.0f, 16.0f), &sfRight, &brWhite);

            RectF rcTrack(rcInsp.X + 16.0f, yPos + 22.0f, inspectorW - 32.0f, 4.0f);
            RectF rcHit(rcTrack.X - 6.0f, yPos + 12.0f, rcTrack.Width + 12.0f, 22.0f);
            RegisterHit(HIT_SLIDER_BASE + sliderIdx, rcHit);

            SolidBrush trackBg(Color(255, 58, 58, 70));
            FillRoundRect(g, rcTrack, 2.0f, trackBg);

            normPos = std::clamp(normPos, 0.0f, 1.0f);
            RectF rcFill(rcTrack.X, rcTrack.Y, rcTrack.Width * normPos, rcTrack.Height);
            FillRoundRect(g, rcFill, 2.0f, brPink);

            float thumbX = rcTrack.X + rcTrack.Width * normPos;
            float thumbY = rcTrack.Y + 2.0f;
            g.FillEllipse(&brWhite, thumbX - 6.5f, thumbY - 6.5f, 13.0f, 13.0f);
        };

        float sy = rcHist.GetBottom() + 14.0f;
        g.DrawString(L"\x2600  Light", -1, &fCardTitle, PointF(rcInsp.X + 14.0f, sy), &brWhite);
        g.FillEllipse(&brPink, rcInsp.GetRight() - 28.0f, sy + 2.0f, 14.0f, 14.0f);

        wchar_t vBuf[32];
        swprintf_s(vBuf, L"%.2f", g_gamma);
        DrawInspectorSlider(0, L"Gamma", vBuf, (float)((g_gamma - 0.50) / 2.00), sy + 24.0f);

        swprintf_s(vBuf, L"%.2f", g_contrast);
        DrawInspectorSlider(1, L"Contrast", vBuf, (float)((g_contrast - 0.50) / 1.00), sy + 62.0f);

        swprintf_s(vBuf, L"%+.2f", g_blackPoint);
        DrawInspectorSlider(2, L"Black Point", vBuf, (float)((g_blackPoint + 0.20) / 0.40), sy + 100.0f);

        swprintf_s(vBuf, L"%+d", g_shadows);
        DrawInspectorSlider(3, L"Shadows", vBuf, (float)((g_shadows + 50) / 100.0), sy + 138.0f);

        float cySec = sy + 182.0f;
        g.DrawString(L"\x2666  Color", -1, &fCardTitle, PointF(rcInsp.X + 14.0f, cySec), &brWhite);
        g.FillEllipse(&brPink, rcInsp.GetRight() - 28.0f, cySec + 2.0f, 14.0f, 14.0f);

        swprintf_s(vBuf, L"%d%%", g_saturation);
        DrawInspectorSlider(4, L"Saturation", vBuf, (float)(g_saturation / 200.0f), cySec + 24.0f);

        swprintf_s(vBuf, L"%d%%", g_vibrance);
        DrawInspectorSlider(5, L"Vibrance", vBuf, (float)(g_vibrance / 250.0f), cySec + 62.0f);

        swprintf_s(vBuf, L"%d K", g_temperatureK);
        DrawInspectorSlider(6, L"Temperature", vBuf, (float)((g_temperatureK - 3500.0f) / 5500.0f), cySec + 100.0f);

        float ey = cySec + 144.0f;
        RectF rcBw(rcInsp.X + 14.0f, ey, inspectorW - 28.0f, 22.0f);
        RegisterHit(HIT_EXTRA_BW, rcBw);
        g.DrawString(g_bwMode ? L"\x25CF  Black & White Mode: ON" : L"\x25CB  Black & White", -1, &fCardSub,
            PointF(rcBw.X, rcBw.Y), g_bwMode ? &brPink : &brMuted);

        RectF rcCb(rcInsp.X + 14.0f, ey + 24.0f, inspectorW - 28.0f, 22.0f);
        RegisterHit(HIT_EXTRA_CB, rcCb);
        const wchar_t* cbLabels[] = { L"\x25CB  Colorblind Assist: Off", L"\x25CF  Colorblind: Protanopia", L"\x25CF  Colorblind: Deuteranopia" };
        g.DrawString(cbLabels[g_colorblindMode], -1, &fCardSub,
            PointF(rcCb.X, rcCb.Y), g_colorblindMode ? &brPink : &brMuted);
    }
    else if (g_currentTab == TAB_FPS_BOOST) {

        g.DrawString(L"FPS Channels & Latency", -1, &fHeader, PointF(padX, 58.0f), &brWhite);
        g.DrawString(L"Select which hardware & OS performance channels stay active while you play. Click any card to toggle it.",
            -1, &fCardSub, PointF(padX, 88.0f), &brMuted);

        RectF rcFlushBtn(clientW - 310.0f, 58.0f, 145.0f, 32.0f);
        RegisterHit(HIT_FPS_FLUSH_RAM, rcFlushBtn);
        FillRoundRect(g, rcFlushBtn, 16.0f, pillBrush);
        DrawRoundRect(g, rcFlushBtn, 16.0f, COL_PINK, 1.0f);
        g.DrawString(L"Flush Standby RAM", -1, &fCardSub, rcFlushBtn, &sfCenter, &brPink);

        RectF rcAllBtn(clientW - 154.0f, 58.0f, 126.0f, 32.0f);
        RegisterHit(HIT_FPS_ALL_TOGGLE, rcAllBtn);
        FillRoundRect(g, rcAllBtn, 16.0f, brPink);
        SolidBrush darkTxt(Color(255, 24, 18, 22));
        g.DrawString(L"Enable All 8", -1, &fBadge, rcAllBtn, &sfCenter, &darkTxt);

        float cardGap = 14.0f;
        float cardW = (innerW - cardGap * 3.0f) / 4.0f;
        float cardH = 112.0f;

        g.DrawString(L"Frame Pacing & Input Channels", -1, &fSecTitle, PointF(padX, 114.0f), &brWhite);
        g.DrawString(L"System & Memory Channels", -1, &fSecTitle, PointF(padX, 114.0f + cardH + 34.0f), &brWhite);

        for (size_t i = 0; i < g_fpsCards.size(); ++i) {
            int row = (int)(i / 4);
            int col = (int)(i % 4);
            float x = padX + col * (cardW + cardGap);
            float y = (row == 0) ? 136.0f : (136.0f + cardH + 34.0f);

            const FpsChannelCard& fc = g_fpsCards[i];
            RectF rcCard(x, y, cardW, cardH);
            RegisterHit(HIT_FPS_CARD_BASE + (int)i, rcCard);

            bool hov = (g_hoverId == HIT_FPS_CARD_BASE + (int)i);
            Color cA = fc.enabled ? fc.gradA : Color(255, 38, 38, 46);
            Color cB = fc.enabled ? fc.gradB : Color(255, 24, 24, 30);
            DrawGradientCard(g, rcCard, 10.0f, cA, cB, fc.enabled, hov);

            RectF rcTag(x + 12.0f, y + 10.0f, 52.0f, 20.0f);
            SolidBrush tagBr(Color(160, 20, 20, 26));
            FillRoundRect(g, rcTag, 5.0f, tagBr);
            g.DrawString(fc.tag.c_str(), -1, &fBadge, rcTag, &sfCenter, fc.enabled ? &brPink : &brMuted);

            RectF rcSw(rcCard.GetRight() - 46.0f, y + 10.0f, 34.0f, 18.0f);
            DrawBabyPinkToggle(g, rcSw, fc.enabled);

            g.DrawString(fc.name.c_str(), -1, &fCardTitle, PointF(x + 12.0f, y + cardH - 56.0f), &brWhite);
            g.DrawString(fc.subtitle.c_str(), -1, &fSmall, RectF(x + 12.0f, y + cardH - 36.0f, cardW - 24.0f, 32.0f), &sfLeft, &brMuted);
        }

        float statY = 136.0f + (cardH + 34.0f) * 2.0f + 2.0f;
        float statH = 110.0f;
        RectF rcStatBox(padX, statY, innerW, statH);
        SolidBrush statBg(COL_BG_PANEL);
        FillRoundRect(g, rcStatBox, 10.0f, statBg);
        DrawRoundRect(g, rcStatBox, 10.0f, Color(70, 255, 183, 197), 1.0f);

        float colW = innerW / 3.0f;

        RectF rcCol1(padX + 16.0f, statY + 12.0f, colW - 32.0f, statH - 24.0f);
        g.DrawString(L"Hardware Timer Resolution", -1, &fCardTitle, PointF(rcCol1.X, rcCol1.Y), &brWhite);
        g.DrawString(g_timerBoostActive ? L"0.50 ms - Maximum Precision Locked" : L"15.6 ms - Windows Default", -1, &fBadge, PointF(rcCol1.X, rcCol1.Y + 22.0f), &brPink);
        g.DrawString(L"Forces Windows scheduler to tick at 0.50ms to minimize frame jitter and input latency in fast-paced shooters.", -1, &fSmall, RectF(rcCol1.X, rcCol1.Y + 44.0f, rcCol1.Width, 42.0f), &sfLeft, &brMuted);

        RectF rcCol2(padX + colW + 16.0f, statY + 12.0f, colW - 32.0f, statH - 24.0f);
        g.DrawString(L"Game CPU / GPU Scheduling", -1, &fCardTitle, PointF(rcCol2.X, rcCol2.Y), &brWhite);
        g.DrawString(L"HIGH_PRIORITY_CLASS & GPU Boost Active", -1, &fBadge, PointF(rcCol2.X, rcCol2.Y + 22.0f), &brPink);
        g.DrawString(L"Grants game process high CPU quantum priority and disables Intel/AMD CPU core power throttling during gameplay.", -1, &fSmall, RectF(rcCol2.X, rcCol2.Y + 44.0f, rcCol2.Width, 42.0f), &sfLeft, &brMuted);

        RectF rcCol3(padX + colW * 2.0f + 16.0f, statY + 12.0f, colW - 32.0f, statH - 24.0f);
        g.DrawString(L"Standby Memory Management", -1, &fCardTitle, PointF(rcCol3.X, rcCol3.Y), &brWhite);
        wchar_t memStatus[64];
        swprintf_s(memStatus, L"Auto-Trimming Active (+%zu MB freed)", g_lastFreedMB);
        g.DrawString(memStatus, -1, &fBadge, PointF(rcCol3.X, rcCol3.Y + 22.0f), &brPink);
        g.DrawString(L"Cleans Windows standby list cache automatically before memory pressure causes micro-stutters in open worlds.", -1, &fSmall, RectF(rcCol3.X, rcCol3.Y + 44.0f, rcCol3.Width, 42.0f), &sfLeft, &brMuted);

        MEMORYSTATUSEX mem = { sizeof(MEMORYSTATUSEX) };
        GlobalMemoryStatusEx(&mem);
        DWORD availMB = (DWORD)(mem.ullAvailPhys / (1024 * 1024));
        int activeCount = 0;
        for (const auto& c : g_fpsCards) if (c.enabled) activeCount++;

        RectF rcTele(padX, (float)clientH - 68.0f, innerW, 44.0f);
        SolidBrush teleBr(COL_BG_PANEL);
        FillRoundRect(g, rcTele, 10.0f, teleBr);
        DrawRoundRect(g, rcTele, 10.0f, Color(80, 255, 183, 197), 1.0f);

        wchar_t teleStr[256];
        swprintf_s(teleStr, L"Active FPS Channels: %d / 8   \x00B7   Timer Resolution: %s   \x00B7   Available RAM: %u MB   \x00B7   Last Standby Flush: +%zu MB",
            activeCount, g_timerBoostActive ? L"0.50 ms (Locked)" : L"15.6 ms (Default)", availMB, g_lastFreedMB);
        g.DrawString(teleStr, -1, &fCardTitle, rcTele, &sfCenter, &brPink);
    }
    else if (g_currentTab == TAB_ENHANCEMENTS) {

        g.DrawString(L"Game Enhancements", -1, &fHeader, PointF(padX, 58.0f), &brWhite);
        g.DrawString(L"Visual assistance & recoil stabilization overlays. All tools run externally via low-overhead desktop overlays.",
            -1, &fCardSub, PointF(padX, 88.0f), &brMuted);

        RectF rcStatusPill(clientW - 210.0f, 58.0f, 182.0f, 32.0f);
        FillRoundRect(g, rcStatusPill, 16.0f, pillBrush);
        DrawRoundRect(g, rcStatusPill, 16.0f, COL_PINK, 1.0f);
        g.DrawString(L"\x25CF  Baby-Pink Engine Ready", -1, &fCardSub, rcStatusPill, &sfCenter, &brPink);

        SolidBrush recBr(COL_BG_PANEL);
        float cardH = 118.0f;
        float cardGap = 16.0f;

        float yAi = 126.0f;
        RectF rcAiBar(padX, yAi, innerW, cardH);
        FillRoundRect(g, rcAiBar, 10.0f, recBr);
        DrawRoundRect(g, rcAiBar, 10.0f, g_aiVisionEnabled ? COL_PINK : COL_BORDER_SUBTLE, g_aiVisionEnabled ? 1.5f : 1.0f);

        g.DrawString(L"AI Vision Target Box Overlay (Player & Driver Detector)", -1, &fCardTitle, PointF(rcAiBar.X + 16.0f, rcAiBar.Y + 12.0f), &brWhite);
        RectF rcAiKey(rcAiBar.X + 440.0f, rcAiBar.Y + 12.0f, 62.0f, 20.0f);
        SolidBrush keyBr(Color(180, 24, 18, 22));
        FillRoundRect(g, rcAiKey, 4.0f, keyBr);
        DrawRoundRect(g, rcAiKey, 4.0f, COL_PINK, 1.0f);
        g.DrawString(L"Alt+F6", -1, &fBadge, rcAiKey, &sfCenter, &brPink);

        RectF rcAiSw(rcAiBar.GetRight() - 56.0f, rcAiBar.Y + 12.0f, 40.0f, 20.0f);
        RegisterHit(HIT_AI_TOGGLE, rcAiSw);
        DrawBabyPinkToggle(g, rcAiSw, g_aiVisionEnabled);

        g.DrawString(L"Scans screen center at 50 Hz & tracks player silhouettes, back-of-head runners, and vehicle drivers with Baby-Pink bounding boxes.",
            -1, &fSmall, RectF(rcAiBar.X + 16.0f, rcAiBar.Y + 38.0f, innerW - 190.0f, 32.0f), &sfLeft, &brMuted);

        RectF rcCrossMode(rcAiBar.GetRight() - 170.0f, rcAiBar.Y + 40.0f, 100.0f, 28.0f);
        RegisterHit(HIT_AI_CROSSHAIR, rcCrossMode);
        FillRoundRect(g, rcCrossMode, 7.0f, pillBrush);
        DrawRoundRect(g, rcCrossMode, 7.0f, COL_PINK, 1.0f);
        g.DrawString(g_aiShowCrosshair ? L"Box + Cross" : L"Box Only", -1, &fBadge, rcCrossMode, &sfCenter, &brPink);

        g.DrawString(L"Detection Profile:", -1, &fSmall, PointF(rcAiBar.X + 16.0f, rcAiBar.Y + 80.0f), &brMuted);
        const wchar_t* aiSensLabels[3] = { L"Strict (75%)", L"Balanced (55%)", L"Max Spot (42%)" };
        float aiStartX = rcAiBar.X + 126.0f;
        for (int a = 0; a < 3; ++a) {
            RectF rcA(aiStartX + a * 104.0f, rcAiBar.Y + 74.0f, 98.0f, 28.0f);
            RegisterHit(HIT_AI_SENS_1 + a, rcA);
            bool isSel = (g_aiSensitivity == a);
            SolidBrush aBr(isSel ? Color(255, 62, 44, 54) : COL_BG_PILL);
            FillRoundRect(g, rcA, 7.0f, aBr);
            DrawRoundRect(g, rcA, 7.0f, isSel ? COL_PINK : Color(255, 52, 52, 64), 1.0f);
            g.DrawString(aiSensLabels[a], -1, isSel ? &fBadge : &fSmall, rcA, &sfCenter, isSel ? &brPink : &brMuted);
        }

        g.DrawString(L"50 Hz Scan \x00B7 Momentum vx,vy \x00B7 Driver Lock", -1, &fSmall,
            PointF(rcAiBar.GetRight() - 280.0f, rcAiBar.Y + 80.0f), &brDim);

        float yRec = yAi + cardH + cardGap;
        RectF rcRecoilBar(padX, yRec, innerW, cardH);
        FillRoundRect(g, rcRecoilBar, 10.0f, recBr);
        DrawRoundRect(g, rcRecoilBar, 10.0f, g_recoilPullEnabled ? COL_PINK : COL_BORDER_SUBTLE, g_recoilPullEnabled ? 1.5f : 1.0f);

        g.DrawString(L"Steady Aim Pull (Recoil Stabilizer)", -1, &fCardTitle, PointF(rcRecoilBar.X + 16.0f, rcRecoilBar.Y + 12.0f), &brWhite);
        RectF rcRecKey(rcRecoilBar.X + 270.0f, rcRecoilBar.Y + 12.0f, 62.0f, 20.0f);
        FillRoundRect(g, rcRecKey, 4.0f, keyBr);
        DrawRoundRect(g, rcRecKey, 4.0f, COL_PINK, 1.0f);
        g.DrawString(L"Alt+F8", -1, &fBadge, rcRecKey, &sfCenter, &brPink);

        RectF rcRecSw(rcRecoilBar.GetRight() - 56.0f, rcRecoilBar.Y + 12.0f, 40.0f, 20.0f);
        RegisterHit(HIT_RECOIL_TOGGLE, rcRecSw);
        DrawBabyPinkToggle(g, rcRecSw, g_recoilPullEnabled);

        g.DrawString(L"Gently pulls aim downward while holding Left Click in-game for smoother automatic spray control and steady grouping.",
            -1, &fSmall, RectF(rcRecoilBar.X + 16.0f, rcRecoilBar.Y + 38.0f, innerW - 190.0f, 32.0f), &sfLeft, &brMuted);

        RectF rcMode(rcRecoilBar.GetRight() - 170.0f, rcRecoilBar.Y + 40.0f, 100.0f, 28.0f);
        RegisterHit(HIT_RECOIL_MODE_ADS, rcMode);
        FillRoundRect(g, rcMode, 7.0f, pillBrush);
        DrawRoundRect(g, rcMode, 7.0f, COL_PINK, 1.0f);
        g.DrawString(g_recoilRequireADS ? L"ADS + LMB" : L"LMB Only", -1, &fBadge, rcMode, &sfCenter, &brPink);

        g.DrawString(L"Pull Strength:", -1, &fSmall, PointF(rcRecoilBar.X + 16.0f, rcRecoilBar.Y + 80.0f), &brMuted);
        const wchar_t* strLabels[6] = { L"1 px", L"2 px", L"3 px", L"3.5 px", L"4 px", L"5 px" };
        float pillStartX = rcRecoilBar.X + 110.0f;
        for (int s = 0; s < 6; ++s) {
            RectF rcS(pillStartX + s * 58.0f, rcRecoilBar.Y + 74.0f, 53.0f, 28.0f);
            RegisterHit(HIT_RECOIL_STR_1 + s, rcS);
            bool isSel = (std::abs(g_recoilPullStrength - RECOIL_STEPS[s]) < 0.15);
            SolidBrush sBr(isSel ? Color(255, 62, 44, 54) : COL_BG_PILL);
            FillRoundRect(g, rcS, 7.0f, sBr);
            DrawRoundRect(g, rcS, 7.0f, isSel ? COL_PINK : Color(255, 52, 52, 64), 1.0f);
            g.DrawString(strLabels[s], -1, isSel ? &fBadge : &fSmall, rcS, &sfCenter, isSel ? &brPink : &brMuted);
        }

        g.DrawString(L"Sub-Pixel Precision \x00B7 55 Hz Recoil Engine", -1, &fSmall,
            PointF(rcRecoilBar.GetRight() - 260.0f, rcRecoilBar.Y + 80.0f), &brDim);

        float yZoom = yRec + cardH + cardGap;
        RectF rcZoomBar(padX, yZoom, innerW, cardH);
        FillRoundRect(g, rcZoomBar, 10.0f, recBr);
        DrawRoundRect(g, rcZoomBar, 10.0f, g_wheelZoomEnabled ? COL_PINK : COL_BORDER_SUBTLE, g_wheelZoomEnabled ? 1.5f : 1.0f);

        wchar_t zoomTitle[128];
        swprintf_s(zoomTitle, L"Eagle-Eye Center Zoom (Instant Scope Zoom)   \x00B7   Current: %.2fx", g_currentZoomLevel);
        g.DrawString(zoomTitle, -1, &fCardTitle, PointF(rcZoomBar.X + 16.0f, rcZoomBar.Y + 12.0f), &brWhite);
        RectF rcZoomKey(rcZoomBar.X + 530.0f, rcZoomBar.Y + 12.0f, 62.0f, 20.0f);
        FillRoundRect(g, rcZoomKey, 4.0f, keyBr);
        DrawRoundRect(g, rcZoomKey, 4.0f, COL_PINK, 1.0f);
        g.DrawString(L"Alt+F7", -1, &fBadge, rcZoomKey, &sfCenter, &brPink);

        RectF rcZoomSw(rcZoomBar.GetRight() - 56.0f, rcZoomBar.Y + 12.0f, 40.0f, 20.0f);
        RegisterHit(HIT_ZOOM_TOGGLE, rcZoomSw);
        DrawBabyPinkToggle(g, rcZoomSw, g_wheelZoomEnabled);

        g.DrawString(L"Scroll Mouse Wheel UP to zoom into screen center, Scroll DOWN to zoom out without opening sniper scope.",
            -1, &fSmall, RectF(rcZoomBar.X + 16.0f, rcZoomBar.Y + 38.0f, innerW - 190.0f, 32.0f), &sfLeft, &brMuted);

        const wchar_t* zModes[3] = { L"Wheel Always", L"ADS + Wheel", L"Alt + Wheel" };
        RectF rcZMode(rcZoomBar.GetRight() - 170.0f, rcZoomBar.Y + 40.0f, 100.0f, 28.0f);
        RegisterHit(HIT_ZOOM_MODE, rcZMode);
        FillRoundRect(g, rcZMode, 7.0f, pillBrush);
        DrawRoundRect(g, rcZMode, 7.0f, COL_PINK, 1.0f);
        g.DrawString(zModes[g_wheelZoomMode], -1, &fBadge, rcZMode, &sfCenter, &brPink);

        g.DrawString(L"Zoom Level:", -1, &fSmall, PointF(rcZoomBar.X + 16.0f, rcZoomBar.Y + 80.0f), &brMuted);
        const wchar_t* zLabels[5] = { L"1.0x", L"1.5x", L"2.0x", L"2.5x", L"3.0x" };
        float zStartX = rcZoomBar.X + 104.0f;
        for (int z = 0; z < 5; ++z) {
            RectF rcZ(zStartX + z * 58.0f, rcZoomBar.Y + 74.0f, 53.0f, 28.0f);
            RegisterHit(HIT_ZOOM_STEP_1 + z, rcZ);
            bool isSel = (std::abs(g_currentZoomLevel - ZOOM_PRESET_STEPS[z]) < 0.13f);
            SolidBrush zBr(isSel ? Color(255, 62, 44, 54) : COL_BG_PILL);
            FillRoundRect(g, rcZ, 7.0f, zBr);
            DrawRoundRect(g, rcZ, 7.0f, isSel ? COL_PINK : Color(255, 52, 52, 64), 1.0f);
            g.DrawString(zLabels[z], -1, isSel ? &fBadge : &fSmall, rcZ, &sfCenter, isSel ? &brPink : &brMuted);
        }

        g.DrawString(L"Hardware Bilinear Scaling \x00B7 Zero Latency", -1, &fSmall,
            PointF(rcZoomBar.GetRight() - 270.0f, rcZoomBar.Y + 80.0f), &brDim);

        RectF rcEnhFoot(padX, (float)clientH - 68.0f, innerW, 44.0f);
        FillRoundRect(g, rcEnhFoot, 10.0f, recBr);
        DrawRoundRect(g, rcEnhFoot, 10.0f, Color(80, 255, 183, 197), 1.0f);
        g.DrawString(L"Alt+F6: Toggle AI Vision   \x00B7   Alt+F8: Toggle Steady Aim   \x00B7   Alt+F7: Toggle Center Zoom   \x00B7   100% External Overlay Safe",
            -1, &fCardTitle, rcEnhFoot, &sfCenter, &brPink);
    }
    else if (g_currentTab == TAB_PROFILES) {

        g.DrawString(L"Game Profiles", -1, &fHeader, PointF(padX, 58.0f), &brWhite);
        g.DrawString(L"Assign a custom Gamma & Vibrance preset to any game (.exe) on your PC. Activates automatically when the game runs.",
            -1, &fCardSub, PointF(padX, 88.0f), &brMuted);

        RectF rcAddExe(clientW - 325.0f, 58.0f, 145.0f, 32.0f);
        RegisterHit(HIT_PROF_ADD_EXE, rcAddExe);
        FillRoundRect(g, rcAddExe, 16.0f, brPink);
        SolidBrush darkTxt(Color(255, 24, 18, 22));
        g.DrawString(L"+ Add Game (.exe)", -1, &fBadge, rcAddExe, &sfCenter, &darkTxt);

        RectF rcAddRun(clientW - 170.0f, 58.0f, 142.0f, 32.0f);
        RegisterHit(HIT_PROF_ADD_RUN, rcAddRun);
        FillRoundRect(g, rcAddRun, 16.0f, pillBrush);
        DrawRoundRect(g, rcAddRun, 16.0f, COL_PINK, 1.0f);
        g.DrawString(L"+ Running Game", -1, &fCardSub, rcAddRun, &sfCenter, &brPink);

        float cardGap = 14.0f;
        float cardW = (innerW - cardGap * 3.0f) / 4.0f;
        float cardH = 124.0f;

        for (size_t i = 0; i < g_gameProfiles.size(); ++i) {
            int row = (int)(i / 4);
            int col = (int)(i % 4);
            float x = padX + col * (cardW + cardGap);
            float y = 132.0f + row * (cardH + cardGap);

            const GameProfileEntry& gp = g_gameProfiles[i];
            const DisplayPreset& linkedP = g_presets[std::clamp(gp.presetIndex, 0, (int)g_presets.size() - 1)];

            RectF rcCard(x, y, cardW, cardH);
            RegisterHit(HIT_PROF_CARD_BASE + (int)i, rcCard);
            bool sel = (g_selectedGameProfile == (int)i);
            bool hov = (g_hoverId == HIT_PROF_CARD_BASE + (int)i);

            DrawGradientCard(g, rcCard, 10.0f, linkedP.gradA, linkedP.gradB, sel, hov);

            g.DrawString((i == 0) ? L"DEFAULT" : L"GAME .EXE", -1, &fBadge, PointF(x + 12.0f, y + 12.0f), &brPink);
            g.DrawString(gp.exeName.c_str(), -1, &fCardTitle, PointF(x + 12.0f, y + cardH - 46.0f), &brWhite);

            wchar_t sub[128];
            swprintf_s(sub, L"Preset: %s (%.2fG)", linkedP.name.c_str(), gp.gamma);
            g.DrawString(sub, -1, &fSmall, PointF(x + 12.0f, y + cardH - 25.0f), &brMuted);
        }

        if (g_selectedGameProfile > 0) {
            RectF rcRem(padX, (float)clientH - 64.0f, 170.0f, 32.0f);
            RegisterHit(HIT_PROF_REMOVE, rcRem);
            FillRoundRect(g, rcRem, 8.0f, pillBrush);
            g.DrawString(L"Remove Selected Game", -1, &fCardSub, rcRem, &sfCenter, &brPink);
        }
    }
    else if (g_currentTab == TAB_DISCORD) {

        g.DrawString(L"Discord & Open Source", -1, &fHeader, PointF(padX, 58.0f), &brWhite);
        g.DrawString(L"GammaBoost+ is 100% open source (MIT License) and built for sharing presets on Discord.",
            -1, &fCardSub, PointF(padX, 88.0f), &brMuted);

        RectF rcBox(padX, 126.0f, innerW, 210.0f);
        SolidBrush panelBr(COL_BG_PANEL);
        FillRoundRect(g, rcBox, 12.0f, panelBr);
        DrawRoundRect(g, rcBox, 12.0f, Color(90, 255, 183, 197), 1.2f);

        g.DrawString(L"Share Your Current Baby-Pink Config on Discord", -1, &fSecTitle, PointF(rcBox.X + 20.0f, rcBox.Y + 18.0f), &brPink);

        wchar_t previewCode[512];
        swprintf_s(previewCode,
            L"[GammaBoost+ Preset: %s]\n"
            L"Gamma: %.2f   |   Contrast: %.2f   |   Black Point: %+.2f   |   Shadows: %+d\n"
            L"Saturation: %d%%   |   Vibrance: %d%%   |   Temperature: %d K\n"
            L"Share Code: GBPLUS:%.2f:%.2f:%.2f:%d:%d:%d:%d",
            g_activePresetName.c_str(), g_gamma, g_contrast, g_blackPoint, g_shadows,
            g_saturation, g_vibrance, g_temperatureK,
            g_gamma, g_contrast, g_blackPoint, g_shadows, g_saturation, g_vibrance, g_temperatureK);

        RectF rcCode(rcBox.X + 20.0f, rcBox.Y + 48.0f, rcBox.Width - 40.0f, 94.0f);
        SolidBrush codeBg(Color(255, 16, 16, 20));
        FillRoundRect(g, rcCode, 8.0f, codeBg);
        g.DrawString(previewCode, -1, &fCardSub, PointF(rcCode.X + 14.0f, rcCode.Y + 12.0f), &brWhite);

        RectF rcCopyBtn(rcBox.X + 20.0f, rcBox.Y + 156.0f, 220.0f, 36.0f);
        RegisterHit(HIT_DISCORD_COPY, rcCopyBtn);
        FillRoundRect(g, rcCopyBtn, 18.0f, brPink);
        SolidBrush darkTxt(Color(255, 24, 18, 22));
        g.DrawString(L"Copy Discord Preset Code", -1, &fCardTitle, rcCopyBtn, &sfCenter, &darkTxt);

        RectF rcImpBtn(rcBox.X + 252.0f, rcBox.Y + 156.0f, 230.0f, 36.0f);
        RegisterHit(HIT_DISCORD_IMPORT, rcImpBtn);
        FillRoundRect(g, rcImpBtn, 18.0f, pillBrush);
        DrawRoundRect(g, rcImpBtn, 18.0f, COL_PINK, 1.2f);
        g.DrawString(L"Import Code from Clipboard", -1, &fCardTitle, rcImpBtn, &sfCenter, &brPink);
    }

    BitBlt(hdc, 0, 0, clientW, clientH, s_memDC, 0, 0, SRCCOPY);
}

void UpdateManualSliderFromMouse(int sliderIdx, int mouseX, int clientW) {
    float inspectorW = 290.0f;
    float trackX = (float)clientW - 28.0f - inspectorW + 16.0f;
    float trackW = inspectorW - 32.0f;
    float norm = std::clamp((mouseX - trackX) / trackW, 0.0f, 1.0f);

    switch (sliderIdx) {
    case 0: g_gamma = std::round((0.50 + norm * 2.00) * 20.0) / 20.0; break;
    case 1: g_contrast = std::round((0.50 + norm * 1.00) * 20.0) / 20.0; break;
    case 2: g_blackPoint = std::round((-0.20 + norm * 0.40) * 100.0) / 100.0; break;
    case 3: g_shadows = (int)std::round(-50.0f + norm * 100.0f); break;
    case 4: g_saturation = (int)std::round(norm * 200.0f); break;
    case 5: g_vibrance = (int)std::round(norm * 250.0f); break;
    case 6: g_temperatureK = (int)(std::round((3500.0f + norm * 5500.0f) / 100.0f) * 100); break;
    }

    g_activePresetName = L"Custom";
    ApplyLiveDisplay();
}

void BrowseAndAddGame(HWND hWnd) {
    wchar_t fileBuf[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Game Executables (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Select Game Executable (.exe)";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring exeName = GetFileNameFromPath(fileBuf);
        int pIdx = std::clamp(g_activePresetIdx, 0, (int)g_presets.size() - 1);
        const DisplayPreset& p = g_presets[pIdx];
        g_gameProfiles.push_back({ exeName, fileBuf, pIdx, p.gamma, p.contrast, p.blackPoint, p.shadows, p.saturation, p.vibrance, p.temperatureK });
        g_selectedGameProfile = (int)g_gameProfiles.size() - 1;
        SaveConfig();
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

BOOL CALLBACK EnumRunningAppsProc(HWND hWnd, LPARAM) {
    if (!IsWindowVisible(hWnd)) return TRUE;
    wchar_t title[128] = { 0 };
    if (GetWindowTextW(hWnd, title, 128) == 0) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == 0 || pid == GetCurrentProcessId()) return TRUE;

    HANDLE hP = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hP) return TRUE;
    wchar_t pathB[MAX_PATH] = { 0 };
    DWORD sz = MAX_PATH;
    if (QueryFullProcessImageNameW(hP, 0, pathB, &sz)) {
        std::wstring exe = GetFileNameFromPath(pathB);
        if (!EqualsIgnoreCase(exe, L"explorer.exe") && !EqualsIgnoreCase(exe, L"ApplicationFrameHost.exe")) {
            bool found = false;
            for (const auto& a : g_runningApps) if (EqualsIgnoreCase(a.exeName, exe)) found = true;
            if (!found && g_runningApps.size() < 20) g_runningApps.push_back({ exe, pathB, title });
        }
    }
    CloseHandle(hP);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {

        BOOL dark = TRUE;
        DwmSetWindowAttribute(hWnd, 20  , &dark, sizeof(dark));

        RegisterHotKey(hWnd, HK_TOGGLE, MOD_ALT, VK_F9);
        RegisterHotKey(hWnd, HK_RESET, MOD_ALT, VK_F10);
        RegisterHotKey(hWnd, HK_GAMMA_UP, MOD_ALT, VK_UP);
        RegisterHotKey(hWnd, HK_GAMMA_DOWN, MOD_ALT, VK_DOWN);
        RegisterHotKey(hWnd, HK_RECOIL_TOGGLE, MOD_ALT, VK_F8);
        RegisterHotKey(hWnd, HK_ZOOM_TOGGLE, MOD_ALT, VK_F7);
        RegisterHotKey(hWnd, HK_AI_TOGGLE, MOD_ALT, VK_F6);
        RegisterHotKey(hWnd, HK_AI_CALIBRATE, MOD_ALT, VK_F5);
        SetTimer(hWnd, TIMER_WATCHDOG, 1500, NULL);
        SetTimer(hWnd, TIMER_RECOIL, 18, NULL);
        SetTimer(hWnd, TIMER_AI_VISION, 20, NULL);
        SetTimer(hWnd, TIMER_BG_ANIM, 50, NULL);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_ENTERSIZEMOVE:

        KillTimer(hWnd, TIMER_BG_ANIM);
        return 0;

    case WM_EXITSIZEMOVE:

        SetTimer(hWnd, TIMER_BG_ANIM, 50, NULL);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        RenderAppUI(hdc, rc.right - rc.left, rc.bottom - rc.top);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        g_mousePos = { mx, my };

        RECT rc;
        GetClientRect(hWnd, &rc);
        int cw = rc.right - rc.left;

        if (g_draggingSplit) {
            float padX = 210.0f + 28.0f;
            float previewW = (cw - 210.0f - 56.0f) - 290.0f - 18.0f;
            g_splitPos = std::clamp((mx - padX) / previewW, 0.08f, 0.92f);
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        if (g_draggingSlider >= 0) {
            UpdateManualSliderFromMouse(g_draggingSlider, mx, cw);
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        int newHover = -1;
        for (const auto& hr : g_hitRects) {
            if (hr.rc.Contains((REAL)mx, (REAL)my)) {
                newHover = hr.id;
                break;
            }
        }
        if (newHover != g_hoverId) {
            g_hoverId = newHover;
            SetCursor(LoadCursorW(NULL, (g_hoverId != -1) ? IDC_HAND : IDC_ARROW));
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc;
        GetClientRect(hWnd, &rc);
        int cw = rc.right - rc.left;

        int clickedId = -1;
        for (const auto& hr : g_hitRects) {
            if (hr.rc.Contains((REAL)mx, (REAL)my)) {
                clickedId = hr.id;
                break;
            }
        }

        if (clickedId == -1) return 0;

        if (clickedId == HIT_TAB_PRESETS)  { g_currentTab = TAB_PRESETS; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_MANUAL)   { g_currentTab = TAB_MANUAL; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_EYECARE)  { g_currentTab = TAB_EYECARE; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_FPS)      { g_currentTab = TAB_FPS_BOOST; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_ENHANCEMENTS) { g_currentTab = TAB_ENHANCEMENTS; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_PROFILES) { g_currentTab = TAB_PROFILES; InvalidateRect(hWnd, NULL, FALSE); }
        else if (clickedId == HIT_TAB_DISCORD)  { g_currentTab = TAB_DISCORD; InvalidateRect(hWnd, NULL, FALSE); }

        else if (clickedId == HIT_TOP_POWER || clickedId == HIT_TOGGLE_SHOW_SCREEN) {
            g_showOnScreen = !g_showOnScreen;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_TOP_MONITOR) {
            RestoreOriginalDisplay();
            g_selectedMonitor = (g_selectedMonitor + 1) % ((int)g_monitors.size() + 1);
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_BTN_NEW_PRESET) {
            g_currentTab = TAB_MANUAL;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_BTN_REVERT) {
            SelectPreset(g_activePresetIdx);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_SPLIT_HANDLE) {
            SetCapture(hWnd);
            g_draggingSplit = true;
            float padX = 210.0f + 28.0f;
            float previewW = (cw - 210.0f - 56.0f) - 290.0f - 18.0f;
            g_splitPos = std::clamp((mx - padX) / previewW, 0.08f, 0.92f);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_SCENE_BASE && clickedId < HIT_SCENE_BASE + 5) {
            g_previewScene = clickedId - HIT_SCENE_BASE;
            UpdatePreviewBitmaps();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_INSPECTOR_TAB && clickedId < HIT_INSPECTOR_TAB + 3) {
            g_inspectorSubTab = clickedId - HIT_INSPECTOR_TAB;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_SLIDER_BASE && clickedId < HIT_SLIDER_BASE + 7) {
            SetCapture(hWnd);
            g_draggingSlider = clickedId - HIT_SLIDER_BASE;
            UpdateManualSliderFromMouse(g_draggingSlider, mx, cw);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_EXTRA_BW) {
            g_bwMode = !g_bwMode;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_EXTRA_CB) {
            g_colorblindMode = (g_colorblindMode + 1) % 3;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_PRESET_CARD_BASE && clickedId < HIT_PRESET_CARD_BASE + 50) {
            SelectPreset(clickedId - HIT_PRESET_CARD_BASE);
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_FPS_CARD_BASE && clickedId < HIT_FPS_CARD_BASE + 20) {
            int fIdx = clickedId - HIT_FPS_CARD_BASE;
            if (fIdx >= 0 && fIdx < (int)g_fpsCards.size()) {
                g_fpsCards[fIdx].enabled = !g_fpsCards[fIdx].enabled;
                ApplyFpsChannelStates();
                RequestSaveConfig(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (clickedId == HIT_FPS_FLUSH_RAM) {
            FlushStandbyMemoryNow();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_FPS_ALL_TOGGLE) {
            for (auto& fc : g_fpsCards) fc.enabled = true;
            ApplyFpsChannelStates();
            FlushStandbyMemoryNow();
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_RECOIL_TOGGLE) {
            g_recoilPullEnabled = !g_recoilPullEnabled;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_RECOIL_STR_1 && clickedId <= HIT_RECOIL_STR_6) {
            g_recoilPullStrength = RECOIL_STEPS[clickedId - HIT_RECOIL_STR_1];
            g_recoilSubPixelAccum = 0.0;
            g_recoilPullEnabled = true;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_RECOIL_MODE_ADS) {
            g_recoilRequireADS = !g_recoilRequireADS;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_ZOOM_TOGGLE) {
            g_wheelZoomEnabled = !g_wheelZoomEnabled;
            if (!g_wheelZoomEnabled) g_currentZoomLevel = 1.0f;
            ApplyEagleEyeZoom();
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_ZOOM_STEP_1 && clickedId <= HIT_ZOOM_STEP_5) {
            g_currentZoomLevel = ZOOM_PRESET_STEPS[clickedId - HIT_ZOOM_STEP_1];
            g_wheelZoomEnabled = true;
            ApplyEagleEyeZoom();
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_ZOOM_MODE) {
            g_wheelZoomMode = (g_wheelZoomMode + 1) % 3;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_AI_TOGGLE) {
            g_aiVisionEnabled = !g_aiVisionEnabled;
            if (!g_aiVisionEnabled && g_hOverlayWnd) ShowWindow(g_hOverlayWnd, SW_HIDE);
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_AI_SENS_1 && clickedId <= HIT_AI_SENS_3) {
            g_aiSensitivity = clickedId - HIT_AI_SENS_1;
            g_aiVisionEnabled = true;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_AI_CROSSHAIR) {
            g_aiShowCrosshair = !g_aiShowCrosshair;
            RequestSaveConfig(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId == HIT_PROF_ADD_EXE) {
            BrowseAndAddGame(hWnd);
        }
        else if (clickedId == HIT_PROF_ADD_RUN) {
            g_runningApps.clear();
            EnumWindows(EnumRunningAppsProc, 0);
            HMENU hMenu = CreatePopupMenu();
            if (g_runningApps.empty()) {
                AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, L"No running games detected");
            } else {
                for (size_t i = 0; i < g_runningApps.size(); ++i) {
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_RUN_BASE + i, g_runningApps[i].exeName.c_str());
                }
            }
            POINT pt = { mx, my };
            ClientToScreen(hWnd, &pt);
            TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (clickedId == HIT_PROF_REMOVE && g_selectedGameProfile > 0) {
            g_gameProfiles.erase(g_gameProfiles.begin() + g_selectedGameProfile);
            g_selectedGameProfile = 0;
            SaveConfig();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (clickedId >= HIT_PROF_CARD_BASE && clickedId < HIT_PROF_CARD_BASE + 50) {
            int pIdx = clickedId - HIT_PROF_CARD_BASE;
            if (pIdx >= 0 && pIdx < (int)g_gameProfiles.size()) {
                g_selectedGameProfile = pIdx;
                SelectPreset(g_gameProfiles[pIdx].presetIndex);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (clickedId == HIT_DISCORD_COPY) {
            CopyDiscordShareConfig(hWnd);
            MessageBoxW(hWnd, L"Copied your Baby-Pink GammaBoost+ config block to clipboard!\nPaste it directly into any Discord channel.",
                L"GammaBoost+ Discord Share", MB_OK | MB_ICONINFORMATION);
        }
        else if (clickedId == HIT_DISCORD_IMPORT) {
            bool imported = false;
            if (OpenClipboard(hWnd)) {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData) {
                    const wchar_t* txt = (const wchar_t*)GlobalLock(hData);
                    if (txt) {
                        const wchar_t* tag = wcsstr(txt, L"GBPLUS:");
                        if (tag) {
                            double ig = 1.4, ic = 1.15, ib = 0.03;
                            int ish = 20, isat = 135, ivib = 155, itemp = 6500;
                            if (swscanf_s(tag + 7, L"%lf:%lf:%lf:%d:%d:%d:%d", &ig, &ic, &ib, &ish, &isat, &ivib, &itemp) == 7) {
                                g_gamma = std::clamp(ig, 0.50, 2.50);
                                g_contrast = std::clamp(ic, 0.50, 1.50);
                                g_blackPoint = std::clamp(ib, -0.20, 0.20);
                                g_shadows = std::clamp(ish, -50, 50);
                                g_saturation = std::clamp(isat, 0, 200);
                                g_vibrance = std::clamp(ivib, 0, 250);
                                g_temperatureK = std::clamp(itemp, 3500, 9000);
                                g_activePresetName = L"Discord Import";
                                g_showOnScreen = true;
                                ApplyLiveDisplay();
                                SaveConfig();
                                imported = true;
                            }
                        }
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            InvalidateRect(hWnd, NULL, FALSE);
            if (imported) {
                MessageBoxW(hWnd, L"Successfully imported and applied Discord preset from clipboard!",
                    L"GammaBoost+ Discord Import", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hWnd, L"No valid GBPLUS: preset code found in clipboard.\nCopy a preset block from Discord first!",
                    L"GammaBoost+ Discord Import", MB_OK | MB_ICONWARNING);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_draggingSplit || g_draggingSlider >= 0) {
            ReleaseCapture();
            g_draggingSplit = false;
            g_draggingSlider = -1;
            SaveConfig();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_TRAY_RESTORE) {
            if (g_inTray) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_inTray = false;
            }
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (id == ID_TRAY_TOGGLE) {
            g_showOnScreen = !g_showOnScreen;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (id == ID_TRAY_EXIT) {
            if (g_inTray) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_inTray = false;
            }
            DestroyWindow(hWnd);
        } else if (id >= ID_MENU_RUN_BASE && id < ID_MENU_RUN_BASE + 50) {
            size_t idx = (size_t)(id - ID_MENU_RUN_BASE);
            if (idx < g_runningApps.size()) {
                int pIdx = std::clamp(g_activePresetIdx, 0, (int)g_presets.size() - 1);
                const DisplayPreset& p = g_presets[pIdx];
                g_gameProfiles.push_back({ g_runningApps[idx].exeName, g_runningApps[idx].fullPath, pIdx,
                    p.gamma, p.contrast, p.blackPoint, p.shadows, p.saturation, p.vibrance, p.temperatureK });
                g_selectedGameProfile = (int)g_gameProfiles.size() - 1;
                SaveConfig();
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            if (g_inTray) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_inTray = false;
            }
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Open GammaBoost+");
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, g_showOnScreen ? L"Disable Screen Boost (Alt+F9)" : L"Enable Screen Boost (Alt+F9)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit GammaBoost+");
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    }

    case WM_CLOSE: {
        int res = MessageBoxW(
            hWnd,
            L"Do you want GammaBoost+ to keep running in the background?\n\n"
            L"• Yes — Minimize to System Tray (keeps your Gamma & FPS Boost active)\n"
            L"• No — Exit completely and restore default display settings\n"
            L"• Cancel — Stay in GammaBoost+",
            L"GammaBoost+ — Run in Background?",
            MB_YESNOCANCEL | MB_ICONQUESTION
        );
        if (res == IDYES) {
            ZeroMemory(&g_nid, sizeof(g_nid));
            g_nid.cbSize = sizeof(NOTIFYICONDATAW);
            g_nid.hWnd = hWnd;
            g_nid.uID = ID_TRAY_ICON;
            g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
            g_nid.uCallbackMessage = WM_TRAYICON;
            g_nid.hIcon = g_hAppIcon ? g_hAppIcon : LoadIconW(NULL, IDI_APPLICATION);
            wcscpy_s(g_nid.szTip, L"GammaBoost+ (Running in Background)");
            wcscpy_s(g_nid.szInfoTitle, L"GammaBoost+ is running in the background");
            wcscpy_s(g_nid.szInfo, L"Your Gamma & FPS Channels stay active. Click the tray icon anytime to reopen.");
            g_nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIconW(NIM_ADD, &g_nid);
            g_inTray = true;
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        } else if (res == IDNO) {
            if (g_inTray) {
                Shell_NotifyIconW(NIM_DELETE, &g_nid);
                g_inTray = false;
            }
            DestroyWindow(hWnd);
            return 0;
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == TIMER_BG_ANIM) {
            if (!g_bgFrames.empty() && !IsIconic(hWnd)) {
                g_bgCurrentFrame = (g_bgCurrentFrame + 1) % (UINT)g_bgFrames.size();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        if (wParam == TIMER_SAVE_CONFIG) {
            KillTimer(hWnd, TIMER_SAVE_CONFIG);
            if (g_configSavePending) {
                g_configSavePending = false;
                SaveConfig();
            }
            return 0;
        }
        if (wParam == TIMER_AI_VISION) {
            RunAiVisionScanTick();
            return 0;
        }
        if (wParam == TIMER_RECOIL) {
            if (g_recoilPullEnabled && GetForegroundWindow() != hWnd) {
                bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                bool rmbOk = !g_recoilRequireADS || ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
                if (lmbDown && rmbOk) {
                    g_recoilSubPixelAccum += g_recoilPullStrength;
                    int stepY = (int)g_recoilSubPixelAccum;
                    if (stepY > 0) {
                        g_recoilSubPixelAccum -= stepY;
                        INPUT inp = {};
                        inp.type = INPUT_MOUSE;
                        inp.mi.dx = 0;
                        inp.mi.dy = stepY;
                        inp.mi.dwFlags = MOUSEEVENTF_MOVE;
                        SendInput(1, &inp, sizeof(INPUT));
                    }
                } else {
                    g_recoilSubPixelAccum = 0.0;
                }
            }
            return 0;
        }
        if (wParam == TIMER_WATCHDOG) {

            HWND hFg = GetForegroundWindow();
            DWORD fgPid = 0;
            if (hFg) GetWindowThreadProcessId(hFg, &fgPid);
            if (fgPid != 0 && fgPid != GetCurrentProcessId()) {
                HANDLE hP = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, FALSE, fgPid);
                if (hP) {
                    wchar_t pathB[MAX_PATH] = { 0 };
                    DWORD sz = MAX_PATH;
                    if (QueryFullProcessImageNameW(hP, 0, pathB, &sz)) {
                        std::wstring fgExe = GetFileNameFromPath(pathB);
                        for (size_t i = 1; i < g_gameProfiles.size(); ++i) {
                            if (EqualsIgnoreCase(g_gameProfiles[i].exeName, fgExe)) {
                                if (g_fpsCards.size() > 1 && g_fpsCards[1].enabled) {
                                    SetPriorityClass(hP, HIGH_PRIORITY_CLASS);
                                }
                                if (g_selectedGameProfile != (int)i) {
                                    g_selectedGameProfile = (int)i;
                                    SelectPreset(g_gameProfiles[i].presetIndex);
                                    InvalidateRect(hWnd, NULL, FALSE);
                                }
                                break;
                            }
                        }
                    }
                    CloseHandle(hP);
                }
            }
            if (g_showOnScreen) {
                ApplyLiveDisplay();
            }
        }
        return 0;
    }

    case WM_HOTKEY: {
        if (wParam == HK_TOGGLE) {
            g_showOnScreen = !g_showOnScreen;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_RESET) {
            g_gamma = 1.00; g_contrast = 1.00; g_blackPoint = 0.0; g_shadows = 0;
            g_saturation = 100; g_vibrance = 100; g_temperatureK = 6500;
            g_currentZoomLevel = 1.0f;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_GAMMA_UP) {
            g_gamma = std::clamp(g_gamma + 0.10, 0.50, 2.50);
            g_activePresetName = L"Custom";
            g_showOnScreen = true;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_GAMMA_DOWN) {
            g_gamma = std::clamp(g_gamma - 0.10, 0.50, 2.50);
            g_activePresetName = L"Custom";
            g_showOnScreen = true;
            ApplyLiveDisplay();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_RECOIL_TOGGLE) {
            g_recoilPullEnabled = !g_recoilPullEnabled;
            SaveConfig();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_ZOOM_TOGGLE) {
            g_wheelZoomEnabled = !g_wheelZoomEnabled;
            if (!g_wheelZoomEnabled) g_currentZoomLevel = 1.0f;
            ApplyEagleEyeZoom();
            SaveConfig();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_AI_TOGGLE) {
            g_aiVisionEnabled = !g_aiVisionEnabled;
            if (!g_aiVisionEnabled && g_hOverlayWnd) ShowWindow(g_hOverlayWnd, SW_HIDE);
            SaveConfig();
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == HK_AI_CALIBRATE) {
            g_calibrateNextTick = true;
            g_aiVisionEnabled = true;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_DESTROY: {
        if (g_inTray) {
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            g_inTray = false;
        }
        if (g_hOverlayWnd) {
            DestroyWindow(g_hOverlayWnd);
            g_hOverlayWnd = NULL;
        }
        KillTimer(hWnd, TIMER_WATCHDOG);
        KillTimer(hWnd, TIMER_RECOIL);
        KillTimer(hWnd, TIMER_AI_VISION);
        KillTimer(hWnd, TIMER_BG_ANIM);
        KillTimer(hWnd, TIMER_SAVE_CONFIG);
        for (Bitmap* b : g_bgFrames) delete b;
        g_bgFrames.clear();
        UnregisterHotKey(hWnd, HK_TOGGLE);
        UnregisterHotKey(hWnd, HK_RESET);
        UnregisterHotKey(hWnd, HK_GAMMA_UP);
        UnregisterHotKey(hWnd, HK_GAMMA_DOWN);
        UnregisterHotKey(hWnd, HK_RECOIL_TOGGLE);
        UnregisterHotKey(hWnd, HK_ZOOM_TOGGLE);
        UnregisterHotKey(hWnd, HK_AI_TOGGLE);
        UnregisterHotKey(hWnd, HK_AI_CALIBRATE);
        SaveConfig();
        ShutdownSystemAPIs();
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    InitCommonControls();

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    InitPaths();
    InitDefaultCatalog();
    InitSystemAPIs();
    InitPreviewBitmaps(hInstance);
    InitAnimatedBackground();
    LoadConfig();
    ApplyFpsChannelStates();

    g_hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!g_hAppIcon) g_hAppIcon = LoadIconW(NULL, IDI_APPLICATION);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_hAppIcon;
    wc.hIconSm = g_hAppIcon;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"GammaBoostPlusBabyPinkUI";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc = { 0, 0, 1180, 690 };
    AdjustWindowRect(&rc, style, FALSE);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"GammaBoost+",
        style,
        (screenW - winW) / 2,
        (screenH - winH) / 2,
        winW,
        winH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 1;

    ApplyLiveDisplay();
    InitAiOverlayWindow(hInstance);
    g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    ShowWindow(g_hWnd, (nCmdShow == SW_HIDE || nCmdShow == 0) ? SW_SHOWNORMAL : nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    delete g_pPreviewRaw;
    delete g_pPreviewOrig;
    delete g_pPreviewAdj;
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
