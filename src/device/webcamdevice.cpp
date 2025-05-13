// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webcamdevice.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QPainter>
#include <QPaintEvent>
#include <QThread>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <algorithm>
#include <cstdio>
#include <exception>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <QMutex>
#include <QMutexLocker>

// WebcamDevice 实现
WebcamDevice::WebcamDevice(QObject *parent)
    : DeviceBase(parent), m_fd(-1), m_currentBuffer(0), m_isInitialized(false), m_deviceSelected(false), m_width(640), m_height(480), m_pixelFormat(V4L2_PIX_FMT_YUYV)
{
    memset(m_buffers, 0, sizeof(m_buffers));
    memset(m_bufferSizes, 0, sizeof(m_bufferSizes));

    m_previewTimer.setInterval(33);   // ~30 fps
    connect(&m_previewTimer, &QTimer::timeout, this, &WebcamDevice::updatePreview);
}

WebcamDevice::~WebcamDevice()
{
    closeDevice();
}

QStringList WebcamDevice::getAvailableDevices()
{
    QStringList devices;
    QDir dir("/dev");
    QStringList filters;
    filters << "video*";
    QFileInfoList entries = dir.entryInfoList(filters, QDir::System);

    for (const QFileInfo &info : entries) {
        int fd = open(info.filePath().toUtf8().constData(), O_RDWR);
        if (fd != -1) {
            v4l2_capability cap;
            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) != -1) {
                // 确保它支持视频捕获并且是物理设备
                if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_DEVICE_CAPS & V4L2_CAP_META_CAPTURE)) {
                    // 检查是否有实际的视频输入设备
                    v4l2_input input;
                    bool hasVideoInput = false;
                    memset(&input, 0, sizeof(input));
                    input.index = 0;
                    while (ioctl(fd, VIDIOC_ENUMINPUT, &input) != -1) {
                        if (input.type == V4L2_INPUT_TYPE_CAMERA) {
                            hasVideoInput = true;
                            break;
                        }
                        input.index++;
                    }

                    if (hasVideoInput) {
                        QString name = QString::fromUtf8((const char *)cap.card);
                        devices << QString("%1 (%2)").arg(name, info.filePath());
                    }
                }
            }
            close(fd);
        }
    }
    return devices;
}

bool WebcamDevice::initialize()
{
    // Webcam设备不需要特殊的初始化，直接返回成功
    setState(Initialized);
    return true;
}

bool WebcamDevice::openDevice(const QString &devicePath)
{
    // 确保先关闭之前的设备
    closeDevice();
    qDebug() << "打开设备:" << devicePath;

    QString actualPath = devicePath;
    int idx = devicePath.indexOf('(');
    if (idx > 0) {
        actualPath = devicePath.mid(idx + 1).chopped(1);
    }

    // 打开设备
    m_fd = open(actualPath.toUtf8().constData(), O_RDWR);
    if (m_fd <= 0) {
        qDebug() << "打开设备失败:" << strerror(errno);
        m_fd = -1;
        return false;
    }

    // 验证设备是否为视频捕获设备
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        qDebug() << "查询设备能力失败:" << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qDebug() << "设备不支持视频捕获功能";
        close(m_fd);
        m_fd = -1;
        return false;
    }

    qDebug() << "设备信息: 驱动=" << reinterpret_cast<const char *>(cap.driver)
             << "名称=" << reinterpret_cast<const char *>(cap.card);

    selectBestPixelFormat();

    // 设置合适的分辨率
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // 先获取当前格式
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == -1) {
        qDebug() << "获取当前格式失败:" << strerror(errno);
    }

    // 更新设备参数
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;
    m_pixelFormat = fmt.fmt.pix.pixelformat;

    qDebug() << "当前分辨率:" << m_width << "x" << m_height;

    auto maxRes = getMaxResolution();
    // try to set default resolution as max resolution
    {
        fmt.fmt.pix.width = maxRes.width();
        fmt.fmt.pix.height = maxRes.height();
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
            m_width = fmt.fmt.pix.width;
            m_height = fmt.fmt.pix.height;
            m_pixelFormat = fmt.fmt.pix.pixelformat;
            qDebug() << "设置分辨率成功:" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
        } else {
            qDebug() << "设置分辨率失败，使用当前分辨率";
        }
    }

    // 初始化内存映射
    if (!initMmap()) {
        qDebug() << "初始化内存映射失败";
        close(m_fd);
        m_fd = -1;
        return false;
    }

    m_isInitialized = true;
    m_deviceSelected = true;
    m_currentDeviceName = devicePath;
    setState(Connected);

    // 枚举支持的分辨率
    enumerateSupportedResolutions();

    return true;
}

