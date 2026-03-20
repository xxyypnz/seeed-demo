import numpy as np
import tensorflow as tf
from tensorflow import keras
from sklearn.model_selection import train_test_split

# ======= 配置 =======
GESTURES     = ["up", "down", "left", "right"]
NUM_CLASSES  = len(GESTURES)   # 4
TIMESTEPS    = 10
FEATURES     = 6
BATCH_SIZE   = 16
EPOCHS       = 100
VAL_SPLIT    = 0.2
PATIENCE     = 15
RANDOM_SEED  = 42
# ====================


# ── 1. 加载数据 ──────────────────────────────────────────────
X = np.load("./train_data/X.npy")   # (198, 10, 6)
y = np.load("./train_data/y.npy")   # (198,)  整数标签 0~3

print(f"X shape: {X.shape}, y shape: {y.shape}")
for i, g in enumerate(GESTURES):
    print(f"  {g}: {np.sum(y == i)} 样本")


# ── 2. 标签 one-hot 编码 ──────────────────────────────────────
# 原始 y 是整数，categorical_crossentropy 需要 one-hot 格式
# [0,1,2,3] -> [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
y_onehot = keras.utils.to_categorical(y, num_classes=NUM_CLASSES)


# ── 3. 划分训练集 / 验证集 ────────────────────────────────────
# stratify=y 保证每类手势在训练集和验证集中比例相同
X_train, X_val, y_train, y_val = train_test_split(
    X, y_onehot,
    test_size=VAL_SPLIT,
    random_state=RANDOM_SEED,
    stratify=y
)
print(f"\n训练集: {X_train.shape[0]} 样本，验证集: {X_val.shape[0]} 样本")


# ── 4. 构建模型 ───────────────────────────────────────────────
# Conv1D 沿时间轴滑动卷积核，提取局部时序模式
# GlobalAveragePooling1D 将时间轴压缩为一个向量，减少参数量
inputs = keras.Input(shape=(TIMESTEPS, FEATURES))

x = keras.layers.Conv1D(filters=16, kernel_size=3, activation="relu", padding="same")(inputs)
x = keras.layers.Conv1D(filters=32, kernel_size=3, activation="relu", padding="same")(x)
x = keras.layers.GlobalAveragePooling1D()(x)
x = keras.layers.Dense(32, activation="relu")(x)
x = keras.layers.Dropout(0.3)(x)   # 随机丢弃30%神经元，缓解小数据集过拟合
outputs = keras.layers.Dense(NUM_CLASSES, activation="softmax")(x)

model = keras.Model(inputs, outputs)
model.summary()


# ── 5. 编译 ───────────────────────────────────────────────────
model.compile(
    optimizer="adam",
    loss="categorical_crossentropy",
    metrics=["accuracy"]
)


# ── 6. 训练 ───────────────────────────────────────────────────
# EarlyStopping: 验证集 accuracy 连续 PATIENCE 轮不改善就停止
# restore_best_weights: 停止时自动恢复验证集最佳权重
early_stop = keras.callbacks.EarlyStopping(
    monitor="val_accuracy",
    patience=PATIENCE,
    restore_best_weights=True,
    verbose=1
)

history = model.fit(
    X_train, y_train,
    validation_data=(X_val, y_val),
    epochs=EPOCHS,
    batch_size=BATCH_SIZE,
    callbacks=[early_stop],
    verbose=1
)


# ── 7. 评估 ───────────────────────────────────────────────────
val_loss, val_acc = model.evaluate(X_val, y_val, verbose=0)
print(f"\n验证集最终准确率: {val_acc:.4f}  Loss: {val_loss:.4f}")


# ── 8. 保存模型 ───────────────────────────────────────────────
model.save("./model/model.h5")
print("已保存 Keras 模型: model.h5")

# 转换为 TFLite（用于嵌入式部署）
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()
with open("./model/model.tflite", "wb") as f:
    f.write(tflite_model)
print("已保存 TFLite 模型: model.tflite")
