/***************************************************************************//**
* @file
* @brief main() function.
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* SPDX-License-Identifier: Zlib
*
* The licensor of this software is Silicon Laboratories Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
******************************************************************************/

#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "app.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else // SL_CATALOG_KERNEL_PRESENT
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT
#include "app_log.h"
#include "sl_sleeptimer.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

#include "sl_udelay.h"

// Size of the buffer for received data
#define BUFLEN 10

#define BSP_TXPORT gpioPortA
#define BSP_RXPORT gpioPortA
#define BSP_TXPIN 5
#define BSP_RXPIN 6
#define BSP_ENABLE_PORT gpioPortD
#define BSP_ENABLE_PIN 4

//Define led
#define BSP_GPIO_LEDS
#define BSP_GPIO_LED0_PORT gpioPortD
#define BSP_GPIO_LED0_PIN 2
#define BSP_GPIO_LED1_PORT gpioPortD
#define BSP_GPIO_LED1_PIN 3
#define BSP_GPIO_PB0_PORT gpioPortB
#define BSP_GPIO_PB0_PIN 0
//DHT11
#define DHT11_PORT gpioPortB
#define DHT11_PIN 1
//


/**************************************************************************//**
* @brief
* GPIO initialization
*****************************************************************************/
void initGPIO(void)
{
// Configure the USART TX pin to the board controller as an output
GPIO_PinModeSet(BSP_TXPORT, BSP_TXPIN, gpioModePushPull, 1);

// Configure the USART RX pin to the board controller as an input
GPIO_PinModeSet(BSP_RXPORT, BSP_RXPIN, gpioModeInput, 0);

/*
* Configure the BCC_ENABLE pin as output and set high. This enables
* the virtual COM port (VCOM) connection to the board controller and
* permits serial port traffic over the debug connection to the host
* PC.
*
* To disable the VCOM connection and use the pins on the kit
* expansion (EXP) header, comment out the following line.
*/
GPIO_PinModeSet(BSP_ENABLE_PORT, BSP_ENABLE_PIN, gpioModePushPull, 1);
}

void initLED_BUTTON(){
// Enable GPIO clock
CMU_ClockEnable(cmuClock_GPIO, true);
// Configure PB0 and PB1 as input with glitch filter enabled
GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPullFilter, 1);
GPIO_PinModeSet(DHT11_PORT, DHT11_PIN, gpioModeInputPullFilter, 1);
// Configure LED0 and LED1 as output
GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);
GPIO_PinModeSet(BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN, gpioModePushPull, 0);
// Enable IRQ for even numbered GPIO pins
NVIC_EnableIRQ(GPIO_EVEN_IRQn);
// Enable IRQ for odd numbered GPIO pins
NVIC_EnableIRQ(GPIO_ODD_IRQn);
// Enable falling-edge interrupts for PB pins
GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN,BSP_GPIO_PB0_PIN, 0, 1, true);
GPIO_ExtIntConfig(DHT11_PORT, DHT11_PIN,DHT11_PIN, 0, 1, true);
}

void GPIO_EVEN_IRQHandler(void)
{
// Clear all even pin interrupt flags
GPIO_IntClear(0x5555);
// Code here

}

void GPIO_ODD_IRQHandler (void)
{
// Clear all odd pin interrupt flags
GPIO_IntClear(0xAAAA);
// Code here

}

// Khai báo extern cho hàm hiển thị LCD (giả định)
extern void memlcd_app_init(uint8_t hum, uint8_t hum_decimal, uint8_t temp, uint8_t temp_decimal);
extern uint8_t humidity, humidity_decimal, temperature, temperature_decimal;
extern void get_temp_hum(uint8_t hum, uint8_t hum_decimal, uint8_t temp, uint8_t temp_decimal);


// Định nghĩa hằng số và giá trị timeout
#define MAX_TIMEOUT 100 // Timeout tối đa trong micro giây
#define DHT11_PIN_HIGH_TIMEOUT 100 // Timeout cho trạng thái pin cao

// Hàm chờ trạng thái pin với timeout
bool wait_for_pin_state(GPIO_Port_TypeDef port, uint8_t pin, bool desiredState, uint32_t timeout) {
uint32_t time = 0;
while (GPIO_PinInGet(port, pin) != desiredState && time++ < timeout) {
sl_udelay_wait(1); // Thời gian trễ nhỏ cho sự thay đổi trạng thái pin
}
return time < timeout;
}


