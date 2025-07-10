// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webcamdevice.h"
#include "ddlog.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QPainter>
#include <QPaintEvent>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

using namespace DDLog;

// WebcamDevice implementation
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
                // Ensure it supports video capture and is a physical device
                if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_DEVICE_CAPS & V4L2_CAP_META_CAPTURE)) {
                    // Check if there is an actual video input device
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
    // Webcam device doesn't need special initialization, return success directly
    setState(Initialized);
    return true;
}

bool WebcamDevice::openDevice(const QString &devicePath)
{
    // Make sure to close previous device first
    closeDevice();
    qCInfo(app) << "Opening webcam device:" << devicePath;

    // the device path include name and driver, may include vender. e.g. UVC Camera (046d:0825) (/dev/video0)
    // we only need the /dev/videoX part
    QString actualPath;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRegularExpression devRegExp("(/dev/video\\d+)");
    QRegularExpressionMatch match = devRegExp.match(devicePath);
    if (match.hasMatch()) {
        actualPath = match.captured(1);
    }
#else
    QRegExp devRegExp("(/dev/video\\d+)");
    if (devRegExp.indexIn(devicePath) != -1) {
        actualPath = devRegExp.cap(1);
    }
#endif

    // If regex matching fails, try old method as fallback
    if (actualPath.isEmpty()) {
        int idx = devicePath.indexOf("/dev");
        if (idx > 0) {
            actualPath = devicePath.mid(idx);
            if (actualPath.endsWith(")")) {
                actualPath.chop(1);
            }
            if (!actualPath.startsWith("/dev/")) {
                actualPath.clear();
            }
        }
    }

    if (actualPath.isEmpty()) {
        qCCritical(app) << "Invalid device path format:" << devicePath;
        return false;
    }
    qCDebug(app) << "Extracted device path:" << actualPath;

    // Open device with O_NONBLOCK flag
    m_fd = open(actualPath.toUtf8().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd <= 0) {
        qCCritical(app) << "Failed to open webcam device:" << strerror(errno) << "(errno:" << errno << ")";
        m_fd = -1;
        return false;
    }

    // Verify device is a video capture device
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
    qCCritical(app) << "Failed to query webcam device capabilities:" << strerror(errno) << "(errno:" << errno << ")";
    close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qCCritical(app) << "Device does not support video capture";
        close(m_fd);
        m_fd = -1;
        return false;
    }

    qCInfo(app) << "Device info - driver:" << reinterpret_cast<const char *>(cap.driver)
                     << "name:" << reinterpret_cast<const char *>(cap.card);

    selectBestPixelFormat();

    // Set appropriate resolution
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // Get current format first
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == -1) {
        qCWarning(app) << "Failed to get current format:" << strerror(errno);
    }

    // Update device parameters
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;
    m_pixelFormat = fmt.fmt.pix.pixelformat;

    qCInfo(app) << "Current resolution:" << m_width << "x" << m_height;

    auto maxRes = getMaxResolution();
    // Try to set default resolution as max resolution
    {
        fmt.fmt.pix.width = maxRes.width();
        fmt.fmt.pix.height = maxRes.height();
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
            m_width = fmt.fmt.pix.width;
            m_height = fmt.fmt.pix.height;
            m_pixelFormat = fmt.fmt.pix.pixelformat;
            qCInfo(app) << "Resolution set successfully:" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
        } else {
            qCWarning(app) << "Failed to set resolution - using current resolution";
        }
    }

    // Initialize memory mapping
    if (!initMmap()) {
        qCCritical(app) << "Failed to initialize memory mapping for webcam device:" << strerror(errno) << "(errno:" << errno << ")";
        close(m_fd);
        m_fd = -1;
        return false;
    }

    m_isInitialized = true;
    m_deviceSelected = true;
    m_currentDeviceName = devicePath;
    setState(Connected);

    // Enumerate supported resolutions
    enumerateSupportedResolutions();

    return true;
}

