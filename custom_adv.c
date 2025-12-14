/***************************************************************************//**
 * @file custom_adv.c
 * @brief customize advertising
 ******************************************************************************/

#include <string.h>
#include "custom_adv.h"
#include "stdio.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app_log.h"

// --- HÀM MỚI: CHUYỂN ĐỔI SỐ THẬP PHÂN SANG "HEX ĐỌC ĐƯỢC" ---
// Ví dụ: Input 24 -> Output 0x24 (thực chất là số 36 thập phân)
uint8_t decimal_to_hex_readable(uint8_t dec_val) {
    if (dec_val > 99) return 0xFF; // Giới hạn 2 chữ số
    return (dec_val / 10) * 16 + (dec_val % 10);
}

// Hàm tạo gói tin ban đầu
void fill_adv_packet(CustomAdv_t *pData, uint8_t flags, uint16_t companyID,
                     uint8_t t_int, uint8_t t_dec, uint8_t h_int, uint8_t h_dec,
                     char *name)
{
  int n;
  pData->len_flags = 0x02;
  pData->type_flags = 0x01;
  pData->val_flags = flags;

  pData->len_manuf = 7;
  pData->type_manuf = 0xFF;
  pData->company_LO = companyID & 0xFF;
  pData->company_HI = (companyID >> 8) & 0xFF;

  // --- ÁP DỤNG CHUYỂN ĐỔI TẠI ĐÂY ---
  pData->temp_int = decimal_to_hex_readable(t_int);
  pData->temp_dec = decimal_to_hex_readable(t_dec);
  pData->hum_int  = decimal_to_hex_readable(h_int);
  pData->hum_dec  = decimal_to_hex_readable(h_dec);

  n = strlen(name);
  if (n > NAME_MAX_LENGTH) {
    pData->type_name = 0x08;
  } else {
    pData->type_name = 0x09;
  }

  if (n > NAME_MAX_LENGTH) n = NAME_MAX_LENGTH;
  strncpy(pData->name, name, n);

  pData->len_name = 1 + n;
  pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);
}

void start_adv(CustomAdv_t *pData, uint8_t advertising_set_handle)
{
  sl_status_t sc;

  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, pData->data_size, (const uint8_t *)pData);
  app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to set advertising data\n", (int)sc);

  sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
  app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to start advertising\n", (int)sc);
}

void update_adv_data(CustomAdv_t *pData, uint8_t advertising_set_handle,
                     uint8_t t_int, uint8_t t_dec, uint8_t h_int, uint8_t h_dec)
{
  sl_status_t sc;

  // --- ÁP DỤNG CHUYỂN ĐỔI TẠI ĐÂY ---
  // Chuyển số 24 thành 0x24, số 5 thành 0x05...
  pData->temp_int = decimal_to_hex_readable(t_int);
  pData->temp_dec = decimal_to_hex_readable(t_dec);
  pData->hum_int  = decimal_to_hex_readable(h_int);
  pData->hum_dec  = decimal_to_hex_readable(h_dec);

  // Thử cập nhật dữ liệu
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, pData->data_size, (const uint8_t *)pData);

  // Xử lý lỗi nếu đang phát quảng bá (Lỗi 0x21)
  if (sc == SL_STATUS_INVALID_STATE) {
      sl_bt_advertiser_stop(advertising_set_handle);

      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, pData->data_size, (const uint8_t *)pData);

      // Start lại
      sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
  }
}