// Hàm đọc dữ liệu từ DHT11
bool DHT11_ReadData(void) {
uint8_t data[5] = {0, 0, 0, 0, 0};
uint32_t timeOut;

// 1. Gửi tín hiệu khởi động
GPIO_PinModeSet(DHT11_PORT, DHT11_PIN, gpioModePushPull, 0); // Set pin to low
GPIO_PinOutClear(DHT11_PORT, DHT11_PIN);
sl_udelay_wait(18000); // Delay 18ms (DHT11 requirement)
GPIO_PinOutSet(DHT11_PORT, DHT11_PIN);
sl_udelay_wait(20); // Delay 20-40µs
GPIO_PinModeSet(DHT11_PORT, DHT11_PIN, gpioModeInputPull, 1); // Set pin to input mode

// 2. Chờ phản hồi từ DHT11
timeOut = 0;
// Chờ pin về LOW (Start signal của DHT11)
if (!wait_for_pin_state(DHT11_PORT, DHT11_PIN, 0, MAX_TIMEOUT)) return false;
timeOut = 0;
// Chờ pin lên HIGH (ACK response)
if (!wait_for_pin_state(DHT11_PORT, DHT11_PIN, 1, MAX_TIMEOUT)) return false;
timeOut = 0;
// Chờ pin về LOW (ACK response kết thúc)
if (!wait_for_pin_state(DHT11_PORT, DHT11_PIN, 0, MAX_TIMEOUT)) return false;


// 3. Đọc dữ liệu 40-bit
for (int i = 0; i < 40; i++)
{
// Chờ đầu xung (HIGH pulse bắt đầu)
timeOut = 0;
while (!GPIO_PinInGet(DHT11_PORT, DHT11_PIN) && timeOut++ < 100) sl_udelay_wait(1);
if (timeOut >= 100) return false;

sl_udelay_wait(30); // Delay 30µs để xác định bit (HIGH pulse 26-28µs là '0', 70µs là '1')

if (GPIO_PinInGet(DHT11_PORT, DHT11_PIN)) // Nếu chân vẫn ở mức cao sau 30µs (là bit '1')
data[i / 8] |= (1 << (7 - (i % 8)));

// Chờ cuối xung (HIGH pulse kết thúc)
timeOut = 0;
while (GPIO_PinInGet(DHT11_PORT, DHT11_PIN) && timeOut++ < 100) sl_udelay_wait(1);
if (timeOut >= 100) return false;
}


// 4. Kiểm tra checksum
if (data[4] != (data[0] + data[1] + data[2] + data[3]))
return false;


// 5. Chuyển đổi dữ liệu
humidity = data[0]; // Độ ẩm phần nguyên
humidity_decimal = data[1]; // Độ ẩm phần thập phân
temperature = data[2]; // Nhiệt độ phần nguyên
temperature_decimal = data[3]; // Nhiệt độ phần thập phân


return true; // Đọc dữ liệu thành công
}



int main(void)
{
// Initialize Silicon Labs device, system, service(s) and protocol stack(s).
// Note that if the kernel is present, processing task(s) will be created by
// this call.
sl_system_init();

// Khởi tạo các ngoại vi (GPIO, LED, Button)
initGPIO();
initLED_BUTTON();

// Initialize the application. For example, create periodic timer(s) or
// task(s) if the kernel is present.
app_init(); // Bắt đầu khởi tạo BLE và Timer

while (1){
  // 1. Đọc dữ liệu từ cảm biến DHT11 và cập nhật các biến toàn cục (humidity, temperature, ...)
  if(DHT11_ReadData()){
      // Log để xác nhận đọc thành công
      app_log("temp: %d.%d, humid: %d.%d, time: 1\r\n", temperature, temperature_decimal, humidity, humidity_decimal);

      // GỌI HÀM HIỂN THỊ LÊN LCD <--- ĐÃ KHÔI PHỤC VÀ ĐẶT ĐÚNG VỊ TRÍ
      memlcd_app_init(humidity, humidity_decimal, temperature, temperature_decimal);

      // GỌI HÀM CẬP NHẬT DỮ LIỆU SANG APP.C DÙNG CHO QUẢNG BÁ
      get_temp_hum(humidity, humidity_decimal, temperature, temperature_decimal);
  } else {
  }

  // 2. Xử lý các tác vụ của hệ thống (bao gồm cả việc kiểm tra timer và cập nhật quảng bá)
  sl_system_process_action();

  // 3. Trễ trước lần đọc cảm biến tiếp theo
  sl_sleeptimer_delay_millisecond(1000);
}
}
