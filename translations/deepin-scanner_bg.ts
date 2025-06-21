<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1">
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
        <translation>Scanner Manager е приложение за скенери, които поддържа различни видове скенери</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="23"/>
        <source>Loading...</source>
        <translation>Зареждане...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Scanner Manager</source>
        <translation>Scanner Manager</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="96"/>
        <source>Loading devices...</source>
        <translation>Зареждане на устройствата...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="124"/>
        <source>Opening device...</source>
        <translation>Отваряне на устройството...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="68"/>
        <source>Scan Settings</source>
        <translation>Настройки за скенери</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="94"/>
        <source>Resolution</source>
        <translation>Резолюция</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="105"/>
        <source>Color Mode</source>
        <translation>Цветен режим</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Image Format</source>
        <translation>Формат на изображението</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="134"/>
        <source>Scan</source>
        <translation>Сканиране</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="141"/>
        <source>View Scanned Image</source>
        <translation>Преглед на скенереното изображение</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="155"/>
        <source>Scan history will be shown here</source>
        <translation>Историята на скенераните файлове ще бъде показана тук</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="202"/>
        <source>Scan Mode</source>
        <translation>Режим на сканиране</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="203"/>
        <source>Flatbed</source>
        <translation>Плоска повърхност</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="206"/>
        <source>Video Format</source>
        <translation>Видео формат</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Color</source>
        <translation>Цветен</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Grayscale</source>
        <translation>Сиво-бял</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Black White</source>
        <translation>Черно-бял</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="251"/>
        <source>Device not initialized</source>
        <translation>Устройството не е инициализирано</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="305"/>
        <source>Initializing preview...</source>
        <translation>Инициализиране на преглед...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="311"/>
        <source>Device preview not available</source>
        <translation>Преглед на устройството не е достъпен</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="336"/>
        <source>No preview image</source>
        <translation>Няма преглед на изображението</translation>
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
        <translation>Не са намерени скенери. Възможни решения:
1. Проверете дали скенера е свързан и включен
2. Изпълнете командата: sudo gpasswd -a $USER scanner
3. Пуснете SANE отново: sudo service saned restart
4. Инсталирайте необходимия драйвер: sudo apt-get install libsane-extras
5. За мрежеви скенери проверете мрежовата конфигурация
6. Повторно свържете USB кабела или рестартирайте компютъра</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="364"/>
        <source>Scanner not opened</source>
        <translation>Скенерът не е отворен</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="374"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>Неуспешно получаване на параметри на скенера: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="389"/>
        <source>Failed to start scan: %1</source>
        <translation>Неуспешно стартиране на сканиране: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="901"/>
        <source>Failed to save test image</source>
        <translation>Неуспешно запазване на изображението за тест</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="29"/>
        <source>Scanner Devices</source>
        <translation>Скенери</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>Scanner</source>
        <translation>Скенер</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="144"/>
        <source>Idle</source>
        <translation>Неактивно</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="78"/>
        <source>Webcam</source>
        <translation>Камера</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="83"/>
        <source>No devices found</source>
        <translation>Няма намерени устройства</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="152"/>
        <source>Model: %1</source>
        <translation>Модел: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="153"/>
        <source>Status: %1</source>
        <translation>Статус: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="128"/>
        <source>Scan</source>
        <translation>Скан</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="148"/>
        <source>Offline</source>
        <translation>Офлайн</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="264"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>Не може да се получи пътят към устройството, не може да се зададе разрешение</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="273"/>
        <source>Failed to reopen device</source>
        <translation>Неуспешно възстановяване на устройството</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="307"/>
        <source>Failed to set requested resolution</source>
        <translation>Неуспешно задаване на изискваното разрешение</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="320"/>
        <source>Memory mapping failed</source>
        <translation>Неуспешно създаване на отображение в паметта</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="360"/>
        <source>Device not properly initialized</source>
        <translation>Устройството не е инициализирано правилно</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="390"/>
        <source>Buffer initialization failed</source>
        <translation>Неуспешно инициализиране на буфер</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="406"/>
        <source>Buffer reinitialization failed</source>
        <translation>Неуспешно повторно инициализиране на буфер</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="437"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>Неуспешно включване на буфер в опашката: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="448"/>
        <source>Failed to start video stream: %1</source>
        <translation>Неуспешно стартиране на видеоток: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="539"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>Устройството не е инициализирано или неправилен файлов дескриптор</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="559"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>Неуспешно стартиране на видеоток, снимка неуспешна</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="600"/>
        <source>Failed to get image frame</source>
        <translation>Неуспешно получаване на кадър</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="654"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>Неуспешно получаване на валидна снимка, моля проверете връзката с камерата</translation>
    </message>
</context>
</TS>
