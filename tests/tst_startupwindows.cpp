#include "switch.hpp"
#include "const.hpp"
#include "application.hpp"
#include "mainwindow.hpp"
#include "treebank.hpp"
#include "toolbar.hpp"
#include "dialog.hpp"
#include "testsupport.hpp"

#include <QtTest>
#include <QPointer>
#include <QMimeData>

class Counting : public QObject {
    Q_OBJECT
public:
    Counting(QEvent::Type type) : m_Type(type), count(0) {}
    bool eventFilter(QObject *, QEvent *ev) Q_DECL_OVERRIDE {
        if(ev->type() == m_Type) count++;
        return false;
    }
    QEvent::Type m_Type;
    int count;
};

class tst_startupwindows : public QObject {
    Q_OBJECT

    static void dropAllWindows(){
        Application::SetCurrentWindow(static_cast<MainWindow*>(nullptr));
        foreach(int id, Application::GetMainWindows().keys()){
            MainWindow *win = Application::Window(id);
            Application::RemoveWindow(id);
            delete win;
        }
    }

private slots:
    void initTestCase(){
        TestSupport::SilenceDebugOutput();
        TestSupport::DisableWidgetAnimation();
        TreeBank::Initialize();
    }

    void cleanup(){
        Application::SetRestoringWindows(false);
        dropAllWindows();
        Application::GlobalSettings().setValue(QStringLiteral("application/@EnableFramelessWindow"), false);
        Application::LoadGlobalSettings();
    }

    void savedWindowIdsComeFromEveryKeyAWindowWritesAscendingAndOnce(){
        Settings s;
        s.setValue(QStringLiteral("mainwindow/toolbar12"), 1);
        s.setValue(QStringLiteral("mainwindow/status12"), 1);
        s.setValue(QStringLiteral("mainwindow/geometry3"), 1);
        s.setValue(QStringLiteral("mainwindow/treebar7"), 1);
        s.setValue(QStringLiteral("mainwindow/inspectordock9"), 1);
        s.setValue(QStringLiteral("mainwindow/sidepaneldock9"), 1);
        s.setValue(QStringLiteral("mainwindow/geometry"), 1);
        s.setValue(QStringLiteral("mainwindow/geometry0"), 1);
        s.setValue(QStringLiteral("application/@EnableAutoLoad"), 1);
        QCOMPARE(Application::SavedWindowIds(s), QList<int>() << 3 << 7 << 12);
        QCOMPARE(Application::SavedWindowIds(Settings()), QList<int>());
    }

    void theKeysOfTheIdsNoWindowHoldsAreSwept(){
        Settings s;
        foreach(const QString &name, QStringList()
                << QStringLiteral("tableview") << QStringLiteral("geometry") << QStringLiteral("notifier")
                << QStringLiteral("receiver") << QStringLiteral("menubar") << QStringLiteral("toolbar")
                << QStringLiteral("treebar") << QStringLiteral("status")){
            s.setValue(QStringLiteral("mainwindow/%1%2").arg(name).arg(1), 1);
            s.setValue(QStringLiteral("mainwindow/%1%2").arg(name).arg(2), 1);
        }
        s.setValue(QStringLiteral("application/@EnableAutoLoad"), 1);
        Application::SweepWindowSettings(s, QList<int>() << 1);
        QCOMPARE(s.allKeys(QStringLiteral("mainwindow")).count(), 8);
        QVERIFY(s.contains(QStringLiteral("mainwindow/geometry1")));
        QVERIFY(!s.contains(QStringLiteral("mainwindow/geometry2")));
        QVERIFY(s.contains(QStringLiteral("application/@EnableAutoLoad")));
        Application::SweepWindowSettings(s, QList<int>());
        QCOMPARE(s.allKeys(QStringLiteral("mainwindow")).count(), 0);
    }

    void theTreeRestoresIntoTheShellOfItsIdAndMakesAWindowOtherwise(){
        MainWindow *shell = Application::NewWindow(950);
        QCOMPARE(Application::GetMainWindows().count(), 1);
        std::unique_ptr<ViewNode> nd(new ViewNode());

        QCOMPARE(TreeBank::RestoreIntoWindow(nd.get(), 950), shell);
        QCOMPARE(Application::GetMainWindows().count(), 1);
        QCOMPARE(Application::GetCurrentWindow(), shell);

        MainWindow *made = TreeBank::RestoreIntoWindow(nd.get(), 951);
        QVERIFY(made);
        QVERIFY(made != shell);
        QCOMPARE(Application::Window(951), made);
        QCOMPARE(Application::GetMainWindows().count(), 2);
        QCOMPARE(Application::GetCurrentWindow(), made);
    }

    void shellsTheTreeDidNotFillGoAndTheWindowRestoredLastIsCurrent(){
        MainWindow *a = Application::NewWindow(960);
        MainWindow *b = Application::NewWindow(961);
        MainWindow *c = Application::NewWindow(962);
        QPointer<MainWindow> ga(a), gb(b), gc(c);
        QCOMPARE(Application::GetCurrentWindow(), c);

        Application::DropIdleShells(QSet<int>() << 961, b);

        QCOMPARE(Application::GetMainWindows().keys(), QList<int>() << 961);
        QCOMPARE(Application::GetCurrentWindow(), b);
        QVERIFY(ga.isNull());
        QVERIFY(!gb.isNull());
        QVERIFY(gc.isNull());
    }

