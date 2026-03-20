import serial
import time
import os

# ======= 配置区 =======
COM_PORT = 'COM9'    # 确认是你的 COM 号
BAUD_RATE = 115200
# 你可以手动改这里，或者取消下方 input() 的注释
FILE_NAME = "./raw_data/right.csv"
# FILE_NAME = input("请输入要保存的文件名 (如 down.csv): ")

LOG_FILE = "./log/pc_recorded_log.txt" # 统一存储芯片发来的所有日志
# =====================

def main():
    try:
        # 打开串口并触发 DTR (重要: 解决 XIAO while(!Serial) 阻塞)
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        ser.setDTR(True)
        ser.setRTS(True)
        
        print(f"--- 已连接 {COM_PORT} ---")
        print(f"--- 原始数据将存入: {FILE_NAME} ---")
        print(f"--- 芯片日志将存入: {LOG_FILE} ---")

        with open(FILE_NAME, "w", encoding='utf-8') as csv_f, \
             open(LOG_FILE, "a", encoding='utf-8') as log_f:
            
            log_f.write(f"\n--- 新采集会话开始: {time.ctime()} ---\n")

            while True:
                if ser.in_waiting > 0:
                    try:
                        line_raw = ser.readline().decode('utf-8', errors='ignore').strip()
                        if not line_raw:
                            continue
                            
                        # 1. 检查终止信号 (不写入文件)
                        if "EOF_DATA" in line_raw:
                            print("\n[通知] 收到终止信号，采集结束。")
                            break

                        # 2. 检查日志信号
                        if line_raw.startswith("[LOG]"):
                            clean_log = line_raw.replace("[LOG]", "").strip()
                            print(f"  [芯片日志]: {clean_log}")
                            log_f.write(f"{time.strftime('%H:%M:%S')} - {clean_log}\n")
                            log_f.flush() # 确保实时写入硬盘

                        # 3. 处理 CSV 数据 (必须包含逗号且不带 LOG 前缀)
                        elif "," in line_raw:
                            csv_f.write(line_raw + "\n")
                            # 为了不让屏幕太乱，这里不 print 每一行数据，只打点
                            print(".", end="", flush=True)

                    except Exception as e:
                        print(f"\n[错误] 处理行时出错: {e}")
        
        print(f"\n--- 保存成功！CSV 大小: {os.path.getsize(FILE_NAME)} bytes ---")

    except Exception as e:
        print(f"\n[严重错误]: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()