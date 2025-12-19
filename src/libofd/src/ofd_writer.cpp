// Copyright (C) 2025, CodingEagle2025
// SPDX-FileCopyrightText: 2025 
//
// SPDX-License-Identifier: MIT

#include "ofd/ofd_writer.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <quazip.h>
#include <quazipfile.h>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QStringConverter>
#endif

namespace ofd {

namespace {
    // 常量定义
    const QString OFD_XML = "OFD.xml";
    const QString DOC_DIR = "Doc_0";
    const QString DOCUMENT_XML = "Document.xml";
    const QString PUBLIC_RES_XML = "PublicRes.xml";
    const QString PAGES_DIR = "Pages";
    const QString RES_DIR = "Res";
    const QString IMAGE_FORMAT = "PNG";
    
    // 页面尺寸常量
    const double PAGE_WIDTH = 210.00;
    const double PAGE_HEIGHT = 297.00;
    const double DPI_FACTOR = 96.0 / 25.4;
}

Writer::Writer() {}
Writer::~Writer() {}

bool Writer::createFromImages(const QString &outputPath,
                            const QVector<QImage> &images,
                            float leftMargin,
                            float topMargin,
                            float pageWidth,
                            float pageHeight)
{
    QFileInfo fileInfo(outputPath);
    QString tempPath = QDir::tempPath() + QDir::separator() + fileInfo.baseName();
    QDir tempDir(tempPath);
    if (!tempDir.exists()) {
        tempDir.mkpath(".");
    }

    QString basePath = tempPath + QDir::separator();
    createOFDXmlFile(basePath);
    createDocFile(basePath, leftMargin, topMargin, images, pageWidth, pageHeight);

    bool success = compressOFD(outputPath, tempPath);
    removeDir(tempPath);

    return success;
}

void Writer::createOFDXmlFile(const QString &path)
{
    QFile file(path + OFD_XML);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    stream.setCodec("UTF-8");
#else
    stream.setEncoding(QStringConverter::Utf8);
#endif
    
    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<ofd:OFD xmlns:ofd=\"http://www.ofdspec.org\" DocType=\"OFD\" Version=\"1.0\">\n"
           << "  <ofd:DocBody>\n"
           << "    <ofd:DocInfo>\n"
           << "      <ofd:Creator>WPS Office</ofd:Creator>\n"
           << "      <ofd:CreationDate>" << currentDate << "</ofd:CreationDate>\n"
           << "      <ofd:ModDate>" << currentDate << "</ofd:ModDate>\n"
           << "      <ofd:DocID>05dc55804b3311e680002f0d00002f0d</ofd:DocID>\n"
           << "      <ofd:Author>Administrator</ofd:Author>\n"
           << "    </ofd:DocInfo>\n"
           << "    <ofd:DocRoot>Doc_0/Document.xml</ofd:DocRoot>\n"
           << "  </ofd:DocBody>\n"
           << "</ofd:OFD>\n";
    
    file.close();
}

void Writer::writeDocumentHeader(QTextStream &stream, int maxId, float pageWidth, float pageHeight)
{
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<ofd:Document xmlns:ofd=\"http://www.ofdspec.org\">\n"
           << "  <ofd:CommonData>\n"
           << "    <ofd:MaxUnitID>" << maxId << "</ofd:MaxUnitID>\n"
           << "    <ofd:PageArea>\n"
           << "      <ofd:PhysicalBox>0.00 0.00 " << pageWidth << " " << pageHeight << "</ofd:PhysicalBox>\n"
           << "    </ofd:PageArea>\n"
           << "    <ofd:PublicRes>PublicRes.xml</ofd:PublicRes>\n"
           << "  </ofd:CommonData>\n"
           << "  <ofd:Pages>";
}

void Writer::writeDocumentFooter(QTextStream &stream)
{
    stream << "</ofd:Pages>\n"
           << "</ofd:Document>\n";
}

void Writer::writePublicResHeader(QTextStream &stream)
{
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<ofd:Res xmlns:ofd=\"http://www.ofdspec.org\" BaseLoc=\"Res\">\n"
           << "  <ofd:ColorSpaces>\n"
           << "    <ofd:ColorSpace ID=\"4\" Type=\"RGB\"/>\n"
           << "  </ofd:ColorSpaces>\n";
}

void Writer::writePublicResFooter(QTextStream &stream, bool hasImages)
{
    if (hasImages) {
        stream << "  </ofd:MultiMedias>\n";
    }
    stream << "</ofd:Res>\n";
}

QSize Writer::calculateImageSize(const QImage &image, float leftM, float topM,
                                  float pageWidth, float pageHeight)
{
    double width = (double)image.width() * 1000 / image.dotsPerMeterX();
    double height = (double)image.height() * 1000 / image.dotsPerMeterY();

    double scaledWidth = width * DPI_FACTOR;
    double scaledHeight = height * DPI_FACTOR;

    QRect pageRect(leftM * 25.4, topM * 25.4, pageWidth * 25.4, pageHeight * 25.4);
    QSize size(scaledWidth, scaledHeight);
    size.scale(pageRect.size(), Qt::KeepAspectRatio);

    return size;
}

void Writer::createDocFile(const QString &path,
                         float leftM,
                         float topM,
                         const QVector<QImage> &images,
                         float pageWidth,
                         float pageHeight)
{
    QString docPath = path + DOC_DIR;
    QDir(docPath).mkpath(".");
    docPath += QDir::separator();

    createResourceFiles(docPath, images);

    QFile docXml(docPath + DOCUMENT_XML);
    QFile publicResXml(docPath + PUBLIC_RES_XML);
    
    if (!docXml.open(QIODevice::WriteOnly) || !publicResXml.open(QIODevice::WriteOnly)) {
        return;
    }

    QTextStream docStream(&docXml);
    QTextStream publicResStream(&publicResXml);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    docStream.setCodec("UTF-8");
    publicResStream.setCodec("UTF-8");
#else
    docStream.setEncoding(QStringConverter::Utf8);
    publicResStream.setEncoding(QStringConverter::Utf8);
#endif

    int maxId = images.isEmpty() ? 5 : 2 + images.count() * 5;
    int currentId = 1;

    writeDocumentHeader(docStream, maxId, pageWidth, pageHeight);
    writePublicResHeader(publicResStream);

    if (!images.isEmpty()) {
        publicResStream << "  <ofd:MultiMedias>\n";
    }

    QString pagesPath = docPath + PAGES_DIR;
    QDir(pagesPath).mkpath(".");
    pagesPath += QDir::separator();

    for (int i = 0; i < (images.isEmpty() ? 1 : images.size()); ++i) {
        writePage(docStream, publicResStream, pagesPath, images, i, leftM, topM, currentId, pageWidth, pageHeight);
    }

    writeDocumentFooter(docStream);
    writePublicResFooter(publicResStream, !images.isEmpty());

    docXml.close();
    publicResXml.close();
}

void Writer::writePage(QTextStream &docStream, QTextStream &publicResStream,
                      const QString &pagesPath, const QVector<QImage> &images,
                      int pageIndex, float leftM, float topM, int &currentId,
                      float pageWidth, float pageHeight)
{
    docStream << QString("<ofd:Page ID=\"%1\" BaseLoc=\"Pages/Page_%2/Content.xml\"/>")
                       .arg(currentId).arg(pageIndex);
    
    QString pageDirPath = pagesPath + QString("Page_%1").arg(pageIndex);
    QDir(pageDirPath).mkpath(".");
    QString contentPath = pageDirPath + QDir::separator() + "Content.xml";

    if (images.isEmpty()) {
        createPageContent(contentPath, "0", "0", "0", "0", currentId, false, pageWidth, pageHeight);
    } else {
        const QImage &image = images.at(pageIndex);
        QSize size = calculateImageSize(image, leftM, topM, pageWidth, pageHeight);
        
        QString maxWidth = QString::number(size.width() / 25.4 - leftM, 'f', 2);
        QString maxHeight = QString::number(size.height() / 25.4 - topM, 'f', 2);
        QString minX = QString::number(leftM, 'f', 2);
        QString minY = QString::number(topM, 'f', 2);

        createPageContent(contentPath, minX, minY, maxWidth, maxHeight, currentId, true, pageWidth, pageHeight);

        publicResStream << QString("    <ofd:MultiMedia ID=\"%1\" Type=\"Image\">\n").arg(currentId)
                       << QString("      <ofd:MediaFile>Image_%1.png</ofd:MediaFile>\n").arg(pageIndex + 1)
                       << "    </ofd:MultiMedia>\n";
    }
    currentId++;
}

void Writer::createPageContent(const QString &path, const QString &minX,
                             const QString &minY, const QString &maxX,
                             const QString &maxY, int &id, bool hasImage,
                             float pageWidth, float pageHeight)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QTextStream stream(&file);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    stream.setCodec("UTF-8");
#else
    stream.setEncoding(QStringConverter::Utf8);
#endif

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<ofd:Page xmlns:ofd=\"http://www.ofdspec.org\">\n"
           << "  <ofd:Area>\n"
           << "    <ofd:PhysicalBox>0.00 0.00 " << pageWidth << " " << pageHeight << "</ofd:PhysicalBox>\n"
           << "  </ofd:Area>\n"
           << "  <ofd:Content>\n"
           << QString("    <ofd:Layer ID=\"%1\">\n").arg(id++);