bool WebcamDevice::setResolution(int width, int height)
{
    qDebug() << "尝试设置分辨率:" << width << "x" << height;

    // 保存原始状态
    bool wasInitialized = m_isInitialized;
    bool wasDeviceSelected = m_deviceSelected;
    bool wasPreviewActive = m_previewTimer.isActive();

    // 保存当前设备路径
    QString devicePath;
    if (m_fd > 0) {
        char devPath[256];
        sprintf(devPath, "/proc/self/fd/%d", m_fd);
        char realPath[256] = { 0 };
        if (readlink(devPath, realPath, sizeof(realPath) - 1) != -1) {
            devicePath = QString(realPath);
            qDebug() << "当前设备路径:" << devicePath;
        }
    }

    // 如果设备路径为空但已初始化，可能是路径获取失败
    if (devicePath.isEmpty() && m_fd > 0) {
        qDebug() << "无法获取当前设备路径，使用备用方法";
        // 这里可以添加备用方法获取设备路径
    }

    // 停止预览并完全关闭设备
    if (wasPreviewActive) {
        stopPreview();
    }

    // 彻底关闭设备，释放所有资源
    closeDevice();

    // 给系统一些时间完全释放设备
    QThread::msleep(500);

    // 如果没有设备路径，无法继续
    if (devicePath.isEmpty()) {
        emit errorOccurred(tr("Cannot get device path, cannot set resolution"));
        return false;
    }

    // 重新打开设备
    qDebug() << "重新打开设备:" << devicePath;
    m_fd = open(devicePath.toUtf8().constData(), O_RDWR);
    if (m_fd <= 0) {
        qDebug() << "重新打开设备失败:" << strerror(errno);
        emit errorOccurred(tr("Failed to reopen device"));
        return false;
    }

    // 验证设备是否为视频捕获设备
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        qDebug() << "验证设备失败:" << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // 设置格式
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = m_pixelFormat;   // 使用之前的像素格式
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    bool formatSet = false;
    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
        formatSet = true;
        qDebug() << "成功设置分辨率:" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
    } else {
        qDebug() << "设置分辨率失败，尝试其他像素格式";
        formatSet = selectBestPixelFormat();
    }

    if (!formatSet) {
        qDebug() << "所有尝试都失败，无法设置分辨率";
        close(m_fd);
        m_fd = -1;
        emit errorOccurred(tr("Failed to set requested resolution"));
        return false;
    }

    // 更新设备参数
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;

    // 初始化内存映射
    if (!initMmap()) {
        qDebug() << "初始化内存映射失败";
        close(m_fd);
        m_fd = -1;
        emit errorOccurred(tr("Memory mapping failed"));
        return false;
    }

    // 恢复设备状态
    m_isInitialized = true;
    m_deviceSelected = wasDeviceSelected;

    // 重新枚举分辨率并通知UI
    enumerateSupportedResolutions();

    // 如果之前在预览，恢复预览
    if (wasPreviewActive) {
        startPreview();
    }

    qDebug() << "成功完成分辨率设置:" << m_width << "x" << m_height;
    return true;
}

void WebcamDevice::startCapture()
{
    captureImage();
}

void WebcamDevice::stopCapture()
{
    stopCapturing();
    setState(Connected);
}

bool WebcamDevice::isCapturing() const
{
    return state() == Capturing;
}

bool WebcamDevice::startCapturing()
{
    if (m_fd <= 0 || !m_isInitialized) {
        qDebug() << "无法启动捕获：设备未初始化或文件描述符无效";
        emit errorOccurred(tr("Device not properly initialized"));
        return false;
    }

    // 确保在入队缓冲区前流已停止
    v4l2_buf_type stopType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(m_fd, VIDIOC_STREAMOFF, &stopType);

    // 等待设备稳定
    QThread::msleep(100);

    // 打印缓冲区状态进行调试
    qDebug() << "缓冲区状态:";
    for (int i = 0; i < 4; i++) {
        qDebug() << "  缓冲区" << i << ": 地址=" << m_buffers[i] << " 大小=" << m_bufferSizes[i];
    }

    // 记录使用的缓冲区数量，避免访问未分配的缓冲区
    int validBufferCount = 0;
    for (int i = 0; i < 4; i++) {
        if (m_buffers[i] && m_bufferSizes[i] > 0) {
            validBufferCount++;
        } else {
            break;   // 假设缓冲区是连续分配的
        }
    }

    if (validBufferCount == 0) {
        qDebug() << "没有有效的缓冲区可用，尝试重新初始化内存映射";
        if (!initMmap()) {
            emit errorOccurred(tr("Buffer initialization failed"));
            return false;
        }

        // 重新检查缓冲区
        validBufferCount = 0;
        for (int i = 0; i < 4; i++) {
            if (m_buffers[i] && m_bufferSizes[i] > 0) {
                validBufferCount++;
            } else {
                break;
            }
        }

        if (validBufferCount == 0) {
            qDebug() << "重新初始化后仍无有效缓冲区";
            emit errorOccurred(tr("Buffer reinitialization failed"));
            return false;
        }
    }

    qDebug() << "使用" << validBufferCount << "个缓冲区开始捕获";

    // 入队所有缓冲区
    for (int i = 0; i < validBufferCount; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        int result = ioctl(m_fd, VIDIOC_QBUF, &buf);
        if (result == -1) {
            qDebug() << "入队缓冲区" << i << "失败: " << strerror(errno) << " (错误码:" << errno << ")";

            // 清理已入队的缓冲区
            stopCapturing();

            // 提供更多诊断信息
            if (errno == EINVAL) {
                qDebug() << "  错误原因: 无效参数 - 可能缓冲区索引超出范围或内存类型不匹配";
            } else if (errno == ENOMEM) {
                qDebug() << "  错误原因: 内存不足";
            } else if (errno == EIO) {
                qDebug() << "  错误原因: I/O错误 - 设备可能已断开连接";
            }

            emit errorOccurred(tr("Failed to enqueue buffer: %1").arg(strerror(errno)));
            return false;
        }

        qDebug() << "成功入队缓冲区" << i;
    }

    // 启动视频流
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
        qDebug() << "启动视频流失败:" << strerror(errno);
        emit errorOccurred(tr("Failed to start video stream: %1").arg(strerror(errno)));
        return false;
    }

    qDebug() << "成功启动视频捕获流";
    return true;
}

void WebcamDevice::stopCapturing()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(m_fd, VIDIOC_STREAMOFF, &type);
}

void WebcamDevice::startPreview()
{
    if (!m_isInitialized) {
        return;
    }

    if (!startCapturing()) {
        return;
    }

    adjustCommonCameraSettings();
    setCameraAutoFocus(true);

    // 清空最新帧
    {
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = QImage();
    }

    qDebug() << "预览开始，已应用相机参数";
    m_previewTimer.start();
    setState(Capturing);
}

void WebcamDevice::stopPreview()
{
    m_previewTimer.stop();
    if (m_isInitialized) {
        stopCapturing();
    }
    setState(Connected);
}

