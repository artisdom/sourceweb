#ifndef NAV_MAINWINDOW_H
#define NAV_MAINWINDOW_H

#include <QFontMetricsF>
#include <QMainWindow>
#include <QMap>
#include <QSplitter>
#include <QTreeView>
#include <QWidget>

#include "History.h"
#include "TextWidthCalculator.h"
#include "TableReportWindow.h"

namespace Nav {

namespace Ui {
class MainWindow;
}

class File;
class FindBar;
class FolderWidget;
class MainWindow;
class Project;
class Ref;
class SourceWidget;

extern MainWindow *theMainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Project &project, QWidget *parent = 0);
    ~MainWindow();
    History::Location currentLocation();
    void navigateToFile(File *file);
    void navigateToRef(const Ref &ref);
    void navigateToRef1(const Ref &ref);

private slots:
    void on_actionEditFind_triggered();
    void onFindBarClose();
    void onFindBarRegexChanged();
    void updateFindBarInfo();
    void on_actionBrowseFiles_triggered();
    TableReportWindow *on_actionBrowseGlobalDefinitions_triggered();
    TableReportWindow *on_actionBrowseSymbols_triggered();
    void actionBack();
    void actionForward();
    void sourceWidgetFileChanged(File *file);
    void folderWidgetSelectionChanged();
    void areBackAndForwardEnabled(bool &backEnabled, bool &forwardEnabled);
    void actionCopyFilePath();
    void actionRevealInSideBar();
    void actionTabStopSizeChanged();

private:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    History m_history;
    Ui::MainWindow *ui;
    QSplitter *m_splitter;
    QSplitter *m_th_splitter;
    QSplitter *m_bh_splitter;
    QSplitter *m_th_lv_splitter;
    QSplitter *m_th_rv_splitter;

    FolderWidget *m_folderWidget;
    SourceWidget *m_sourceWidget;
    FindBar *m_findBar;

    FolderWidget *m_folderWidget1;
    FolderWidget *m_folderWidget2;
    FolderWidget *m_folderWidget3;

    FolderWidget *m_folderWidget4;
    FolderWidget *m_folderWidget5;

    TableReportWindow *m_globalDefinitionsWidget;
    TableReportWindow *m_symbolsWidget;
    SourceWidget *m_sourceWidget1;
};

} // namespace Nav

#endif // NAV_MAINWINDOW_H
