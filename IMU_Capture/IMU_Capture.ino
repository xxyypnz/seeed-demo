#include <LSM6DS3.h>
#include <Wire.h>

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
float aX, aY, aZ, gX, gY, gZ;
// Q=== 这里能不能提高加速度的阈值确保一定探测到动作发出?
// 原来是2.5G
const float accelerationThreshold = 5; // threshold of significant in G's
const int numSamples = 119;
int samplesRead = numSamples;

int gestureCount = 0;
const int gestureMax = 10;
bool isFinished = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  // added on 2026-03-08
  // 开始时刻阻塞亮白灯
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

  Serial.begin(115200);
  while (!Serial);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH); // 灯熄灭表示Serial Monitor成功打开
  delay(2000);

  //Call .begin() to configure the IMUs
  if (myIMU.begin() != 0) {
    Serial.println("Device error");
    while(1);
  }
  for(int i = 0; i < 3; i++){
    digitalWrite(LED_RED, LOW);
    delay(400);
    digitalWrite(LED_RED, HIGH);
    delay(600);
  }
  Serial.println("aX,aY,aZ,gX,gY,gZ");
  digitalWrite(LED_BLUE, LOW); // 蓝灯常亮标志表示可以发出动作了
}

void loop() {
  if(isFinished) return;
  // wait for significant motion
  while (samplesRead == numSamples) {
    // read the acceleration data
    aX = myIMU.readFloatAccelX();
    aY = myIMU.readFloatAccelY();
    aZ = myIMU.readFloatAccelZ();

    // sum up the absolutes
    float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

    // check if it's above the threshold
    if (aSum >= accelerationThreshold) {
      // reset the sample read count
      // while(!Serial){}
      samplesRead = 0;
      digitalWrite(LED_BLUE, HIGH);
      digitalWrite(LED_RED, LOW); // 蓝灯熄灭, 红灯常亮标志每一次有效动作的开始
      break;
    }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // check if both new acceleration and gyroscope data is
    // available
    // read the acceleration and gyroscope data

    samplesRead++;

#if 1
    // print the data in CSV format
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
#endif

    // 这里改成保存到一个csv文件

    if (samplesRead == numSamples) {
      gestureCount++;
      digitalWrite(LED_RED, HIGH);
      // add an empty line if it's the last sample
      // added on 2026-03-06 tensorflow以一行作为区分样本的标志
      Serial.println();

      if(gestureCount >= gestureMax){
        isFinished = true;
        digitalWrite(LED_GREEN, LOW);
      }else{
        digitalWrite(LED_BLUE, LOW);
      }

      // added on 2026-03-06
      // sampling_count--;
    }
  }
}