bool WebcamDevice::setResolution(int width, int height)
{
    qCDebug(app) << "Attempting to set resolution:" << width << "x" << height;

    // Save original state
    bool wasInitialized = m_isInitialized;
    bool wasDeviceSelected = m_deviceSelected;
    bool wasPreviewActive = m_previewTimer.isActive();

    // Save current device path
    QString devicePath;
    if (m_fd > 0) {
        char devPath[256];
        sprintf(devPath, "/proc/self/fd/%d", m_fd);
        char realPath[256] = { 0 };
        if (readlink(devPath, realPath, sizeof(realPath) - 1) != -1) {
            devicePath = QString(realPath);
            qCDebug(app) << "Current device path:" << devicePath;
        }
    }

    // If device path is empty but initialized, path retrieval may have failed
    if (devicePath.isEmpty() && m_fd > 0) {
        qCDebug(app) << "Unable to get current device path, using fallback method";
        // Here we can add fallback method to get device path
    }

    // Stop preview and completely close device
    if (wasPreviewActive) {
        stopPreview();
    }

    // Completely close device and release all resources
    closeDevice();

    // Give system some time to fully release device
    QThread::msleep(500);

    // Without device path, cannot continue
    if (devicePath.isEmpty()) {
        emit errorOccurred(tr("Cannot get device path, cannot set resolution"));
        return false;
    }

    // Reopen device
    qCDebug(app) << "Reopening device:" << devicePath;
    m_fd = open(devicePath.toUtf8().constData(), O_RDWR);
    if (m_fd <= 0) {
        qCDebug(app) << "Failed to reopen device:" << strerror(errno);
        emit errorOccurred(tr("Failed to reopen device"));
        return false;
    }

    // Verify device is a video capture device
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        qCDebug(app) << "Device verification failed:" << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // Set format
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = m_pixelFormat;   // Use previous pixel format
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    bool formatSet = false;
    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
        formatSet = true;
        qCDebug(app) << "Successfully set resolution:" << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height;
    } else {
        qCDebug(app) << "Failed to set resolution, trying other pixel formats";
        formatSet = selectBestPixelFormat();
    }

    if (!formatSet) {
        qCDebug(app) << "All attempts failed, unable to set resolution";
        close(m_fd);
        m_fd = -1;
        emit errorOccurred(tr("Failed to set requested resolution"));
        return false;
    }

    // Update device parameters
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;

    // Initialize memory mapping
    if (!initMmap()) {
        qCDebug(app) << "Failed to initialize memory mapping";
        close(m_fd);
        m_fd = -1;
        emit errorOccurred(tr("Memory mapping failed"));
        return false;
    }

    // Restore device state
    m_isInitialized = true;
    m_deviceSelected = wasDeviceSelected;

    // Re-enumerate resolutions and notify UI
    enumerateSupportedResolutions();

    // If preview was active before, restore it
    if (wasPreviewActive) {
        startPreview();
    }

    qCDebug(app) << "Successfully completed resolution setting:" << m_width << "x" << m_height;
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
        qCCritical(app) << "Cannot start capture: device not initialized or invalid file descriptor";
        emit errorOccurred(tr("Device not properly initialized"));
        return false;
    }

    // Ensure stream is stopped before enqueuing buffers
    v4l2_buf_type stopType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(m_fd, VIDIOC_STREAMOFF, &stopType);

    // Wait for device to stabilize
    QThread::msleep(100);

    // Debug buffer status
    qCDebug(app) << "Buffer status:";
    for (int i = 0; i < 4; i++) {
        qCDebug(app) << "  Buffer" << i << ": address=" << m_buffers[i] << "size=" << m_bufferSizes[i];
    }

    // Track number of valid buffers to avoid accessing unallocated ones
    int validBufferCount = 0;
    for (int i = 0; i < 4; i++) {
        if (m_buffers[i] && m_bufferSizes[i] > 0) {
            validBufferCount++;
        } else {
            break;   // Assume buffers are allocated contiguously
        }
    }

    if (validBufferCount == 0) {
        qCWarning(app) << "No valid buffers available - attempting to reinitialize memory mapping";
        if (!initMmap()) {
            emit errorOccurred(tr("Buffer initialization failed"));
            return false;
        }

        // Re-check buffer status
        validBufferCount = 0;
        for (int i = 0; i < 4; i++) {
            if (m_buffers[i] && m_bufferSizes[i] > 0) {
                validBufferCount++;
            } else {
                break;
            }
        }

        if (validBufferCount == 0) {
            qCCritical(app) << "No valid buffers after reinitialization";
            emit errorOccurred(tr("Buffer reinitialization failed"));
            return false;
        }
    }

    qCInfo(app) << "Starting capture with" << validBufferCount << "buffers";

    // Enqueue all buffers
    for (int i = 0; i < validBufferCount; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        int result = ioctl(m_fd, VIDIOC_QBUF, &buf);
        if (result == -1) {
            // Handle special cases in non-blocking mode
            if (errno == EAGAIN) {
                // In non-blocking mode, we may need multiple attempts
                qCDebug(app) << "Buffer not ready yet (EAGAIN), retrying...";
                QThread::msleep(10);
                i--; // Retry current buffer
                continue;
            }
            
            qCWarning(app) << "Failed to enqueue buffer" << i << ":" << strerror(errno) << "(errno:" << errno << ")";

            // Clean up any already enqueued buffers
            stopCapturing();

            // Provide more diagnostic info
            if (errno == EINVAL) {
                qCDebug(app) << "  Reason: Invalid argument - possible buffer index out of range or memory type mismatch";
            } else if (errno == ENOMEM) {
                qCDebug(app) << "  Reason: Out of memory";
            } else if (errno == EIO) {
                qCDebug(app) << "  Reason: I/O error - device may be disconnected";
            }

            emit errorOccurred(tr("Failed to enqueue buffer: %1").arg(strerror(errno)));
            return false;
        }

        qCDebug(app) << "Successfully enqueued buffer" << i;
    }

    // Start video stream
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) == -1) {
        // Handle special cases in non-blocking mode
        if (errno == EAGAIN) {
            // In non-blocking mode, we may need multiple attempts
            for (int retry = 0; retry < 5; retry++) {
                QThread::msleep(50);
                if (ioctl(m_fd, VIDIOC_STREAMON, &type) != -1) {
                    qCInfo(app) << "Video capture stream started successfully after retry";
                    return true;
                }
                if (errno != EAGAIN) {
                    break; // If error is not EAGAIN, stop retrying
                }
            }
        }
        
        qCCritical(app) << "Failed to start video stream:" << strerror(errno);
        emit errorOccurred(tr("Failed to start video stream: %1").arg(strerror(errno)));
        return false;
    }

    qCInfo(app) << "Video capture stream started successfully";
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

    // Clear latest frame
    {
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = QImage();
    }

    qCDebug(app) << "Preview started, camera parameters applied";
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

    // Try to get a frame in non-blocking mode (device is already opened with O_NONBLOCK)
    if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1) {
        // EAGAIN means no frame is available at the moment, which is normal in non-blocking mode
        if (errno == EAGAIN) {
            return;
        }
        
        qCWarning(app) << "Preview update failed:" << strerror(errno);
        return;
    }

    // Ensure buffer index is valid
    if (buf.index >= 4 || !m_buffers[buf.index]) {
        qCWarning(app) << "Invalid buffer index:" << buf.index;
        ioctl(m_fd, VIDIOC_QBUF, &buf);   // Try to requeue buffer
        return;
    }

    QImage image = frameToQImage(m_buffers[buf.index], m_width, m_height, m_pixelFormat);

    // Return buffer - must do this regardless of image conversion success
    if (ioctl(m_fd, VIDIOC_QBUF, &buf) == -1) {
        qCWarning(app) << "Failed to requeue buffer:" << strerror(errno);
        return;
    }

    if (!image.isNull()) {
        // Save adjusted image as latest frame for capture
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = image;

        // emit image
        emit previewUpdated(image);
    }
}

