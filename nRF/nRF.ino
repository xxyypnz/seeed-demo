// XIAO BLE Sense LSM6DS3 加速度计数据读取
#include <LSM6DS3.h>
#include <Wire.h>

// 创建实例，XIAO Sense 的 I2C 地址是 0x6A
LSM6DS3 myIMU(I2C_MODE, 0x6A); 

#define CONVERT_G_TO_MS2 9.80665f
#define FREQUENCY_HZ 50
// 修正间隔计算逻辑
const long INTERVAL_MS = 1000 / FREQUENCY_HZ;

unsigned long last_interval_ms = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // 关键：强制等待串口连接，不打开监视器程序就不往下走

  Serial.println("--- 系统启动 ---");

  if (myIMU.begin() != 0) {
    Serial.println("IMU 初始化失败！");
    // 如果失败了，让 LED 灯亮红色提醒
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, LOW); 
  } else {
    Serial.println("IMU 初始化成功!");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 定时采集数据
  if (currentMillis - last_interval_ms >= INTERVAL_MS) {
    last_interval_ms = currentMillis;

    // 读取加速度数据
    float ax = myIMU.readFloatAccelX();
    float ay = myIMU.readFloatAccelY();
    float az = myIMU.readFloatAccelZ();

    // 打印到串口（适合串口绘图器查看曲线）
    Serial.print(ax * CONVERT_G_TO_MS2, 4);
    Serial.print(",");
    Serial.print(ay * CONVERT_G_TO_MS2, 4);
    Serial.print(",");
    Serial.println(az * CONVERT_G_TO_MS2, 4);
  }
}