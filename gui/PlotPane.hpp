#pragma once
#include <wx/wx.h>
#include <deque>

class PlotPane : public wxPanel{
public:
  PlotPane(wxFrame* parent, wxPoint pos, wxSize size);
  void paintEvent(wxPaintEvent & evt);
  void paintNow();
  // set x axis length of plot
  void setHistoryLength( unsigned int l );
  // add a new value to the plot and push off the oldest value
  void addPoint( float p );

  DECLARE_EVENT_TABLE()
  
private:
  void render( wxDC& dc );
  std::deque<float> history;
};
