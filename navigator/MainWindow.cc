#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QTreeView>
#include <QListView>
#include <QTextEdit>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include <cassert>

#include "File.h"
#include "FindBar.h"
#include "FileManager.h"
#include "FolderWidget.h"
#include "Project.h"
#include "Regex.h"
#include "ReportDefList.h"
#include "ReportFileList.h"
#include "ReportRefList.h"
#include "ReportSymList.h"
#include "SourceWidget.h"
#include "TableReport.h"
#include "TableReportWindow.h"
#include "ui_MainWindow.h"

namespace Nav {

const int kDefaultSideBarSizePx = 300;
const Qt::FocusPolicy kDefaultSourceWidgetFocusPolicy = Qt::WheelFocus;

MainWindow *theMainWindow;

MainWindow::MainWindow(Project &project, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setUnifiedTitleAndToolBarOnMac(true);

    // Left pane: folder widget.
    m_folderWidget = new FolderWidget(project.fileManager());
    m_folderWidget1 = new FolderWidget(project.fileManager());
    m_folderWidget2 = new FolderWidget(project.fileManager());
    m_folderWidget3 = new FolderWidget(project.fileManager());
    m_folderWidget4 = new FolderWidget(project.fileManager());
    //m_folderWidget5 = new FolderWidget(project.fileManager());

    m_globalDefinitionsWidget = on_actionBrowseGlobalDefinitions_triggered();
    m_symbolsWidget = on_actionBrowseSymbols_triggered();
    m_sourceWidget1 = new SourceWidget(project);
    m_sourceWidget1->setFocusPolicy(kDefaultSourceWidgetFocusPolicy);
    m_sourceWidget2 = new SourceWidget(project);
    m_sourceWidget2->setFocusPolicy(kDefaultSourceWidgetFocusPolicy);

    // Find bar.
    m_findBar = new FindBar;
    m_findBar->setVisible(false);

    // Right pane: source panel and find box.
    QWidget *sourcePane = new QWidget;
    {
        QVBoxLayout *sourcePaneLayout = new QVBoxLayout(sourcePane);
        sourcePaneLayout->setMargin(0);
        sourcePaneLayout->setSpacing(0);
        m_sourceWidget = new SourceWidget(project);
        m_sourceWidget->setFocusPolicy(kDefaultSourceWidgetFocusPolicy);
        sourcePaneLayout->addWidget(m_sourceWidget);
        sourcePaneLayout->addWidget(m_findBar);
    }

    // Splitter.
    m_splitter = new QSplitter(Qt::Vertical); //the root splitter
    m_th_splitter = new QSplitter(Qt::Horizontal); //top horizontal splitter, where the main windows is.
    m_bh_splitter = new QSplitter(Qt::Horizontal); //bottom horizontal splitter. other context-sensitive windows here.
    m_th_lv_splitter = new QSplitter(Qt::Vertical); //top left vertical splitter, where folder view and symbol windows is.
    m_th_rv_splitter = new QSplitter(Qt::Vertical); //top right vertical splitter, TBD, can put symbol window and file list here temporarily.

    m_splitter->addWidget(m_th_splitter);
    m_splitter->addWidget(m_bh_splitter);

    m_th_splitter->addWidget(m_th_lv_splitter);
    m_th_splitter->addWidget(sourcePane);
    m_th_splitter->addWidget(m_th_rv_splitter);

    m_th_lv_splitter->addWidget(m_folderWidget);
    m_th_lv_splitter->addWidget(m_folderWidget1);

    m_th_rv_splitter->addWidget(m_globalDefinitionsWidget);
    m_th_rv_splitter->addWidget(m_symbolsWidget);

    m_bh_splitter->addWidget(m_sourceWidget1);
    m_bh_splitter->addWidget(m_sourceWidget2);
    //m_bh_splitter->addWidget(m_folderWidget5);
    //QSplitter *splitter = new QSplitter(Qt::Horizontal);
    //m_th_splitter->addWidget(splitter);

    //m_th_splitter->show();
    setCentralWidget(m_splitter);
    m_th_splitter->setStretchFactor(0, 0);
    m_th_splitter->setStretchFactor(1, 1);
    QList<int> sizes;
    sizes << kDefaultSideBarSizePx;
    sizes << 1;
    m_th_splitter->setSizes(sizes);

    // Tab width menu.
    for (int i = 1; i <= 8; ++i) {
        QAction *action = ui->menuViewTabWidth->addAction(
                    QString("Tab Width: &" + QString::number(i)));
        action->setCheckable(true);
        action->setChecked(i == m_sourceWidget->tabStopSize());
        action->setData(QVariant(i));
        connect(action, SIGNAL(triggered()), SLOT(actionTabStopSizeChanged()));
    }

    connect(m_sourceWidget,
            SIGNAL(fileChanged(File*)),
            SLOT(sourceWidgetFileChanged(File*)));
    connect(m_sourceWidget,
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)),
            SLOT(areBackAndForwardEnabled(bool&,bool&)));
    connect(m_folderWidget,
            SIGNAL(selectionChanged()),
            SLOT(folderWidgetSelectionChanged()));
    connect(m_sourceWidget, SIGNAL(goBack()), SLOT(actionBack()));
    connect(m_sourceWidget, SIGNAL(goForward()), SLOT(actionForward()));
    connect(m_sourceWidget, SIGNAL(copyFilePath()), SLOT(actionCopyFilePath()));
    connect(m_sourceWidget,
            SIGNAL(revealInSideBar()),
            SLOT(actionRevealInSideBar()));
    connect(m_sourceWidget,
            SIGNAL(findMatchListChanged()),
            SLOT(updateFindBarInfo()));
    connect(m_sourceWidget,
            SIGNAL(findMatchSelectionChanged(int)),
            SLOT(updateFindBarInfo()));
    connect(ui->actionFileExit, SIGNAL(triggered()), SLOT(close()));
    connect(ui->actionEditCopy, SIGNAL(triggered()),
            m_sourceWidget, SLOT(copy()));
    connect(m_findBar, SIGNAL(closeBar()), SLOT(onFindBarClose()));
    connect(m_findBar, SIGNAL(regexChanged()), SLOT(onFindBarRegexChanged()));
    connect(m_findBar, SIGNAL(previous()), m_sourceWidget, SLOT(selectPreviousMatch()));
    connect(m_findBar, SIGNAL(next()), m_sourceWidget, SLOT(selectNextMatch()));

    // Intercept some keyboard events directed at the FindBar and direct them
    // towards the SourceWidget instead.
    m_findBar->installEventFilter(this);

    QIcon backIcon = QIcon::fromTheme("go-previous");
    QIcon forwardIcon = QIcon::fromTheme("go-next");
    if (backIcon.isNull() || forwardIcon.isNull()) {
        backIcon = QIcon(":/icons/resultset_previous.png");
        forwardIcon = QIcon(":/icons/resultset_next.png");
    }
    QAction *backAction = ui->toolBar->addAction(
                backIcon, "Back", this, SLOT(actionBack()));
    QAction *forwardAction = ui->toolBar->addAction(
                forwardIcon, "Forward", this, SLOT(actionForward()));