void WebcamDevice::captureImage()
{
    // Check device state
    if (!m_isInitialized || m_fd <= 0) {
        emit errorOccurred(tr("Device not initialized or invalid file descriptor"));
        return;
    }

    emit captureStarted();
    qCInfo(app) << "Starting image capture...";

    // Check if preview is running
    bool previewWasRunning = m_previewTimer.isActive();

    // If preview not running, start a temporary capture process
    if (!previewWasRunning) {
        qCDebug(app) << "Preview not running - setting up temporary capture process";

        // Ensure current state is cleaned up
        stopCapturing();
        QThread::msleep(100);

        // Start capture process
        if (!startCapturing()) {
            emit errorOccurred(tr("Failed to start video stream, capture failed"));
            return;
        }

        // Wait for device to be ready
        QThread::msleep(300);
    } else {
        // If preview is running, pause timer but keep stream active
        m_previewTimer.stop();
    }

    // Try to get frame directly from device, regardless of preview state
    for (int retry = 0; retry < 3; retry++) {
        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        qCDebug(app) << "Attempting to get frame directly from device (attempt" << (retry + 1) << ")";

        // Temporarily switch to blocking mode to ensure frame capture
        int flags = fcntl(m_fd, F_GETFL, 0);
        fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);

        if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1) {
            // Restore non-blocking mode
            fcntl(m_fd, F_SETFL, flags);
            
            qCWarning(app) << "Failed to get frame:" << strerror(errno);
            if (retry == 2) {   // Last attempt failed
                // Try to use existing preview frame (if available)
                QMutexLocker locker(&m_frameMutex);
                if (!m_latestFrame.isNull()) {
                    qCWarning(app) << "Using existing preview frame after capture failure";
                    QImage capturedImage = m_latestFrame.copy();
                    locker.unlock();

                    // Restore preview state
                    if (previewWasRunning) {
                        m_previewTimer.start();
                    } else {
                        stopCapturing();
                    }

                    emit imageCaptured(capturedImage);
                    return;
                }

                // If no preview frame available, report error
                emit errorOccurred(tr("Failed to get image frame"));

                // Restore preview state
                if (previewWasRunning) {
                    m_previewTimer.start();
                } else {
                    stopCapturing();
                }
                return;
            }
            // Brief wait before retry
            QThread::msleep(100);
            continue;
        }

        // Restore non-blocking mode
        fcntl(m_fd, F_SETFL, flags);

        // Ensure buffer index is valid
        if (buf.index >= 4 || !m_buffers[buf.index]) {
            qCWarning(app) << "Invalid buffer index:" << buf.index;
            ioctl(m_fd, VIDIOC_QBUF, &buf);   // Requeue buffer
            continue;
        }

        // Successfully got frame
        QImage image = frameToQImage(m_buffers[buf.index], m_width, m_height, m_pixelFormat);

        // Return buffer
        ioctl(m_fd, VIDIOC_QBUF, &buf);

        if (!image.isNull()) {
            // Save current frame to preview buffer
            {
                QMutexLocker locker(&m_frameMutex);
                m_latestFrame = image;
            }

            // If preview was running before, restore timer
            if (previewWasRunning) {
                m_previewTimer.start();
            } else {
                stopCapturing();
            }

            qCInfo(app) << "Image captured successfully";
            emit imageCaptured(image);
            return;
        } else {
            qCWarning(app) << "Image conversion failed - retrying";
        }

        // Brief wait before retry
        QThread::msleep(100);
    }

    // If all attempts failed
    emit errorOccurred(tr("Failed to capture valid image, please check camera connection"));

    // Restore preview state
    if (previewWasRunning) {
        m_previewTimer.start();
    } else {
        stopCapturing();
    }
}

