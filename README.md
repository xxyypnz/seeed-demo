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

4. demo
nRF: 初步演示如何采集加速度信息, 对应的LSM6DS3.cpp可作参考