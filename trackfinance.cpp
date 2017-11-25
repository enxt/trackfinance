#include <QApplication>
#include <QMouseEvent>
#include <QDebug>
#include "FinanceChart.h"
#include "trackfinance.h"
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    TrackFinance demo;
    demo.show();
    return app.exec();
}


TrackFinance::TrackFinance(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Finance Chart Track Line");

    // Create the QChartViewer and draw the chart
    m_ChartViewer = new QChartViewer(this);

    // Load the data
//    loadData();

//    drawChart(m_ChartViewer);

    // Set the window to be of the same size as the chart
//    setFixedSize(m_ChartViewer->width(), m_ChartViewer->height()+20);
    setFixedSize(720, 500);

    // Horizontal scroll bar
    m_HScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_HScrollBar->setGeometry(40, 470, 650, 17);
    connect(m_HScrollBar, SIGNAL(valueChanged(int)), SLOT(onHScrollBarChanged(int)));

    // Set up the mouseMovePlotArea handler for drawing the track cursor
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)), SLOT(onMouseMovePlotArea(QMouseEvent*)));

    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseWheel(QWheelEvent*)), SLOT(onMouseWheelChart(QWheelEvent*)));

    // Load the data
    loadData();

    // Initialize the QChartViewer
    initChartViewer(m_ChartViewer);

    // Initially set the mouse to drag to scroll mode
//    pointerPB->click();
    m_ChartViewer->setMouseUsage(Chart::MouseUsageScroll);

    // Trigger the ViewPortChanged event to draw the chart
    m_ChartViewer->updateViewPort(true, true);
}

TrackFinance::~TrackFinance()
{
    delete rantable;
    delete m_ChartViewer->getChart();
}

//
// Load the data
//
void TrackFinance::loadData()
{
    // In this exammple, we use a random number generator utility to simulate the data. We set up the
    // random table to create 6 cols x (noOfDays + extraDays) rows, using 9 as the seed.
    rantable = new RanTable(9, 6, noOfDays + extraDays);

    // Set the 1st col to be the timeStamp, starting from Sep 4, 2011, with each row representing one
    // day, and counting week days only (jump over Sat and Sun)
    rantable->setDateCol(0, Chart::chartTime(2011, 9, 4), 86400, true);

    // Set the 2nd, 3rd, 4th and 5th columns to be high, low, open and close data. The open value
    // starts from 100, and the daily change is random from -5 to 5.
    rantable->setHLOCCols(1, 100, -5, 5);

    // Set the 6th column as the vol data from 5 to 25 million
    rantable->setCol(5, 50000000, 250000000);

    // Now we read the data from the table into arrays
    timeStamps = rantable->getCol(0);
    highData = rantable->getCol(1);
    lowData = rantable->getCol(2);
    openData = rantable->getCol(3);
    closeData = rantable->getCol(4);
    volData = rantable->getCol(5);
}

//
// Initialize the QChartViewer
//
void TrackFinance::initChartViewer(QChartViewer *viewer)
{
    // Set the full x range to be the duration of the data
    viewer->setFullRange("x", timeStamps[0], timeStamps[timeStamps.len - 1]);

    // Initialize the view port to show the latest 20% of the time range
    viewer->setViewPortWidth(0.5);
    viewer->setViewPortLeft(1 - viewer->getViewPortWidth());

    // Set the maximum zoom to 10 points
    viewer->setZoomInWidthLimit(30.0 / timeStamps.len);
}

//
// The ViewPortChanged event handler. This event occurs if the user scrolls or zooms in
// or out the chart by dragging or clicking on the chart. It can also be triggered by
// calling WinChartViewer.updateViewPort.
//
void TrackFinance::onViewPortChanged()
{
    // In addition to updating the chart, we may also need to update other controls that
    // changes based on the view port.
    updateControls(m_ChartViewer);

    // Update chart if necessary
    if (m_ChartViewer->needUpdateChart())
        drawChart(m_ChartViewer);
}

