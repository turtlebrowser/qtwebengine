/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <httpsserver.h>
#include <util.h>

#include <QWebEngineCertificateError>
#include <QWebEnginePage>
#include <QWebEngineSettings>

#include <QtTest/QtTest>

class tst_CertificateError : public QObject
{
    Q_OBJECT
public:
    tst_CertificateError() { }

private Q_SLOTS:
    void handleError_data();
    void handleError();
};

struct PageWithCertificateErrorHandler : QWebEnginePage
{
    PageWithCertificateErrorHandler(bool defer, bool accept, QObject *p = nullptr)
        : QWebEnginePage(p), deferError(defer), acceptCertificate(accept) {
        connect(this, &QWebEnginePage::loadFinished, [&] (bool result) { spyLoad(result); });
    }

    bool deferError, acceptCertificate;

    CallbackSpy<bool> spyLoad;
    QScopedPointer<QWebEngineCertificateError> error;

    bool certificateError(const QWebEngineCertificateError &e) override {
        error.reset(new QWebEngineCertificateError(e));
        if (deferError)
            error->defer();
        return acceptCertificate;
    }
};

void tst_CertificateError::handleError_data()
{
    QTest::addColumn<bool>("deferError");
    QTest::addColumn<bool>("acceptCertificate");
    QTest::addColumn<QString>("expectedContent");
    QTest::addRow("Reject") << false << false << QString();
    QTest::addRow("DeferReject") << true << false << QString();
    QTest::addRow("DeferAccept") << true << true << "TEST";
}

void tst_CertificateError::handleError()
{
    HttpsServer server;
    server.setExpectError(true);
    QVERIFY(server.start());

    connect(&server, &HttpsServer::newRequest, [&] (HttpReqRep *rr) {
        rr->setResponseBody(QByteArrayLiteral("<html><body>TEST</body></html>"));
        rr->sendResponse();
    });

    QFETCH(bool, deferError);
    QFETCH(bool, acceptCertificate);
    QFETCH(QString, expectedContent);

    PageWithCertificateErrorHandler page(deferError, acceptCertificate);
    page.settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);

    page.setUrl(server.url());
    QTRY_VERIFY(page.error);
    QVERIFY(page.error->isOverridable());

    if (deferError) {
        QVERIFY(page.error->deferred());
        QVERIFY(!page.error->answered());
        QVERIFY(!page.spyLoad.wasCalled());
        QCOMPARE(toPlainTextSync(&page), QString());

        if (acceptCertificate)
            page.error->ignoreCertificateError();
        else
            page.error->rejectCertificate();

        QVERIFY(page.error->answered());
        page.error.reset();
    }

    bool loadResult = page.spyLoad.waitForResult();
    QVERIFY(page.spyLoad.wasCalled());
    QCOMPARE(loadResult, acceptCertificate);
    QCOMPARE(toPlainTextSync(&page), expectedContent);
}

QTEST_MAIN(tst_CertificateError)
#include <tst_certificateerror.moc>