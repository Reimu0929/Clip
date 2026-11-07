#pragma once
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

static HWND g_hwnd, g_next;
static volatile SOCKET g_sock = INVALID_SOCKET;
static std::wstring g_last;                       // ループ防止用
static const UINT WM_REMOTE = WM_APP + 1;
static const u_short PORT = 5000;

inline bool sendAll(SOCKET s, const char* p, int n) {
    while (n > 0) { int r = send(s, p, n, 0); if (r <= 0) return false; p += r; n -= r; }
    return true;
}
inline bool recvAll(SOCKET s, char* p, int n) {
    while (n > 0) { int r = recv(s, p, n, 0); if (r <= 0) return false; p += r; n -= r; }
    return true;
}

inline bool getClipText(std::wstring& out) {
    if (!OpenClipboard(g_hwnd)) return false;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        const wchar_t* p = (const wchar_t*)GlobalLock(h);
        if (p) { out = p; GlobalUnlock(h); }
    }
    CloseClipboard();
    return h != NULL;
}

inline void setClipText(const std::wstring& t) {
    if (!OpenClipboard(g_hwnd)) return;
    EmptyClipboard();
    size_t bytes = (t.size() + 1) * sizeof(wchar_t);
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
    memcpy(GlobalLock(h), t.c_str(), bytes);
    GlobalUnlock(h);
    SetClipboardData(CF_UNICODETEXT, h);
    CloseClipboard();
}

inline void sendText(const std::wstring& t) {
    SOCKET s = g_sock;
    if (s == INVALID_SOCKET) return;
    u_long len = htonl((u_long)(t.size() * sizeof(wchar_t)));
    sendAll(s, (const char*)&len, 4);
    sendAll(s, (const char*)t.data(), (int)(t.size() * sizeof(wchar_t)));
}

// 切断されるまで受信し続ける(両側共通)
inline void receiveLoop(SOCKET s) {
    for (;;) {
        u_long len;
        if (!recvAll(s, (char*)&len, 4)) break;
        len = ntohl(len);
        if (len > 16 * 1024 * 1024) break;
        std::vector<wchar_t> buf(len / sizeof(wchar_t));
        if (len && !recvAll(s, (char*)&buf[0], (int)len)) break;
        PostMessage(g_hwnd, WM_REMOTE, 0, (LPARAM)new std::wstring(buf.begin(), buf.end()));
    }
}

inline LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_DRAWCLIPBOARD: {
        if (g_next) SendMessage(g_next, m, w, l);
        std::wstring t;
        if (getClipText(t) && t != g_last) { g_last = t; sendText(t); }
        return 0;
    }
    case WM_CHANGECBCHAIN:
        if ((HWND)w == g_next) g_next = (HWND)l;
        else if (g_next) SendMessage(g_next, m, w, l);
        return 0;
    case WM_REMOTE: {
        std::wstring* s = (std::wstring*)l;
        g_last = *s; setClipText(*s); delete s;
        return 0;
    }
    case WM_DESTROY:
        ChangeClipboardChain(h, g_next);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(h, m, w, l);
}

// ウィンドウ作成 → 通信スレッド開始 → メッセージループ
inline int runApp(LPTHREAD_START_ROUTINE netThread) {
    WSADATA wd; WSAStartup(MAKEWORD(2, 2), &wd);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ClipSync";
    RegisterClassW(&wc);
    g_hwnd = CreateWindowW(L"ClipSync", L"", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    g_next = SetClipboardViewer(g_hwnd);
    CreateThread(NULL, 0, netThread, NULL, 0, NULL);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}