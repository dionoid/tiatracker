#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QString>

namespace ApplicationData {
QString path(const QString &relativePath = QString());

// Seed the Documents folder before resources or dialog paths are read.
bool initialize(QString &error);

// Merge bundled defaults recursively, never replacing an existing user file.
bool copyMissingFiles(const QString &source, const QString &destination, QString &error);
}

#endif
