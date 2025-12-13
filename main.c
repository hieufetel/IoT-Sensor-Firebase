#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

// === CẤU HÌNH ===
#define COMPORT "COM4"
#define BAUDRATE 115200

// Cấu hình Firebase
#define FIREBASE_URL_HISTORY "https://anhbaolmao1-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json"
#define FIREBASE_URL_CURRENT "https://anhbaolmao1-default-rtdb.asia-southeast1.firebasedatabase.app/current.json"

// ======================================================
// GỬI HTTP LÊN FIREBASE
// ======================================================
void http_post_firebase(float temperature, float humidity)
{
    time_t now = time(NULL);
    struct tm *p = localtime(&now);
    char timebuf[40];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", p);

    FILE *f = fopen("data.json", "w");
    if (!f)
        return;
    fprintf(f, "{ \"temperature\": %.2f, \"humidity\": %.2f, \"time\": \"%s\" }", temperature, humidity, timebuf);
    fclose(f);

    char cmd[1024];
    // 1. Post Lịch sử
    sprintf(cmd, "curl -s -k -X POST -H \"Content-Type: application/json\" -d @data.json \"%s\"", FIREBASE_URL_HISTORY);
    system(cmd);
    // 2. Put Hiện tại
    sprintf(cmd, "curl -s -k -X PUT -H \"Content-Type: application/json\" -d @data.json \"%s\"", FIREBASE_URL_CURRENT);
    system(cmd);

    printf(" >> [Firebase] OK: T=%.2f, H=%.2f\n", temperature, humidity);
}

// ======================================================
// XỬ LÝ DỮ LIỆU
// ======================================================
void process_line(char *line)
{
    float t = 0, h = 0;
    int parsed = 0;
    if (sscanf(line, "Nhiet do: %f C, Do am: %f %%", &t, &h) == 2)
        parsed = 1;
    else if (sscanf(line, "Temp:%f,Humid:%f", &t, &h) == 2)
        parsed = 1;
    else if (sscanf(line, "temp: %f, humid: %f", &t, &h) == 2)
        parsed = 1;

    if (parsed)
        http_post_firebase(t, h);
}

// ======================================================
// HÀM MỞ CỔNG COM (Tách riêng để tái sử dụng)
// ======================================================
HANDLE open_uart(const char *port_name)
{
    HANDLE hSerial = CreateFile(port_name, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(hSerial, &dcb))
    {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    dcb.BaudRate = BAUDRATE;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcb))
    {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    return hSerial;
}

// ======================================================
// MAIN (Cơ chế lặp vô tận để Reconnect)
// ======================================================
int main()
{
    printf("--- C GATEWAY (AUTO RECONNECT) ---\n");
    printf(" >> Port: %s\n", COMPORT);

    char buffer[1];
    char line[256];
    int idx = 0;
    DWORD n;
    HANDLE hSerial = INVALID_HANDLE_VALUE;

    while (1) // Vòng lặp lớn: Quản lý kết nối
    {
        // 1. Cố gắng kết nối
        if (hSerial == INVALID_HANDLE_VALUE)
        {
            printf("Dang tim thiet bi tai %s...\n", COMPORT);
            hSerial = open_uart(COMPORT);

            if (hSerial != INVALID_HANDLE_VALUE)
            {
                printf(" >> KET NOI THANH CONG! Dang nhan du lieu...\n");
                idx = 0; // Reset bộ đệm
            }
            else
            {
                Sleep(2000); // Nếu chưa thấy, đợi 2s rồi tìm lại
                continue;
            }
        }

        // 2. Đọc dữ liệu
        if (ReadFile(hSerial, buffer, 1, &n, NULL))
        {
            if (n > 0)
            {
                // Có dữ liệu, xử lý bình thường
                char c = buffer[0];
                if (c == '\n' || c == '\r')
                {
                    if (idx > 0)
                    {
                        line[idx] = '\0';
                        printf("[UART] %s\n", line);
                        process_line(line);
                        idx = 0;
                    }
                }
                else
                {
                    if (idx < sizeof(line) - 1)
                        line[idx++] = c;
                }
            }
        }
        else
        {
            // 3. Xử lý khi bị ngắt kết nối (Rút dây)
            printf("WARN: Mat ket noi! Dang thu ket noi lai...\n");
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE; // Đánh dấu là đã mất để vòng lặp sau kết nối lại
            Sleep(1000);
        }
    }

    return 0;
}