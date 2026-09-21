#include "applicationdata.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstdlib>

static void check(bool condition, const QString &message) {
    if (!condition) {
        QTextStream(stderr) << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static void writeFile(const QString &filename, const QByteArray &contents) {
    check(QDir().mkpath(QFileInfo(filename).absolutePath()), "create fixture directory");
    QFile file(filename);
    check(file.open(QIODevice::WriteOnly), "open fixture: " + filename);
    check(file.write(contents) == contents.size(), "write fixture: " + filename);
}

static QByteArray readFile(const QString &filename) {
    QFile file(filename);
    check(file.open(QIODevice::ReadOnly), "read fixture: " + filename);
    return file.readAll();
}

#ifdef Q_OS_MACOS
static void runStartup(const QString &executable, const QString &home, bool succeeds = true) {
    QProcess child;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("HOME", home);
    environment.insert("CFFIXED_USER_HOME", home);
    child.setProcessEnvironment(environment);
    child.setWorkingDirectory("/");
    child.start(executable, {"--initialize", home});
    check(child.waitForStarted(), "start relocated resource probe");
    check(child.waitForFinished(30000), "resource probe completed");
    check(child.exitStatus() == QProcess::NormalExit
          && child.exitCode() == (succeeds ? 0 : 1),
          "resource probe: " + QString::fromUtf8(child.readAllStandardError()));
}
#endif

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
#ifdef Q_OS_MACOS
    if (app.arguments().size() == 3 && app.arguments().at(1) == "--initialize") {
        const QString home = app.arguments().at(2);
        // Never let an environment-isolation failure touch real Documents.
        check(QDir::homePath() == home && home.contains("tiatracker-resources-"), "isolated home");
        check(ApplicationData::path() == home + "/Documents/TIATracker", "Documents resource path");
        QString error;
        if (!ApplicationData::initialize(error)) {
            QTextStream(stderr) << error << '\n';
            return 1;
        }
        check(ApplicationData::path("player/dasm/test.asm")
              == home + "/Documents/TIATracker/player/dasm/test.asm", "runtime resource lookup");
        return 0;
    }
#endif

    QTemporaryDir temporary(QDir::tempPath() + "/tiatracker-resources-XXXXXX");
    check(temporary.isValid(), "temporary directory");
    const QString source = temporary.path() + "/bundled defaults";
    const QString destination = temporary.path() + "/personal data";
    const QStringList files = {"keymap.cfg", "license.txt", "TIATracker_manual.pdf",
                               "songs/A song.ttt", "instruments/Bass.tti", "guides/PAL guide.ttg",
                               "player/dasm/test.asm", "player/k65/test.k65", "player/mads/test.asm"};
    for (const QString &relative : files) {
        writeFile(source + '/' + relative, QByteArray("default\0contents", 16));
    }
    check(QFile::setPermissions(source + "/keymap.cfg", QFileDevice::ReadOwner), "read-only source");
    QString error;
    check(ApplicationData::copyMissingFiles(source, destination, error), error);
    for (const QString &relative : files) {
        check(readFile(destination + '/' + relative) == readFile(source + '/' + relative), "first-run copy");
    }
    check(QFileInfo(destination + "/keymap.cfg").isWritable(), "editable copy of read-only default");

    for (const QString &relative : files) {
        writeFile(destination + '/' + relative, "user edits");
    }
    writeFile(destination + "/songs/My own song.ttt", "personal song");
    writeFile(source + "/player/dasm/new.asm", "new bundled template");
    check(ApplicationData::copyMissingFiles(source, destination, error), error);
    for (const QString &relative : files) {
        check(readFile(destination + '/' + relative) == "user edits", "preserve edits on repeated launch");
    }
    check(readFile(destination + "/songs/My own song.ttt") == "personal song", "preserve user-only song");
    check(readFile(destination + "/player/dasm/new.asm") == "new bundled template", "add new nested resource");
    check(QFile::remove(destination + "/songs/A song.ttt"), "remove seeded example");
    check(ApplicationData::copyMissingFiles(source, destination, error), error);
    check(readFile(destination + "/songs/A song.ttt") == readFile(source + "/songs/A song.ttt"), "restore missing example");
    check(!ApplicationData::copyMissingFiles(source + "/missing", destination, error)
          && !error.isEmpty(), "missing source reports failure");
    writeFile(temporary.path() + "/blocked", "not a directory");
    check(!ApplicationData::copyMissingFiles(source, temporary.path() + "/blocked", error), "directory collision fails");
    check(!ApplicationData::copyMissingFiles(source + "/license.txt", destination + "/songs", error), "file collision fails");
    check(readFile(temporary.path() + "/blocked") == "not a directory", "collision does not clobber user file");

#ifdef Q_OS_MACOS
    // Exercise real startup resolution from a relocated .app with no sibling data.
    const QString bundle = temporary.path() + "/Moved Applications/TIATracker.app/Contents";
    const QString executable = bundle + "/MacOS/resource-probe";
    check(QDir().mkpath(bundle + "/MacOS"), "create probe bundle");
    check(QFile::copy(QCoreApplication::applicationFilePath(), executable), "copy probe executable");
    check(ApplicationData::copyMissingFiles(source, bundle + "/Resources/data", error), error);
    const QString home = temporary.path() + "/test home";
    check(QDir().mkpath(home), "create isolated home");
    runStartup(executable, home);
    const QString personal = home + "/Documents/TIATracker";
    for (const QString &relative : files) {
        check(readFile(personal + '/' + relative) == readFile(source + '/' + relative), "startup copied resource");
    }
    writeFile(personal + "/keymap.cfg", "custom shortcuts");
    runStartup(executable, home);
    check(readFile(personal + "/keymap.cfg") == "custom shortcuts", "startup preserves edited keymap");
    check(readFile(bundle + "/Resources/data/keymap.cfg") == readFile(source + "/keymap.cfg"), "bundle unchanged");
    check(QFile::remove(bundle + "/Resources/data/keymap.cfg"), "remove bundled keymap");
    runStartup(executable, home, false);
    check(readFile(personal + "/keymap.cfg") == "custom shortcuts", "failed startup preserves user files");
#elif defined(Q_OS_LINUX)
    qputenv("TIATRACKER_DATA_DIR", destination.toUtf8());
    check(ApplicationData::path("keymap.cfg") == destination + "/keymap.cfg", "Linux launcher path unchanged");
    qunsetenv("TIATRACKER_DATA_DIR");
    check(ApplicationData::path("keymap.cfg") == QDir::current().filePath("keymap.cfg"), "working-directory fallback");
#endif
    QTextStream(stdout) << "Resource setup tests passed.\n";
    return 0;
}