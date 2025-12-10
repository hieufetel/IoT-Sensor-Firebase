import streamlit as st
import socket
import threading
import time
import re
import pandas as pd
from datetime import datetime
from streamlit.runtime.scriptrunner import add_script_run_ctx
import altair as alt

# --- CẤU HÌNH ---
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
MAX_HISTORY = 20  

st.set_page_config(page_title="IoT Monitor Pro", page_icon="📈", layout="wide")

# --- KHỞI TẠO DỮ LIỆU ---
if 'temp' not in st.session_state: st.session_state.temp = 0.0
if 'hum' not in st.session_state: st.session_state.hum = 0.0
if 'history' not in st.session_state: st.session_state.history = []
if 'alert' not in st.session_state: st.session_state.alert = ""   # <<< ALERT NEW

# --- LUỒNG NHẬN UDP ---
def udp_listener_job():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        sock.bind((UDP_IP, UDP_PORT))
    except OSError:
        return

    while True:
        try:
            data, _ = sock.recvfrom(1024)
            text = data.decode('utf-8')

            numbers = re.findall(r"[-+]?\d*\.\d+|\d+", text)
            if len(numbers) >= 2:
                new_temp = float(numbers[0])
                new_hum = float(numbers[1])

                st.session_state.temp = new_temp
                st.session_state.hum = new_hum

                # --- ALERT REALTIME ---
                if new_temp > 40:
                    st.session_state.alert = f"QUÁ NHIỆT! ({new_temp} °C)"
                else:
                    st.session_state.alert = ""

                # Lưu lịch sử nếu giá trị thay đổi
                save_flag = False
                if not st.session_state.history:
                    save_flag = True
                else:
                    last = st.session_state.history[-1]
                    if new_temp != last["Nhiệt độ"] or new_hum != last["Độ ẩm"]:
                        save_flag = True

                if save_flag:
                    current_time = datetime.now().strftime("%H:%M:%S")
                    st.session_state.history.append({
                        "Time": current_time,
                        "Nhiệt độ": new_temp,
                        "Độ ẩm": new_hum
                    })

                    if len(st.session_state.history) > MAX_HISTORY:
                        st.session_state.history.pop(0)

        except Exception:
            pass

@st.cache_resource
def start_background_listener():
    t = threading.Thread(target=udp_listener_job, daemon=True)
    add_script_run_ctx(t)
    t.start()
    return t

start_background_listener()

# --- UI ---
st.title("📈 Dashboard")
st.markdown("---")

# --- ALERT HIỂN THỊ ---
if st.session_state.alert:
    st.error(st.session_state.alert)

# HIỂN THỊ CHỈ SỐ
col1, col2 = st.columns(2)
with col1:
    st.metric("Nhiệt độ (°C)", f"{st.session_state.temp} °C")
with col2:
    st.metric("Độ ẩm (%)", f"{st.session_state.hum} %")

# BIỂU ĐỒ
st.markdown("### Biểu đồ Nhiệt độ và Độ ẩm RealTime")

if len(st.session_state.history) > 0:
    df = pd.DataFrame(st.session_state.history)
    chart = alt.Chart(df).transform_fold(
        ["Nhiệt độ", "Độ ẩm"],
        as_=["Loại", "Giá trị"]
    ).mark_line(point=True).encode(
        x=alt.X("Time:N", axis=alt.Axis(labelAngle=0)),
        y=alt.Y("Giá trị:Q"),
        color="Loại:N",
        tooltip=["Time", "Loại:N", "Giá trị:Q"]
    ).properties(
        width="container",
        height=350
    )
    st.altair_chart(chart, use_container_width=True)
else:
    st.info("Đang chờ dữ liệu...")

# AUTO REFRESH
time.sleep(5)
st.rerun()