    // 背景路径对象
    stream << QString("      <ofd:PathObject ID=\"%1\" Boundary=\"0.00 0.00 %2 %3\" ")
                     .arg(id++)
                     .arg(pageWidth + 0.01)
                     .arg(pageHeight)
           << "MiterLimit=\"4.23\" Stroke=\"false\" Fill=\"true\" Rule=\"Even-Odd\">\n"
           << "        <ofd:FillColor Value=\"255 255 255\" ColorSpace=\"4\"/>\n"
           << "        <ofd:AbbreviatedData>M 0.00 0.00 L "
           << pageWidth + 0.01 << " 0.00 L "
           << pageWidth + 0.01 << " " << pageHeight << " L "
           << "0.00 " << pageHeight << " L "
           << "0.00 0.00 C </ofd:AbbreviatedData>\n"
           << "      </ofd:PathObject>\n";

    if (hasImage) {
        if (id == 4) id++;
        stream << QString("      <ofd:ImageObject ID=\"%1\" CTM=\"").arg(id)
               << maxX << " 0.00 0.00 " << maxY << " 0.00 0.00\" Boundary=\""
               << minX << " " << minY << " " << maxX << " " << maxY
               << QString("\" MiterLimit=\"4.23\" ResourceID=\"%1\"/>\n").arg(id + 1);
        id++;
    }