void WebcamDevice::updatePreview()
{
    if (!m_isInitialized || m_fd <= 0) {
        return;
    }

    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1) {
        qDebug() << "预览更新出错:" << strerror(errno);
        return;
    }

    // 确保缓冲区索引有效
    if (buf.index >= 4 || !m_buffers[buf.index]) {
        qDebug() << "无效的缓冲区索引:" << buf.index;
        ioctl(m_fd, VIDIOC_QBUF, &buf);   // 尝试放回缓冲区
        return;
    }

    QImage image = frameToQImage(m_buffers[buf.index], m_width, m_height, m_pixelFormat);

    // 放回缓冲区 - 不论图像转换是否成功都要放回
    if (ioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
        qDebug() << "放回缓冲区失败:" << strerror(errno);
        return;
    }

    if (!image.isNull()) {
        // 保存调整后的图像作为最新帧，用于捕获
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = image;

        // emit image
        emit previewUpdated(image);
    }
}

void WebcamDevice::captureImage()
{
    // 检查设备状态
    if (!m_isInitialized || m_fd <= 0) {
        emit errorOccurred(tr("Device not initialized or invalid file descriptor"));
        return;
    }

    emit captureStarted();
    qDebug() << "开始捕获图像...";

    // 检查预览是否在运行
    bool previewWasRunning = m_previewTimer.isActive();

    // 如果预览未运行，启动一个临时捕获流程
    if (!previewWasRunning) {
        qDebug() << "预览未运行，设置临时捕获流程...";

        // 确保清理当前状态
        stopCapturing();
        QThread::msleep(100);

        // 开始捕获流程
        if (!startCapturing()) {
            emit errorOccurred(tr("Failed to start video stream, capture failed"));
            return;
        }

        // 等待设备准备好
        QThread::msleep(300);
    } else {
        // 如果预览正在运行，暂停预览计时器但保持流活动
        m_previewTimer.stop();
    }

    // 尝试从设备直接获取一帧，无论预览是否在运行
    for (int retry = 0; retry < 3; retry++) {
        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        qDebug() << "尝试直接从设备获取帧...第" << (retry + 1) << "次尝试";

        if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1) {
            qDebug() << "获取帧失败:" << strerror(errno);
            if (retry == 2) {   // 最后一次尝试失败
                // 尝试使用现有的预览帧（如果有的话）
                QMutexLocker locker(&m_frameMutex);
                if (!m_latestFrame.isNull()) {
                    qDebug() << "获取帧失败，但使用现有的预览帧";
                    QImage capturedImage = m_latestFrame.copy();
                    locker.unlock();

                    // 恢复预览状态
                    if (previewWasRunning) {
                        m_previewTimer.start();
                    } else {
                        stopCapturing();
                    }

                    emit imageCaptured(capturedImage);
                    return;
                }

                // 如果没有可用的预览帧，报告错误
                emit errorOccurred(tr("Failed to get image frame"));

                // 恢复预览状态
                if (previewWasRunning) {
                    m_previewTimer.start();
                } else {
                    stopCapturing();
                }
                return;
            }
            // 重试前短暂等待
            QThread::msleep(100);
            continue;
        }

        // 确保缓冲区索引有效
        if (buf.index >= 4 || !m_buffers[buf.index]) {
            qDebug() << "无效的缓冲区索引:" << buf.index;
            ioctl(m_fd, VIDIOC_QBUF, &buf);   // 放回缓冲区
            continue;
        }

        // 成功获取到帧
        QImage image = frameToQImage(m_buffers[buf.index], m_width, m_height, m_pixelFormat);

        // 放回缓冲区
        ioctl(m_fd, VIDIOC_QBUF, &buf);

        if (!image.isNull()) {
            // 保存当前帧到预览缓冲区
            {
                QMutexLocker locker(&m_frameMutex);
                m_latestFrame = image;
            }

            // 如果预览之前是运行的，恢复预览计时器
            if (previewWasRunning) {
                m_previewTimer.start();
            } else {
                stopCapturing();
            }

            qDebug() << "成功捕获图像";
            emit imageCaptured(image);
            return;
        } else {
            qDebug() << "图像转换失败，重试";
        }

        // 重试前短暂等待
        QThread::msleep(100);
    }

    // 如果所有尝试都失败
    emit errorOccurred(tr("Failed to capture valid image, please check camera connection"));

    // 恢复预览状态
    if (previewWasRunning) {
        m_previewTimer.start();
    } else {
        stopCapturing();
    }
}

