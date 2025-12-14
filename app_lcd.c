/*
 * lcd.c
 *
 *  Created on: Nov 4, 2024
 *      Author: Phat_Dang
 */
#include <stdio.h>

#include "sl_board_control.h"
#include "em_assert.h"
#include "glib.h"
#include "dmd.h"
#include <stdio.h>
#include <string.h>
#include "sl_sleeptimer.h"

#ifndef LCD_MAX_LINES
#define LCD_MAX_LINES      11
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static GLIB_Context_t glibContext;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void memlcd_app_init(uint8_t humidity, uint8_t humidity_decimal, uint8_t temperature, uint8_t temperature_decimal)
{
  uint32_t status;

  /* Enable the memory lcd */
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  /* Initialize the DMD support for memory lcd display */
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  /* Initialize the glib context */
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  /* Use Narrow font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);
  //edit here
  char temp[10], hum[10];  // Dùng mảng đủ lớn để chứa kết quả
  char ten_hum[] = "Hum: ";
  char ten_temp[] = "Temp: ";
  char dv_hum[] = "%";
  char dv_temp[] = "°C";

      // Tạo chuỗi độ ẩm
      hum[0] = (humidity / 10) + '0';    // Chục của độ ẩm
      hum[1] = (humidity % 10) + '0';    // Đơn vị của độ ẩm
      hum[2] = '.';                      // Dấu thập phân
      hum[3] = (humidity_decimal % 10) + '0';  // Phần thập phân của độ ẩm
      hum[4] = '\0';  // Kết thúc chuỗi

      // Tạo chuỗi nhiệt độ
      temp[0] = (temperature / 10) + '0';  // Chục của nhiệt độ
      temp[1] = (temperature % 10) + '0';  // Đơn vị của nhiệt độ
      temp[2] = '.';                       // Dấu thập phân
      temp[3] = (temperature_decimal % 10) + '0';  // Phần thập phân của nhiệt độ
      temp[4] = '\0';  // Kết thúc chuỗi

      // Ghép các chuỗi lại thành chuỗi cuối
      char tempu[50], humi[50];  // Dùng mảng đủ lớn để chứa kết quả

      strcpy(tempu, ten_temp);    // Sao chép "Temperature: "
      strcat(tempu, temp);        // Ghép nhiệt độ vào

      strcpy(humi, ten_hum);      // Sao chép "Humidity: "
      strcat(humi, hum);          // Ghép độ ẩm vào

      // Thêm đơn vị vào cuối
      strcat(humi, dv_hum);       // Ghép đơn vị độ ẩm
      strcat(tempu, dv_temp);     // Ghép đơn vị nhiệt độ

  /* Draw text on the memory lcd display*/

  GLIB_drawStringOnLine(&glibContext,
                        "Ca 1 - Nhom 1",
                        1,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);
  GLIB_drawStringOnLine(&glibContext,
                        "FETEL CLC",
                        2,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);

  GLIB_drawStringOnLine(&glibContext,
                        tempu,
                        3,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);
  GLIB_drawStringOnLine(&glibContext,
                        humi,
                        4,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);
  DMD_updateDisplay();

}


/***************************************************************************//**
 * Ticking function.
 ******************************************************************************/
void memlcd_app_process_action(void)
{
  return;
}


