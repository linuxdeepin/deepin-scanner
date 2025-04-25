# deepin-scanner

deepin-scanner 是 Deepin 操作系统上的文档扫描工具，支持扫描仪设备和摄像头，可以将扫描文档保存为PDF、JPEG、PNG和TIFF等多种格式。

## 功能特性

- 通过SANE后端支持扫描仪设备
- 支持摄像头拍摄
- 多种输出格式：PDF、JPEG、PNG、TIFF
- 实时预览功能
- 可调整扫描设置（分辨率、色彩模式）
- 简洁直观的Deepin风格界面

## 依赖项

### 运行时依赖
- Qt5/Qt6 Core, Gui, Widgets
- DTK Widget, Gui, Core
- SANE (Scanner Access Now Easy)
- libtiff, libjpeg, libpng

### 构建依赖
- CMake 3.15+
- Qt5/Qt6开发包
- DTK开发包
- libsane-dev
- libtiff-dev, libjpeg-dev, libpng-dev

## 安装方法

### 通过Debian包安装
```bash
sudo apt install deepin-scanner
```

### 从源码构建
```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

## 参与贡献

我们欢迎您报告问题并贡献代码

* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)
* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)

## 许可证

deepin-scanner 使用 [GPL-3.0-or-later](LICENSE) 许可证。