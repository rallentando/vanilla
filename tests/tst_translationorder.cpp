#include "switch.hpp"
#include "const.hpp"

#include <QtTest>
#include <QTranslator>

#include "translationorder.hpp"

class tst_translationorder : public QObject {
    Q_OBJECT

private slots:
    void thePreferredLanguageGoesInLast();
    void aLanguageNamedTwiceGoesInOnce();
    void thePreferredLanguageAnswersWithTheRealFiles();
};

void tst_translationorder::thePreferredLanguageGoesInLast(){
    QCOMPARE(TranslationOrder::Locales(QStringList() << QStringLiteral("ja-JP") << QStringLiteral("en-US")),
             QStringList() << QStringLiteral("en_US") << QStringLiteral("ja_JP"));
    QCOMPARE(TranslationOrder::Locales(QStringList() << QStringLiteral("en-US") << QStringLiteral("ja-JP")),
             QStringList() << QStringLiteral("ja_JP") << QStringLiteral("en_US"));
    QCOMPARE(TranslationOrder::Locales(QStringList()), QStringList());
}

void tst_translationorder::aLanguageNamedTwiceGoesInOnce(){
    QCOMPARE(TranslationOrder::Locales(QStringList() << QStringLiteral("ja-Jpan-JP") << QStringLiteral("ja-JP")
                                                     << QStringLiteral("ja-Jpan") << QStringLiteral("ja")),
             QStringList() << QStringLiteral("ja_JP"));
    QCOMPARE(TranslationOrder::Locales(QStringList() << QStringLiteral("ja-JP") << QStringLiteral("en-US")
                                                     << QStringLiteral("ja")),
             QStringList() << QStringLiteral("en_US") << QStringLiteral("ja_JP"));
}

void tst_translationorder::thePreferredLanguageAnswersWithTheRealFiles(){
    const QString dir = QStringLiteral(VANILLA_QM_DIR);
    QVERIFY2(QFile::exists(dir + QStringLiteral("/vanilla_ja.qm")), qPrintable(dir));
    QVERIFY2(QFile::exists(dir + QStringLiteral("/vanilla_en.qm")), qPrintable(dir));

    QList<QTranslator*> installed;
    for(const QString &locale : TranslationOrder::Locales(QStringList() << QStringLiteral("ja-JP") << QStringLiteral("en-US"))){
        QTranslator *translator = new QTranslator(this);
        QVERIFY(translator->load(QStringLiteral("vanilla_") + locale, dir));
        QVERIFY(QCoreApplication::installTranslator(translator));
        installed << translator;
    }
    const QString text = QCoreApplication::translate("Page", "NewViewNode");
    for(QTranslator *translator : installed){
        QCoreApplication::removeTranslator(translator);
        delete translator;
    }
    QCOMPARE(text, QStringLiteral("新規タブ"));
}

QTEST_MAIN(tst_translationorder)
#include "tst_translationorder.moc"
