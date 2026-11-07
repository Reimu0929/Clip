#include "common.h"

static const char* ALLOWED_CLIENT = "192.168.137.2";   // このIP以外は切断

DWORD WINAPI netThread(LPVOID) {
    SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);
    a.sin_addr.s_addr = inet_addr("192.168.137.1");    // このNICだけで待ち受け
    if (bind(ls, (sockaddr*)&a, sizeof(a)) != 0) return 1;
    listen(ls, 1);

    for (;;) {
        sockaddr_in pa; int pl = sizeof(pa);
        SOCKET s = accept(ls, (sockaddr*)&pa, &pl);
        if (s == INVALID_SOCKET) { Sleep(1000); continue; }
        if (pa.sin_addr.s_addr != inet_addr(ALLOWED_CLIENT)) { closesocket(s); continue; }
        g_sock = s;
        receiveLoop(s);
        g_sock = INVALID_SOCKET;
        closesocket(s);
    }
}

int main() { return runApp(netThread); }