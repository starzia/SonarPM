#include "PlotPane.hpp"
#include "App.hpp" // for constants

#include <iostream>
using namespace std;

BEGIN_EVENT_TABLE( PlotPane, wxPanel )
EVT_PAINT( PlotPane::paintEvent )
END_EVENT_TABLE()

PlotPane::PlotPane( wxWindow* parent, wxPoint pos, wxSize size ) :
wxPanel(parent, wxID_ANY, pos, size ){}

void PlotPane::paintEvent(wxPaintEvent & evt){
  wxPaintDC dc(this);
  render(dc);
}
void PlotPane::paintNow(){
  wxClientDC dc(this);
  render(dc);
}

void PlotPane::render(wxDC&  dc){
  dc.SetBackground( *wxWHITE_BRUSH );
  dc.Clear();

  // draw Plot lines
  float min=1e32, max=-1e32;
  unsigned int i, N = sonar_history.size();
  for( i=0; i < N; i++ ){
    if( sonar_history[i]  < min ) min = sonar_history[i];
    if( thresh_history[i] < min ) min = thresh_history[i];
    if( window_history[i] < min ) min = window_history[i];

    if( sonar_history[i]  > max ) max = sonar_history[i];
    if( thresh_history[i] > max ) max = thresh_history[i];
    if( window_history[i] > max ) max = window_history[i];
  }
  if( min > 0 ) min = 0; // force min to be zero.

  this->drawLinePlot( dc, sonar_history, wxColor(200,200,255), 1, min, max );
  this->drawLinePlot( dc, thresh_history, wxColor(255,0,0), 2, min, max );
  this->drawLinePlot( dc, window_history, wxColor(0,0,0), 4, min, max );
}

void PlotPane::drawLinePlot( wxDC& dc, std::deque<float>& values,
                             wxColor color, unsigned int lineThickness,
                             float min, float max ){
  if( min == max ) return; // avoid divide by zero
  wxCoord w, h;
  dc.GetSize(&w, &h);

  dc.SetPen( wxPen( color, lineThickness ) );
  unsigned int i, N = values.size();
  float x_stride =  ((float)w)/(N-1), y_stride = ((float)h)/(max-min);
  for( i=1; i < N; i++ ){
    // check that value is not NaN
    if( (values[i] >= 0) && ( values[i-1] >= 0 ) ){ // test for NaN
      dc.DrawLine( w - (i-1)*x_stride, h - y_stride*(values[i-1]-min),
                   w - (i)*x_stride, h - y_stride*(values[i]-min) );
    }
  }
}

void PlotPane::setHistoryLength( unsigned int l ){
  unsigned int currentLength = this->sonar_history.size();
  this->sonar_history.resize( l, 0.0/0.0 ); // make new entries NaN
  this->window_history.resize( l, 0.0/0.0 );
  this->thresh_history.resize( l, 0.0/0.0 );
}

// add a new value to the plot and push off the oldest value
void PlotPane::addPoint( float sonar, float window_avg, float thresh ){
  this->sonar_history.push_front( sonar );
  this->sonar_history.pop_back();
  this->thresh_history.push_front(thresh);
  this->thresh_history.pop_back();
  this->window_history.push_front( window_avg );
  this->window_history.pop_back();

  this->paintNow();
}