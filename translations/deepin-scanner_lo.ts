<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="34"/>
        <source>Deepin Scanner</source>
        <translation>ເຄື່ອງສັນນະການດີປິນ</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="38"/>
        <source>Scanner Manager is a scanner tool that supports a variety of scanning devices</source>
        <translation>ມື້ນຳການສັນນະການແມ່ນເຄື່ອງມືສັນນະການທີ່ສະໜັບສະໜູນອຸປະກອນສັນນະການຕ່າງໆ</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="23"/>
        <source>Loading...</source>
        <translation>ກຳລັງໂຫຼດ...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Scanner Manager</source>
        <translation>ມື້ນຳການສັນນະການ</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="96"/>
        <source>Loading devices...</source>
        <translation>ກຳລັງໂຫຼດອຸປະກອນ...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="124"/>
        <source>Opening device...</source>
        <translation>ກຳລັງເປີດອຸປະກອນ...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="68"/>
        <source>Scan Settings</source>
        <translation>ການຕັ້ງຄ່າການສັນນະ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="94"/>
        <source>Resolution</source>
        <translation>ຄວາມລະອຽດ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="105"/>
        <source>Color Mode</source>
        <translation>ຮູບແບບສີ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Image Format</source>
        <translation>ຮູບແບບຮູບ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="134"/>
        <source>Scan</source>
        <translation>ສັນນະ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="141"/>
        <source>View Scanned Image</source>
        <translation>ເບິ່ງຮູບທີ່ສັນນະ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="155"/>
        <source>Scan history will be shown here</source>
        <translation>ປະຫວັດສາດສັນນະຈະຖືກສະແດງຢູ່ທີ່ນີ້</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="202"/>
        <source>Scan Mode</source>
        <translation>ຮູບແບບສັນນະ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="203"/>
        <source>Flatbed</source>
        <translation>ພື້ນຜິວ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="206"/>
        <source>Video Format</source>
        <translation>ຮູບແບບວີດີໂອ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Color</source>
        <translation>ສີ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Grayscale</source>
        <translation>ສີດຳສີຂາວ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Black White</source>
        <translation>ສີດຳສີຂາວ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="251"/>
        <source>Device not initialized</source>
        <translation>ອຸປະກອນບໍ່ໄດ້ຕັ້ງຄ່າ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="305"/>
        <source>Initializing preview...</source>
        <translation>ກຳລັງເປີດເບິ່ງກ່ອນ...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="311"/>
        <source>Device preview not available</source>
        <translation>ບໍ່ມີການສະແດງເບິ່ງກ່ອນຂອງອຸປະກອນ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="336"/>
        <source>No preview image</source>
        <translation>ບໍ່ມີຮູບເບິ່ງກ່ອນ</translation>
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
        <translation>ບໍ່ພົບອຸປະກອນສະແກນ. ວິທີແກ້ໄຂທີ່ອາດເປັນໄປໄດ້:
1. ແນ່ໃຈວ່າສະແກນເຊື່ອມຕໍ່ແລະເປີດຢູ່
2. ລອງຄຳສັ່ງ: sudo gpasswd -a $USER scanner
3. ເລີ່ມຕົ້ນ SANE ໃໝ່: sudo service saned restart
4. ຕິດຕັ້ງໄດເວີທີ່ຈຳເປັນ: sudo apt-get install libsane-extras
5. ສຳລັບສະແກນເນັດເວີກ, ກວດສອບການຕັ້ງຄ່າເນັດເວີກ
6. ຖອດແລະເຊື່ອມສາຍ USB ໃໝ່ ຫຼື ເລີ່ມຄອມພິວເຕີໃໝ່</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="364"/>
        <source>Scanner not opened</source>
        <translation>ບໍ່ໄດ້ເປີດສະແກນເຄື່ອງຈັກ</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="374"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>ບໍ່ສາມາດຮັບຂໍ້ມູນການຕັ້ງຄ່າສະແກນເຄື່ອງຈັກ: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="389"/>
        <source>Failed to start scan: %1</source>
        <translation>ບໍ່ສາມາດເລີ່ມສະແກນ: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="901"/>
        <source>Failed to save test image</source>
        <translation>ບໍ່ສາມາດບັນທຶກຮູບທດສອບ: %1</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="29"/>
        <source>Scanner Devices</source>
        <translation>ອຸປະກອນສະແກນເຄື່ອງຈັກ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>Scanner</source>
        <translation>ສະແກນເຄື່ອງຈັກ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="144"/>
        <source>Idle</source>
        <translation>ຢູ່ໃນສະພາບພັກ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="78"/>
        <source>Webcam</source>
        <translation>ກ້ອງຖ່າຍຮູບອອນລາຍ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="83"/>
        <source>No devices found</source>
        <translation>ບໍ່ມີອຸປະກອນໃດໆເລີຍ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="152"/>
        <source>Model: %1</source>
        <translation>ຮູບແບບ: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="153"/>
        <source>Status: %1</source>
        <translation>ສະຖານະ: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="128"/>
        <source>Scan</source>
        <translation>ສກັນ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="148"/>
        <source>Offline</source>
        <translation>ອອກອິນລານລາຍ</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="264"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>ບໍ່ສາມາດຮັບເສັ້ນທາງອຸປະກອນໄດ້, ບໍ່ສາມາດຕັ້ງຄວາມລະອຽດໄດ້</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="273"/>
        <source>Failed to reopen device</source>
        <translation>ບໍ່ສາມາດເປີດອຸປະກອນຄືນໄດ້</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="307"/>
        <source>Failed to set requested resolution</source>
        <translation>ບໍ່ສາມາດຕັ້ງຄວາມລະອຽດທີ່ຂໍໄວ້ໄດ້</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="320"/>
        <source>Memory mapping failed</source>
        <translation>ບໍ່ສາມາດຈັດສະໜາມຫມືຂໍ້ມູນໄດ້</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="360"/>
        <source>Device not properly initialized</source>
        <translation>ອຸປະກອນບໍ່ໄດ້ຖືກຕັ້ງຕົ້ນຢ່າງຖືກຕ້ອງ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="390"/>
        <source>Buffer initialization failed</source>
        <translation>ການຕັ້ງຕົ້ນຂອງບັຟເຟີບໍ່ສຳເລັດ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="406"/>
        <source>Buffer reinitialization failed</source>
        <translation>ການຕັ້ງຕົ້ນຂອງບັຟເຟີຄືນບໍ່ສຳເລັດ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="437"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>ບໍ່ສາມາດເພີ່ມບັຟເຟີໃສ່ລາຍຊື່: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="448"/>
        <source>Failed to start video stream: %1</source>
        <translation>ບໍ່ສາມາດເລີ່ມສະໜາມວິດີໂອ: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="539"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>ອຸປະກອນບໍ່ໄດ້ຕັ້ງຄ່າ ຫຼື ດັດຊະນີເອກະສານບໍ່ຖືກຕ້ອງ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="559"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>ບໍ່ສາມາດເລີ່ມສະໜາມວິດີໂອ, ການຈັບພາບບໍ່ສຳເລັດ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="600"/>
        <source>Failed to get image frame</source>
        <translation>ບໍ່ສາມາດໃຫ້ບໍ່ລິເວນຮູບພາບ</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="654"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>ບໍ່ສາມາດຈັບຮູບພາບທີ່ຖືກຕ້ອງ, ກະລຸນາກວດເບິ່ງການເຊື່ອມຕໍ່ກັບເຄື່ອງຖ່າຍຮູບ</translation>
    </message>
</context>
</TS>