//
// Update controls in the user interface when the view port changed
//
void TrackFinance::updateControls(QChartViewer *viewer)
{
    // The logical length of the scrollbar. It can be any large value. The actual value does
    // not matter.
    const int scrollBarLen = 1000000000;

    // Update the horizontal scroll bar
    m_HScrollBar->setEnabled(viewer->getViewPortWidth() < 1);
    m_HScrollBar->setPageStep((int)ceil(viewer->getViewPortWidth() * scrollBarLen));
    m_HScrollBar->setSingleStep(min(scrollBarLen / 100, m_HScrollBar->pageStep()));
    m_HScrollBar->setRange(0, scrollBarLen - m_HScrollBar->pageStep());
    m_HScrollBar->setValue((int)(0.5 + viewer->getViewPortLeft() * scrollBarLen));
}

//
// Draw the chart and display it in the given viewer
//
void TrackFinance::drawChart(QChartViewer *viewer)
{
    // Get the start date and end date that are visible on the chart.
    double viewPortStartDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft());
    double viewPortEndDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft() + viewer->getViewPortWidth());

    // Get the array indexes that corresponds to the visible start and end dates
    int startIndex = (int)floor(Chart::bSearch(timeStamps, viewPortStartDate));
    int endIndex = (int)ceil(Chart::bSearch(timeStamps, viewPortEndDate));
    int noOfPoints = endIndex - startIndex + 1;

    // Extract the part of the data array that are visible.
    DoubleArray viewPortTimeStamps = DoubleArray(timeStamps.data + startIndex, noOfPoints);
    DoubleArray viewPortDataHigh = DoubleArray(highData.data + startIndex, noOfPoints);
    DoubleArray viewPortDataLow = DoubleArray(lowData.data + startIndex, noOfPoints);
    DoubleArray viewPortDataOpen = DoubleArray(openData.data + startIndex, noOfPoints);
    DoubleArray viewPortDataClose = DoubleArray(closeData.data + startIndex, noOfPoints);
    DoubleArray viewPortDataVolume = DoubleArray(volData.data + startIndex, noOfPoints);

    //
    // At this stage, we have extracted the visible data. We can use those data to plot the chart.
    //


    // Create a FinanceChart object of width 720 pixels
    FinanceChart *c = new FinanceChart(720);

    // Add a title to the chart
    c->addTitle("Finance Chart Demonstration");

    // Disable default legend box, as we are using dynamic legend
    c->setLegendStyle("normal", 8, Chart::Transparent, Chart::Transparent);

    // Set the data into the finance chart object
    c->setData(viewPortTimeStamps, viewPortDataHigh, viewPortDataLow, viewPortDataOpen, viewPortDataClose, viewPortDataVolume, extraDays);

    // Add the main chart with 240 pixels in height
    c->addMainChart(240);

    // Add a 10 period simple moving average to the main chart, using brown color
//    c->addSimpleMovingAvg(10, 0x663300);

    // Add a 20 period simple moving average to the main chart, using purple color
    c->addSimpleMovingAvg(20, 0x9900ff);

    // Add candlestick symbols to the main chart, using green/red for up/down days
    c->addCandleStick(0x00ff00, 0xff0000);

    // Add 20 days bollinger band to the main chart, using light blue (9999ff) as the border and
    // semi-transparent blue (c06666ff) as the fill color
//    c->addBollingerBand(20, 2, 0x9999ff, 0xc06666ff);

    // Add a 75 pixels volume bars sub-chart to the bottom of the main chart, using green/red/grey for
    // up/down/flat days
    c->addVolBars(75, 0x99ff99, 0xff9999, 0x808080);

    // Append a 14-days RSI indicator chart (75 pixels high) after the main chart. The main RSI line
    // is purple (800080). Set threshold region to +/- 20 (that is, RSI = 50 +/- 25). The upper/lower
    // threshold regions will be filled with red (ff0000)/blue (0000ff).
    c->addRSI(75, 14, 0x800080, 20, 0xff0000, 0x0000ff);

    // Append a MACD(26, 12) indicator chart (75 pixels high) after the main chart, using 9 days for
    // computing divergence.
    c->addMACD(75, 26, 12, 9, 0x0000ff, 0xff00ff, 0x008000);

    // Include track line with legend for the latest data values
    trackFinance(c, ((XYChart *)c->getChart(0))->getPlotArea()->getRightX());
    
    // Set the chart image to the QChartViewer
    viewer->setChart(c);
}

