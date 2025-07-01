<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="34"/>
        <source>Deepin Scanner</source>
        <translation>Δηπιν Σκάνερ</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="38"/>
        <source>Scanner Manager is a scanner tool that supports a variety of scanning devices</source>
        <translation>Ο Διαχειριστής Σκανερ είναι ένας εργαλείο σκανερ που υποστηρίζει πολλά είδη σκανερ</translation>
    </message>
</context>
<context>
    <name>LoadingDialog</name>
    <message>
        <location filename="../src/ui/loadingdialog.cpp" line="23"/>
        <source>Loading...</source>
        <translation>Φόρτωση...</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/mainwindow.cpp" line="40"/>
        <source>Scanner Manager</source>
        <translation>Διαχειριστής Σκανερ</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="96"/>
        <source>Loading devices...</source>
        <translation>Φόρτωση σκανερ...</translation>
    </message>
    <message>
        <location filename="../src/mainwindow.cpp" line="124"/>
        <source>Opening device...</source>
        <translation>Άνοιγμα σκανερ...</translation>
    </message>
</context>
<context>
    <name>ScanWidget</name>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="68"/>
        <source>Scan Settings</source>
        <translation>Ρυθμίσεις Σκάνερ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="94"/>
        <source>Resolution</source>
        <translation>Καθαρισμός</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="105"/>
        <source>Color Mode</source>
        <translation>Προσαρμογή Χρωμάτων</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="116"/>
        <source>Image Format</source>
        <translation>Διαμόρφωση Εικόνας</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="134"/>
        <source>Scan</source>
        <translation>Σκάνερ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="141"/>
        <source>View Scanned Image</source>
        <translation>Προβολή Εικόνας Σκάνερ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="155"/>
        <source>Scan history will be shown here</source>
        <translation>Η ιστορία σκάνερ θα εμφανιστεί εδώ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="202"/>
        <source>Scan Mode</source>
        <translation>Λειτουργία Σκάνερ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="203"/>
        <source>Flatbed</source>
        <translation>Πλατφόρμα</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="206"/>
        <source>Video Format</source>
        <translation>Διαμόρφωση Βίντεο</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Color</source>
        <translation>Χρώμα</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Grayscale</source>
        <translation>Γκρισκάλ</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="210"/>
        <source>Black White</source>
        <translation>Μαύρο Λευκό</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="251"/>
        <source>Device not initialized</source>
        <translation>Ο σκανερ δεν έχει επεξεργαστεί</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="305"/>
        <source>Initializing preview...</source>
        <translation>Επεξεργασία προεπισκόπησης...</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="311"/>
        <source>Device preview not available</source>
        <translation>Η προεπισκόπηση του σκανερ δεν είναι διαθέσιμη</translation>
    </message>
    <message>
        <location filename="../src/ui/scanwidget.cpp" line="336"/>
        <source>No preview image</source>
        <translation>Δεν υπάρχει εικόνα προεπισκόπησης</translation>
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
        <translation>Δεν βρέθηκαν σκανερ. Πιθανές λύσεις:
1. Αποδεσμεύστε το σκανερ και ενεργοποιήστε την ηλεκτρονική του
2. Εκτελέστε την εντολή: sudo gpasswd -a $USER scanner
3. Ξαναρχίστε το SANE: sudo service saned restart
4. Εγκαταστήστε το απαραίτητο πακέτο ελέγχου: sudo apt-get install libsane-extras
5. Για σκανερ δικτύου, ελέγξτε την δικτυακή διαμόρφωση
6. Ξανασυνδέστε το καλώδιο USB ή επανεκκινήστε τον υπολογιστή</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="364"/>
        <source>Scanner not opened</source>
        <translation>Το σκανερ δεν έχει ανοιχτεί</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="374"/>
        <source>Failed to get scanner parameters: %1</source>
        <translation>Αποτυχία παραλαβής παραμέτρων σκανερ: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="389"/>
        <source>Failed to start scan: %1</source>
        <translation>Αποτυχία εκκίνησης σκάνερ: %1</translation>
    </message>
    <message>
        <location filename="../src/device/scannerdevice.cpp" line="901"/>
        <source>Failed to save test image</source>
        <translation>Αποτυχία αποθήκευσης εικόνας δοκιμής</translation>
    </message>
</context>
<context>
    <name>ScannersWidget</name>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="29"/>
        <source>Scanner Devices</source>
        <translation>Σκανερ Συσκευές</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="72"/>
        <source>Scanner</source>
        <translation>Σκανερ</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="144"/>
        <source>Idle</source>
        <translation>Αδρανής</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="78"/>
        <source>Webcam</source>
        <translation>Κάμερα</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="83"/>
        <source>No devices found</source>
        <translation>Δεν βρέθηκαν συσκευές</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="152"/>
        <source>Model: %1</source>
        <translation>Μοντέλο: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="153"/>
        <source>Status: %1</source>
        <translation>Κατάσταση: %1</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="128"/>
        <source>Scan</source>
        <translation>Σάρωση</translation>
    </message>
    <message>
        <location filename="../src/ui/scannerswidget.cpp" line="148"/>
        <source>Offline</source>
        <translation>Αποσυνδεδεμένο</translation>
    </message>
</context>
<context>
    <name>WebcamDevice</name>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="264"/>
        <source>Cannot get device path, cannot set resolution</source>
        <translation>Δεν μπορείτε να πάρετε το μονοπάτι της συσκευής, δεν μπορείτε να ρυθμίσετε την ανάλυση</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="273"/>
        <source>Failed to reopen device</source>
        <translation>Αποτυχία επαναπρόσδεσης της συσκευής</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="307"/>
        <source>Failed to set requested resolution</source>
        <translation>Αποτυχία ρύθμισης της ζητούμενης ανάλυσης</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="320"/>
        <source>Memory mapping failed</source>
        <translation>Αποτυχία αντιστοίχισης μνήμης</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="360"/>
        <source>Device not properly initialized</source>
        <translation>Η συσκευή δεν έχει επαναπροσδεθεί σωστά</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="390"/>
        <source>Buffer initialization failed</source>
        <translation>Αποτυχία αρχικοποίησης του buffer</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="406"/>
        <source>Buffer reinitialization failed</source>
        <translation>Αποτυχία επαναρχικοποίησης του buffer</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="437"/>
        <source>Failed to enqueue buffer: %1</source>
        <translation>Αποτυχία προσθήκης buffer στην ουρά: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="448"/>
        <source>Failed to start video stream: %1</source>
        <translation>Αποτυχία εκκίνησης του ροής βίντεο: %1</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="539"/>
        <source>Device not initialized or invalid file descriptor</source>
        <translation>Η συσκευή δεν έχει επαναπροσδεθεί ή το περιγραφικό αρχείο είναι άκυρο</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="559"/>
        <source>Failed to start video stream, capture failed</source>
        <translation>Αποτυχία εκκίνησης του ροής βίντεο, η ανάληψη απέτυχε</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="600"/>
        <source>Failed to get image frame</source>
        <translation>Αποτυχία ανάληψης εικόνας</translation>
    </message>
    <message>
        <location filename="../src/device/webcamdevice.cpp" line="654"/>
        <source>Failed to capture valid image, please check camera connection</source>
        <translation>Αποτυχία ανάληψης έγκυρης εικόνας, ελέγξτε τη σύνδεση με την κάμερα</translation>
    </message>
</context>
</TS>
