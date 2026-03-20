#include <LSM6DS3.h>
#include <Wire.h>
#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model_data.h"

// ── 配置（必须与训练完全一致） ──────────────────────────────
const int   SAMPLES_PER_GESTURE = 10;
const int   NUM_AXES            = 6;
const float ACCEL_SCALE         = 4.0f;
const float GYRO_SCALE          = 2000.0f;
const float TRIGGER_THRESHOLD   = 2.5f;
const char* GESTURE_LABELS[]    = {"up", "down", "left", "right"};
const int   NUM_CLASSES         = 4;
// ────────────────────────────────────────────────────────────

constexpr int TENSOR_ARENA_SIZE = 8 * 1024;
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

tflite::ErrorReporter*    error_reporter = nullptr;
const tflite::Model*      tfl_model      = nullptr;
tflite::MicroInterpreter* interpreter    = nullptr;
TfLiteTensor*             input_tensor   = nullptr;
TfLiteTensor*             output_tensor  = nullptr;

LSM6DS3 myIMU(I2C_MODE, 0x6A);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // ErrorReporter：DebugLog 由 debug_log_impl.cpp 提供
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED,   OUTPUT);
    pinMode(LED_BLUE,  OUTPUT);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_BLUE,  HIGH);

    // 初始化 IMU
    if (myIMU.begin() != 0) {
        Serial.println("[ERR] IMU init failed");
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_RED, LOW);  delay(300);
            digitalWrite(LED_RED, HIGH); delay(300);
        }
        while (1);
    }

    // 加载模型
    tfl_model = tflite::GetModel(model_tflite);
    if (tfl_model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[ERR] Model schema version mismatch");
        while (1);
    }

    // 模型实际算子: EXPAND_DIMS x2, CONV_2D x2, RESHAPE x2, MEAN, FULLY_CONNECTED x2, SOFTMAX
    static tflite::MicroMutableOpResolver<6> resolver;
    resolver.AddExpandDims();
    resolver.AddConv2D();
    resolver.AddReshape();
    resolver.AddMean();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();

    static tflite::MicroInterpreter static_interpreter(
        tfl_model, resolver, tensor_arena, TENSOR_ARENA_SIZE, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[ERR] AllocateTensors failed");
        while (1);
    }

    input_tensor  = interpreter->input(0);
    output_tensor = interpreter->output(0);

    Serial.println("[OK] Model loaded. Waiting for gesture...");
    digitalWrite(LED_GREEN, LOW);  // 绿灯常亮：就绪
}

void loop() {
    // ── 1. 等待触发 ──────────────────────────────────────────
    float ax = myIMU.readFloatAccelX();
    float ay = myIMU.readFloatAccelY();
    float az = myIMU.readFloatAccelZ();

    if ((fabs(ax) + fabs(ay) + fabs(az)) < TRIGGER_THRESHOLD) {
        return;
    }

    // ── 2. 采集 10 行，归一化后填入输入张量 ─────────────────
    digitalWrite(LED_BLUE, LOW);

    int idx = 0;
    for (int s = 0; s < SAMPLES_PER_GESTURE; s++) {
        input_tensor->data.f[idx++] = myIMU.readFloatAccelX() / ACCEL_SCALE;
        input_tensor->data.f[idx++] = myIMU.readFloatAccelY() / ACCEL_SCALE;
        input_tensor->data.f[idx++] = myIMU.readFloatAccelZ() / ACCEL_SCALE;
        input_tensor->data.f[idx++] = myIMU.readFloatGyroX()  / GYRO_SCALE;
        input_tensor->data.f[idx++] = myIMU.readFloatGyroY()  / GYRO_SCALE;
        input_tensor->data.f[idx++] = myIMU.readFloatGyroZ()  / GYRO_SCALE;
    }

    digitalWrite(LED_BLUE, HIGH);

    // ── 3. 推理 ───────────────────────────────────────────────
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("[ERR] Invoke failed");
        return;
    }

    // ── 4. 找最大概率类别 ────────────────────────────────────
    int   best_idx   = 0;
    float best_score = output_tensor->data.f[0];
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (output_tensor->data.f[i] > best_score) {
            best_score = output_tensor->data.f[i];
            best_idx   = i;
        }
    }

    // ── 5. 输出到 Serial Monitor ─────────────────────────────
    Serial.print(GESTURE_LABELS[best_idx]);
    Serial.print("  (");
    for (int i = 0; i < NUM_CLASSES; i++) {
        Serial.print(GESTURE_LABELS[i]);
        Serial.print(":");
        Serial.print(output_tensor->data.f[i], 3);
        if (i < NUM_CLASSES - 1) Serial.print("  ");
    }
    Serial.println(")");

    // 绿灯闪一下：识别完成
    digitalWrite(LED_GREEN, HIGH); delay(150);
    digitalWrite(LED_GREEN, LOW);

    // 冷却 1 秒，防止同一次挥动重复触发
    delay(1000);
}
