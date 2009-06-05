#include "PlotPane.hpp"

BEGIN_EVENT_TABLE( PlotPane, wxPanel )
EVT_PAINT( PlotPane::paintEvent )
END_EVENT_TABLE()

PlotPane::PlotPane( wxFrame* parent, wxPoint pos, wxSize size ) : 
wxPanel(parent, wxID_ANY, pos, size ) {
  this->threshold=0;
}

void PlotPane::paintEvent(wxPaintEvent & evt){
  wxPaintDC dc(this);
  render(dc);
}
void PlotPane::paintNow(){
  wxClientDC dc(this);
  render(dc);
}

void PlotPane::render(wxDC&  dc)
{
  dc.SetBackground( *wxWHITE_BRUSH );
  dc.Clear();
  wxCoord w, h;
  dc.GetSize(&w, &h);

  // draw Plot lines
  dc.SetPen( wxPen( wxColor(0,0,0), 3 ) ); // black line, 3 pixels thick
  int N = history.size();
  unsigned int i;
  float min=1e32, max=-1e32;
  for( i=0; i < N; i++ ){
    if( history[i] < min ) min = history[i];
    if( history[i] > max ) max = history[i];
  }
  if( min > 0 ) min = 0; // force min to be zero.
  if( max < this->threshold ) max = this->threshold; //thresh is always visible
  float x_stride =  ((float)w)/(N-1), y_stride = ((float)h)/(max-min);
  for( i=1; i < N; i++ ){
    dc.DrawLine( w - (i-1)*x_stride, h - y_stride*(history[i-1]-min), 
		 w - (i)*x_stride, h - y_stride*(history[i]-min) );
  }

  // draw threshold line
  dc.SetPen( wxPen( wxColor(255,0,0), 1 ) ); // red line, 1 pixels thick
  dc.DrawLine( 0, h - y_stride*(this->threshold-min),
	       x_stride*(this->history.size()-1), 
	       h - y_stride*(this->threshold-min) );
  /*
  // zoom to fit?
  double scaleX=(double)(dc.MaxX()/w);
  double scaleY=(double)(dc.MaxY()/h);
  dc.SetUserScale(std::min(scaleX,scaleY),std::min(scaleX,scaleY));

  dc.SetPen( wxPen( wxColor(0,0,0), 3 ) ); // black line, 3 pixels thick
  for( i=1; i < this->history.size(); i++ ){
    dc.DrawLine( i-1, this->history[i-1], i, this->history[i] );
  }
  */
}

void PlotPane::setHistoryLength( unsigned int l ){
  unsigned int currentLength = this->history.size();
  this->history.resize( l, 0 ); // make new entries 0
}

// add a new value to the plot and push off the oldest value
void PlotPane::addPoint( float p ){
  this->history.push_front(p);
  this->history.pop_back();
  this->paintNow();
}

void PlotPane::setThreshold( float t ){
  this->threshold = t;
  this->paintNow();
}
