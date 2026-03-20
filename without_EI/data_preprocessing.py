import pandas as pd
import numpy as np
import os

# 配置信息
GESTURES = ["up", "down", "left", "right"]
SAMPLES_PER_GESTURE = 10  # 每组10行
AXES = 6                  # 6轴数据

def load_data(folder_path):
    all_inputs = []
    all_outputs = []

    for index, gesture in enumerate(GESTURES):
        file_path = os.path.join(folder_path, f"./raw_data/{gesture}.csv")

        # 逐行读取，过滤掉列数不等于6的异常行
        valid_rows = []
        with open(file_path, "r") as f:
            header = f.readline()  # 跳过表头
            for line in f:
                line = line.strip()
                if not line:
                    continue
                parts = line.split(",")
                if len(parts) != AXES:
                    continue  # 异常行，丢弃
                try:
                    row = [float(x) for x in parts]
                except ValueError:
                    continue  # 含非数字字段，丢弃
                valid_rows.append(row)

        data = np.array(valid_rows)  # shape: (有效行数, 6)
        total_valid = data.shape[0]
        num_groups = total_valid // SAMPLES_PER_GESTURE
        discarded = total_valid - num_groups * SAMPLES_PER_GESTURE

        print(f"处理动作: {gesture}")
        print(f"  有效行数: {total_valid}, 切出组数: {num_groups}, 尾部丢弃: {discarded} 行")

        for i in range(num_groups):
            tensor = data[i * SAMPLES_PER_GESTURE : (i + 1) * SAMPLES_PER_GESTURE].copy()

            # 归一化：加速度量程 ±4g，陀螺仪量程 ±2000 dps -> 缩放到 [-1, 1]
            tensor[:, 0:3] = tensor[:, 0:3] / 4.0
            tensor[:, 3:6] = tensor[:, 3:6] / 2000.0

            all_inputs.append(tensor)
            all_outputs.append(index)  # 标签: up=0, down=1, left=2, right=3

    X = np.array(all_inputs)   # shape: (总组数, 10, 6)
    y = np.array(all_outputs)  # shape: (总组数,)
    return X, y


X, y = load_data("./train_data")
print("\n输入数据形状 (样本数, 时间步, 轴数):", X.shape)
print("标签形状:", y.shape)

# 保存到当前目录
np.save("X.npy", X)
np.save("y.npy", y)
print("\n已保存:")
print("  X.npy ->", os.path.abspath("X.npy"))
print("  y.npy ->", os.path.abspath("y.npy"))
