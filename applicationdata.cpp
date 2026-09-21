#include "applicationdata.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace ApplicationData {

QString path(const QString &relativePath) {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                + "/TIATracker").absoluteFilePath(relativePath);
}

bool copyMissingFiles(const QString &source, const QString &destination, QString &error) {
    const QFileInfo sourceInfo(source);
    const QFileInfo destinationInfo(destination);
    if (sourceInfo.isDir()) {
        if (!sourceInfo.isReadable()) {
            error = QString("Cannot read bundled resources from %1.").arg(source);
            return false;
        }
        if (!QDir().mkpath(destination)) {
            error = QString("Cannot create resource folder %1.").arg(destination);
            return false;
        }
        const auto entries = QDir(source).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            if (!copyMissingFiles(entry.absoluteFilePath(),
                                  QDir(destination).filePath(entry.fileName()), error)) {
                return false;
            }
        }
        return true;
    }
    if (!sourceInfo.isFile()) {
        error = QString("Bundled resource is missing: %1.").arg(source);
        return false;
    }
    if (destinationInfo.exists() || destinationInfo.isSymLink()) {
        if (destinationInfo.isFile()) {
            return true;
        }
        error = QString("Expected a resource file at %1, but found a folder or broken link.").arg(destination);
        return false;
    }

    QFile file(source);
    if (!file.copy(destination)) {
        error = QString("Cannot copy %1 to %2: %3").arg(source, destination, file.errorString());
        return false;
    }
    // A read-only installation must still produce editable personal copies.
    QFile copy(destination);
    if (!copy.setPermissions(copy.permissions() | QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        error = QString("Cannot make resource writable: %1: %2").arg(destination, copy.errorString());
        return false;
    }
    return true;
}

bool initialize(QString &error) {
    error.clear();
    const QDir executableDirectory(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
    const QString bundledData = ":/defaults";
#elif defined(Q_OS_MACOS)
    const QString bundledData = executableDirectory.absoluteFilePath("../Resources/data");
#else
    // Development builds use data/; Debian packages use /usr/share/tiatracker.
    QString bundledData = executableDirectory.absoluteFilePath("data");
    if (!QFileInfo(QDir(bundledData).filePath("keymap.cfg")).isFile()) {
        bundledData = executableDirectory.absoluteFilePath("../../share/tiatracker");
    }
#endif
    if (!QFileInfo(QDir(bundledData).filePath("keymap.cfg")).isFile()) {
        error = QString("Bundled resources are missing from %1. Please reinstall TIATracker.").arg(bundledData);
        return false;
    }
    if (!copyMissingFiles(bundledData, path(), error)) {
        return false;
    }
    if (!QDir().mkpath(path("exports"))) {
        error = QString("Cannot create exports folder %1.").arg(path("exports"));
        return false;
    }
    return true;
}

}