QImage WebcamDevice::frameToQImage(const void *data, int width, int height, int format)
{
    if (!data) {
        qDebug() << "帧数据为空";
        return QImage();
    }

    // qDebug() << "转换帧数据:" << width << "x" << height << "格式:" << format;

    if (format == V4L2_PIX_FMT_YUYV) {
        QImage image(width, height, QImage::Format_RGB888);
        const uint8_t *yuyv = static_cast<const uint8_t *>(data);

        // 安全检查 - 确保内存足够
        size_t expectedSize = width * height * 2;   // YUYV格式每个像素需要2字节
        if (expectedSize > 0) {   // 合理的大小上限
            try {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x += 2) {
                        int i = y * width * 2 + x * 2;

                        // 边界检查
                        if (i + 3 >= expectedSize) {
                            qDebug() << "帧数据边界检查失败，可能数据不完整";
                            continue;
                        }

                        int y0 = yuyv[i];
                        int u = yuyv[i + 1];
                        int y1 = yuyv[i + 2];
                        int v = yuyv[i + 3];

                        // 保持原始亮度值
                        y0 = y0;
                        y1 = y1;

                        int r, g, b;

                        // 第一个像素 - 使用更准确的YUV到RGB转换公式
                        r = static_cast<int>(y0 + 1.370705 * (v - 128));
                        g = static_cast<int>(y0 - 0.337633 * (u - 128) - 0.698001 * (v - 128));
                        b = static_cast<int>(y0 + 1.732446 * (u - 128));

                        r = qBound(0, r, 255);
                        g = qBound(0, g, 255);
                        b = qBound(0, b, 255);

                        if (x < width) {   // 安全检查
                            image.setPixel(x, y, qRgb(r, g, b));
                        }

                        // 第二个像素 - 使用相同的转换公式
                        r = static_cast<int>(y1 + 1.370705 * (v - 128));
                        g = static_cast<int>(y1 - 0.337633 * (u - 128) - 0.698001 * (v - 128));
                        b = static_cast<int>(y1 + 1.732446 * (u - 128));

                        r = qBound(0, r, 255);
                        g = qBound(0, g, 255);
                        b = qBound(0, b, 255);

                        if (x + 1 < width) {   // 安全检查
                            image.setPixel(x + 1, y, qRgb(r, g, b));
                        }
                    }
                }
                return image;
            } catch (const std::exception &e) {
                qDebug() << "转换帧时发生异常:" << e.what();
            } catch (...) {
                qDebug() << "转换帧时发生未知异常";
            }
        } else {
            qDebug() << "不合理的帧大小:" << expectedSize;
        }
    } else if (format == V4L2_PIX_FMT_MJPEG) {
        // 获取实际数据大小，而不是使用计算值
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = m_currentBuffer;

        size_t actualSize = 0;
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) != -1) {
            actualSize = buf.bytesused;
            // qDebug() << "MJPEG实际数据大小:" << actualSize;
        } else {
            actualSize = width * height;
            qDebug() << "使用估计大小:" << actualSize;
        }

        QImage image;
        if (image.loadFromData(static_cast<const uchar *>(data), actualSize, "JPEG")) {
            return image;
        }
    } else {
        qDebug() << "不支持的像素格式:" << format;
    }

    return QImage();
}

void WebcamDevice::closeDevice()
{
    // 先停止预览和流
    m_previewTimer.stop();
    stopCapturing();

    // 清空最新的预览帧
    {
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = QImage();
    }

    // 释放内存映射
    uninitMmap();

    // 关闭文件描述符
    if (m_fd > 0) {
        close(m_fd);
        qDebug() << "关闭设备文件描述符:" << m_fd;
        m_fd = -1;
    }

    m_deviceSelected = false;
    m_isInitialized = false;
    setState(Disconnected);
}

// 新增方法：获取摄像头支持的最大分辨率
QSize WebcamDevice::getMaxResolution()
{
    if (m_fd <= 0) {
        qDebug() << "文件描述符无效，无法获取最大分辨率";
        return QSize(640, 480);   // 返回默认值
    }

    // 存储最大分辨率
    int maxWidth = 0;
    int maxHeight = 0;
    bool resolutionsFound = false;

    // 首先尝试获取设备支持的所有离散分辨率
    QList<QSize> supportedSizes;
    v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = m_pixelFormat;

    qDebug() << "尝试获取设备支持的分辨率列表:";

    for (frmsize.index = 0; ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
        resolutionsFound = true;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            supportedSizes.append(QSize(frmsize.discrete.width, frmsize.discrete.height));
            qDebug() << "  支持的分辨率:" << frmsize.discrete.width << "x" << frmsize.discrete.height;
        } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE || frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
            // 对于可变分辨率，我们选择一个安全的较大值
            qDebug() << "  支持范围:" << frmsize.stepwise.min_width << "x" << frmsize.stepwise.min_height
                     << " 到 " << frmsize.stepwise.max_width << "x" << frmsize.stepwise.max_height;

            // 对于可变分辨率，限制最大值为1280x720，除非确认设备支持更高
            int maxW = frmsize.stepwise.max_width;
            int maxH = frmsize.stepwise.max_height;

            // 如果分辨率过高，可能会导致问题，我们限制为常见的高清分辨率
            if (maxW > 1920 || maxH > 1080) {
                maxW = 1280;   // 默认限制为720p
                maxH = 720;
            }

            maxWidth = maxW;
            maxHeight = maxH;

            qDebug() << "  选择使用分辨率:" << maxWidth << "x" << maxHeight;
            return QSize(maxWidth, maxHeight);
        }
    }

    // 如果找到了离散分辨率，找出其中最大的（但不超过1280x720，除非明确支持）
    if (resolutionsFound && !supportedSizes.isEmpty()) {
        // 按照分辨率（像素总数）排序
        std::sort(supportedSizes.begin(), supportedSizes.end(),
                  [](const QSize &a, const QSize &b) {
                      return a.width() * a.height() > b.width() * b.height();
                  });

        // 选择最大但不超过720p的分辨率（除非明确支持更高）
        for (const QSize &size : supportedSizes) {
            // 如果我们找到了一个合理的分辨率，就使用它
            maxWidth = size.width();
            maxHeight = size.height();

            // 如果发现至少有720p的分辨率，就直接使用它
            if (maxWidth >= 1280 && maxHeight >= 720) {
                // 优先选择720p或1080p这样的标准分辨率
                if (maxWidth == 1280 && maxHeight == 720) {
                    qDebug() << "选择标准分辨率:" << maxWidth << "x" << maxHeight;
                    return QSize(maxWidth, maxHeight);
                }

                if (maxWidth == 1920 && maxHeight == 1080) {
                    qDebug() << "选择标准分辨率:" << maxWidth << "x" << maxHeight;
                    return QSize(maxWidth, maxHeight);
                }

                // 如果找不到完全匹配的标准分辨率，就使用列表中的第一个（最大的）
                qDebug() << "选择最大分辨率:" << supportedSizes.first().width() << "x" << supportedSizes.first().height();
                return supportedSizes.first();
            }

            // 如果没有找到够大的分辨率，就使用列表中最大的
            break;
        }
    }

    // 如果前面的方法失败，尝试获取当前格式
    if (maxWidth == 0 || maxHeight == 0) {
        v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == 0) {
            maxWidth = fmt.fmt.pix.width;
            maxHeight = fmt.fmt.pix.height;
            qDebug() << "使用当前分辨率:" << maxWidth << "x" << maxHeight;
        }
    }

    // 如果还是获取不到，返回安全的默认值
    if (maxWidth <= 0 || maxHeight <= 0) {
        maxWidth = 640;
        maxHeight = 480;
        qDebug() << "使用默认分辨率:" << maxWidth << "x" << maxHeight;
    }

    return QSize(maxWidth, maxHeight);
}

