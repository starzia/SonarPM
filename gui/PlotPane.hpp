#pragma once
#include <wx/wx.h>
#include <deque>

class PlotPane : public wxPanel{
public:
  PlotPane(wxWindow* parent, wxPoint pos, wxSize size);
  void paintEvent(wxPaintEvent & evt);
  void paintNow();
  // set x axis length of plot
  void setHistoryLength( unsigned int l );
  // add a new value to the plot and push off the oldest value
  void addPoint( float sonar, float window_avg );
  // change threshold line height;
  void setThreshold( float t );

  static const unsigned int HISTORY_WINDOW=60; // default xyange of plot

  DECLARE_EVENT_TABLE()
  
private:
  void render( wxDC& dc );
  /** helper function to draw a line graph on a dc */
  void drawLinePlot( wxDC& dc, std::deque<float>& values,
                            wxColor color, unsigned int lineThickness,
                            float min_y, float max_y );

  std::deque<float> sonar_history; // sonar readings
  std::deque<float> window_history; // sliding window averages
  std::deque<float> thresh_history; // threshold history
  float threshold; // horizontal line to be plotted
};
