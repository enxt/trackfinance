#ifndef TRACKFINANCE_H
#define TRACKFINANCE_H

#include <QDialog>
#include <QScrollBar>
#include "qchartviewer.h"


class TrackFinance : public QDialog {
    Q_OBJECT
public:
    TrackFinance(QWidget *parent = 0);
    ~TrackFinance();

private:
    // Create a finance chart demo containing 100 days of data
    int noOfDays = 100;

    // To compute moving averages starting from the first day, we need to get extra data points before
    // the first day
    int extraDays = 30;

    RanTable *rantable;
    DoubleArray timeStamps;
    DoubleArray highData;
    DoubleArray lowData;
    DoubleArray openData;
    DoubleArray closeData;
    DoubleArray volData;

    QChartViewer *m_ChartViewer;
    QScrollBar *m_HScrollBar;

    void loadData();                                // Load data into data arrays
    void initChartViewer(QChartViewer *viewer);     // Initialize the QChartViewer
    void drawChart(QChartViewer *viewer);            // Draw chart
    void trackFinance(MultiChart *m, int mouseX);    // Draw Track Cursor
    void updateControls(QChartViewer *viewer);      // Update other controls

private slots:
    void onHScrollBarChanged(int value);
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent *event);
    void onMouseWheelChart(QWheelEvent *event);
};

#endif // TRACKFINANCE_H