QImage WebcamDevice::frameToQImage(const void *data, int width, int height, int format)
{
    if (!data) {
        qCWarning(app) << "Frame data is empty";
        return QImage();
    }

    // qCDebug(app) << "Converting frame data:" << width << "x" << height << "format:" << format;

    if (format == V4L2_PIX_FMT_YUYV) {
        QImage image(width, height, QImage::Format_RGB888);
        const uint8_t *yuyv = static_cast<const uint8_t *>(data);

        // Safety check - ensure enough memory
        size_t expectedSize = width * height * 2;   // YUYV格式每个像素需要2字节
        if (expectedSize > 0) {   // 合理的大小上限
            try {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x += 2) {
                        int i = y * width * 2 + x * 2;

                        // Boundary check
                        if (i + 3 >= expectedSize) {
                            qCWarning(app) << "Frame data boundary check failed - possible incomplete data";
                            continue;
                        }

                        int y0 = yuyv[i];
                        int u = yuyv[i + 1];
                        int y1 = yuyv[i + 2];
                        int v = yuyv[i + 3];

                        // Keep original brightness value
                        y0 = y0;
                        y1 = y1;

                        int r, g, b;

                        // First pixel - use more accurate YUV to RGB conversion
                        r = static_cast<int>(y0 + 1.370705 * (v - 128));
                        g = static_cast<int>(y0 - 0.337633 * (u - 128) - 0.698001 * (v - 128));
                        b = static_cast<int>(y0 + 1.732446 * (u - 128));

                        r = qBound(0, r, 255);
                        g = qBound(0, g, 255);
                        b = qBound(0, b, 255);

                        if (x < width) {   // Safety check
                            image.setPixel(x, y, qRgb(r, g, b));
                        }

                        // Second pixel - use same conversion formula
                        r = static_cast<int>(y1 + 1.370705 * (v - 128));
                        g = static_cast<int>(y1 - 0.337633 * (u - 128) - 0.698001 * (v - 128));
                        b = static_cast<int>(y1 + 1.732446 * (u - 128));

                        r = qBound(0, r, 255);
                        g = qBound(0, g, 255);
                        b = qBound(0, b, 255);

                        if (x + 1 < width) {   // Safety check
                            image.setPixel(x + 1, y, qRgb(r, g, b));
                        }
                    }
                }
                return image;
            } catch (const std::exception &e) {
            qCCritical(app) << "Frame conversion exception:" << e.what();
            } catch (...) {
            qCCritical(app) << "Unknown exception during frame conversion";
            }
        } else {
            qCWarning(app) << "Invalid frame size:" << expectedSize;
        }
    } else if (format == V4L2_PIX_FMT_MJPEG) {
        // Get actual data size instead of calculated value
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = m_currentBuffer;

        size_t actualSize = 0;
        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) != -1) {
            actualSize = buf.bytesused;
            // qCDebug(app) << "MJPEG actual data size:" << actualSize;
        } else {
            actualSize = width * height;
            qCDebug(app) << "Using estimated size:" << actualSize;
        }

        QImage image;
        if (image.loadFromData(static_cast<const uchar *>(data), actualSize, "JPEG")) {
            return image;
        }
    } else {
        qCDebug(app) << "Unsupported pixel format:" << format;
    }

    return QImage();
}

void WebcamDevice::closeDevice()
{
    // First stop preview and stream
    m_previewTimer.stop();
    stopCapturing();

    // Clear latest preview frame
    {
        QMutexLocker locker(&m_frameMutex);
        m_latestFrame = QImage();
    }

    // Release memory mapping
    uninitMmap();

    // Close file descriptor
    if (m_fd > 0) {
        close(m_fd);
        qCDebug(app) << "Closed device file descriptor:" << m_fd;
        m_fd = -1;
    }

    m_deviceSelected = false;
    m_isInitialized = false;
    setState(Disconnected);
}

