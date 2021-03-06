// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#ifndef CURVES_H
#define CURVES_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP	
#include "wx/wx.h"

#include "Spline/Spline.h"
#include "wx/dcbuffer.h"
#endif

#include "GUI/Colors/Colors.h"
#include "Debugging/MemoryLeakCheck.h"

enum {
	CURVE_CHANNEL_BRIGHT,
	CURVE_CHANNEL_RED,
	CURVE_CHANNEL_GREEN,
	CURVE_CHANNEL_BLUE,
	CURVE_CHANNEL_BLUE_TO_YELLOW,
	CURVE_CHANNEL_GREEN_TO_RED,
	CURVE_CHANNEL_GREY_TO_WHITE,
	CURVE_CHANNEL_HUE
};

enum{
	ID_CURVE_CHANGED
};

wxDECLARE_EVENT(CURVE_CHANGED_EVENT, wxCommandEvent);

class CurvePanel : public wxPanel {

public:
	CurvePanel(wxWindow * Parent, int Channel);
	void GetColorCurveMap(int numSteps, int * outCurve, float scale);
	void SetControlPoints(wxVector<Point> newPoints);
	wxVector<Point> GetControlPoints();
	~CurvePanel();

private:

	wxWindow * par;

	int curvePaddingSize;
	int extraLefttX;
	int extraLefttY;
	int channelColor;

	wxVector<Point> lastPoints;
	Spline * splineCurve;
	Spline * displayCurve;
		
	void RightClick(wxMouseEvent& evt);
	void LeftClick(wxMouseEvent& evt);
	void PaintEvent(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& WXUNUSED(evt));
	void Render(wxDC& dc);
	void PaintNow();
	void DestroySpline();
	bool CheckForChanges();
	wxVector<Point> ClipCurve(wxVector<Point> points);
	wxColour GetRGBFromHue(double degree);

};

#endif