//
// User clicks on the the horizontal scroll bar
//
void TrackFinance::onHScrollBarChanged(int value)
{
    if (!m_ChartViewer->isInViewPortChangedEvent())
    {
        // Set the view port based on the scroll bar
        int scrollBarLen = m_HScrollBar->maximum() + m_HScrollBar->pageStep();
        m_ChartViewer->setViewPortLeft(value / (double)scrollBarLen);

        // Update the chart display without updating the image maps. (We can delay updating
        // the image map until scrolling is completed and the chart display is stable.)
        m_ChartViewer->updateViewPort(true, false);
    }
}

//
// When the mouse enters the chart, we will generate an image map for hot spots and tooltips
// support if it has not already been generated.
//
void TrackFinance::onMouseWheelChart(QWheelEvent *event)
{
    // Process the mouse wheel only if the mouse is over the plot area
    if (!m_ChartViewer->isMouseOnPlotArea())
    {
        event->ignore();
        return;
    }

    // We zoom in or out by 10% depending on the mouse wheel direction.
    double newVpWidth = m_ChartViewer->getViewPortWidth() * (event->delta() > 0 ? 0.9 : 1 / 0.9);
    double newVpHeight = m_ChartViewer->getViewPortHeight() * (event->delta() > 0 ? 0.9 : 1 / 0.9);

    // We do not zoom beyond the zoom width or height limits.
    newVpWidth = max(m_ChartViewer->getZoomInWidthLimit(), min(newVpWidth, m_ChartViewer->getZoomOutWidthLimit()));
    newVpHeight = max(m_ChartViewer->getZoomInHeightLimit(), min(newVpWidth, m_ChartViewer->getZoomOutHeightLimit()));

    if ((newVpWidth != m_ChartViewer->getViewPortWidth()) || (newVpHeight != m_ChartViewer->getViewPortHeight()))
    {
        // Set the view port position and size so that the point under the mouse remains under
        // the mouse after zooming.

        double deltaX = (m_ChartViewer->getPlotAreaMouseX() - m_ChartViewer->getPlotAreaLeft()) * (m_ChartViewer->getViewPortWidth() - newVpWidth) / m_ChartViewer->getPlotAreaWidth();
        m_ChartViewer->setViewPortLeft(m_ChartViewer->getViewPortLeft() + deltaX);
        m_ChartViewer->setViewPortWidth(newVpWidth);

        double deltaY = (m_ChartViewer->getPlotAreaMouseY() - m_ChartViewer->getPlotAreaTop()) * (m_ChartViewer->getViewPortHeight() - newVpHeight) / m_ChartViewer->getPlotAreaHeight();
        m_ChartViewer->setViewPortTop(m_ChartViewer->getViewPortTop() + deltaY);
        m_ChartViewer->setViewPortHeight(newVpHeight);

        m_ChartViewer->updateViewPort(true, false);
    }
}

//
// Draw track cursor when mouse is moving over plotarea
//
void TrackFinance::onMouseMovePlotArea(QMouseEvent *)
{
    trackFinance((MultiChart *)m_ChartViewer->getChart(), m_ChartViewer->getPlotAreaMouseX());
    m_ChartViewer->updateDisplay();
}

