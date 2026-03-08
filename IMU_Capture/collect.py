import serial
import time

# ======= 配置区 =======
COM_PORT = 'COM9'    # 记得改成你 Arduino 显示的 COM 号
BAUD_RATE = 115200
FILE_NAME = "flex.csv" # 想要存的文件名
# =====================

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    print(f"--- 正在连接 {COM_PORT} ---")
    
    with open(FILE_NAME, "w", encoding='utf-8') as f:
        print(f"--- 文件 {FILE_NAME} 已创建，等待板子数据... ---")
        
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                
                # 在控制台显示，让你看到实时反馈（相当于 Serial Monitor）
                print(line)
                
                # 写入文件
                f.write(line + "\n")
                
                # 逻辑终点：检测到板子发出的结束信号
                if "ALL_GESTURES_RECORDED" in line:
                    print("\n[完成] 10组数据已全部录制并保存！")
                    break
except Exception as e:
    print(f"错误: {e}")
finally:
    if 'ser' in locals():
        ser.close()