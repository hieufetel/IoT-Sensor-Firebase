#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// === CẤU HÌNH ===
#define COMPORT "COM3" // <--- KIỂM TRA LẠI CỔNG COM
#define BAUDRATE 115200

// ĐỊNH NGHĨA 2 CỔNG ĐÍCH
#define PORT_DASHBOARD 5005
#define PORT_FIREBASE 5006 // <--- Cổng mới cho Firebase

int main()
{
    // 1. KHỞI TẠO MẠNG (WINSOCK)
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Tạo socket (Chỉ cần 1 socket là đủ để gửi đi nhiều nơi)
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // --- CẤU HÌNH ĐỊA CHỈ 1: DASHBOARD (5005) ---
    struct sockaddr_in addr_dashboard;
    addr_dashboard.sin_family = AF_INET;
    addr_dashboard.sin_port = htons(PORT_DASHBOARD);
    addr_dashboard.sin_addr.s_addr = inet_addr("127.0.0.1");

    // --- CẤU HÌNH ĐỊA CHỈ 2: FIREBASE (5006) ---
    struct sockaddr_in addr_firebase;
    addr_firebase.sin_family = AF_INET;
    addr_firebase.sin_port = htons(PORT_FIREBASE);
    addr_firebase.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 2. KHỞI TẠO UART
    HANDLE hSerial = CreateFile(COMPORT, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("LOI: Khong mo duoc %s. Kiem tra day cam hoac tat MobaXterm!\n", COMPORT);
        return 1;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = BAUDRATE;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    SetCommState(hSerial, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    printf("--- C GATEWAY IS RUNNING (DUAL SEND) ---\n");
    printf(" >> Target 1 (Dashboard): Port %d\n", PORT_DASHBOARD);
    printf(" >> Target 2 (Firebase) : Port %d\n", PORT_FIREBASE);
    printf("Listening on %s...\n", COMPORT);

    char buffer[256];
    DWORD bytesRead;

    while (1)
    {
        // 3. ĐỌC TỪ EFR32
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            if (bytesRead > 0)
            {
                buffer[bytesRead] = '\0'; // Ngắt chuỗi

                // In ra màn hình đen để kiểm tra
                printf("%s", buffer);

                int len = strlen(buffer);

                // 4. GỬI LẦN 1: CHO DASHBOARD (5005)
                sendto(udp_socket, buffer, len, 0, (struct sockaddr *)&addr_dashboard, sizeof(addr_dashboard));

                // 5. GỬI LẦN 2: CHO FIREBASE (5006)
                sendto(udp_socket, buffer, len, 0, (struct sockaddr *)&addr_firebase, sizeof(addr_firebase));
            }
        }
    }

    CloseHandle(hSerial);
    closesocket(udp_socket);
    WSACleanup();
    return 0;
}