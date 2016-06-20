#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "plotwidget.h"
#include "plotmatrix.h"
#include "filterablelistwidget.h"
#include "tabbedplotwidget.h"

#include "../plugins/dataloader_base.h"
#include "../plugins/statepublisher_base.h"
#include "../plugins/datastreamer_base.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_undoableChange();

private slots:

    void onTrackerTimeUpdated(double current_time );

    void onTrackerPositionUpdated(QPointF pos );

    void on_splitter_splitterMoved(int, int);

    void resizeEvent(QResizeEvent *) ;

    void on_plotAdded(PlotWidget* plot);

    void onActionSaveLayout();

    void onActionLoadLayout(bool reload_previous = false);

    void onActionLoadDataFile(bool reload_from_settings = false);

    void onActionLoadDataFile(QString filename);

    void onActionReloadDataFileFromSettings();

    void onActionReloadSameDataFile();

    void onActionReloadLayout();

    void onActionLoadStreamer();

    void on_pushButtonActivateTracker_toggled(bool checked);

    void on_UndoInvoked();

    void on_RedoInvoked();

    void on_horizontalSlider_sliderMoved(int position);

    void on_tabbedAreaDestroyed(QObject*object);

    void on_floatingWindowDestroyed(QObject*object);

    void on_createFloatingWindow(PlotMatrix* first_tab = NULL);

    void on_pushButtonAddSubwindow_pressed();

    void on_swapPlots(PlotWidget* source, PlotWidget* destination);

    void on_pushButtonStreaming_toggled(bool checked);

    void on_replotRequested();

    void on_streamingSpinBox_valueChanged(int value);

private:
    Ui::MainWindow *ui;

    std::vector<TabbedPlotWidget *> _tabbed_plotarea;
    std::vector<QMainWindow *>      _floating_window;

    QAction* _action_loadRecentFile;
    QAction* _action_reloadFile;
    QAction* _action_loadRecentLayout;

    QAction* _action_SaveLayout;
    QAction* _action_LoadLayout;
    QAction* _action_LoadData;

    QAction* _action_Undo;
    QAction* _action_Redo;

    QAction* _action_startDataStream;

    void createActions();

    FilterableListWidget* curvelist_widget;

    std::vector<PlotMatrix*> _plot_matrix_list;

    void updateInternalState();

    void getMaximumRangeX(double* minX, double* maxX);

    void buildData();

    PlotDataMap    _mapped_plot_data;

    void rearrangeGridLayout();

    QColor colorHint();

    void loadPlugins(QString subdir_name);

    std::map<QString,DataLoader*> data_loader;
    std::vector<StatePublisher*>  state_publisher;
    std::vector<DataStreamer*>    data_streamer;

    QDomDocument xmlSaveState();
    bool xmlLoadState(QDomDocument state_document);

    std::deque<QDomDocument> _undo_states;
    std::deque<QDomDocument> _redo_states;

    QElapsedTimer _undo_timer;

    QString _loaded_datafile;

    void createTabbedDialog(PlotMatrix *first_tab, bool undoable);

protected:
    void mousePressEvent(QMouseEvent *event) ;

    void dragEnterEvent(QDragEnterEvent *event) ;

    void deleteLoadedData();
    void deleteLoadedData(const QString &curve_name);

    QTimer *_replot_timer;

};

#endif // MAINWINDOW_H