//
// Draw finance chart track line with legend
//
void TrackFinance::trackFinance(MultiChart *m, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = m->initDynamicLayer();

    // It is possible for a FinanceChart to be empty, so we need to check for it.
    if (m->getChartCount() == 0)
        return ;

    // Get the data x-value that is nearest to the mouse
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));

    // Iterate the XY charts (main price chart and indicator charts) in the FinanceChart
    XYChart *c = 0;
    for(int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);

        // Variables to hold the legend entries
        ostringstream ohlcLegend;
        vector<string> legendEntries;

        // Iterate through all layers to find the highest data point
        for(int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();

            // In a FinanceChart, only layers showing OHLC data can have 4 data sets
            if (dataSetCount == 4) {
                double highValue = layer->getDataSet(0)->getValue(xIndex);
                double lowValue = layer->getDataSet(1)->getValue(xIndex);
                double openValue = layer->getDataSet(2)->getValue(xIndex);
                double closeValue = layer->getDataSet(3)->getValue(xIndex);

                if (closeValue != Chart::NoValue) {
                    // Build the OHLC legend
					ohlcLegend << "      <*block*>";
					ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
					ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}"); 
					ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}"); 
					ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");

                    // We also draw an upward or downward triangle for up and down days and the %
                    // change
                    double lastCloseValue = layer->getDataSet(3)->getValue(xIndex - 1);
                    if (lastCloseValue != Chart::NoValue) {
                        double change = closeValue - lastCloseValue;
                        double percent = change * 100 / closeValue;
                        string symbol = (change >= 0) ?
                            "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                            "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";

                        ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
						ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                    }

					ohlcLegend << "<*/*>";
                }
            } else {
                // Iterate through all the data sets in the layer
                for(int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet *dataSet = layer->getDataSetByZ(k);

                    string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {

                        // In a FinanceChart, the data set name consists of the indicator name and its
                        // latest value. It is like "Vol: 123M" or "RSI (14): 55.34". As we are
                        // generating the values dynamically, we need to extract the indictor name
                        // out, and also the volume unit (if any).

						// The volume unit
						string unitChar;

                        // The indicator name is the part of the name up to the colon character.
						int delimiterPosition = (int)name.find(':');
                        if ((int)name.npos != delimiterPosition) {
							
							// The unit, if any, is the trailing non-digit character(s).
							int lastDigitPos = (int)name.find_last_of("0123456789");
                            if (((int)name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size())
                                && (lastDigitPos > delimiterPosition))
								unitChar = name.substr(lastDigitPos + 1);

							name.resize(delimiterPosition);
                        }

                        // In a FinanceChart, if there are two data sets, it must be representing a
                        // range.
                        if (dataSetCount == 2) {
                            // We show both values in the range in a single legend entry
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            name = name + ": " + c->formatValue(min(value, value2), "{value|P3}");
							name = name + " - " + c->formatValue(max(value, value2), "{value|P3}");
                        } else {
                            // In a FinanceChart, only the layer for volume bars has 3 data sets for
                            // up/down/flat days
                            if (dataSetCount == 3) {
                                // The actual volume is the sum of the 3 data sets.
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }

                            // Create the legend entry
                            name = name + ": " + c->formatValue(value, "{value|P3}") + unitChar;
                        }

                        // Build the legend entry, consist of a colored square box and the name (with
                        // the data value in it).
						ostringstream legendEntry;
						legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color=" 
							<< hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }

        // Get the plot area position relative to the entire FinanceChart
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();

		// The legend begins with the date label, then the ohlcLegend (if any), and then the
		// entries for the indicators.
		ostringstream legendText;
		legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5) 
			<< "*><*font=arialbd.ttf*>[" << c->xAxis()->getFormattedLabel(xValue, "mmm dd, yyyy")
			<< "]<*/font*>" << ohlcLegend.str();
		for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
			legendText << "      " << legendEntries[i];
		}
		legendText << "<*/*>";

        // Draw a vertical track line at the x-position
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, 0x0101));

        // Display the legend on the top of the plot area
        TTFText *t = d->text(legendText.str().c_str(), "arial.ttf", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 3, 0x000000, Chart::TopLeft);
		t->destroy();
    }
}
