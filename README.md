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