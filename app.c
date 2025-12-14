/***************************************************************************/ /**
                                                                               * @file
                                                                               * @brief Core application logic.
                                                                               ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h" // File này chứa ID: gattdb_temperature_0
#include "app.h"
#include "custom_adv.h" // File chứa hàm update_adv_data
#include "app_timer.h"
#include "app_log.h"

#include "em_cmu.h"
#include "em_gpio.h"

// --- BIẾN TOÀN CỤC ---
static uint8_t connection_handle = 0xff; // 0xff: Chưa kết nối

CustomAdv_t sData;
static app_timer_t update_timer;
static uint8_t advertising_set_handle = 0xff;

// ID và Tên cho phần Quảng bá (Advertising)
#define MY_COMPANY_ID 0x02FF
#define MY_DEV_NAME "nhom1"

// Biến lưu nhiệt độ độ ẩm (được extern từ main.c)
uint8_t humidity, humidity_decimal, temperature, temperature_decimal;

void get_temp_hum(uint8_t hum, uint8_t hum_decimal, uint8_t temp, uint8_t temp_decimal)
{
  humidity = hum;
  humidity_decimal = hum_decimal;
  temperature = temp;
  temperature_decimal = temp_decimal;
}

// --- TIMER CALLBACK (XỬ LÝ CHÍNH) ---
static void update_timer_cb(app_timer_t *timer, void *data)
{
  (void)data;
  (void)timer;
  sl_status_t sc;

  // --- PHẦN 1: TÍNH TOÁN GIÁ TRỊ ---
  // Cho GATT (Nhân 100 để lấy 2 số thập phân)
  int16_t temp_val = (int16_t)(temperature * 100) + (int16_t)temperature_decimal;
  uint16_t hum_val = (uint16_t)(humidity * 100) + (uint16_t)humidity_decimal;

  // --- PHẦN 2: CẬP NHẬT GATT (ĐỂ APP HIỂN THỊ SỐ) ---

  // A. GHI VÀO BỘ NHỚ (Sửa lỗi "Zero bro")
  // Việc này giúp lệnh "Read" của App thấy số ngay lập tức
  sl_bt_gatt_server_write_attribute_value(gattdb_temperature_0, 0, 2, (uint8_t *)&temp_val);
  sl_bt_gatt_server_write_attribute_value(gattdb_humidity_0, 0, 2, (uint8_t *)&hum_val);

  // B. GỬI THÔNG BÁO (NOTIFY)
  // Chỉ gửi nếu đang có kết nối
  if (connection_handle != 0xff)
  {
    sl_bt_gatt_server_send_notification(connection_handle, gattdb_temperature_0, 2, (uint8_t *)&temp_val);
    sl_bt_gatt_server_send_notification(connection_handle, gattdb_humidity_0, 2, (uint8_t *)&hum_val);
  }

  // --- PHẦN 3: CẬP NHẬT QUẢNG BÁ (ĐỂ SCANNER THẤY HEX) ---
  // Hàm này cập nhật gói tin quảng bá mà không cần ngắt kết nối
  update_adv_data(&sData, advertising_set_handle, temperature, temperature_decimal, humidity, humidity_decimal);

  // Log ngắn gọn
  // app_log("Timer Tick: T=%d.%d H=%d.%d\r\n", temperature, temperature_decimal, humidity, humidity_decimal);
}

SL_WEAK void app_init(void)
{
  sl_status_t sc;
  // Timer chạy mỗi 1s (1000ms)
  sc = app_timer_start(&update_timer, 1000, update_timer_cb, NULL, true);
  app_assert_status(sc);
}

SL_WEAK void app_process_action(void)
{
}

void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header))
  {

  // --- KHỞI ĐỘNG ---
  case sl_bt_evt_system_boot_id:
    sc = sl_bt_system_get_identity_address(&address, &address_type);
    app_assert_status(sc);

    // Tạo System ID
    system_id[0] = address.addr[5];
    system_id[1] = address.addr[4];
    system_id[2] = address.addr[3];
    system_id[3] = 0xFF;
    system_id[4] = 0xFE;
    system_id[5] = address.addr[2];
    system_id[6] = address.addr[1];
    system_id[7] = address.addr[0];

    sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id, 0, sizeof(system_id), system_id);
    app_assert_status(sc);

    // Tạo Advertising Set
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);

    // Cấu hình tần suất (100ms min, 100ms max để kết nối nhanh)
    sc = sl_bt_advertiser_set_timing(advertising_set_handle, 160, 160, 0, 0);
    app_assert_status(sc);

    // --- CẤU HÌNH GÓI QUẢNG BÁ (CUSTOM ADV) ---
    // Tạo gói tin ban đầu với dữ liệu 0
    fill_adv_packet(&sData, FLAG, MY_COMPANY_ID, 0, 0, 0, 0, MY_DEV_NAME);

    // Bắt đầu quảng bá (Connectable + Scannable)
    sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);

    app_log("System Booted. Custom Advertising started.\r\n");
    break;

  // --- KẾT NỐI MỞ ---
  case sl_bt_evt_connection_opened_id:
    connection_handle = evt->data.evt_connection_opened.connection;
    app_log("Connected! Handle: %d\r\n", connection_handle);

    // Tùy chọn: Có thể dừng quảng bá khi đã kết nối để tiết kiệm pin
    // sl_bt_advertiser_stop(advertising_set_handle);
    break;

  // --- NGẮT KẾT NỐI ---
  case sl_bt_evt_connection_closed_id:
    app_log("Disconnected. Restarting Advertising...\r\n");
    connection_handle = 0xff;

    // Cập nhật lại gói tin quảng bá lần cuối trước khi phát lại
    // (Dữ liệu sData vẫn được update liên tục trong Timer nên không cần fill lại)
    sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, sData.data_size, (const uint8_t *)&sData);

    // Bắt đầu quảng bá lại
    sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
    app_assert_status(sc);
    break;

  default:
    break;
  }
}
