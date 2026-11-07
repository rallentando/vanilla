#include "application.hpp"
#include "const.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "notifier.hpp"
#include "testsupport.hpp"

#include <QtTest>
#include <QPainter>
#include <QTimer>

class tst_notifier : public QObject {
    Q_OBJECT
    QScopedPointer<MainWindow> m_Window;
    Notifier *m_Notifier = nullptr;

    void SetScrollArea(bool on){
        Application::GlobalSettings().setValue(QStringLiteral("application/@EnableNotifierScrollArea"), on);
        TreeBank::LoadSettings();
        m_Notifier->ResizeNotify(m_Window->GetTreeBank()->size());
    }
private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
        m_Window.reset(new MainWindow(941, QPoint(100, 100)));
        m_Window->show();
        m_Notifier = m_Window->GetTreeBank()->GetNotifier();
        QVERIFY(m_Notifier);
    }
    void init(){
        m_Notifier->m_LinkTimer->stop();
        m_Notifier->m_LinkTimer->setInterval(10);
        m_Notifier->ResetLink();
        m_Notifier->SetStatus(QString(), QString());
        SetScrollArea(false);
    }

    void withNothingToSayItIsNotThere(){
        QVERIFY(!m_Notifier->isVisible());
    }

    void theTextIsAPixelAboveTheMiddleOfTheCard(){
        const auto firstInk = [](const QImage &image, int from, int to){
            const int left = image.width() / 8;
            const QRgb ground = image.pixel(left, from);
            for(int y = from; y < to; y++)
                for(int x = left; x < image.width() - left; x++)
                    if(image.pixel(x, y) != ground) return y;
            return -1;
        };
        const QString text = QStringLiteral("HHHHHHHH");
        m_Notifier->SetStatus(text);
        QVERIFY(m_Notifier->isVisible());
        const QImage card = m_Notifier->grab().toImage().convertToFormat(QImage::Format_RGB32);

        class Centred : public QWidget {
        public:
            QString m_Text;
            int m_Line = 0;
        protected:
            void paintEvent(QPaintEvent*) override {
                QPainter painter(this);
                painter.fillRect(rect(), Qt::white);
                painter.setFont(NotifierFont());
                painter.setPen(Qt::black);
                painter.drawText(QRect(0, (height() - m_Line) / 2, width(), m_Line),
                                 Qt::AlignLeft | Qt::AlignVCenter, m_Text);
            }
        } reference;
        reference.m_Text = text;
        reference.m_Line = m_Notifier->ScaleByDevice(NOTIFIER_LINE_HEIGHT);
        reference.resize(m_Notifier->size());
        const QImage centred = reference.grab().toImage().convertToFormat(QImage::Format_RGB32);

        const int inside = qCeil(m_Notifier->ScaleByDevice(NOTIFIER_PADDING_Y) * card.devicePixelRatio());
        const int ink = firstInk(card, inside, card.height() - inside);
        QVERIFY(ink >= 0);
        const int lifted = firstInk(centred, inside, card.height() - inside) - ink;
        const qreal pixels = m_Notifier->ScaleByDevice(1) * card.devicePixelRatio();
        QVERIFY2(lifted >= qFloor(pixels) && lifted <= qCeil(pixels), qPrintable(QString::number(lifted)));
    }

    void aLinkStaysForAsLongAsItIsHovered(){
        m_Notifier->m_StatusTimer->setInterval(10);
        m_Notifier->SetLink(QStringLiteral("https://example.com/"),
                            QStringLiteral("Example"), QString());
        QVERIFY(m_Notifier->isVisible());
        QTest::qWait(60);
        QVERIFY(m_Notifier->isVisible());
        QCOMPARE(m_Notifier->m_UpperText, QStringLiteral("Example"));

        m_Notifier->SetLink(QString(), QString(), QString());
        QTRY_VERIFY_WITH_TIMEOUT(!m_Notifier->isVisible(), 2000);
        QVERIFY(m_Notifier->m_UpperText.isEmpty());
        QVERIFY(m_Notifier->m_LowerText.isEmpty());
        m_Notifier->m_StatusTimer->setInterval(NOTIFIER_STATUS_DURATION);
    }

    void aLeftLinkWaitsAndTheNextOneTakesOver(){
        QCOMPARE(NOTIFIER_LINK_LINGER, 500);
        m_Notifier->m_LinkTimer->setInterval(50);
        m_Notifier->SetLink(QStringLiteral("https://example.com/a"), QString(), QString());
        m_Notifier->SetLink(QString(), QString(), QString());
        QVERIFY(m_Notifier->isVisible());
        QCOMPARE(m_Notifier->m_LowerText, QStringLiteral("https://example.com/a"));

        m_Notifier->SetLink(QStringLiteral("https://example.com/b"), QString(), QString());
        QTest::qWait(120);
        QVERIFY(m_Notifier->isVisible());
        QCOMPARE(m_Notifier->m_LowerText, QStringLiteral("https://example.com/b"));
    }

    void aStatusGoesByItself(){
        m_Notifier->m_StatusTimer->setInterval(10);
        m_Notifier->SetStatus(QStringLiteral("Saved"), QStringLiteral("a.txt"));
        QVERIFY(m_Notifier->isVisible());
        QTRY_VERIFY_WITH_TIMEOUT(!m_Notifier->isVisible(), 2000);
        QVERIFY(m_Notifier->m_LowerText.isEmpty());
        m_Notifier->m_StatusTimer->setInterval(NOTIFIER_STATUS_DURATION);
    }

    void theEndOfAStatusLeavesTheHoveredLink(){
        m_Notifier->m_StatusTimer->setInterval(10);
        m_Notifier->SetStatus(QStringLiteral("Started loading."));
        m_Notifier->SetLink(QStringLiteral("https://example.com/"), QString(), QString());
        QTRY_VERIFY_WITH_TIMEOUT(m_Notifier->m_UpperText.isEmpty(), 2000);
        QVERIFY(m_Notifier->isVisible());
        QCOMPARE(m_Notifier->m_LowerText, QStringLiteral("https://example.com/"));
        m_Notifier->m_StatusTimer->setInterval(NOTIFIER_STATUS_DURATION);
    }

    void theCardIsAsWideAsItsTextUpToItsWidest(){
        m_Notifier->SetStatus(QStringLiteral("a"));
        const int narrow = m_Notifier->width();
        m_Notifier->SetStatus(QString(2000, QLatin1Char('a')));
        QVERIFY(m_Notifier->width() > narrow);
        const int widest = qMax(m_Window->GetTreeBank()->width() * NOTIFIER_WIDTH_PERCENTAGE / 100,
                                m_Notifier->ScaleByDevice(NOTIFIER_MINIMUM_WIDTH));
        QVERIFY(m_Notifier->width() <= widest);
    }

    void itSitsInItsCornerOffTheEdge(){
        m_Notifier->SetStatus(QStringLiteral("a"));
        const int margin = m_Notifier->ScaleByDevice(NOTIFIER_MARGIN);
        const QRect tb = m_Window->GetTreeBank()->rect();
        QCOMPARE(m_Notifier->m_Position, Notifier::SouthWest);
        QCOMPARE(m_Notifier->geometry().left(), margin);
        QCOMPARE(m_Notifier->geometry().bottom() + 1, tb.height() - margin);
    }

    void theScrollAreaIsThereOnlyIfAskedFor(){
        QSignalSpy requests(m_Notifier, &Notifier::ScrollRequest);
        const QPoint inArea(m_Notifier->ScaleByDevice(NOTIFIER_PADDING_X) + 5,
                            m_Notifier->ScaleByDevice(NOTIFIER_PADDING_Y) + 5);

        m_Notifier->SetStatus(QStringLiteral("a"));
        QVERIFY(!m_Notifier->EmitScrollRequest(inArea));
        QCOMPARE(requests.count(), 0);
        m_Notifier->ResetStatus();

        SetScrollArea(true);
        QVERIFY(m_Notifier->isVisible());
        QVERIFY(m_Notifier->EmitScrollRequest(inArea));
        QCOMPARE(requests.count(), 1);
        QVERIFY(m_Notifier->height() >= m_Notifier->ScaleByDevice(SCROLL_AREA_HEIGHT));

        SetScrollArea(false);
        QVERIFY(!m_Notifier->isVisible());
    }

    void theScrollAreaIsInTheMiddleOfAnEmptyCard(){
        SetScrollArea(true);
        const QRect area = m_Notifier->ScrollAreaRect();
        const QRect card = m_Notifier->rect();
        QCOMPARE(area.left() - card.left(), card.right() - area.right());
        QCOMPARE(area.top() - card.top(), card.bottom() - area.bottom());

        const QString text = QStringLiteral("a");
        m_Notifier->SetStatus(text);
        QCOMPARE(m_Notifier->width(), card.width() + m_Notifier->ScaleByDevice(NOTIFIER_PADDING_X)
                 + QFontMetrics(NotifierFont(), m_Notifier).horizontalAdvance(text) + 1);
        m_Notifier->ResetStatus();
        SetScrollArea(false);
    }

    void aPressInTheScrollAreaAsksForWhereTheMarkerWouldBe(){
        SetScrollArea(true);
        QSignalSpy requests(m_Notifier, &Notifier::ScrollRequest);
        const QRect area = m_Notifier->ScrollAreaRect();
        const auto press = [&](QPoint pos){
            requests.clear();
            const bool taken = m_Notifier->EmitScrollRequest(pos);
            return taken ? requests.at(0).at(0).toPointF() : QPointF(-1, -1);
        };

        QCOMPARE(press(area.topLeft()), QPointF(0, 0));
        QCOMPARE(press(area.bottomRight()), QPointF(1, 1));
        const int inset = m_Notifier->ScrollMarkerInset();
        QCOMPARE(press(area.topLeft() + QPoint(inset, inset)), QPointF(0, 0));
        QVERIFY(press(area.topLeft() + QPoint(inset + 1, inset + 1)).x() > 0);
        const QPointF middle = press(area.center());
        QVERIFY(qAbs(middle.x() - 0.5) < 0.05);
        QVERIFY(qAbs(middle.y() - 0.5) < 0.05);

        QCOMPARE(press(area.topLeft() - QPoint(1, 0)), QPointF(-1, -1));
        QCOMPARE(press(area.bottomRight() + QPoint(0, 1)), QPointF(-1, -1));
        SetScrollArea(false);
    }

    void cleanupTestCase(){
        m_Window.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

int main(int argc, char **argv){
    Application app(argc, argv);
    tst_notifier test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_notifier.moc"
