#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QString>

namespace ApplicationData {
QString path(const QString &relativePath = QString());

// Seed the macOS Documents folder before any resources or dialog paths are read.
// Other platforms retain their existing launcher/working-directory behavior.
bool initialize(QString &error);

// Merge bundled defaults recursively, never replacing an existing user file.
bool copyMissingFiles(const QString &source, const QString &destination, QString &error);
}

#endif