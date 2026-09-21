#include "applicationdata.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ApplicationData {

QString path(const QString &relativePath) {
#ifdef Q_OS_MACOS
    return QDir(QDir::home().filePath("Documents/TIATracker")).absoluteFilePath(relativePath);
#else
#ifdef Q_OS_LINUX
    const QString userDataDirectory = qEnvironmentVariable("TIATRACKER_DATA_DIR");
    if (!userDataDirectory.isEmpty()) {
        return QDir(userDataDirectory).absoluteFilePath(relativePath);
    }
#endif
    return QDir::current().absoluteFilePath(relativePath);
#endif
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
#ifdef Q_OS_MACOS
    const QString bundledData = QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath("../Resources/data");
    if (!QFileInfo(QDir(bundledData).filePath("keymap.cfg")).isFile()) {
        error = QString("Bundled resources are missing from %1. Please reinstall TIATracker.").arg(bundledData);
        return false;
    }
    if (!copyMissingFiles(bundledData, path(), error)) {
        return false;
    }
#endif
    return true;
}

}