// 新增方法：尝试选择最佳像素格式
bool WebcamDevice::selectBestPixelFormat()
{
    if (m_fd == -1) {
        return false;
    }

    // 优先级顺序：MJPEG > YUYV > 其他
    const uint32_t preferredFormats[] = {
        V4L2_PIX_FMT_MJPEG,   // MJPEG性能更好
        V4L2_PIX_FMT_YUYV,   // 常见格式
        V4L2_PIX_FMT_YUV420,   // 另一种常见格式
        V4L2_PIX_FMT_RGB24   // RGB格式
    };

    // 获取当前格式作为备选
    v4l2_format currentFmt = {};
    currentFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &currentFmt) != -1) {
        qDebug() << "当前像素格式:" << currentFmt.fmt.pix.pixelformat;
    }

    // 遍历可用的像素格式
    v4l2_fmtdesc fmtdesc = {};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    QVector<uint32_t> supportedFormats;

    for (fmtdesc.index = 0; ioctl(m_fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1; fmtdesc.index++) {
        qDebug() << "支持的像素格式:" << fmtdesc.pixelformat
                 << "描述:" << reinterpret_cast<const char *>(fmtdesc.description);
        supportedFormats.append(fmtdesc.pixelformat);
    }

    // 按优先级尝试设置
    for (uint32_t format : preferredFormats) {
        if (supportedFormats.contains(format)) {
            v4l2_format fmt = {};
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            // 先获取当前格式
            if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == -1) {
                continue;
            }

            // 只修改像素格式，保留其他参数
            fmt.fmt.pix.pixelformat = format;

            if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
                if (fmt.fmt.pix.pixelformat == format) {
                    m_pixelFormat = format;
                    qDebug() << "成功设置像素格式:" << format;
                    return true;
                }
            }
        }
    }

    // 如果所有尝试都失败，至少确保我们知道当前的像素格式
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) != -1) {
        m_pixelFormat = fmt.fmt.pix.pixelformat;
        qDebug() << "使用当前像素格式:" << m_pixelFormat;
        return true;
    }

    return false;
}

// 新增函数：诊断摄像头设备问题
void WebcamDevice::diagnoseCameraIssues()
{
    if (m_fd == -1) {
        qDebug() << "无法诊断: 设备未打开";
        return;
    }

    qDebug() << "开始摄像头诊断...";

    // 检查设备权限
    struct stat st;
    if (fstat(m_fd, &st) == 0) {
        qDebug() << "设备权限:"
                 << (st.st_mode & S_IRUSR ? "r" : "-")
                 << (st.st_mode & S_IWUSR ? "w" : "-")
                 << (st.st_mode & S_IXUSR ? "x" : "-")
                 << " 所有者:" << st.st_uid;

        if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
            qDebug() << "警告: 设备没有足够的读写权限";
        }
    }

    // 检查内存映射支持
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) != -1) {
        qDebug() << "设备能力:";
        qDebug() << "  视频捕获: " << ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? "是" : "否");
        qDebug() << "  视频输出: " << ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) ? "是" : "否");
        qDebug() << "  流支持: " << ((cap.capabilities & V4L2_CAP_STREAMING) ? "是" : "否");
        qDebug() << "  读写支持: " << ((cap.capabilities & V4L2_CAP_READWRITE) ? "是" : "否");

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            qDebug() << "错误: 设备不支持流操作，无法使用内存映射";
        }
    }

    // 检查当前格式
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) != -1) {
        qDebug() << "当前格式:";
        qDebug() << "  宽度: " << fmt.fmt.pix.width;
        qDebug() << "  高度: " << fmt.fmt.pix.height;
        qDebug() << "  像素格式: " << fmt.fmt.pix.pixelformat;
        qDebug() << "  字段类型: " << fmt.fmt.pix.field;
        qDebug() << "  每行字节数: " << fmt.fmt.pix.bytesperline;
        qDebug() << "  图像大小: " << fmt.fmt.pix.sizeimage << " 字节";

        if (fmt.fmt.pix.bytesperline == 0 || fmt.fmt.pix.sizeimage == 0) {
            qDebug() << "警告: 无效的图像大小参数";
        }
    }

    // 检查系统内存
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        unsigned long freeRam = sys_info.freeram * sys_info.mem_unit / (1024 * 1024);
        qDebug() << "系统空闲内存: " << freeRam << " MB";

        if (freeRam < 50) {
            qDebug() << "警告: 系统内存不足，可能无法分配缓冲区";
        }
    }

    // 尝试只使用一个缓冲区
    v4l2_requestbuffers req = {};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    qDebug() << "尝试请求1个缓冲区...";
    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        qDebug() << "请求缓冲区失败: " << strerror(errno);

        // 检查特定的错误代码
        if (errno == EINVAL) {
            qDebug() << "错误: 设备不支持内存映射";
        } else if (errno == EBUSY) {
            qDebug() << "错误: 设备忙或已被其他进程打开";
        }
    } else {
        qDebug() << "成功请求到 " << req.count << " 个缓冲区";

        // 释放刚刚分配的缓冲区
        req.count = 0;
        ioctl(m_fd, VIDIOC_REQBUFS, &req);
    }

    qDebug() << "摄像头诊断完成";
}