    void aRestoredWindowStaysThoughItHoldsNoTab(){
        MainWindow *a = Application::NewWindow(960);
        MainWindow *b = Application::NewWindow(961);
        QVERIFY(!a->GetTreeBank()->GetCurrentViewNode());
        Application::SetCurrentWindow(a);

        Application::DropIdleShells(QSet<int>() << 960 << 961, b);

        QCOMPARE(Application::GetMainWindows().keys(), QList<int>() << 960 << 961);
        QCOMPARE(Application::GetCurrentWindow(), b);
    }

    void withNothingRestoredTheCurrentShellStays(){
        MainWindow *a = Application::NewWindow(960);
        MainWindow *b = Application::NewWindow(961);
        QPointer<MainWindow> ga(a);
        QCOMPARE(Application::GetCurrentWindow(), b);

        Application::DropIdleShells(QSet<int>(), nullptr);

        QCOMPARE(Application::GetMainWindows().keys(), QList<int>() << 961);
        QCOMPARE(Application::GetCurrentWindow(), b);
        QVERIFY(ga.isNull());
    }

    void aNoticeOnAShellMovesToTheWindowThatStays(){
        MainWindow *a = Application::NewWindow(960);
        MainWindow *b = Application::NewWindow(961);
        QCOMPARE(Application::GetCurrentWindow(), b);
        ModelessDialog::Information(QStringLiteral("t"), QStringLiteral("x"));
        QTRY_COMPARE(b->DialogFrame()->Dialogs().count(), 1);
        QCOMPARE(a->DialogFrame()->Dialogs().count(), 0);
        ModelessDialog *dialog = b->DialogFrame()->Dialogs().first();
        QPointer<ModelessDialog> alive(dialog);

        Application::DropIdleShells(QSet<int>() << 960, a);

        QVERIFY(!alive.isNull());
        QCOMPARE(a->DialogFrame()->Dialogs(), QList<ModelessDialog*>() << dialog);
        QCOMPARE(dialog->parentWidget(), a->DialogFrame());
        dialog->deleteLater();
    }

    void aShellNeitherClosesNorTakesInputWhileTheTreeIsRead(){
        MainWindow *shell = Application::NewWindow(970);
        Application::SetRestoringWindows(true);

        QCloseEvent close;
        QApplication::sendEvent(shell, &close);
        QVERIFY(!close.isAccepted());
        QCOMPARE(Application::Window(970), shell);
        QVERIFY(shell->isVisible());

        QKeyEvent key(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent move(QEvent::MouseMove, QPointF(1, 1), QPointF(1, 1),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QVERIFY(Application::SwallowsDuringRestore(shell, &key));
        QVERIFY(Application::SwallowsDuringRestore(shell->GetTreeBank(), &key));
        QVERIFY(Application::SwallowsDuringRestore(shell->GetToolBar(), &press));
        QVERIFY(!Application::SwallowsDuringRestore(shell, &move));
        QWidget other;
        QVERIFY(!Application::SwallowsDuringRestore(&other, &key));
        QVERIFY(!Application::SwallowsDuringRestore(&other, &press));

        Application::SetRestoringWindows(false);
        QVERIFY(!Application::SwallowsDuringRestore(shell, &key));
        QVERIFY(!Application::SwallowsDuringRestore(shell->GetToolBar(), &press));
    }

    void theTitleBarAndADropWaitToo(){
        Application::GlobalSettings().setValue(QStringLiteral("application/@EnableFramelessWindow"), true);
        Application::LoadGlobalSettings();
        QSet<QWidget*> before;
        foreach(QWidget *widget, QApplication::topLevelWidgets())
            if(qobject_cast<TitleBar*>(widget)) before << widget;
        MainWindow *shell = Application::NewWindow(980);
        TitleBar *title = nullptr;
        foreach(QWidget *widget, QApplication::topLevelWidgets())
            if(TitleBar *bar = qobject_cast<TitleBar*>(widget))
                if(!before.contains(bar)) title = bar;
        QVERIFY(title);
        Counting presses(QEvent::MouseButtonPress);
        title->installEventFilter(&presses);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMimeData data;
        QDragEnterEvent enter(QPoint(1, 1), Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
        QDropEvent drop(QPointF(1, 1), Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);

        Application::SetRestoringWindows(true);
        QApplication::sendEvent(title, &press);
        QCOMPARE(presses.count, 0);
        QVERIFY(Application::SwallowsDuringRestore(shell->GetTreeBank(), &enter));
        QVERIFY(Application::SwallowsDuringRestore(shell->GetTreeBank(), &drop));

        Application::SetRestoringWindows(false);
        QApplication::sendEvent(title, &press);
        QCOMPARE(presses.count, 1);
        QVERIFY(!Application::SwallowsDuringRestore(shell->GetTreeBank(), &drop));
    }
};

int main(int argc, char **argv){
    Application app(argc, argv);
    tst_startupwindows test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_startupwindows.moc"
