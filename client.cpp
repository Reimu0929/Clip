#include "common.h"

static const char* SERVER_IP = "192.168.137.1";

DWORD WINAPI netThread(LPVOID) {
    for (;;) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_port = htons(PORT);
        a.sin_addr.s_addr = inet_addr(SERVER_IP);
        if (connect(s, (sockaddr*)&a, sizeof(a)) == 0) {
            g_sock = s;
            receiveLoop(s);
            g_sock = INVALID_SOCKET;
        }
        closesocket(s);
        Sleep(3000);                                   // 3秒おきに再接続
    }
}

int main() { return runApp(netThread); }