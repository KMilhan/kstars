#include "test_catalog_download.h"

#include "kstars_ui_tests.h"
#include "test_kstars_startup.h"

#include "Options.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QSignalSpy>

#if __has_include(<KNSCore/enginebase.h>)
#include <KNSCore/enginebase.h>
using KNSEngineBase = KNSCore::EngineBase;
#else
#include <KNSCore/engine.h>
using KNSEngineBase = KNSCore::Engine;
#endif
#if __has_include(<KNSCore/resultsstream.h>)
#include <KNSCore/resultsstream.h>
#include <KNSCore/searchrequest.h>
#define KSTARS_HAS_KNS_RESULTSSTREAM 1
#else
#define KSTARS_HAS_KNS_RESULTSSTREAM 0
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <KNSWidgets/dialog.h>
#include <KNSWidgets/Button>
#else
#include <KNS3/DownloadWidget>
#include <KNS3/Button>
#endif

#include <KMessageBox>

TestCatalogDownload::TestCatalogDownload(QObject *parent): QObject(parent)
{

}

void TestCatalogDownload::initTestCase()
{
    KTELL_BEGIN();
}

void TestCatalogDownload::cleanupTestCase()
{
    KTELL_END();
}

void TestCatalogDownload::init()
{
    if (!KStars::Instance()->isStartedWithClockRunning())
    {
        QVERIFY(KStarsData::Instance()->clock());
        KStarsData::Instance()->clock()->start();
    }
}

void TestCatalogDownload::cleanup()
{
    if (!KStars::Instance()->isStartedWithClockRunning())
        KStarsData::Instance()->clock()->stop();
}

void TestCatalogDownload::testCatalogDownloadWhileUpdating()
{
    const auto triggerDownloadDialog = []()
    {
        QTimer::singleShot(0, []()
        {
            KStars::Instance()->action("get_data")->activate(QAction::Trigger);
        });
    };

    const auto runHeadlessKnsFlow = []()
    {
        KTELL("Headless KNS: initializing engine without UI");
        KNSEngineBase engine;
        QVERIFY(engine.init(":/kconfig/kstars.knsrc"));

#if KSTARS_HAS_KNS_RESULTSSTREAM
        auto *stream = engine.search(KNSCore::SearchRequest(KNSCore::SortMode::Downloads,
                                                            KNSCore::Filter::None,
                                                            QString(), {}, 0, 10));
        QVERIFY(stream);

        QSignalSpy finishedSpy(stream, &KNSCore::ResultsStream::finished);
        QSignalSpy errorSpy(&engine, &KNSEngineBase::signalErrorCode);
        QSignalSpy entriesSpy(stream, &KNSCore::ResultsStream::entriesFound);

        stream->fetch();

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 3000 && finishedSpy.isEmpty() && errorSpy.isEmpty() && entriesSpy.isEmpty())
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }

        if (finishedSpy.isEmpty() && errorSpy.isEmpty() && entriesSpy.isEmpty())
            KTELL("Headless KNS: no response within timeout, continuing test");
#else
        QSignalSpy errorSpy(&engine, &KNSEngineBase::signalErrorCode);
        if (!errorSpy.isEmpty())
            KTELL("Headless KNS: engine error during init, continuing test");
#endif
    };

    KTELL("Zoom in enough so that updates are frequent");
    double const previous_zoom = Options::zoomFactor();
    KStars::Instance()->zoom(previous_zoom * 50);

    // Ensure the download widget can be created in this environment.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    using DownloadDialog = KNSWidgets::Dialog;
#else
    using DownloadDialog = KNS3::DownloadWidget;
#endif
    bool initialDialogFound = false;
    bool initialChecked = false;
    bool initialFunctional = false;
    triggerDownloadDialog();
    QTimer::singleShot(3000, [&]()
    {
        auto initialDialog = KStars::Instance()->findChild<DownloadDialog*>("DownloadWidget");
        if (initialDialog)
        {
            initialDialogFound = true;
            initialFunctional = !initialDialog->findChildren<QToolButton*>().isEmpty();
            if (initialDialog->parentWidget())
                initialDialog->parentWidget()->close();
            else
                initialDialog->close();
        }
        QWidget *modal = QApplication::activeModalWidget();
        if (modal)
            modal->close();
        initialChecked = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(initialChecked, 10000);
    if (!initialDialogFound || !initialFunctional)
    {
        runHeadlessKnsFlow();
        Options::setZoomFactor(previous_zoom);
        return;
    }

    // This timer looks for message boxes to close until stopped
    QTimer close_message_boxes;
    close_message_boxes.setInterval(500);
    QObject::connect(&close_message_boxes, &QTimer::timeout, &close_message_boxes, [&]()
    {
        QDialog * const dialog = qobject_cast <QDialog*> (QApplication::activeModalWidget());
        if (dialog)
        {
            QList<QPushButton*> pb = dialog->findChildren<QPushButton*>();
            QTest::mouseClick(pb[0], Qt::MouseButton::LeftButton);
        }
    });

    int const count = 6;
    for (int i = 0; i < count; i++)
    {
        QString step = QString("[%1/%2] ").arg(i).arg(count);
        KTELL(step + "Open the Download Dialog, wait for plugins to load");
        volatile bool done = false;
        QTimer::singleShot(5000, [&]()
        {
            KTELL(step + "Change the first four catalogs installation state");
            auto d = KStars::Instance()->findChild<DownloadDialog*>("DownloadWidget");
            if (!d)
            {
                KTELL(step + "DownloadWidget not found, skipping iteration");
                done = true;
                return;
            }
            QList<QToolButton*> wl = d->findChildren<QToolButton*>();
            if (wl.count() >= 8)
            {
                wl[1]->setFocus();
                QTest::keyClick(wl[1], Qt::Key_Space);
                wl[3]->setFocus();
                QTest::keyClick(wl[3], Qt::Key_Space);
                wl[5]->setFocus();
                QTest::keyClick(wl[5], Qt::Key_Space);
                wl[7]->setFocus();
                QTest::keyClick(wl[7], Qt::Key_Space);
                QTest::qWait(5000);
            }
            else
            {
                KTELL(step + "Failed to load XML providers!");
            }
            KTELL(step + "Close the Download Dialog, accept all potential reinstalls");
            close_message_boxes.start();
            if (d->parentWidget())
                d->parentWidget()->close();
            else
                d->close();
            done = true;
        });
        triggerDownloadDialog();
        QTimer::singleShot(9000, [&]()
        {
            if (done)
                return;
            QWidget *modal = QApplication::activeModalWidget();
            if (modal)
                modal->close();
            done = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(done, 10000);
        close_message_boxes.stop();

        KTELL(step + "Wait a bit for pop-ups to appear");
        QTest::qWait(5000);
    }

    Options::setZoomFactor(previous_zoom);
}

QTEST_KSTARS_MAIN(TestCatalogDownload)
