// Copyright (C) 2025, CodingEagle2025
// SPDX-FileCopyrightText: 2025 
//
// SPDX-License-Identifier: MIT

#ifndef OFD_WRITER_H
#define OFD_WRITER_H

#include <QString>
#include <QVector>
#include <QImage>
#include <QFileInfo>
#include <QTextStream>

// 前向声明
class QuaZip;

namespace ofd {

class Writer
{
public:
    Writer();
    ~Writer();

    // Create OFD file from images
    bool createFromImages(const QString &outputPath,
                          const QVector<QImage> &images,
                          float leftMargin = 0.0f,
                          float topMargin = 0.0f,
                          float pageWidth = 210.0f,
                          float pageHeight = 297.0f);

private:
    void createOFDXmlFile(const QString &path);
    void createDocFile(const QString &path, float leftM, float topM,
                       const QVector<QImage> &images,
                       float pageWidth = 210.0f, float pageHeight = 297.0f);
    void createResourceFiles(const QString &path, const QVector<QImage> &images);
    
    // XML写入辅助函数
    void writeDocumentHeader(QTextStream &stream, int maxId, 
                            float pageWidth = 210.0f, float pageHeight = 297.0f);
    void writeDocumentFooter(QTextStream &stream);
    void writePublicResHeader(QTextStream &stream);
    void writePublicResFooter(QTextStream &stream, bool hasImages);
    void writePage(QTextStream &docStream, QTextStream &publicResStream,
                  const QString &pagesPath, const QVector<QImage> &images,
                  int pageIndex, float leftM, float topM, int &currentId,
                  float pageWidth = 210.0f, float pageHeight = 297.0f);
    void createPageContent(const QString &path, const QString &minX, 
                         const QString &minY, const QString &maxX,
                         const QString &maxY, int &id, bool hasImage,
                         float pageWidth = 210.0f, float pageHeight = 297.0f);

    // 压缩相关函数
    bool compressOFD(const QString &outputPath, const QString &sourcePath);
    void addToFileList(const QDir &dir, const QString &relativePath,
                      QFileInfoList &files);
    bool addFileToZip(QuaZip *zip, const QString &sourcePath,
                     const QFileInfo &fileInfo);
    bool removeDir(const QString &dirPath);

    // 图像处理辅助函数
    QSize calculateImageSize(const QImage &image, float leftM, float topM,
                             float pageWidth = 210.0f, float pageHeight = 297.0f);
};

}   // namespace ofd

#endif   // OFD_WRITER_H