    stream << "    </ofd:Layer>\n"
           << "  </ofd:Content>\n"
           << "</ofd:Page>\n";

    file.close();
}

void Writer::createResourceFiles(const QString &path, const QVector<QImage> &images)
{
    QString resPath = path + RES_DIR;
    QDir(resPath).mkpath(".");

    for (int i = 0; i < images.size(); ++i) {
        QString imagePath = resPath + QDir::separator() + QString("Image_%1.png").arg(i + 1);
        images[i].save(imagePath, "PNG");
    }
}

bool Writer::compressOFD(const QString &outputPath, const QString &sourcePath)
{
    QuaZip zip(outputPath);
    if (!zip.open(QuaZip::mdCreate)) {
        return false;
    }

    QFileInfoList fileList;
    addToFileList(QDir(sourcePath), "", fileList);

    for (const QFileInfo &fileInfo : fileList) {
        if (!addFileToZip(&zip, sourcePath, fileInfo)) {
            zip.close();
            return false;
        }
    }

    zip.close();
    return true;
}

void Writer::addToFileList(const QDir &dir, const QString &relativePath, QFileInfoList &files)
{
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            addToFileList(QDir(entry.filePath()),
                         relativePath + entry.fileName() + "/",
                         files);
        } else {
            files.append(entry);
        }
    }
}

bool Writer::addFileToZip(QuaZip *zip, const QString &sourcePath, const QFileInfo &fileInfo)
{
    QuaZipFile zipFile(zip);
    QFile sourceFile(fileInfo.filePath());

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QString relativePath = fileInfo.filePath().mid(sourcePath.length() + 1);
    if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(relativePath))) {
        return false;
    }

    zipFile.write(sourceFile.readAll());
    zipFile.close();
    sourceFile.close();

    return true;
}

bool Writer::removeDir(const QString &dirPath)
{
    QDir dir(dirPath);
    return dir.exists() ? dir.removeRecursively() : true;
}

} // namespace ofd
