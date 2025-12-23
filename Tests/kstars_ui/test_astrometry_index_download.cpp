/*
    KStars UI test for astrometry index download

    SPDX-FileCopyrightText: 2025

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kstars_ui_tests.h"
#include "test_ekos.h"
#include "test_ekos_simulator.h"

#include "Options.h"
#include "ekos/align/align.h"
#include "ekos/align/opsastrometryindexfiles.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

class TestAstrometryIndexDownload : public QObject
{
        Q_OBJECT
    private slots:
        void initTestCase();
        void cleanupTestCase();
        void testLocalIndexDownload();
};

void TestAstrometryIndexDownload::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KVERIFY_EKOS_IS_HIDDEN();
    KTRY_OPEN_EKOS();
    KVERIFY_EKOS_IS_OPENED();
    KTRY_EKOS_START_SIMULATORS();
    QTRY_VERIFY_WITH_TIMEOUT(Ekos::Manager::Instance()->alignModule() != nullptr, 5000);
}

void TestAstrometryIndexDownload::cleanupTestCase()
{
    KTRY_EKOS_STOP_SIMULATORS();
    KTRY_CLOSE_EKOS();
}

void TestAstrometryIndexDownload::testLocalIndexDownload()
{
    QTemporaryDir serverDir;
    QVERIFY(serverDir.isValid());
    QDir serverRoot(serverDir.path());
    QVERIFY(serverRoot.mkpath("4100"));

    const QString sourcePath = serverRoot.filePath("4100/index-4112.fits");
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    const QByteArray payload("FAKE_INDEX");
    QCOMPARE(sourceFile.write(payload), payload.size());
    sourceFile.close();

    QTemporaryDir destDir;
    QVERIFY(destDir.isValid());
    const QStringList previousDirs = Options::astrometryIndexFolderList();
    Options::setAstrometryIndexFolderList(QStringList() << destDir.path());

    auto *align = Ekos::Manager::Instance()->alignModule();
    QVERIFY(align != nullptr);
    Ekos::OpsAstrometryIndexFiles indexFiles(align);
    const QString baseUrl = QUrl::fromLocalFile(serverDir.path() + '/').toString();
    indexFiles.indexURL->setText(baseUrl);
    indexFiles.downloadSingleIndexFile("index-4112.fits");

    const QString downloadedPath = QDir(destDir.path()).filePath("index-4112.fits");
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(downloadedPath), 5000);

    QFile downloadedFile(downloadedPath);
    QVERIFY(downloadedFile.open(QIODevice::ReadOnly));
    QCOMPARE(downloadedFile.readAll(), payload);

    Options::setAstrometryIndexFolderList(previousDirs);
}

QTEST_KSTARS_MAIN(TestAstrometryIndexDownload)

#include "test_astrometry_index_download.moc"
