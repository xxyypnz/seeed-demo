// #define EI_CLASSIFIER_DISABLE_HW_FFT 1

#include <TinyML-test_inferencing.h>
#include <LSM6DS3.h>
#include <bluefruit.h>
#include <Wire.h>

LSM6DS3 myIMU(I2C_MODE, 0x6A);
BLEUart bleuart;

float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
int buffer_pos = 0;

void setup() {
    Serial.begin(115200);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_RED, HIGH); // 先关掉
    digitalWrite(LED_BLUE, HIGH); // 先关掉

    while(!Serial){
        digitalWrite(LED_RED, LOW); delay(500);
        digitalWrite(LED_RED, HIGH); delay(500);
    }

    delay(3000);
    // for(int i=0; i<10; i++) Serial.println();
    Serial.println("===============================");
    Serial.println("   USB SERIAL DETECTED!        ");
    Serial.println("===============================");

    // 传感器初始化
    pinMode(PIN_LSM6DS3TR_C_POWER, OUTPUT);
    digitalWrite(PIN_LSM6DS3TR_C_POWER, HIGH);
    delay(100);
    if (myIMU.begin() != 0) Serial.println("IMU Error!");
    else Serial.println("IMU initialized.");

    // 蓝牙配置
    Bluefruit.begin();
    Bluefruit.setName("XIAO-AI-Terminal");
    bleuart.begin();

    // 广播配置
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.start(0);

    Serial.println("System Ready. Connect phone and wave the board!");
}

void loop() {
    unsigned long start_time = millis();

    // --- 逻辑 A: 手机消息转发 (Phone -> PC) ---
    if (bleuart.available()) {
        Serial.print("[BLE MSG]: ");
        while (bleuart.available()) {
            Serial.print((char)bleuart.read());
        }
        Serial.println();
    }

    // --- 逻辑 B: 数据采样 ---
    buffer[buffer_pos++] = myIMU.readFloatAccelX();
    buffer[buffer_pos++] = myIMU.readFloatAccelY();
    buffer[buffer_pos++] = myIMU.readFloatAccelZ();

    if (buffer_pos >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        signal_t signal;
        numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

        if (res == EI_IMPULSE_OK) {
            // 无论结果如何，我们都调用输出函数，内部去做细节判断
            handle_inference_result(result);
        } else {
            Serial.print("Inference Error Code: ");
            Serial.println(res);
        }
        buffer_pos = 0;
    }

    // 保持采样频率 (50Hz)
    int elapsed = millis() - start_time;
    if (elapsed < 20) delay(20 - elapsed);
}

void handle_inference_result(ei_impulse_result_t result) {
    int max_idx = -1;
    float max_val = 0.0;

    // 1. 找出得分最高的项
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > max_val) {
            max_val = result.classification[ix].value;
            max_idx = ix;
        }
    }

    // 2. 调试输出：每一轮推理都印一个点，代表 AI 还在跑
    // Serial.print("."); 

    // 3. 核心判断：降低门槛到 0.5 观察情况
    if (max_val > 0.5) {
        const char* label = result.classification[max_idx].label;
        
        // 如果是有效动作（非 idle）
        if (strcmp(label, "idle") != 0) {
            Serial.print("\n>>> Detected: ");
            Serial.print(label);
            Serial.print(" (");
            Serial.print(max_val);
            Serial.println(")");

            if (Bluefruit.connected()) {
                bleuart.print("AI: ");
                bleuart.println(label);
            }
        }
    }
}