#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  // UNIX standard function definitions
#include <fcntl.h>   // File control definitions
#include <errno.h>   // Error number definitions
#include <termios.h> // POSIX terminal control definitions

int serial_fd; // File descriptor for Serial Port

// ======================================================
// OPEN UART (Linux Version)
// ======================================================
void open_serial(const char* port)
{
    // Open the Port.
    // O_RDWR: Read+Write
    // O_NOCTTY: Do not make this port the controlling terminal
    // O_NDELAY: Do not care about the state of DCD signal line
    serial_fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);

    if (serial_fd == -1) {
        printf("Error: Unable to open port %s\n", port);
        perror("open_serial");
        exit(1);
    }

    // Configure Port (Equivalent to DCB in Windows)
    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        printf("Error: tcgetattr failed\n");
        exit(1);
    }

    // Set Baud Rate to 115200
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    // 8N1 Configuration (8 Data bits, No Parity, 1 Stop bit)
    tty.c_cflag &= ~PARENB;     // No Parity
    tty.c_cflag &= ~CSTOPB;     // 1 Stop bit
    tty.c_cflag &= ~CSIZE;      // Clear size bits
    tty.c_cflag |= CS8;         // 8 bits per byte

    // Disable Hardware Flow Control
    tty.c_cflag &= ~CRTSCTS;

    // Enable Reading & Ignore Modem Control Lines
    tty.c_cflag |= CREAD | CLOCAL;

    // Disable Canonical Mode (Read raw bytes, not line by line)
    // We handle line buffering manually in read_loop()
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;       // Disable Echo
    tty.c_lflag &= ~ECHOE;      // Disable Erasure
    tty.c_lflag &= ~ISIG;       // Disable Signals

    // Disable Software Flow Control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Disable Special Output Processing
    tty.c_oflag &= ~OPOST;

    // Apply Settings
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        printf("Error: tcsetattr failed\n");
        exit(1);
    }

    // Flush buffer to remove old data
    tcflush(serial_fd, TCIFLUSH);

    printf(" [Linux] Serial Port Opened: %s\n", port);
}


// ======================================================
// POST DATA TO FIREBASE (Same logic, system calls work in Linux)
// ======================================================
// ======================================================
// POST DATA TO FIREBASE (FIXED)
// ======================================================
void http_post(float temperature, float humidity)
{
    const char* url_history =
        "https://anhbaolmao1-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json";
    const char* url_current =
        "https://anhbaolmao1-default-rtdb.asia-southeast1.firebasedatabase.app/current.json";

    // --- 1. PREPARE DATA ---
    time_t now = time(NULL);
    struct tm* p = localtime(&now);
    char timebuf[40];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", p);

    FILE* f = fopen("data.json", "w");
    if (!f) {
        printf(" Error: Cannot create data.json\n");
        return;
    }
    fprintf(f,
        "{ \"temperature\": %.2f, \"humidity\": %.2f, \"time\": \"%s\" }",
        temperature, humidity, timebuf);
    fclose(f);

    char cmd[600];

    // --- 2. UPLOAD HISTORY (POST) ---
    // Use POST to add a new row to the list
    sprintf(cmd,
        "curl -s -X POST -H \"Content-Type: application/json\" "
        "-d @data.json \"%s\"",
        url_history);
    
    printf("➡ Uploading History Node...\n");
    system(cmd); // <--- EXECUTE HERE


    // --- 3. UPDATE CURRENT STATUS (PUT) ---
    // Use PUT to overwrite the specific location
    sprintf(cmd,
        "curl -s -X PUT -H \"Content-Type: application/json\" "
        "-d @data.json \"%s\"",
        url_current);

    printf("➡ Updating Current Node...\n");
    system(cmd); // <--- EXECUTE HERE AGAIN

    printf(" Done.\n\n");
}


// ======================================================
// PROCESS ONE LINE OF UART
// ======================================================
void process_line(const char* line)
{
    float t = 0, h = 0;

    // Format 1: "Nhiet do: 29.7 C, Do am: 62.0 %"
    if (sscanf(line, "Nhiet do: %f C, Do am: %f %%", &t, &h) == 2)
    {
        printf(" Parsed OK  →  T=%.2f  H=%.2f\n", t, h);
        http_post(t, h);
        return;
    }

    // Format 2: "Temp:29.7,Humid:62.0"
    if (sscanf(line, "Temp:%f,Humid:%f", &t, &h) == 2)
    {
        printf(" Parsed OK  →  T=%.2f  H=%.2f\n", t, h);
        http_post(t, h);
        return;
    }

    // Format 3: "temp: 35, humid: 60" (Based on your Python example)
    if (sscanf(line, "temp: %f, humid: %f", &t, &h) == 2)
    {
        printf(" Parsed OK  →  T=%.2f  H=%.2f\n", t, h);
        http_post(t, h);
        return;
    }

    printf(" Unrecognized format: %s\n", line);
}


// ======================================================
// UART READER LOOP (Linux Version)
// ======================================================
void read_loop()
{
    char byte;
    char line[256];
    int idx = 0;
    int n;

    printf("Listening to UART...\n\n");

    while (1)
    {
        // Read 1 byte from file descriptor
        n = read(serial_fd, &byte, 1);

        if (n > 0)
        {
            // Handle Newline (End of data packet)
            if (byte == '\n' || byte == '\r')
            {
                if (idx > 0)
                {
                    line[idx] = '\0'; // Null-terminate string
                    printf("[UART] %s\n", line);

                    process_line(line);
                    idx = 0; // Reset buffer
                }
            }
            else
            {
                // Add byte to buffer if space exists
                if (idx < sizeof(line) - 1)
                    line[idx++] = byte;
            }
        }
        else if (n < 0) {
            // Check for errors, ignore "Resource temporarily unavailable"
            if (errno != EAGAIN) {
                perror("Read error");
            }
        }
    }
}


// ======================================================
// MAIN
// ======================================================
int main()
{
    // On Linux, EFR32/Arduino usually appears as /dev/ttyACM0
    // If not, check: ls /dev/tty*
    const char* port_name = "/dev/ttyACM0";

    open_serial(port_name);

    printf("\n=== TOOL SEND SENSOR TO FIREBASE (LINUX) ===\n");
    printf("Firebase path: /sensor.json\n");
    printf("Port: %s\n\n", port_name);

    read_loop();

    close(serial_fd);
    return 0;
}