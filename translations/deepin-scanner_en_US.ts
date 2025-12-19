<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="34"/>
        <source>Deepin Scanner</source>
        <translation>Deepin Scanner</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="38"/>
        <source>Scanner Manager is a scanner tool that supports a variety of scanning devices</source>
        <translation>Scanner Manager is a scanner tool that supports a variety of scanning devices</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="23"/>
        <source>Loading...</source>
        <translation>Loading...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="41"/>
        <source>Scanner Manager</source>
        <translation>Scanner Manager</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="97"/>
        <source>Loading devices...</source>
        <translation>Loading devices...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="141"/>
        <source>Opening device...</source>
        <translation>Opening device...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="73"/>
        <source>Scan Settings</source>
        <translation>Scan Settings</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="99"/>
        <source>Resolution</source>
        <translation>Resolution</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="110"/>
        <source>Color Mode</source>
        <translation>Color Mode</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="121"/>
        <source>Image Format</source>
        <translation>Image Format</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="132"/>
        <source>Paper Size</source>
        <translation>Paper Size</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="150"/>
        <source>Scan</source>
        <translation>Scan</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="157"/>
        <source>View Scanned Image</source>
        <translation>View Scanned Image</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="171"/>
        <source>Scan history will be shown here</source>
        <translation>Scan history will be shown here</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="219"/>
        <source>Scan Mode</source>
        <translation>Scan Mode</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="220"/>
        <source>Flatbed</source>
        <translation>Flatbed</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="223"/>
        <source>Video Format</source>
        <translation>Video Format</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Color</source>
        <translation>Color</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Grayscale</source>
        <translation>Grayscale</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="227"/>
        <source>Black White</source>
        <translation>Black White</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="233"/>
        <source>Auto</source>
        <translation>Auto</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="297"/>
        <source>Device not initialized</source>
        <translation>Device not initialized</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="352"/>
        <source>Initializing preview...</source>
        <translation>Initializing preview...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="358"/>
        <source>Device preview not available</source>
        <translation>Device preview not available</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="383"/>
        <source>No preview image</source>
        <translation>No preview image</translation>
    </message>
</context>
<context>
    <name>ScannerDevice</name>
    <message>
        <source>No scanner devices found. Possible solutions:
1. Ensure scanner is connected and powered on
2. Run command: sudo gpasswd -a $USER scanner
3. Restart SANE: sudo service saned restart
4. Install required driver package: sudo apt-get install libsane-extras
5. For network scanners, check network configuration
6. Reconnect USB cable or restart computer</source>
        <translation type="vanished">No scanner devices found. Possible solutions:
1. Ensure scanner is connected and powered on
2. Run command: sudo gpasswd -a $USER scanner
3. Restart SANE: sudo service saned restart
4. Install required driver package: sudo apt-get install libsane-extras
5. For network scanners, check network configuration
6. Reconnect USB cable or restart computer</translation>
    </message>
    <message>
        <source>Scanner not opened</source>
        <translation type="vanished">Scanner not opened</translation>
    </message>
    <message>
        <source>Failed to get scanner parameters: %1</source>
        <translation type="vanished">Failed to get scanner parameters: %1</translation>
    </message>
    <message>
        <source>Failed to start scan: %1</source>
        <translation type="vanished">Failed to start scan: %1</translation>
    </message>
    <message>
        <source>Failed to save test image</source>
        <translation type="vanished">Failed to save test image</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="91"/>
        <source>Failed to get device list: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="140"/>
        <source>A scan is already in progress.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="296"/>
        <source>Failed to load scanned image from temp file.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ScannerWorker</name>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="355"/>
        <source>Failed to open SANE device &apos;%1&apos;: %2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="398"/>
        <source>Scanner not opened</source>
        <translation type="unfinished">Scanner not opened</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="412"/>
        <source>Failed to start scan: %1</source>
        <translation type="unfinished">Failed to start scan: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="420"/>
        <source>Failed to open temporary output file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="432"/>
        <source>Scan canceled by user</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="437"/>
        <source>Scan failed during read: %1</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="29"/>
        <source>Scanner Devices</source>
        <translation>Scanner Devices</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>Scanner</source>
        <translation>Scanner</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="144"/>
        <source>Idle</source>
        <translation>Idle</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="78"/>
        <source>Webcam</source>
        <translation>Webcam</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="83"/>
        <source>No devices found</source>
        <translation>No devices found</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="152"/>
        <source>Model: %1</source>
        <translation>Model: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="153"/>
        <source>Status: %1</source>
        <translation>Status: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="128"/>
        <source>Scan</source>
        <translation>Scan</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="148"/>
        <source>Offline</source>
        <translation>Offline</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="264"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>Cannot get device path, cannot set resolution</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="273"/>
        <source>Failed to reopen device</source>
        <translation>Failed to reopen device</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="307"/>
        <source>Failed to set requested resolution</source>
        <translation>Failed to set requested resolution</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="320"/>
        <source>Memory mapping failed</source>
        <translation>Memory mapping failed</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="360"/>
        <source>Device not properly initialized</source>
        <translation>Device not properly initialized</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="390"/>
        <source>Buffer initialization failed</source>
        <translation>Buffer initialization failed</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="406"/>
        <source>Buffer reinitialization failed</source>
        <translation>Buffer reinitialization failed</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="446"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>Failed to enqueue buffer: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="472"/>
        <source>Failed to start video stream: %1</source>
        <translation>Failed to start video stream: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="569"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>Device not initialized or invalid file descriptor</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="589"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>Failed to start video stream, capture failed</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="637"/>
        <source>Failed to get image frame</source>
        <translation>Failed to get image frame</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="694"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>Failed to capture valid image, please check camera connection</translation>
    </message>
</context>
</TS>