// New method: Get maximum resolution supported by camera
QSize WebcamDevice::getMaxResolution()
{
    if (m_fd <= 0) {
        qCWarning(app) << "Invalid file descriptor - cannot get max resolution";
        return QSize(640, 480);   // Return default value
    }

    // Store maximum resolution
    int maxWidth = 0;
    int maxHeight = 0;
    bool resolutionsFound = false;

    // First try to get all discrete resolutions supported by device
    QList<QSize> supportedSizes;
    v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = m_pixelFormat;

    qCDebug(app) << "Querying supported resolutions from device";

    for (frmsize.index = 0; ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
        resolutionsFound = true;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            supportedSizes.append(QSize(frmsize.discrete.width, frmsize.discrete.height));
            qCDebug(app) << "  Supported resolution:" << frmsize.discrete.width << "x" << frmsize.discrete.height;
        } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE || frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
            // For variable resolutions, choose a safe larger value
            qCDebug(app) << "  Supported range:" << frmsize.stepwise.min_width << "x" << frmsize.stepwise.min_height
                     << " to " << frmsize.stepwise.max_width << "x" << frmsize.stepwise.max_height;

            // For variable resolutions, limit max to 1280x720 unless device definitely supports higher
            int maxW = frmsize.stepwise.max_width;
            int maxH = frmsize.stepwise.max_height;

            // If resolution is too high, may cause issues - limit to common HD resolutions
            if (maxW > 1920 || maxH > 1080) {
                maxW = 1280;   // Default limit to 720p
                maxH = 720;
            }

            maxWidth = maxW;
            maxHeight = maxH;

            qCInfo(app) << "Selected resolution:" << maxWidth << "x" << maxHeight;
            return QSize(maxWidth, maxHeight);
        }
    }

    // If discrete resolutions found, get largest one (but no more than 1280x720 unless explicitly supported)
    if (resolutionsFound && !supportedSizes.isEmpty()) {
        // Sort by resolution (total pixels)
        std::sort(supportedSizes.begin(), supportedSizes.end(),
                  [](const QSize &a, const QSize &b) {
                      return a.width() * a.height() > b.width() * b.height();
                  });

        // Select largest resolution not exceeding 720p (unless explicitly supports higher)
        for (const QSize &size : supportedSizes) {
            // If we find a reasonable resolution, use it
            maxWidth = size.width();
            maxHeight = size.height();

            // If at least 720p resolution found, use it directly
            if (maxWidth >= 1280 && maxHeight >= 720) {
                // Prefer standard resolutions like 720p or 1080p
                if (maxWidth == 1280 && maxHeight == 720) {
                    qCInfo(app) << "Selected standard resolution: 1280x720";
                    return QSize(maxWidth, maxHeight);
                }

                if (maxWidth == 1920 && maxHeight == 1080) {
                    qCInfo(app) << "Selected standard resolution: 1920x1080";
                    return QSize(maxWidth, maxHeight);
                }

                // If no exact match, use the first (largest) resolution
                qCInfo(app) << "Selected maximum resolution:" << supportedSizes.first().width() << "x" << supportedSizes.first().height();
                return supportedSizes.first();
            }

            // If no sufficiently large resolution found, use largest in list
            break;
        }
    }

    // If previous methods failed, try to get current format
    if (maxWidth == 0 || maxHeight == 0) {
        v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == 0) {
            maxWidth = fmt.fmt.pix.width;
            maxHeight = fmt.fmt.pix.height;
            qCDebug(app) << "Using current resolution:" << maxWidth << "x" << maxHeight;
        }
    }

    // If still can't get, return safe default value
    if (maxWidth <= 0 || maxHeight <= 0) {
        maxWidth = 640;
        maxHeight = 480;
        qCDebug(app) << "Using default resolution:" << maxWidth << "x" << maxHeight;
    }

    return QSize(maxWidth, maxHeight);
}

// New method: Try to select best pixel format
bool WebcamDevice::selectBestPixelFormat()
{
    if (m_fd == -1) {
        return false;
    }

    // Priority order: MJPEG > YUYV > others
    const uint32_t preferredFormats[] = {
        V4L2_PIX_FMT_MJPEG,   // MJPEG has better performance
        V4L2_PIX_FMT_YUYV,   // Common format
        V4L2_PIX_FMT_YUV420,   // Another common format
        V4L2_PIX_FMT_RGB24   // RGB format
    };

    // Get current format as fallback
    v4l2_format currentFmt = {};
    currentFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &currentFmt) != -1) {
        qCInfo(app) << "Current pixel format:" << QString::number(currentFmt.fmt.pix.pixelformat, 16);
    }

    // Iterate through available pixel formats
    v4l2_fmtdesc fmtdesc = {};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    QVector<uint32_t> supportedFormats;

    for (fmtdesc.index = 0; ioctl(m_fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1; fmtdesc.index++) {
        qCDebug(app) << "Supported pixel format:" << QString::number(fmtdesc.pixelformat, 16)
                 << "description:" << reinterpret_cast<const char *>(fmtdesc.description);
        supportedFormats.append(fmtdesc.pixelformat);
    }

    // Try to set in priority order
    for (uint32_t format : preferredFormats) {
        if (supportedFormats.contains(format)) {
            v4l2_format fmt = {};
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            // First get current format
            if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == -1) {
                continue;
            }

            // Only modify pixel format, keep other parameters
            fmt.fmt.pix.pixelformat = format;

            if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) != -1) {
                if (fmt.fmt.pix.pixelformat == format) {
                    m_pixelFormat = format;
                    qCInfo(app) << "Successfully set pixel format:" << QString::number(format, 16);
                    return true;
                }
            }
        }
    }

    // If all attempts fail, at least ensure we know current pixel format
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) != -1) {
        m_pixelFormat = fmt.fmt.pix.pixelformat;
        qCInfo(app) << "Using current pixel format:" << QString::number(m_pixelFormat, 16);
        return true;
    }

    return false;
}