QList<QSize> WebcamDevice::getSupportedResolutions()
{
    if (m_supportedResolutions.isEmpty() && m_fd > 0) {
        enumerateSupportedResolutions();
    }

    // 添加调试信息
    qDebug() << "获取支持的分辨率数量:" << m_supportedResolutions.size();
    for (const QSize &res : m_supportedResolutions) {
        qDebug() << "  支持的分辨率:" << res.width() << "x" << res.height();
    }

    return m_supportedResolutions;
}

void WebcamDevice::enumerateSupportedResolutions()
{
    // 清空分辨率列表，确保每次重新获取
    m_supportedResolutions.clear();

    if (m_fd <= 0) {
        qDebug() << "设备未打开，无法获取支持的分辨率";
        return;
    }

    // 检查是否支持VIDIOC_ENUM_FRAMESIZES
    v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = m_pixelFormat;

    qDebug() << "枚举设备支持的分辨率:";

    // 标记是否找到任何分辨率
    bool foundAnyResolution = false;

    for (frmsize.index = 0; ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
        foundAnyResolution = true;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            QSize resolution(frmsize.discrete.width, frmsize.discrete.height);
            // 检查重复，避免添加相同的分辨率
            if (!m_supportedResolutions.contains(resolution)) {
                m_supportedResolutions.append(resolution);
                qDebug() << "  发现离散分辨率:" << resolution.width() << "x" << resolution.height();
            }
        } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE || frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
            // 对于可变范围分辨率，添加几个标准尺寸
            qDebug() << "  支持范围:" << frmsize.stepwise.min_width << "x" << frmsize.stepwise.min_height
                     << " 到 " << frmsize.stepwise.max_width << "x" << frmsize.stepwise.max_height;

            // 生成一些标准分辨率在支持范围内
            QList<QSize> standardResolutions;
            standardResolutions << QSize(640, 480) << QSize(800, 600)
                                << QSize(1024, 768) << QSize(1280, 720)
                                << QSize(1920, 1080) << QSize(2560, 1440)
                                << QSize(3840, 2160);

            for (const QSize &res : standardResolutions) {
                if (res.width() >= frmsize.stepwise.min_width && res.width() <= frmsize.stepwise.max_width && res.height() >= frmsize.stepwise.min_height && res.height() <= frmsize.stepwise.max_height) {
                    // 检查重复，避免添加相同的分辨率
                    if (!m_supportedResolutions.contains(res)) {
                        m_supportedResolutions.append(res);
                        qDebug() << "  添加标准分辨率:" << res.width() << "x" << res.height();
                    }
                }
            }

            // 找到了步进范围后退出循环
            break;
        }
    }

    // 如果无法枚举分辨率，至少添加一些常见的分辨率
    if (!foundAnyResolution || m_supportedResolutions.isEmpty()) {
        qDebug() << "无法枚举分辨率或未找到分辨率，添加常见分辨率";
        // 确保不添加重复的分辨率
        if (!m_supportedResolutions.contains(QSize(640, 480)))
            m_supportedResolutions << QSize(640, 480);
        if (!m_supportedResolutions.contains(QSize(800, 600)))
            m_supportedResolutions << QSize(800, 600);
        if (!m_supportedResolutions.contains(QSize(1280, 720)))
            m_supportedResolutions << QSize(1280, 720);
        if (!m_supportedResolutions.contains(QSize(1920, 1080)))
            m_supportedResolutions << QSize(1920, 1080);
    }

    // 排序分辨率从小到大
    std::sort(m_supportedResolutions.begin(), m_supportedResolutions.end(),
              [](const QSize &a, const QSize &b) {
                  return a.width() * a.height() < b.width() * b.height();
              });

    qDebug() << "最终枚举到的分辨率数量:" << m_supportedResolutions.size();

    // 发出信号通知分辨率列表已更新
    emit resolutionsChanged(m_supportedResolutions);
}

bool WebcamDevice::setCameraControl(uint32_t controlId, int value)
{
    if (m_fd <= 0) {
        qDebug() << "文件描述符无效，无法设置控制参数";
        return false;
    }

    // 先查询控制项是否存在及其范围
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = controlId;

    if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        if (errno == EINVAL) {
            qDebug() << "摄像头不支持此控制项:" << controlId;
        } else {
            qDebug() << "查询控制项失败:" << strerror(errno);
        }
        return false;
    }
    auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
    qDebug() << "控制项:" << controlName << " 范围:" << queryctrl.minimum << "-" << queryctrl.maximum << " 默认值:" << queryctrl.default_value;

    if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        qDebug() << "控制项被禁用:" << controlName;
        return false;
    }

    if (V4L2_CID_BRIGHTNESS == controlId) {
        int mid = 0;
        if (queryctrl.minimum < 0) {
            // 亮度范围是负数
            mid = (queryctrl.maximum + queryctrl.minimum) / 2;
            if (mid <= 0) {
                mid = 1;
            }
        } else {
            // 亮度范围是正数
            mid = (queryctrl.maximum - queryctrl.minimum) / 2;
        }
        // set percentage value of middle for brightness
        value = mid * value / 100;
    } else {
        // 确保值在有效范围内
        if (value <= queryctrl.minimum) value = queryctrl.minimum + 1;
        if (value >= queryctrl.maximum) value = queryctrl.maximum - 1;
    }
    qDebug() << "设置控制项:" << controlName << "值为" << value;

    // 设置控制项的值
    struct v4l2_control control;
    control.id = controlId;
    control.value = value;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
        qDebug() << "设置控制项失败:" << controlName << "值:" << value << "错误:" << strerror(errno);
        return false;
    }

    qDebug() << "成功设置控制项:" << controlName << "值:" << value;
    return true;
}

