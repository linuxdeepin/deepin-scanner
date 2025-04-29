<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_HK">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="33"/>
        <source>Deepin Scanner</source>
        <translation>文檔掃描器</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="37"/>
        <source>Document Scanner is a scanner tool that supports a variety of scanning devices</source>
        <translation>文檔掃描器是一款支援多種掃描設備的工具</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="24"/>
        <source>Loading...</source>
        <translation>加載中...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="36"/>
        <source>Failed to initialize SANE backend.
Please ensure SANE libraries (e.g. sane-backends) are installed and you may need to configure permissions (e.g. add user to &apos;scanner&apos; or &apos;saned&apos; group).
Scanner functionality will be unavailable.</source>
        <translation>初始化SANE後端失敗。
請確保已安裝SANE庫(如sane-backends)，您可能需要配置權限(如將用戶添加到&apos;scanner&apos;或&apos;saned&apos;組)。
掃描功能將不可用。</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="36"/>
        <source>Scanner error</source>
        <translation>掃描器錯誤</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Document Scanner</source>
        <translation>文檔掃描器</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="100"/>
        <source>Loading devices...</source>
        <translation>加載設備...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="128"/>
        <source>Opening device...</source>
        <translation>正在打開設備...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="75"/>
        <source>Scan Settings</source>
        <translation>掃描設定</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="103"/>
        <source>Resolution</source>
        <translation>解析度</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Color Mode</source>
        <translation>色彩模式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="129"/>
        <source>Image Format</source>
        <translation>圖像格式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="156"/>
        <source>View Scanned Image</source>
        <translation>查看掃描圖像</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="149"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
    <message>
        <source>Save</source>
        <translation type="vanished">儲存</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="219"/>
        <source>Scan Mode</source>
        <translation>掃描模式</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="220"/>
        <source>Flatbed</source>
        <translation>平板掃描</translation>
    </message>
    <message>
        <source>Duplex</source>
        <translation type="vanished">雙面掃描</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="223"/>
        <source>Video Format</source>
        <translation>視頻格式</translation>
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
        <translation>掃描歷史將顯示在這裡</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="254"/>
        <source>Device not initialized</source>
        <translation>設備未初始化</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="293"/>
        <source>Initializing preview...</source>
        <translation>正在初始化預覽...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="300"/>
        <source>Device preview not available</source>
        <translation>設備預覽不可用</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="341"/>
        <source>No preview image</source>
        <translation>無預覽圖像</translation>
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
        <translation>未找到掃描設備。可能的解決方案：
1. 確保掃描儀已連接並通電
2. 運行命令：sudo gpasswd -a $USER scanner
3. 重啟SANE：sudo service saned restart
4. 安裝所需驅動包：sudo apt-get install libsane-extras
5. 對於網絡掃描儀，檢查網絡配置
6. 重新連接USB線或重啟電腦</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="420"/>
        <source>Scanner not opened</source>
        <translation>掃描儀未打開</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="430"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>獲取掃描儀參數失敗：%1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="445"/>
        <source>Failed to start scan: %1</source>
        <translation>開始掃描失敗：%1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="951"/>
        <source>Failed to save test image</source>
        <translation>保存測試圖像失敗</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="28"/>
        <source>Scanner Devices</source>
        <translation>掃描設備</translation>
    </message>
    <message>
        <source>Refresh</source>
        <translation type="vanished">刷新</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="61"/>
        <source>Scanner</source>
        <translation>掃描儀</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="138"/>
        <source>Idle</source>
        <translation>空閒</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="67"/>
        <source>Webcam</source>
        <translation>攝像頭</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>No devices found</source>
        <translation>未找到設備</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="142"/>
        <source>Offline</source>
        <translation>離線</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="146"/>
        <source>Model: %1</source>
        <translation>型號：%1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="147"/>
        <source>Status: %1</source>
        <translation>狀態：%1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="122"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="262"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>無法獲取設備路徑，無法設置解析度</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="271"/>
        <source>Failed to reopen device</source>
        <translation>重新打開設備失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="321"/>
        <source>Failed to set requested resolution</source>
        <translation>設置請求的解析度失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="335"/>
        <source>Memory mapping failed</source>
        <translation>記憶體映射失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="378"/>
        <source>Device not properly initialized</source>
        <translation>設備未正確初始化</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="408"/>
        <source>Buffer initialization failed</source>
        <translation>緩衝區初始化失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="424"/>
        <source>Buffer reinitialization failed</source>
        <translation>緩衝區重新初始化失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="455"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>緩衝隊列失敗：%1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="466"/>
        <source>Failed to start video stream: %1</source>
        <translation>啟動視頻流失敗：%1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="560"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>設備未初始化或文件描述符無效</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="585"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>啟動視頻流失敗，捕獲失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="626"/>
        <source>Failed to get image frame</source>
        <translation>獲取圖像幀失敗</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="680"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>捕獲有效圖像失敗，請檢查攝像頭連接</translation>
    </message>
</context>
</TS>
