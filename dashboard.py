import streamlit as st
import pandas as pd
import altair as alt
import time
import requests  

# --- CẤU HÌNH ---

DB_URL = 'https://anhbaolmao1-default-rtdb.asia-southeast1.firebasedatabase.app'

st.set_page_config(page_title="Firebase Monitor", layout="wide")

# Hàm lấy dữ liệu qua HTTP (Không cần Key)
def get_data_http():
    try:
        # 1. Lấy dữ liệu hiện tại (Current)
        # Link: .../current.json
        resp_curr = requests.get(f"{DB_URL}/current.json")
        curr_data = resp_curr.json()
        
        # Debug in ra terminal để xem có lấy được không
        print(f"📡 Current Raw: {curr_data}")

        # 2. Lấy lịch sử (Sensor) - Lấy 30 cái cuối
        # Link: .../sensor.json?orderBy="$key"&limitToLast=30
        resp_hist = requests.get(f"{DB_URL}/sensor.json?orderBy=\"$key\"&limitToLast=30")
        hist_data = resp_hist.json()

        hist_list = []
        if hist_data:
            for key, val in hist_data.items():
                hist_list.append({
                    "Time": val.get('time', 'N/A'),
                    "Nhiệt độ": val.get('temperature', 0),
                    "Độ ẩm": val.get('humidity', 0)
                })
        
        return curr_data, pd.DataFrame(hist_list)

    except Exception as e:
        print(f"Lỗi HTTP: {e}")
        return None, pd.DataFrame()

# --- GIAO DIỆN ---
st.title("Dashboard ")
st.markdown("---")

current, df = get_data_http()

# Hiện số
col1, col2, col3 = st.columns(3)
if current:
    temp = current.get('temperature', 0)
    hum = current.get('humidity', 0)
    time_val = current.get('time', 'N/A')

    with col1: st.metric("Nhiệt độ", f"{temp} °C")
    with col2: st.metric("Độ ẩm", f"{hum} %")
    with col3: st.metric("Cập nhật", time_val)
    
    if temp > 40:
        st.error(f"CẢNH BÁO QUÁ NHIỆT!")
else:
    st.warning("Không lấy được dữ liệu. Hãy chắc chắn bạn đã set Rules trên Firebase thành '.read': true")

# Hiện biểu đồ
if not df.empty:
    st.markdown("### Biểu đồ Realtime")
    
    df_long = df.melt('Time', var_name='Loại', value_name='Giá trị')

    chart = alt.Chart(df_long).mark_line(point=True).encode(
        # X trục thời gian, thêm :N để báo là dữ liệu danh định (chuỗi/thời gian)
        x=alt.X('Time:N', axis=alt.Axis(labelAngle=-45, title='Thời gian')),
        
        # Y trục giá trị, thêm :Q để báo là dữ liệu định lượng (số)
        y=alt.Y('Giá trị:Q', title='Giá trị'),
        
        # Màu sắc phân biệt theo Loại, thêm :N (danh định)
        color=alt.Color('Loại:N', title='Thông số'),
        
        # Tooltip khi di chuột vào
        tooltip=['Time', 'Loại', 'Giá trị']
    ).properties(height=400)
    
    st.altair_chart(chart, use_container_width=True)

time.sleep(5)
st.rerun()