void WebcamDevice::listCameraControls()
{
    if (m_fd <= 0) {
        qDebug() << "文件描述符无效，无法列出控制参数";
        return;
    }

    qDebug() << "摄像头支持的控制参数:";

    // 标准控制项
    struct v4l2_queryctrl queryctrl;
    for (uint32_t id = V4L2_CID_BASE; id < V4L2_CID_LASTP1; id++) {
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = id;

        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
                auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
                struct v4l2_control control;
                control.id = id;
                if (ioctl(m_fd, VIDIOC_G_CTRL, &control) == 0) {
                    qDebug() << "  控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "当前值:" << control.value
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value;
                } else {
                    qDebug() << "  控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value
                             << "(无法获取当前值)";
                }
            }
        }
    }

    // 相机类控制项
    for (uint32_t id = V4L2_CID_CAMERA_CLASS_BASE; id < V4L2_CID_CAMERA_CLASS_BASE + 100; id++) {
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = id;

        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
                auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
                struct v4l2_control control;
                control.id = id;
                if (ioctl(m_fd, VIDIOC_G_CTRL, &control) == 0) {
                    qDebug() << "  相机控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "当前值:" << control.value
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value;
                } else {
                    qDebug() << "  相机控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value
                             << "(无法获取当前值)";
                }
            }
        }
    }

    // 私有控制项
    for (uint32_t id = V4L2_CID_PRIVATE_BASE; id < V4L2_CID_PRIVATE_BASE + 100; id++) {
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = id;

        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
                auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
                struct v4l2_control control;
                control.id = id;
                if (ioctl(m_fd, VIDIOC_G_CTRL, &control) == 0) {
                    qDebug() << "  私有控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "当前值:" << control.value
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value;
                } else {
                    qDebug() << "  私有控制项:" << controlName
                             << "ID:" << queryctrl.id
                             << "范围:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "默认值:" << queryctrl.default_value
                             << "(无法获取当前值)";
                }
            }
        }
    }
}

bool WebcamDevice::adjustCommonCameraSettings()
{
    if (m_fd <= 0) {
        qDebug() << "[Error] Invalid file descriptor";
        return false;
    }

    // 调试信息：列出摄像头支持的控制项
    qDebug() << "=== Starting camera optimization ===";
    listCameraControls();

    // 参数配置策略（带智能适应）
    struct CameraControlConfig {
        uint32_t id;
        int target;  // -1=默认值, -2=不修改, -3=自动计算
        const char* name;
        bool required;
    };

    const CameraControlConfig configTable[] = {
        // 自动控制组（必须先处理）
        { V4L2_CID_EXPOSURE_AUTO,       V4L2_EXPOSURE_APERTURE_PRIORITY, "Exposure Mode",    true },
        { V4L2_CID_AUTOGAIN,            0,                             "Auto Gain",     false },
        
        // 核心画质参数
        { V4L2_CID_BRIGHTNESS,          -1,                            "Brightness",    false },
        { V4L2_CID_CONTRAST,            38,                            "Contrast",      false },
        { V4L2_CID_SATURATION,          88,                            "Saturation",    false },
        { V4L2_CID_SHARPNESS,           4,                             "Sharpness",     false },
        
        // 动态计算参数
        { V4L2_CID_EXPOSURE_ABSOLUTE,   -3,                            "Exposure Time", true },
        { V4L2_CID_GAIN,                -3,                            "Gain",          true },
        { V4L2_CID_GAMMA,               110,                           "Gamma",         false },
        
        // 自动优化参数
        { V4L2_CID_AUTO_WHITE_BALANCE,  1,                             "Auto WB",       false },
        { V4L2_CID_POWER_LINE_FREQUENCY,1,                             "Power Line",    false }
    };

    bool successFlag = false;
    QElapsedTimer timer;
    timer.start();

    // 第一阶段：参数可行性检查
    QMap<uint32_t, v4l2_queryctrl> validControls;
    for (const auto& cfg : configTable) {
        v4l2_queryctrl query = {};
        query.id = cfg.id;
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &query) == 0) {
            if (!(query.flags & V4L2_CTRL_FLAG_DISABLED)) {
                validControls.insert(cfg.id, query);
                qDebug() << "[Precheck] Supported:" << cfg.name 
                         << "Range:" << query.minimum << "-" << query.maximum;
            }
        }
    }

    // 第二阶段：智能参数设置
    for (const auto& cfg : configTable) {
        if (!validControls.contains(cfg.id)) {
            if (cfg.required) {
                qDebug() << "[Critical] Missing required control:" << cfg.name;
            }
            continue;
        }

        const auto& query = validControls[cfg.id];
        int finalValue = 0;

        // 动态值计算策略
        if (cfg.target == -1) {  // 使用默认值
            finalValue = query.default_value;
        } 
        else if (cfg.target == -2) {  // 不修改
            continue;
        }
        else if (cfg.target == -3) {  // 智能计算
            if (cfg.id == V4L2_CID_EXPOSURE_ABSOLUTE) {
                finalValue = std::clamp(
                    (query.default_value + query.maximum) / 2,
                    static_cast<int>(query.minimum),
                    static_cast<int>(query.maximum)
                );
            } 
            else if (cfg.id == V4L2_CID_GAIN) {
                finalValue = std::min(
                    query.default_value + 5,
                    static_cast<int>(query.maximum)
                );
            }
        }
        else {  // 固定值
            finalValue = std::clamp(
                cfg.target,
                static_cast<int>(query.minimum),
                static_cast<int>(query.maximum)
            );
        }

        // 设置参数（带重试机制）
        v4l2_control ctrl = {};
        ctrl.id = cfg.id;
        ctrl.value = finalValue;
        
        for (int retry = 0; retry < 3; ++retry) {
            if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl) == 0) {
                qDebug() << "[Success] Set" << cfg.name << "=" << finalValue 
                         << "(Default:" << query.default_value << ")";
                successFlag = true;
                break;
            }
            
            if (retry == 2) {
                qDebug() << "[Error] Failed to set" << cfg.name 
                         << "Error:" << strerror(errno);
                // 关键参数失败时恢复自动模式
                if (cfg.required && cfg.id == V4L2_CID_EXPOSURE_ABSOLUTE) {
                    v4l2_control fallback = {};
                    fallback.id = V4L2_CID_EXPOSURE_AUTO;
                    fallback.value = V4L2_EXPOSURE_AUTO;
                    ioctl(m_fd, VIDIOC_S_CTRL, &fallback);
                }
            }
            QThread::msleep(50 * (retry + 1));
        }
    }

    qDebug() << "Optimization completed in" << timer.elapsed() << "ms";
    return successFlag;
}