// New function: Diagnose camera device issues
void WebcamDevice::diagnoseCameraIssues()
{
    if (m_fd == -1) {
        qCWarning(app) << "Cannot diagnose: device not open";
        return;
    }

    qCInfo(app) << "Starting camera diagnostics...";

    // Check device permissions
    struct stat st;
    if (fstat(m_fd, &st) == 0) {
        qCDebug(app) << "Device permissions:"
                 << (st.st_mode & S_IRUSR ? "r" : "-")
                 << (st.st_mode & S_IWUSR ? "w" : "-")
                 << (st.st_mode & S_IXUSR ? "x" : "-")
                 << " Owner:" << st.st_uid;

        if (!(st.st_mode & S_IRUSR) || !(st.st_mode & S_IWUSR)) {
            qCWarning(app) << "Warning: insufficient device permissions (rw)";
        }
    }

    // Check memory mapping support
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) != -1) {
        qCDebug(app) << "Device capabilities:";
        qCDebug(app) << "  Video capture: " << ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? "Yes" : "No");
        qCDebug(app) << "  Video output: " << ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) ? "Yes" : "No");
        qCDebug(app) << "  Streaming: " << ((cap.capabilities & V4L2_CAP_STREAMING) ? "Yes" : "No");
        qCDebug(app) << "  Read/write: " << ((cap.capabilities & V4L2_CAP_READWRITE) ? "Yes" : "No");

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            qCWarning(app) << "Error: Device does not support streaming - memory mapping unavailable";
        }
    }

    // Check current format
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) != -1) {
        qCDebug(app) << "Current format:";
        qCDebug(app) << "  Width: " << fmt.fmt.pix.width;
        qCDebug(app) << "  Height: " << fmt.fmt.pix.height;
        qCDebug(app) << "  Pixel format: " << QString::number(fmt.fmt.pix.pixelformat, 16);
        qCDebug(app) << "  Field type: " << fmt.fmt.pix.field;
        qCDebug(app) << "  Bytes per line: " << fmt.fmt.pix.bytesperline;
        qCDebug(app) << "  Image size: " << fmt.fmt.pix.sizeimage << " bytes";

        if (fmt.fmt.pix.bytesperline == 0 || fmt.fmt.pix.sizeimage == 0) {
            qCWarning(app) << "Warning: Invalid image size parameters";
        }
    }

    // Check system memory
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        unsigned long freeRam = sys_info.freeram * sys_info.mem_unit / (1024 * 1024);
        qCDebug(app) << "System free memory: " << freeRam << " MB";

        if (freeRam < 50) {
            qCWarning(app) << "Warning: Low system memory - buffer allocation may fail";
        }
    }

    // Try using just one buffer
    v4l2_requestbuffers req = {};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    qCDebug(app) << "Attempting to request 1 buffer...";
    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        qCDebug(app) << "Failed to request buffer: " << strerror(errno);

        // 检查特定的错误代码
        if (errno == EINVAL) {
            qCDebug(app) << "Error: Device does not support memory mapping";
        } else if (errno == EBUSY) {
            qCDebug(app) << "Error: Device busy or already opened by another process";
        }
    } else {
        qCDebug(app) << "Successfully requested " << req.count << " buffers";

        // Release just allocated buffers
        req.count = 0;
        ioctl(m_fd, VIDIOC_REQBUFS, &req);
    }

    qCDebug(app) << "Camera diagnostics completed";
}

QList<QSize> WebcamDevice::getSupportedResolutions()
{
    if (m_supportedResolutions.isEmpty() && m_fd > 0) {
        enumerateSupportedResolutions();
    }

    // Add debug info
    qCDebug(app) << "Number of supported resolutions:" << m_supportedResolutions.size();
    for (const QSize &res : m_supportedResolutions) {
        qCDebug(app) << "  Supported resolution:" << res.width() << "x" << res.height();
    }

    return m_supportedResolutions;
}

void WebcamDevice::enumerateSupportedResolutions()
{
    // Clear resolution list to ensure fresh retrieval each time
    m_supportedResolutions.clear();

    if (m_fd <= 0) {
        qCDebug(app) << "Device not open, cannot get supported resolutions";
        return;
    }

    // Check if VIDIOC_ENUM_FRAMESIZES is supported
    v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.pixel_format = m_pixelFormat;

    qCDebug(app) << "Enumerating device supported resolutions:";

    // Flag whether any resolutions found
    bool foundAnyResolution = false;

    for (frmsize.index = 0; ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
        foundAnyResolution = true;
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            QSize resolution(frmsize.discrete.width, frmsize.discrete.height);
            // Check for duplicates to avoid adding same resolution
            if (!m_supportedResolutions.contains(resolution)) {
                m_supportedResolutions.append(resolution);
                qCDebug(app) << "  Found discrete resolution:" << resolution.width() << "x" << resolution.height();
            }
        } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE || frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
            // For variable range resolutions, add some standard sizes
            qCDebug(app) << "  Supported range:" << frmsize.stepwise.min_width << "x" << frmsize.stepwise.min_height
                     << " to " << frmsize.stepwise.max_width << "x" << frmsize.stepwise.max_height;

            // Generate some standard resolutions within supported range
            QList<QSize> standardResolutions;
            standardResolutions << QSize(640, 480) << QSize(800, 600)
                                << QSize(1024, 768) << QSize(1280, 720)
                                << QSize(1920, 1080) << QSize(2560, 1440)
                                << QSize(3840, 2160);

            for (const QSize &res : standardResolutions) {
                if (res.width() >= frmsize.stepwise.min_width && res.width() <= frmsize.stepwise.max_width && res.height() >= frmsize.stepwise.min_height && res.height() <= frmsize.stepwise.max_height) {
                    // Check for duplicates to avoid adding same resolution
                    if (!m_supportedResolutions.contains(res)) {
                        m_supportedResolutions.append(res);
                        qCDebug(app) << "  Added standard resolution:" << res.width() << "x" << res.height();
                    }
                }
            }

            // Exit loop after finding step range
            break;
        }
    }

    // If can't enumerate resolutions, at least add some common ones
    if (!foundAnyResolution || m_supportedResolutions.isEmpty()) {
        qCDebug(app) << "Unable to enumerate resolutions or none found, adding common resolutions";
        // Ensure no duplicate resolutions added
        if (!m_supportedResolutions.contains(QSize(640, 480)))
            m_supportedResolutions << QSize(640, 480);
        if (!m_supportedResolutions.contains(QSize(800, 600)))
            m_supportedResolutions << QSize(800, 600);
        if (!m_supportedResolutions.contains(QSize(1280, 720)))
            m_supportedResolutions << QSize(1280, 720);
        if (!m_supportedResolutions.contains(QSize(1920, 1080)))
            m_supportedResolutions << QSize(1920, 1080);
    }

    // Sort resolutions from small to large
    std::sort(m_supportedResolutions.begin(), m_supportedResolutions.end(),
              [](const QSize &a, const QSize &b) {
                  return a.width() * a.height() < b.width() * b.height();
              });

    qCDebug(app) << "Total enumerated resolutions:" << m_supportedResolutions.size();

    // Emit signal that resolution list was updated
    emit resolutionsChanged(m_supportedResolutions);
}

