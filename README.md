1. arduino
保证wsl没有占用串口, 必要时需要在powershell(管理员)中先usbipd detach --busid 3-2
进入arduino烧录.ino代码

2. powershell(管理员)
运行usbipd attach --wsl --busid 3-2
具体号数根据usbipd list确定

3. wsl
切换成root, 将node的版本nvm use 20
运行edge-impulse-data-forwarder

4. EI
利用平台直接实现模型构建的全部过程, 在deployment中选择arduino library

5. demo
注意在运行demo的时候和edge-impulse就没关系了, 要从wsl中收回串口控制权  
nRF: 初步演示如何采集加速度信息, 对应的LSM6DS3.cpp可作参考  
TinyML-test: 采用自行训练出的模型, 芯片的运动既可以打印到Serial Monitor, 也可以通过蓝牙发送给手机; 同时手机也可以发送消息被board接收. 所有消息当前限制在UTF8格式  
IMU_Capture: ~~先保证Windows获取串口控制权(detach)然后烧录; 完成后即可让wsl获得串口控制权,此时能够执行cat /dev/ttyACM0 > punch.csv和cat /dev/ttyACM0 > flex.csv~~
直接使用python程序接管Serial Monitor, 可以保存文件; 但是系统tensorflow库版本比较旧, 没有进一步解决版本冲突问题  
micro_speech: ~~考虑在阅读代码的基础上进行二次开发~~ 同样存在版本问题无法通过编译环节
without_EI: `[Claude]` 不依赖 Edge Impulse 平台，从零完成数据采集→预处理→训练→部署的完整 TinyML 流程

### without_EI 完成过程

**硬件**: Seeed XIAO nRF52840 Sense，板载 LSM6DS3 IMU（加速度计 + 陀螺仪）
**任务**: 识别上下左右四种挥动手势，通过 Serial Monitor 输出分类结果

#### 数据采集
使用 `main.py` 通过串口接管 Serial Monitor，`acceleration_capture.ino` 负责在检测到加速度阈值触发后采集 100 行 IMU 数据并发送。每种手势各采集一个 CSV 文件：

| 文件 | 有效行数 | 说明 |
|------|---------|------|
| up.csv | 498 | 2 行异常（列数不足 6）被过滤 |
| down.csv | 500 | 无异常 |
| left.csv | 496 | 4 行异常被过滤 |
| right.csv | 500 | 无异常 |

#### 数据预处理（`data_preprocessing.py`）
原始数据存在两个问题：误采样导致某行列数不足 6；所有录制连续写入没有分组标记。处理策略：
- 逐行读取，过滤列数 ≠ 6 的异常行
- 将原来每组 100 行改为每组 **10 行**切片，大幅扩充样本量
- 各轴归一化：加速度 ÷ 4.0（量程 ±4g），陀螺仪 ÷ 2000.0（量程 ±2000 dps）→ 值域 [-1, 1]

| 项目 | 值 |
|------|-----|
| 输出 X.shape | (198, 10, 6) |
| 输出 y.shape | (198,) |
| 单样本含义 | 10 个时间步 × 6 轴，约 0.1 秒的连续 IMU 读数 |

#### 模型训练（`train.py`）
选用 Conv1D 而非 LSTM：参数量更小、小数据集不易过拟合、短时序窗口优势足够、TFLite 部署更友好。

| 层 | 输出形状 | 参数量 |
|----|---------|--------|
| Input | (10, 6) | 0 |
| Conv1D(16, k=3) | (10, 16) | 304 |
| Conv1D(32, k=3) | (10, 32) | 1,568 |
| GlobalAveragePooling1D | (32,) | 0 |
| Dense(32) + Dropout(0.3) | (32,) | 1,056 |
| Dense(4, softmax) | (4,) | 132 |
| **总计** | | **3,060** |

训练结果：EarlyStopping 在第 52 轮触发，恢复第 37 轮最优权重，验证集准确率 **82.5%**。

#### 部署（`gesture_inference/`）
模型通过 Python 脚本转为 C 数组 `model_data.h` 内嵌进固件，使用 `Arduino_TensorFlowLite` 库在芯片上完成推理。

TFLite 实际算子（Conv1D 在转换时展开）：

| 算子 | 来源 |
|------|------|
| EXPAND_DIMS × 2 | Conv1D → Conv2D 展开时插入 |
| CONV_2D × 2 | Conv1D 主体 |
| RESHAPE × 2 | 维度还原 |
| MEAN | GlobalAveragePooling1D |
| FULLY_CONNECTED × 2 | Dense 层 |
| SOFTMAX | 输出层 |

库的预编译 `.a` 文件缺少 nRF52840 平台的 `DebugLog`/`SerialWrite` 符号，通过在 sketch 目录中添加 `debug_log_impl.cpp` 手动补充实现解决链接报错。

Serial Monitor 输出格式：
```
up    (up:0.821  down:0.043  left:0.091  right:0.045)
```

**待改进**: 当前每类仅约 49 组样本，总计 198 组，数据量明显不足。准确率受限于此，需要采集更大规模的数据集（建议每类 200 组以上）才能进一步提升泛化能力。