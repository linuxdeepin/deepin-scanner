<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="33"/>
        <source>Deepin Scanner</source>
        <translation>文档扫描器</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="37"/>
        <source>Document Scanner is a scanner tool that supports a variety of scanning devices</source>
        <translation>文档扫描器是一款支持多种扫描设备的工具</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="24"/>
        <source>Loading...</source>
        <translation>加载中...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="36"/>
        <source>Failed to initialize SANE backend.
Please ensure SANE libraries (e.g. sane-backends) are installed and you may need to configure permissions (e.g. add user to &apos;scanner&apos; or &apos;saned&apos; group).
Scanner functionality will be unavailable.</source>
        <translation>初始化SANE后端失败。
请确保已安装SANE库(如sane-backends)，您可能需要配置权限(如将用户添加到&apos;scanner&apos;或&apos;saned&apos;组)。
扫描功能将不可用。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="36"/>
        <source>Scanner error</source>
        <translation>扫描器错误</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Document Scanner</source>
        <translation>文档扫描器</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="100"/>
        <source>Loading devices...</source>
        <translation>正在加载设备...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="128"/>
        <source>Opening device...</source>
        <translation>正在打开设备...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="75"/>
        <source>Scan Settings</source>
        <translation>扫描设置</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="103"/>
        <source>Resolution</source>
        <translation>分辨率</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Color Mode</source>
        <translation>色彩模式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="129"/>
        <source>Image Format</source>
        <translation>图像格式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="156"/>
        <source>View Scanned Image</source>
        <translation>查看已扫描图像</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="149"/>
        <source>Scan</source>
        <translation>扫描</translation>
    </message>
    <message>
        <source>Save</source>
        <translation type="vanished">保存</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="219"/>
        <source>Scan Mode</source>
        <translation>扫描模式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="220"/>
        <source>Flatbed</source>
        <translation>平板扫描</translation>
    </message>
    <message>
        <source>Duplex</source>
        <translation type="vanished">双面扫描</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="223"/>
        <source>Video Format</source>
        <translation>视频格式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Color</source>
        <translation>彩色</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Grayscale</source>
        <translation>灰度</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Black White</source>
        <translation>黑白</translation>
    </message>
    <message>
        <source>ADF</source>
        <translation type="vanished">ADF</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="170"/>
        <source>Scan history will be shown here</source>
        <translation>扫描历史将在此处显示</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="254"/>
        <source>Device not initialized</source>
        <translation>设备未初始化</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="293"/>
        <source>Initializing preview...</source>
        <translation>正在初始化预览...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="300"/>
        <source>Device preview not available</source>
        <translation>设备预览不可用</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="341"/>
        <source>No preview image</source>
        <translation>无预览图像</translation>
    </message>
</context>
<context>
    <name>ScannerDevice</name>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="284"/>
        <source>No scanner devices found. Possible solutions:
1. Ensure scanner is connected and powered on
2. Run command: sudo gpasswd -a $USER scanner
3. Restart SANE: sudo service saned restart
4. Install required driver package: sudo apt-get install libsane-extras
5. For network scanners, check network configuration
6. Reconnect USB cable or restart computer</source>
        <translation>未找到扫描设备。可能的解决方案：
1. 确保扫描仪已连接并通电
2. 运行命令：sudo gpasswd -a $USER scanner
3. 重启SANE：sudo service saned restart
4. 安装所需驱动包：sudo apt-get install libsane-extras
5. 对于网络扫描仪，检查网络配置
6. 重新连接USB线或重启电脑</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="420"/>
        <source>Scanner not opened</source>
        <translation>扫描仪未打开</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="430"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>获取扫描仪参数失败：%1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="445"/>
        <source>Failed to start scan: %1</source>
        <translation>开始扫描失败：%1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="951"/>
        <source>Failed to save test image</source>
        <translation>保存测试图像失败</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="28"/>
        <source>Scanner Devices</source>
        <translation>扫描设备</translation>
    </message>
    <message>
        <source>Refresh</source>
        <translation type="vanished">刷新</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="61"/>
        <source>Scanner</source>
        <translation>扫描仪</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="138"/>
        <source>Idle</source>
        <translation>空闲</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="67"/>
        <source>Webcam</source>
        <translation>摄像头</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>No devices found</source>
        <translation>未找到设备</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="142"/>
        <source>Offline</source>
        <translation>未连接</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="146"/>
        <source>Model: %1</source>
        <translation>型号：%1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="147"/>
        <source>Status: %1</source>
        <translation>状态：%1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="122"/>
        <source>Scan</source>
        <translation>扫描</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="262"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>无法获取设备路径，无法设置分辨率</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="271"/>
        <source>Failed to reopen device</source>
        <translation>重新打开设备失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="321"/>
        <source>Failed to set requested resolution</source>
        <translation>设置请求的分辨率失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="335"/>
        <source>Memory mapping failed</source>
        <translation>内存映射失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="378"/>
        <source>Device not properly initialized</source>
        <translation>设备未正确初始化</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="408"/>
        <source>Buffer initialization failed</source>
        <translation>缓冲区初始化失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="424"/>
        <source>Buffer reinitialization failed</source>
        <translation>缓冲区重新初始化失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="455"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>缓冲队列失败：%1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="466"/>
        <source>Failed to start video stream: %1</source>
        <translation>启动视频流失败：%1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="560"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>设备未初始化或文件描述符无效</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="585"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>启动视频流失败，捕获失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="626"/>
        <source>Failed to get image frame</source>
        <translation>获取图像帧失败</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="680"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>捕获有效图像失败，请检查摄像头连接</translation>
    </message>
</context>
</TS>