bool WebcamDevice::setCameraControl(uint32_t controlId, int value)
{
    if (m_fd <= 0) {
        qCDebug(app) << "Invalid file descriptor, cannot set control parameters";
        return false;
    }

    // First query if control exists and its range
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = controlId;

    if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        if (errno == EINVAL) {
            qCDebug(app) << "Camera does not support this control:" << controlId;
        } else {
            qCDebug(app) << "Failed to query control:" << strerror(errno);
        }
        return false;
    }
    auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
    qCDebug(app) << "Control:" << controlName << " Range:" << queryctrl.minimum << "-" << queryctrl.maximum << " Default:" << queryctrl.default_value;

    if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        qCDebug(app) << "Control disabled:" << controlName;
        return false;
    }

    if (V4L2_CID_BRIGHTNESS == controlId) {
        int mid = 0;
        if (queryctrl.minimum < 0) {
            // Brightness range is negative
            mid = (queryctrl.maximum + queryctrl.minimum) / 2;
            if (mid <= 0) {
                mid = 1;
            }
        } else {
            // Brightness range is positive
            mid = (queryctrl.maximum - queryctrl.minimum) / 2;
        }
        // set percentage value of middle for brightness
        value = mid * value / 100;
    } else {
        // Ensure value is within valid range
        if (value <= queryctrl.minimum) value = queryctrl.minimum + 1;
        if (value >= queryctrl.maximum) value = queryctrl.maximum - 1;
    }
    qCDebug(app) << "Setting control:" << controlName << "to" << value;

    // Set control value
    struct v4l2_control control;
    control.id = controlId;
    control.value = value;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
        qCDebug(app) << "Failed to set control:" << controlName << "Value:" << value << "Error:" << strerror(errno);
        return false;
    }

    qCDebug(app) << "Successfully set control:" << controlName << "Value:" << value;
    return true;
}

void WebcamDevice::listCameraControls()
{
    if (m_fd <= 0) {
        qCDebug(app) << "Invalid file descriptor, cannot list control parameters";
        return;
    }

    qCDebug(app) << "Camera supported control parameters:";

    // Standard controls
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
                    qCDebug(app) << "  Control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Current value:" << control.value
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value;
                } else {
                    qCDebug(app) << "  Control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value
                             << "(Failed to get current value)";
                }
            }
        }
    }

    // Camera class controls
    for (uint32_t id = V4L2_CID_CAMERA_CLASS_BASE; id < V4L2_CID_CAMERA_CLASS_BASE + 100; id++) {
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = id;

        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
                auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
                struct v4l2_control control;
                control.id = id;
                if (ioctl(m_fd, VIDIOC_G_CTRL, &control) == 0) {
                    qCDebug(app) << "  Camera control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Current value:" << control.value
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value;
                } else {
                    qCDebug(app) << "  Camera control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value
                             << "(Failed to get current value)";
                }
            }
        }
    }

    // Private controls
    for (uint32_t id = V4L2_CID_PRIVATE_BASE; id < V4L2_CID_PRIVATE_BASE + 100; id++) {
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = id;

        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
                auto controlName = QString::fromUtf8(reinterpret_cast<const char*>(queryctrl.name));
                struct v4l2_control control;
                control.id = id;
                if (ioctl(m_fd, VIDIOC_G_CTRL, &control) == 0) {
                    qCDebug(app) << "  Private control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Current value:" << control.value
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value;
                } else {
                    qCDebug(app) << "  Private control:" << controlName
                             << "ID:" << queryctrl.id
                             << "Range:" << queryctrl.minimum << "-" << queryctrl.maximum
                             << "Default value:" << queryctrl.default_value
                             << "(Failed to get current value)";
                }
            }
        }
    }
}

