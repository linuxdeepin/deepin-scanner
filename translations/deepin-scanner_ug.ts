<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="34"/>
        <source>Deepin Scanner</source>
        <translation>دېپىن سكانر</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="38"/>
        <source>Scanner Manager is a scanner tool that supports a variety of scanning devices</source>
        <translation>سايانېر باشقۇرغۇچى كۆپ خىل سايانېر ئۈسكۈنىلىرىنى قوللايدىغان سايانېر قورالى</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="23"/>
        <source>Loading...</source>
        <translation>يۈكلىنىۋاتىدۇ...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Scanner Manager</source>
        <translation>سايانېر باشقۇرغۇچ</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="96"/>
        <source>Loading devices...</source>
        <translation>ئۈسكۈنىلەر يۈكلىنىۋاتىدۇ...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="124"/>
        <source>Opening device...</source>
        <translation>ئۈسكۈنە ئېچىلىۋاتىدۇ...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="68"/>
        <source>Scan Settings</source>
        <translation>سايانېر تەڭشىكى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="94"/>
        <source>Resolution</source>
        <translation>ئېنىقلىق دەرىجىسى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="105"/>
        <source>Color Mode</source>
        <translation>رەڭ ھالىتى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Image Format</source>
        <translation>رەسىم فورماتى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="134"/>
        <source>Scan</source>
        <translation>سايانېر</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="141"/>
        <source>View Scanned Image</source>
        <translation>سايانېرلانغان رەسىمنى كۆرسىتىش</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="155"/>
        <source>Scan history will be shown here</source>
        <translation>سايانېر تارىخى بۇ يەردە كۆرسىتىلىدۇ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="202"/>
        <source>Scan Mode</source>
        <translation>سايانېر ھالىتى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="203"/>
        <source>Flatbed</source>
        <translation>تەكشى تاختا</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="206"/>
        <source>Video Format</source>
        <translation>سىن فورماتى</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Color</source>
        <translation>رەڭلىك</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Grayscale</source>
        <translation>كۈلرەڭ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Black White</source>
        <translation>ئاق قارا</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="251"/>
        <source>Device not initialized</source>
        <translation>ئۈسكۈنە دەسلەپلەشتۈرۈلمىگەن</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="305"/>
        <source>Initializing preview...</source>
        <translation>ئالدىن كۆرۈش دەسلەپلەشتۈرۈلىۋاتىدۇ...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="311"/>
        <source>Device preview not available</source>
        <translation>ئۈسكۈنە ئالدىن كۆرۈش ئىشلەتكىلى بولمايدۇ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="336"/>
        <source>No preview image</source>
        <translation>ئالدىن كۆرۈش رەسىمى يوق</translation>
    </message>
</context>
<context>
    <name>ScannerDevice</name>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="227"/>
        <source>No scanner devices found. Possible solutions:
1. Ensure scanner is connected and powered on
2. Run command: sudo gpasswd -a $USER scanner
3. Restart SANE: sudo service saned restart
4. Install required driver package: sudo apt-get install libsane-extras
5. For network scanners, check network configuration
6. Reconnect USB cable or restart computer</source>
        <translation>سايانېر ئۈسكۈنىسى تېپىلمىدى. مۇمكىن بولغان ھەل قىلىش چارىلىرى:
1. سايانېرنىڭ ئۇلانغان ۋە توكقا چېتىلغانلىقىنى جەزملەشتۈرۈڭ
2. بۇيرۇقنى ئىجرا قىلىڭ: sudo gpasswd -a $USER scanner
3. SANE نى قايتا قوزغىتىڭ: sudo service saned restart
4. زۆرۈر قوزغاتقۇچ بولىقىنى قاچىلاڭ: sudo apt-get install libsane-extras
5. تور سايانېرلىرى ئۈچۈن، تور تەڭشىكىنى تەكشۈرۈڭ
6. USB سىمىنى قايتا ئۇلاڭ ياكى كومپيۇتېرنى قايتا قوزغىتىڭ</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="364"/>
        <source>Scanner not opened</source>
        <translation>سايانېر ئېچىلمىغان</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="374"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>سايانېر پارامېتىرلىرىغا ئېرىشىش مەغلۇپ بولدى: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="389"/>
        <source>Failed to start scan: %1</source>
        <translation>سايانېرنى باشلاش مەغلۇپ بولدى: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="901"/>
        <source>Failed to save test image</source>
        <translation>سىناق رەسىمىنى ساقلاش مەغلۇپ بولدى</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="150"/>
        <source>Scanner has been disconnected</source>
        <translation>سايانېر ئۈزۈلدى</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="29"/>
        <source>Scanner Devices</source>
        <translation>سايانېر ئۈسكۈنىلىرى</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>Scanner</source>
        <translation>سايانېر</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="144"/>
        <source>Idle</source>
        <translation>ئەھەملىك</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="78"/>
        <source>Webcam</source>
        <translation>ۋېبکام</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="83"/>
        <source>No devices found</source>
        <translation>ھەنچە ھەندىسە چۈشىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="152"/>
        <source>Model: %1</source>
        <translation>مودېل: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="153"/>
        <source>Status: %1</source>
        <translation>ھال: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="128"/>
        <source>Scan</source>
        <translation>سەن</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="148"/>
        <source>Offline</source>
        <translation>ئاڧلاين</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="264"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>ھەندىسە يېتىشىنىپ قالدى، ئېھتىياتلىق ھەجەملىك ئەپەتىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="273"/>
        <source>Failed to reopen device</source>
        <translation>ھەندىسەنى ئەپەتىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="307"/>
        <source>Failed to set requested resolution</source>
        <translation>تەلەپ قىلغان ھەجەملىك ئەپەتىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="320"/>
        <source>Memory mapping failed</source>
        <translation>ئىنىمىۋەتىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="360"/>
        <source>Device not properly initialized</source>
        <translation>ھەندىسە ياخشىلاشىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="390"/>
        <source>Buffer initialization failed</source>
        <translation>بەفر ياخشىلاشىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="406"/>
        <source>Buffer reinitialization failed</source>
        <translation>بەفرنى ئەپەتىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="437"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>بەفرنى سېرۋەتىنىپ قالدى: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="448"/>
        <source>Failed to start video stream: %1</source>
        <translation>ۋېدىئو سىرۋەتىنىپ قالدى: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="539"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>ھەندىسە ياخشىلاشىنىپ قالدى یاكى ھەققىقىي فايل دېسكىرېتورنىڭ ھەققىقىيلىقىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="559"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>ۋېدىئو سىرۋەتىنىپ قالدى، ئىمەجە چەكىشىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="600"/>
        <source>Failed to get image frame</source>
        <translation>ئىمەجە ئىمەجىنى ئېتىشىنىپ قالدى</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="654"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>ياخشى ئىمەجىنى ئېتىشىنىپ قالدى، كامېرا ئۇلاشىنى تەكشۈرۈڭ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="543"/>
        <source>Device has been disconnected</source>
        <translation>ئۈسكۈنە ئۈزۈلدى</translation>
    </message>
</context>
</TS>
