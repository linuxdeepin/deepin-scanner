# deepin-scanner

Deepin Scanner is a document scanning tool for Deepin OS, supporting both scanner devices and webcams. It can save scanned documents in multiple formats including PDF, JPEG, PNG, and TIFF.

## Features

- Support for scanner devices via SANE backend
- Support for webcam capture
- Multiple output formats: PDF, JPEG, PNG, TIFF
- Real-time preview
- Adjustable scan settings (resolution, color mode)
- Clean and intuitive Deepin-style UI

## Dependencies

### Runtime Dependencies
- Qt5/Qt6 Core, Gui, Widgets
- DTK Widget, Gui, Core
- SANE (Scanner Access Now Easy)
- libtiff, libjpeg, libpng

### Build Dependencies
- CMake 3.15+
- Qt5/Qt6 development packages
- DTK development packages
- libsane-dev
- libtiff-dev, libjpeg-dev, libpng-dev

## Install

### From Debian Package
```bash
sudo apt install deepin-scanner
```

### Build from Source
```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

deepin-scanner is licensed under [GPL-3.0-or-later](LICENSE).