bool WebcamDevice::adjustCommonCameraSettings()
{
    if (m_fd <= 0) {
        qCDebug(app) << "[Error] Invalid file descriptor";
        return false;
    }

    // Debug info: List camera supported controls
    qCDebug(app) << "=== Starting camera optimization ===";
    listCameraControls();

    // Parameter configuration strategy (with smart adaptation)
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

    // Phase 1: Parameter feasibility check
    QMap<uint32_t, v4l2_queryctrl> validControls;
    for (const auto& cfg : configTable) {
        v4l2_queryctrl query = {};
        query.id = cfg.id;
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &query) == 0) {
            if (!(query.flags & V4L2_CTRL_FLAG_DISABLED)) {
                validControls.insert(cfg.id, query);
                qCDebug(app) << "[Precheck] Supported:" << cfg.name 
                         << "Range:" << query.minimum << "-" << query.maximum;
            }
        }
    }

    // Phase 2: Smart parameter setting
    for (const auto& cfg : configTable) {
        if (!validControls.contains(cfg.id)) {
            if (cfg.required) {
                qCDebug(app) << "[Critical] Missing required control:" << cfg.name;
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

        // Set parameters (with retry mechanism)
        v4l2_control ctrl = {};
        ctrl.id = cfg.id;
        ctrl.value = finalValue;
        
        for (int retry = 0; retry < 3; ++retry) {
            if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl) == 0) {
                qCDebug(app) << "[Success] Set" << cfg.name << "=" << finalValue 
                         << "(Default:" << query.default_value << ")";
                successFlag = true;
                break;
            }
            
            if (retry == 2) {
                qCDebug(app) << "[Error] Failed to set" << cfg.name 
                         << "Error:" << strerror(errno);
                // Restore auto mode when critical parameters fail
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

    qCDebug(app) << "Optimization completed in" << timer.elapsed() << "ms";
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
    // 1. First check if device supports auto focus control
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CID_FOCUS_AUTO;
    
    if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) == -1) {
        qCDebug(app) << "Camera does not support auto focus control";
        return false;
    }

    // 2. Set auto focus mode (auto or manual)
    struct v4l2_control control;
    control.id = V4L2_CID_FOCUS_AUTO;
    control.value = enable ? 1 : 0;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
        qCDebug(app) << "Failed to set auto focus mode:" << strerror(errno);
        return false;
    }

    // 3. If auto focus enabled, start focus process
    if (enable) {
        // Check if starting auto focus is supported
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = V4L2_CID_AUTO_FOCUS_START;
        
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) != -1) {
            // Start auto focus
            control.id = V4L2_CID_AUTO_FOCUS_START;
            control.value = 1;
            
            if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
                qCDebug(app) << "Failed to start auto focus:" << strerror(errno);
                // Do not return failure here, as the device may support auto mode but not starting focus
            } else {
                qCDebug(app) << "Auto focus has started";
            }
        } else {
            qCDebug(app) << "Camera does not support starting auto focus";
        }
    } else {
        // If disabling auto focus, try to stop any ongoing focus
        memset(&queryctrl, 0, sizeof(queryctrl));
        queryctrl.id = V4L2_CID_AUTO_FOCUS_STOP;
        
        if (ioctl(m_fd, VIDIOC_QUERYCTRL, &queryctrl) != -1) {
            control.id = V4L2_CID_AUTO_FOCUS_STOP;
            control.value = 1;
            
            if (ioctl(m_fd, VIDIOC_S_CTRL, &control) == -1) {
                qCDebug(app) << "Failed to stop auto focus:" << strerror(errno);
            } else {
                qCDebug(app) << "Auto focus has stopped";
            }
        }
    }

    return true;
}

bool WebcamDevice::initMmap()
{
    // First check if device is open
    if (m_fd == -1) {
        qCDebug(app) << "Device is not open, unable to initialize memory mapping";
        return false;
    }

    // Ensure previous mapping is cleaned up
    uninitMmap();

    // Stop any possibly running stream
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(m_fd, VIDIOC_STREAMOFF, &type);

    // Release any existing buffers
    v4l2_requestbuffers release_req = {};
    release_req.count = 0;
    release_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    release_req.memory = V4L2_MEMORY_MMAP;
    int release_result = ioctl(m_fd, VIDIOC_REQBUFS, &release_req);
    if (release_result == -1) {
        qCDebug(app) << "Failed to release existing buffers:" << strerror(errno);
    }

    // Give device some recovery time
    QThread::msleep(200);

    // Initialize buffer array
    for (int i = 0; i < 4; i++) {
        m_buffers[i] = nullptr;
        m_bufferSizes[i] = 0;
    }

    // Request buffers - use just 1 to reduce complexity
    v4l2_requestbuffers req = {};
    req.count = 1;   // Use 1 buffer to simplify operations
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1) {
        qCDebug(app) << "Failed to request buffers:" << strerror(errno);
        return false;
    }

    if (req.count < 1) {
        qCDebug(app) << "Insufficient number of buffers returned by the device:" << req.count;
        return false;
    }

    qCDebug(app) << "Successfully requested " << req.count << " buffers";

    // Record actual number of allocated buffers
    int bufferCount = req.count;
    if (bufferCount > 4) bufferCount = 4;

    // Map buffers
    bool success = true;
    for (int i = 0; i < bufferCount; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            qCDebug(app) << "Failed to query buffer " << i << ":" << strerror(errno);
            success = false;
            break;
        }

        qCDebug(app) << "Buffer " << i << " information: offset=" << buf.m.offset
                 << " length=" << buf.length;

        void *ptr = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                         MAP_SHARED, m_fd, buf.m.offset);

        if (ptr == MAP_FAILED) {
            qCDebug(app) << "Failed to map buffer " << i << ":" << strerror(errno);
            success = false;
            break;
        }

        m_buffers[i] = ptr;
        m_bufferSizes[i] = buf.length;

        qCDebug(app) << "Successfully mapped buffer " << i << " size:" << buf.length << " bytes, address:" << ptr;
    }

    // If any mapping fails, clean up all mapped buffers
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
