// Copyright (C) 2025, CodingEagle2025
// SPDX-FileCopyrightText: 2025 
//
// SPDX-License-Identifier: MIT

#include "ofd/ofd_reader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <quazip.h>
#include <quazipfile.h>
#include <QDomDocument>

namespace ofd {

Reader::Reader() {}
Reader::~Reader() {}

bool Reader::readFile(const QString &path, QVector<QImage> &images)
{
    // 创建临时目录
    QFileInfo fileInfo(path);
    QString tempPath = QDir::tempPath() + QDir::separator() + fileInfo.baseName();
    QDir tempDir(tempPath);
    if (!tempDir.exists()) {
        tempDir.mkpath(".");
    }

    // 解压OFD文件
    if (!uncompressFile(path, tempPath)) {
        removeDir(tempPath);
        return false;
    }

    // 提取图像
    bool success = extractImages(tempPath, images);

    // 清理临时文件
    removeDir(tempPath);

    return success;
}

bool Reader::uncompressFile(const QString &path, const QString &extractPath)
{
    QuaZip zip(path);
    if (!zip.open(QuaZip::mdUnzip)) {
        return false;
    }

    // 规范化解压根目录，用于 Zip Slip（路径穿越）防护
    const QString cleanRoot = QDir::cleanPath(extractPath);

    for (bool f = zip.goToFirstFile(); f; f = zip.goToNextFile()) {
        QuaZipFile zipFile(&zip);
        if (!zipFile.open(QIODevice::ReadOnly)) {
            zip.close();
            return false;
        }

        // 拼接并规范化目标路径，解析条目名中的 "."/".."，防止逃逸出解压根目录
        QString filePath = QDir::cleanPath(cleanRoot + QLatin1Char('/') + zip.getCurrentFileName());

        // 校验目标路径必须位于解压根目录之内，否则跳过该条目（不创建目录、不写入）
        if (!filePath.startsWith(cleanRoot + QLatin1Char('/'), Qt::CaseSensitive)) {
            zipFile.close();
            continue;
        }

        QFileInfo fileInfo(filePath);
        QDir().mkpath(fileInfo.path());

        QFile outFile(filePath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            zipFile.close();
            zip.close();
            return false;
        }

        outFile.write(zipFile.readAll());
        outFile.close();
        zipFile.close();
    }

    zip.close();
    return true;
}

bool Reader::extractImages(const QString &path, QVector<QImage> &images)
{
    // 读取Document.xml获取页面信息
    QString docPath = path + "/Doc_0/Document.xml";
    QFile docFile(docPath);
    if (!docFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&docFile)) {
        docFile.close();
        return false;
    }
    docFile.close();

    // 查找所有图像资源
    QString resPath = path + "/Doc_0/Res";
    QDir resDir(resPath);
    QStringList filters;
    filters << "*.png"
            << "*.jpg"
            << "*.jpeg";
    QFileInfoList imageFiles = resDir.entryInfoList(filters, QDir::Files);

    // 加载图像
    for (const QFileInfo &imageFile : imageFiles) {
        QImage image(imageFile.filePath());
        if (!image.isNull()) {
            images.append(image);
        }
    }

    return !images.isEmpty();
}

bool Reader::removeDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (dir.exists()) {
        return dir.removeRecursively();
    }
    return true;
}

}   // namespace ofd
