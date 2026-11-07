#include "switch.hpp"

#include <QtTest>

#include "profilelist.hpp"

namespace {
    QString H(const QString &id){
        return QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Md5).toHex());
    }

    ProfileList::Input Make(){
        ProfileList::Input input;
        input.storageName = [](const QString &id){ return H(id);};
        return input;
    }

    const ProfileList::Row *Named(const QList<ProfileList::Row> &rows, const QString &name){
        for(const ProfileList::Row &row : rows) if(row.name == name) return &row;
        return nullptr;
    }
}

class tst_profilelist : public QObject {
    Q_OBJECT

private slots:

    void foldersAreGatheredByTheNameTheyAreOf(){
        ProfileList::Input input = Make();
        input.present[ProfileList::Widget] = {H("a"), H("lost")};
        input.present[ProfileList::WidgetCache] = {H("a"), H("quick:a")};
        input.present[ProfileList::Quick] = {H("quick:a")};
        input.present[ProfileList::Edge] = {H("a"), QStringLiteral("not-a-digest")};
        input.knownNames = {QStringLiteral("a")};
        const QList<ProfileList::Row> rows = ProfileList::Rows(input);
        QCOMPARE(rows.size(), 2);
        const ProfileList::Row *a = Named(rows, QStringLiteral("a"));
        QVERIFY(a);
        QCOMPARE(a->id, H("a"));
        QCOMPARE(a->folders.size(), 5);
        QVERIFY(!a->used && !a->held);
        const ProfileList::Row *lost = ProfileList::Find(rows, H("lost"));
        QVERIFY(lost);
        QVERIFY(lost->name.isEmpty());
        QCOMPARE(lost->folders.size(), 1);
    }

    void whatAUsedNameWorksOutToIsInUse(){
        ProfileList::Input input = Make();
        input.present[ProfileList::Widget] = {H("A"), H("quick:A"), H("free")};
        input.present[ProfileList::Quick] = {H("quick:A")};
        input.usedNames = {QStringLiteral("A")};
        input.knownNames = {QStringLiteral("quick:A"), QStringLiteral("free")};
        const QList<ProfileList::Row> rows = ProfileList::Rows(input);
        QVERIFY(Named(rows, QStringLiteral("A"))->used);
        QVERIFY(Named(rows, QStringLiteral("quick:A"))->used);
        QVERIFY(!Named(rows, QStringLiteral("free"))->used);
    }

    void aRowAViewIsOnIsHeld(){
        ProfileList::Input input = Make();
        input.present[ProfileList::Edge] = {H("b")};
        input.heldStorages = {H("b")};
        const QList<ProfileList::Row> rows = ProfileList::Rows(input);
        QCOMPARE(rows.size(), 1);
        QVERIFY(rows.first().held);
    }

    void aSpaceAPrivateTabIsOnIsHeld(){
        ProfileList::Input input = Make();
        input.present[ProfileList::Widget] = {H("p"), H("other")};
        input.present[ProfileList::Quick] = {H("quick:q")};
        input.knownNames = {QStringLiteral("p"), QStringLiteral("q"), QStringLiteral("other")};
        input.heldNames = {QStringLiteral("p"), QStringLiteral("q")};
        const QList<ProfileList::Row> rows = ProfileList::Rows(input);
        QVERIFY(Named(rows, QStringLiteral("p"))->held);
        QVERIFY(Named(rows, QStringLiteral("q"))->held);
        QVERIFY(!Named(rows, QStringLiteral("other"))->held);
    }

    void onlyADigestNamesARow(){
        ProfileList::Input input = Make();
        input.present[ProfileList::Widget] = {H("c")};
        const QList<ProfileList::Row> rows = ProfileList::Rows(input);
        QVERIFY(ProfileList::Find(rows, H("c")));
        QVERIFY(!ProfileList::Find(rows, QStringLiteral("../") + H("c")));
        QVERIFY(!ProfileList::Find(rows, H("c").toUpper()));
        QVERIFY(!ProfileList::Find(rows, H("d")));
        QVERIFY(!ProfileList::IsDigest(QStringLiteral("C:/x")));
    }
};

QTEST_APPLESS_MAIN(tst_profilelist)
#include "tst_profilelist.moc"
