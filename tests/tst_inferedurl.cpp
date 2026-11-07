#include "switch.hpp"
#include "const.hpp"

#include <QtTest>

#include "view.hpp"

namespace {

    class Link : public WebElement {
    public:
        Link(const QString &text, const QString &url)
            : m_Text(text), m_Url(QUrl(url)){}
        QString InnerText() const override { return m_Text;}
        QUrl LinkUrl() const override { return m_Url;}
        bool IsNull() const override { return false;}
    private:
        QString m_Text;
        QUrl m_Url;
    };

    SharedWebElementList Links(const QList<QPair<QString, QString>> &links){
        SharedWebElementList list;
        for(const auto &link : links)
            list << std::make_shared<Link>(link.first, link.second);
        return list;
    }

    QString Back(const QString &current, const QList<QPair<QString, QString>> &links){
        return View::InferedUrl(QUrl(current), Links(links), false).toString();
    }

    QString Forward(const QString &current, const QList<QPair<QString, QString>> &links){
        return View::InferedUrl(QUrl(current), Links(links), true).toString();
    }
}

class tst_inferedurl : public QObject {
    Q_OBJECT

private slots:

    void theTextSaysWhichWay(){
        const QString page = QStringLiteral("https://example.com/list");
        const QList<QPair<QString, QString>> links = {
            { QStringLiteral("Prev"),     QStringLiteral("javascript:void(0)") },
            { QStringLiteral("Previous"), QStringLiteral("https://example.com/a") },
            { QStringLiteral("Next >>"),  QStringLiteral("https://example.com/b") },
        };
        QCOMPARE(Back(page, links),    QStringLiteral("https://example.com/a"));
        QCOMPARE(Forward(page, links), QStringLiteral("https://example.com/b"));
    }

    void theNearestNumberInThePath(){
        const QString page = QStringLiteral("https://example.com/page/5");
        const QList<QPair<QString, QString>> links = {
            { QStringLiteral("x"), QStringLiteral("https://example.com/page/3") },
            { QStringLiteral("x"), QStringLiteral("https://example.com/page/4") },
            { QStringLiteral("x"), QStringLiteral("https://example.com/page/7") },
            { QStringLiteral("x"), QStringLiteral("https://example.com/page/6") },
        };
        QCOMPARE(Back(page, links),    QStringLiteral("https://example.com/page/4"));
        QCOMPARE(Forward(page, links), QStringLiteral("https://example.com/page/6"));

        QList<QPair<QString, QString>> reversed;
        for(const auto &link : links) reversed.prepend(link);
        QCOMPARE(Back(page, reversed),    QStringLiteral("https://example.com/page/4"));
        QCOMPARE(Forward(page, reversed), QStringLiteral("https://example.com/page/6"));
    }

    void aPageNumberInTheQuery(){
        const QString page = QStringLiteral("https://example.com/list?p=5");
        const QList<QPair<QString, QString>> links = {
            { QStringLiteral("older"), QStringLiteral("https://example.com/list?p=3") },
            { QStringLiteral("4"),     QStringLiteral("https://example.com/list?p=4#top") },
            { QStringLiteral("6"),     QStringLiteral("https://example.com/list?p=6#top") },
        };
        QCOMPARE(Back(page, links),    QStringLiteral("https://example.com/list?p=4#top"));
        QCOMPARE(Forward(page, links), QStringLiteral("https://example.com/list?p=6#top"));
    }

    void aStepInTheQueryWithWords(){
        const QString page = QStringLiteral("https://example.com/list?p=5");
        QCOMPARE(Back(page,    {{ QStringLiteral("older"), QStringLiteral("https://example.com/list?p=4") }}),
                 QStringLiteral("https://example.com/list?p=4"));
        QCOMPARE(Forward(page, {{ QStringLiteral("newer"), QStringLiteral("https://example.com/list?p=6") }}),
                 QStringLiteral("https://example.com/list?p=6"));
        QCOMPARE(Back(page,    {{ QStringLiteral("newer"), QStringLiteral("https://example.com/list?p=6") }}), QString());
        QCOMPARE(Forward(page, {{ QStringLiteral("older"), QStringLiteral("https://example.com/list?p=4") }}), QString());
    }

    void nothingToGoBy(){
        const QString page = QStringLiteral("https://example.com/page/5");
        const QList<QPair<QString, QString>> links = {
            { QStringLiteral("x"), QStringLiteral("https://other.org/page/4") },
            { QStringLiteral("x"), QStringLiteral("https://other.org/page/6") },
            { QStringLiteral("x"), QStringLiteral("https://example.com/page/5") },
        };
        QCOMPARE(Back(page, links),    QString());
        QCOMPARE(Forward(page, links), QString());
        QCOMPARE(Back(page, {}),       QString());
    }

    void aSubdomainIsTheSameSite(){
        const QString page = QStringLiteral("https://example.com/page/5");
        QCOMPARE(Back(page,    {{ QStringLiteral("x"), QStringLiteral("https://www.example.com/page/4") }}),
                 QStringLiteral("https://www.example.com/page/4"));
        QCOMPARE(Forward(page, {{ QStringLiteral("x"), QStringLiteral("https://www.example.com/page/6") }}),
                 QStringLiteral("https://www.example.com/page/6"));
    }

    void aDeeperOrShallowerPath(){
        const QString page = QStringLiteral("https://example.com/a/2");
        QCOMPARE(Forward(page, {{ QStringLiteral("x"), QStringLiteral("https://example.com/a/2/3") }}),
                 QStringLiteral("https://example.com/a/2/3"));
        QCOMPARE(Back(page,    {{ QStringLiteral("x"), QStringLiteral("https://example.com/a/2/3") }}), QString());
        QCOMPARE(Back(QStringLiteral("https://example.com/a/2/3"),
                      {{ QStringLiteral("x"), QStringLiteral("https://example.com/a/2") }}),
                 QStringLiteral("https://example.com/a/2"));
    }
};

QTEST_MAIN(tst_inferedurl)
#include "tst_inferedurl.moc"