// Implementation of camera control shortcut methods
bool WebcamDevice::setCameraBrightness(int value)
{
    return setCameraControl(V4L2_CID_BRIGHTNESS, value);
}

bool WebcamDevice::setCameraContrast(int value)
{
    return setCameraControl(V4L2_CID_CONTRAST, value);
}

bool WebcamDevice::setCameraExposure(int value)
{
    return setCameraControl(V4L2_CID_EXPOSURE, value);
}

bool WebcamDevice::setCameraAutoExposure(bool enable)
{
    return setCameraControl(V4L2_CID_EXPOSURE_AUTO, enable ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL);
}

bool WebcamDevice::setCameraAutoFocus(bool enable)
{
    // 1. 首先检查设备是否支持自动对焦控制
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CID_FOCUS_AUTO;
    
    if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        qDebug() << "摄像头不支持自动对焦控制";
        return false;
    }

    // 2. 设置自动对焦模式（自动或手动）
    struct v4l2_control control;
    control.id = V4L2_CID_FOCUS_AUTO;
    control.value = enable ? 1 : 0;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
        qDebug() << "设置自动对焦模式失败:" << strerror(errno);
        return false;
    }

    // 3. 如果启用自动对焦，则启动对焦过程
    if (enable) {
        // 检查是否支持自动对焦操作
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = V4L2_CID_AUTO_FOCUS_START;
        
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) != -1) {
            // 启动自动对焦
            control.id = V4L2_CID_AUTO_FOCUS_START;
            control.value = 1;
            
            if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
                qDebug() << "启动自动对焦失败:" << strerror(errno);
                // 这里不返回失败，因为可能设备不支持启动但支持自动模式
            } else {
                qDebug() << "自动对焦已启动";
            }
        } else {
            qDebug() << "摄像头不支持启动自动对焦";
        }
    } else {
        // 如果是禁用自动对焦，尝试停止可能正在进行的对焦
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = V4L2_CID_AUTO_FOCUS_STOP;
        
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) != -1) {
            control.id = V4L2_CID_AUTO_FOCUS_STOP;
            control.value = 1;
            
            if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
                qDebug() << "停止自动对焦失败:" << strerror(errno);
            } else {
                qDebug() << "自动对焦已停止";
            }
        }
    }

    return true;
}

bool WebcamDevice::initMmap()
{
    // 首先检查设备是否打开
    if (m_fd == -1) {
        qDebug() << "设备未打开，无法初始化内存映射";
        return false;
    }

    // 确保之前的映射已清理
    uninitMmap();

    // 停止任何可能正在运行的流
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(m_fd, VIDIOC_STREAMOFF, &type);

    // 释放已经存在的任何缓冲区
    v4l2_requestbuffers release_req = {};
    release_req.count = 0;   // 请求0个缓冲区会释放所有现有缓冲区
    release_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    release_req.memory = V4L2_MEMORY_MMAP;
    int release_result = ioctl(m_fd, VIDIOC_REQBUFS, &release_req);
    if (release_result == -1) {
        qDebug() << "释放已有缓冲区失败:" << strerror(errno);
    }

    // 给设备一些恢复时间
    QThread::msleep(200);

    // 初始化缓冲区数组
    for (int i = 0; i < 4; i++) {
        m_buffers[i] = nullptr;
        m_bufferSizes[i] = 0;
    }

    // 请求缓冲区 - 只使用1个缓冲区以降低复杂度
    v4l2_requestbuffers req = {};
    req.count = 1;   // 使用1个缓冲区，简化操作
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        qDebug() << "请求缓冲区失败:" << strerror(errno);
        return false;
    }

    if (req.count < 1) {
        qDebug() << "设备返回的缓冲区数量不足:" << req.count;
        return false;
    }

    qDebug() << "成功请求到" << req.count << "个缓冲区";

    // 记录实际分配的缓冲区数量
    int bufferCount = req.count;
    if (bufferCount > 4) bufferCount = 4;

    // 映射缓冲区
    bool success = true;
    for (int i = 0; i < bufferCount; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            qDebug() << "查询缓冲区" << i << "失败:" << strerror(errno);
            success = false;
            break;
        }

        qDebug() << "缓冲区" << i << "信息: 偏移量=" << buf.m.offset
                 << " 长度=" << buf.length;

        void *ptr = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                         MAP_SHARED, m_fd, buf.m.offset);

        if (ptr == MAP_FAILED) {
            qDebug() << "映射缓冲区" << i << "失败:" << strerror(errno);
            success = false;
            break;
        }

        m_buffers[i] = ptr;
        m_bufferSizes[i] = buf.length;

        qDebug() << "成功映射缓冲区" << i << "大小:" << buf.length << "字节, 地址:" << ptr;
    }

    // 如果任何映射失败，清理所有已映射的缓冲区
    if (!success) {
        uninitMmap();
        return false;
    }

    return true;
}

void WebcamDevice::uninitMmap()
{
    for (int i = 0; i < 4; ++i) {
        if (m_buffers[i]) {
            munmap(m_buffers[i], m_bufferSizes[i]);
            m_buffers[i] = nullptr;
            m_bufferSizes[i] = 0;
        }
    }
}

QImage WebcamDevice::getLatestFrame()
{
    QMutexLocker locker(&m_frameMutex);
    return m_latestFrame.copy();
}
