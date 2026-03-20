// 依次挥动 up down left right

#include <LSM6DS3.h>
#include <Wire.h>
// #include <Adafruit_LittleFS.h>
// #include <InternalFileSystem.h>
// using namespace Adafruit_LittleFS_Namespace;

// File logFile(InternalFS);
LSM6DS3 myIMU(I2C_MODE, 0x6A);
float aX, aY, aZ, gX, gY, gZ;

int accelerationThreshold = 10;
int loopCount = 0;
// int gestureCount = 0;
// const int gestureMax = 4; // 上下左右四种动作
const int samplesPerType = 5; // 每种动作采样次数
const int sampleMax = 100;
int sampleRead = sampleMax;

void setup(){
    Serial.begin(115200);
    // while(!Serial); // 最开始没有亮灯, 先打开python程序接管Serial Monitor

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BLUE, HIGH);

    // if(!InternalFS.begin()) return;
    // digitalWrite(LED_BLUE, LOW); // 挂载成功蓝灯亮3s后熄灭
    // delay(3000);
    // logFile = InternalFS.open("/log.txt", FILE_O_WRITE);
    // if(!logFile){
    //     // digitalWrite(LED_GREEN, HIGH);
    //     digitalWrite(LED_RED, LOW); // 文件打开失败红灯亮3s
    //     delay(3000);
    //     return;
    // }else{
    //     logFile.println("Hello XIAO nRF52840 Sense");
    // }
    
    if(myIMU.begin() != 0){
        // logFile.println("IMU failed to begin");
        // digitalWrite(LED_GREEN, HIGH);
        // Serial.println("[LOG] IMU failed to begin");
        for(int i = 0; i < 3; i++){
            digitalWrite(LED_RED, LOW);
            delay(500);
            digitalWrite(LED_RED, HIGH);
            delay(500); // 红灯亮暗忽闪三次后退出, 表示myIMU初始化失败
        }
        exit(1);
    }
    // Serial.println("[LOG] Hello XIAO nRF52840 Sense");

    // Serial.println("ax,ay,az,gx,gy,gz");
    // digitalWrite(LED_GREEN, HIGH);
    // digitalWrite(LED_BLUE, LOW);
    // delay(3000); // 蓝灯亮3s之后熄灭, 开始loop
    // digitalWrite(LED_BLUE, HIGH);
    // Serial.println("[LOG] setup done and loop started to capture");
}

void loop(){
    // if(gestureCount == gestureMax){
    //     for(int i = 0; i < 3; i++){
    //         digitalWrite(LED_RED, LOW);
    //         delay(500);
    //         digitalWrite(LED_RED, HIGH);
    //         delay(500); // 红灯亮暗忽闪三次, 数据收集全部完成, 程序退出
    //     }
    //     digitalWrite(LED_RED, HIGH);
    //     return;
    // }
    while(!Serial);

    digitalWrite(LED_BLUE, LOW);
    delay(3000);
    digitalWrite(LED_BLUE, HIGH); // 蓝灯亮3s之后熄灭,表示可以开始采样
    Serial.println("[LOG] Hello XIAO nRF52840 Sense");

    Serial.println("ax,ay,az,gx,gy,gz");
    for(int i = 0; i < samplesPerType; i++){
        while(sampleRead == sampleMax){
            float ax = myIMU.readFloatAccelX();
            float ay = myIMU.readFloatAccelY();
            float az = myIMU.readFloatAccelZ();

            float aSum = fabs(ax) + fabs(ay) + fabs(az);
            if(aSum >= accelerationThreshold){
                sampleRead = 0;
            }
        }
        while(sampleRead < sampleMax){
            sampleRead++;

            Serial.print(myIMU.readFloatAccelX(), 3);
            Serial.print(',');
            Serial.print(myIMU.readFloatAccelY(), 3);
            Serial.print(',');
            Serial.print(myIMU.readFloatAccelZ(), 3);
            Serial.print(',');
            Serial.print(myIMU.readFloatGyroX(), 3);
            Serial.print(',');
            Serial.print(myIMU.readFloatGyroY(), 3);
            Serial.print(',');
            Serial.print(myIMU.readFloatGyroZ(), 3);
            Serial.println();
        }
        // Serial.println();
    }
    // Serial.println("END_OF_GESTURE_TYPE");

    // digitalWrite(LED_GREEN, LOW);
    // delay(3000);
    // digitalWrite(LED_GREEN, HIGH); // 绿灯亮3s之后熄灭, 一个gesture采样完成

    char buf[50];
    sprintf(buf, "[LOG] gesture loopCount: %d", ++loopCount);
    Serial.println(buf);
    // Serial.println("[LOG] gesture loopCount: %d", ++loopCount);
    Serial.println("EOF_DATA");
    for(int i = 0; i < 3; i++){
        digitalWrite(LED_GREEN, LOW);
        delay(500);
        digitalWrite(LED_GREEN, HIGH);
        delay(500); // 绿灯亮暗忽闪三次, 单次数据收集完成
    }

    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    // exit(0);
    // char buf[50];
    // sprintf(buf, "gesture number %d", gestureCount + 1);
    // logFile.println(buf);
    // gestureCount++;
}