#ifdef __APPLE__
    backAction->setShortcut(QKeySequence("Ctrl+["));
    forwardAction->setShortcut(QKeySequence("Ctrl+]"));
    backAction->setToolTip("Back (⌘[)");
    forwardAction->setToolTip("Forward (⌘])");
#else
    backAction->setShortcut(QKeySequence("Alt+Left"));
    forwardAction->setShortcut(QKeySequence("Alt+Right"));
    backAction->setToolTip("Back (Alt+Left)");
    forwardAction->setToolTip("Forward (Alt+Right)");
#endif

    ui->actionFileExit->setIcon(QIcon::fromTheme("application-exit"));
    ui->actionEditCopy->setIcon(QIcon::fromTheme("edit-copy"));
    ui->actionEditFind->setIcon(QIcon::fromTheme("edit-find"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

History::Location MainWindow::currentLocation()
{
    return History::Location(
                m_sourceWidget->file(),
                m_sourceWidget->viewportOrigin());
}

void MainWindow::navigateToFile(File *file)
{    if (file == NULL)
        return;
    History::Location loc = currentLocation();
    m_sourceWidget->setFile(file);
    m_history.recordJump(loc, currentLocation());
}

void MainWindow::navigateToRef(const Ref &ref)
{
    if (ref.isNull())
        return;
    History::Location loc = currentLocation();
    bool forceCenter = m_sourceWidget->file() != &ref.file();
    m_sourceWidget->setFile(&ref.file());
    m_sourceWidget->selectIdentifier(ref.line(),
                                     ref.column(),
                                     ref.endColumn(),
                                     forceCenter);
    m_history.recordJump(loc, currentLocation());
    ui->statusBar->showMessage(tr(ref.fileNameCStr()));
}

void MainWindow::navigateToRef1(const Ref &ref)
{
    if (ref.isNull())
        return;
    //History::Location loc = currentLocation();
    //bool forceCenter = m_sourceWidget1->file() != &ref.file();
    bool forceCenter = true;
    m_sourceWidget1->setFile(&ref.file());
    m_sourceWidget1->selectIdentifier(ref.line(),
                                      ref.column(),
                                      ref.endColumn(),
                                      forceCenter);
    //m_history.recordJump(loc, currentLocation());
    ui->statusBar->showMessage(tr(ref.fileNameCStr()));
}

void MainWindow::navigateToRef2(const Ref &ref)
{
    if (ref.isNull())
        return;
    //History::Location loc = currentLocation();
    //bool forceCenter = m_sourceWidget1->file() != &ref.file();
    bool forceCenter = true;
    m_sourceWidget2->setFile(&ref.file());
    m_sourceWidget2->selectIdentifier(ref.line(),
                                      ref.column(),
                                      ref.endColumn(),
                                      forceCenter);
    //m_history.recordJump(loc, currentLocation());
    ui->statusBar->showMessage(tr(ref.fileNameCStr()));
}

void MainWindow::on_actionEditFind_triggered()
{
    // On Linux, it appears that the first time a QLineEdit receives focus, Qt
    // does some kind of one-time initialization that takes ~50-300ms.  In the
    // code below, all of the time is spent in the setFocus call.  It appears
    // that QLineEdit::setFocus eventually calls
    // QXIMInputContext::setFocusWidget, where I suspect all of the time is
    // spent.

    m_findBar->setVisible(true);
    m_findBar->setFocus();
    m_findBar->selectAll();
    m_sourceWidget->setFocusPolicy(Qt::NoFocus);
    m_sourceWidget->recordFindStart();
    m_sourceWidget->setFindRegex(m_findBar->regex(), /*advanceToMatch=*/false);
}

void MainWindow::onFindBarClose()
{
    m_findBar->hide();
    m_sourceWidget->setFocusPolicy(kDefaultSourceWidgetFocusPolicy);
    m_sourceWidget->setFocus();
    m_sourceWidget->endFind();
}

void MainWindow::onFindBarRegexChanged()
{
    m_sourceWidget->setFindRegex(m_findBar->regex(), /*advanceToMatch=*/true);
}

void MainWindow::updateFindBarInfo()
{
    m_findBar->setMatchInfo(
                m_sourceWidget->selectedMatchIndex(),
                m_sourceWidget->matchCount());
}

void MainWindow::on_actionBrowseFiles_triggered()
{
    TableReportWindow *tw = new TableReportWindow;
    ReportFileList *r = new ReportFileList(*theProject, tw);
    tw->setTableReport(r);
    tw->setFilterBoxVisible(true);
    tw->show();
}

TableReportWindow *MainWindow::on_actionBrowseGlobalDefinitions_triggered()
{
    TableReportWindow *tw = new TableReportWindow;
    ReportDefList *r = new ReportDefList(*theProject, tw);
    tw->setTableReport(r);
    tw->setFilterBoxVisible(true);
    tw->resize(kReportDefListDefaultSize);
    tw->show();
    return tw;
}

TableReportWindow *MainWindow::on_actionBrowseSymbols_triggered()
{
    TableReportWindow *tw = new TableReportWindow;
    ReportSymList *r = new ReportSymList(*theProject, tw);
    tw->setTableReport(r);
    tw->setFilterBoxVisible(true);
    tw->resize(kReportSymListDefaultSize);
    tw->show();
    return tw;
}

void MainWindow::actionBack()
{
    if (m_history.canGoBack()) {
        m_history.recordLocation(currentLocation());
        const History::Location *loc = m_history.goBack();
        m_sourceWidget->setFile(loc->file);
        m_sourceWidget->setViewportOrigin(loc->offset);
    }
}

void MainWindow::actionForward()
{
    if (m_history.canGoForward()) {
        m_history.recordLocation(currentLocation());
        const History::Location *loc = m_history.goForward();
        m_sourceWidget->setFile(loc->file);
        m_sourceWidget->setViewportOrigin(loc->offset);
    }
}

void MainWindow::sourceWidgetFileChanged(File *file)
{
    setWindowTitle(file->path() + " - SourceWeb");
    m_folderWidget->selectFile(file);
}

void MainWindow::folderWidgetSelectionChanged()
{
    File *file = m_folderWidget->selectedFile();
    navigateToFile(file);
}

void MainWindow::areBackAndForwardEnabled(
        bool &backEnabled, bool &forwardEnabled)
{
    backEnabled = m_history.canGoBack();
    forwardEnabled = m_history.canGoForward();
}

void MainWindow::actionCopyFilePath()
{
    File *file = m_sourceWidget->file();
    if (file != NULL)
        QApplication::clipboard()->setText(file->path());
}

void MainWindow::actionRevealInSideBar()
{
    File *file = m_sourceWidget->file();
    if (file != NULL) {
        if (m_th_splitter->sizes()[0] == 0) {
            QList<int> sizes;
            sizes << kDefaultSideBarSizePx;
            sizes << 1;
            m_th_splitter->setSizes(sizes);
        }
        m_folderWidget->ensureItemVisible(file);
    }
}

void MainWindow::actionTabStopSizeChanged()
{
    {
        QAction *action = qobject_cast<QAction*>(QObject::sender());
        assert(action != NULL && "Tab stop action is NULL.");
        int size = action->data().toInt();
        assert(size >= 1 && "Tab stop value is not positive.");
        m_sourceWidget->setTabStopSize(size);
    }

    for (QAction *action : ui->menuViewTabWidth->actions()) {
        int size = action->data().toInt();
        assert(size >= 1 && "Tab stop value is not positive.");
        action->setChecked(size == m_sourceWidget->tabStopSize());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        const int key = keyEvent->key();
        const bool ctrl = keyEvent->modifiers() & Qt::ControlModifier;
        if (key == Qt::Key_Up ||
                key == Qt::Key_Down ||
                key == Qt::Key_PageUp ||
                key == Qt::Key_PageDown ||
                (ctrl && key == Qt::Key_Home) ||
                (ctrl && key == Qt::Key_End)) {
            QApplication::sendEvent(m_sourceWidget, event);
            return true;
        }
    }
    return false;
}

} // namespace Nav
