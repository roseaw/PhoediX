// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "CurvesPanel.h"

wxDEFINE_EVENT(CURVE_CHANGED_EVENT, wxCommandEvent);

CurvePanel::CurvePanel(wxWindow * parent, int channel) :wxPanel(parent) {

	#if defined(__WXMSW__) || defined(__WXGTK__)
		this->SetDoubleBuffered(true);
	#endif

	par = parent;

	this->SetMinSize(wxSize(50, 50));
	curvePaddingSize = 7;
	channelColor = channel;

	// Create a spline with 1000  points between each control point
	displayCurve = new Spline(1000, true);

	// add the left and right control points
	displayCurve->AddPoint(0.0, 1.0);
	displayCurve->AddPoint(1.0, 0.0);

	PaintNow();

	this->Bind(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CurvePanel::RightClick, this);
	this->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CurvePanel::LeftClick, this);
	this->Bind(wxEVT_SIZE, (wxObjectEventFunction)&CurvePanel::OnSize, this);
	this->Bind(wxEVT_PAINT, (wxObjectEventFunction)&CurvePanel::PaintEvent, this);
}

CurvePanel::~CurvePanel() {

	if (displayCurve != NULL) {
		delete displayCurve;
		displayCurve = NULL;
	}
}

// left click is used to modify the position of control points
void CurvePanel::LeftClick(wxMouseEvent& evt) {

	// set the background to grey (to prevent white flickering)
	wxBrush brush(Colors::BackMedDarkGrey);

	int id = 0;
	int dx = 1;
	int dy = 1;
	int OldX = 0;
	int OldY = 0;
	int x = 0;
	int y = 0;
	double scaledX = 0.0f;
	double scaledY = 0.0f;

	// get the current mouse state
	wxMouseState mouse(wxGetMouseState());
	mouse.SetState(wxGetMouseState());

	// get the local mouse position (relative to the panel)
	wxPoint Local = this->ScreenToClient(mouse.GetPosition());
	x = Local.x;
	y = Local.y;

	// Scale the x and y values between 0 and 1
	wxSize curSize = this->GetClientSize();
	scaledX = (double)x / (double)curSize.GetWidth();
	scaledY = (double)y / (double)curSize.GetHeight();

	// get the ID of the control point to be modified, with +- 7 pixel
	// tolerance (so it is not as difficult to select the control point)
	id = displayCurve->PointExists(scaledX, scaledY, 0.018);

	while (mouse.LeftIsDown()) {

		mouse.SetState(wxGetMouseState());

		// get the local mouse position (relative to the panel)
		Local = this->ScreenToClient(mouse.GetPosition());
		x = Local.x;
		y = Local.y;

		scaledX = (double)x / (double)curSize.GetWidth();
		scaledY = (double)y / (double)curSize.GetHeight();

		dx = x - OldX;
		dy = y - OldY;

		// if the mouse position has changed since last time...
		if (dx != 0 || dy != 0) {

			// if a valid control point has been selected
			if (id > -1) {

				// if this is the left most point on the plot...
				if (id == 0) {

					// keep it pushed all the way to the left
					scaledX = 0.0;
				}

				// if this is the right most point on the plot...
				else if (id == displayCurve->NumControlPoint - 1) {

					// keep it pushed all the way to the right
					scaledX = 1.0;
				}

				// if this is a cotnrol point between the left and right most points...
				else {
					if (displayCurve->ControlPoints[id - 1].x > scaledX) {
						scaledX = displayCurve->ControlPoints[id - 1].x;
					}
					if (displayCurve->ControlPoints[id + 1].x < scaledX) {
						scaledX = displayCurve->ControlPoints[id + 1].x;
					}
				}

				// Make sure the new conrol points are in correct bounds
				scaledX = (scaledX < 0.0) ? 0.0 : scaledX;
				scaledX = (scaledX > 1.0) ? 1.0 : scaledX;

				scaledY = (scaledY < 0.0) ? 0.0 : scaledY;
				scaledY = (scaledY > 1.0) ? 1.0 : scaledY;

				// Set the control point that was selected to the new value.
				displayCurve->ModifyPoint(id, scaledX, scaledY);

				// draw the new spline
				PaintNow();
			}
		}

		// used to detect changes in mouse position
		OldX = x;
		OldY = y;

		this->Refresh();
		this->Update();
		wxSafeYield();
	}

	// Send event to parent to inform curve has changed
	wxCommandEvent curveEvt(CURVE_CHANGED_EVENT, ID_CURVE_CHANGED);
	wxPostEvent(par, curveEvt);

	evt.Skip(false);
}

// right click is used to create a remove control points
void CurvePanel::RightClick(wxMouseEvent& evt) {

	int x = 0;
	int y = 0;
	double scaledX = 0.0;
	double scaledY = 0.0;

	// get the current mouse state
	wxMouseState mouse(wxGetMouseState());
	wxKeyboardState keyboard(wxGetMouseState());

	// get the local mouse position (relative to the panel)
	wxPoint Local = this->ScreenToClient(mouse.GetPosition());
	x = Local.x;
	y = Local.y;

	// Scale the x and y values between 0 and 1
	wxSize curSize = this->GetClientSize();
	scaledX = (double)x / (double)curSize.GetWidth();
	scaledY = (double)y / (double)curSize.GetHeight();

	wxVector<Point> AllControls = displayCurve->GetControlPoints();

	// If shift is pressed during a right click
	if (keyboard.ShiftDown() == true) {

		// find the ID of the point to be removed
		int delID = displayCurve->PointExists(scaledX, scaledY, 0.018);

		// If the ID is not of the left or right points
		if (delID != 0 && delID != displayCurve->NumControlPoint - 1 &&
			delID > -1 && displayCurve->NumControlPoint > 2) {

			// remove that point
			displayCurve->RemovePoint(delID);

			// Send event to parent to inform curve has changed
			wxCommandEvent curveEvt(CURVE_CHANGED_EVENT, ID_CURVE_CHANGED);
			wxPostEvent(par, curveEvt);
		}
	}

	// If just a right click occured
	else {
		if (displayCurve->NumControlPoint > 1) {

			for (int i = 0; i < displayCurve->NumControlPoint - 1; i++) {

				if (scaledX > AllControls[i].x && scaledX < AllControls[i + 1].x) {

					// Add the point to the spline
					displayCurve->AddPoint(i + 1, scaledX, scaledY);

					// Send event to parent to inform curve has changed
					wxCommandEvent curveEvt(CURVE_CHANGED_EVENT, ID_CURVE_CHANGED);
					wxPostEvent(par, curveEvt);
				}
			}
		}
		else {
			displayCurve->AddPoint(scaledX, scaledY);
		}
	}
	PaintNow();
	evt.Skip(false);
}

void CurvePanel::OnSize(wxSizeEvent& WXUNUSED(evt)) {
	PaintNow();
}

void CurvePanel::PaintEvent(wxPaintEvent& evt) {

	// get the drawing context
	wxBufferedPaintDC dc(this);

	// set the background to grey
	wxBrush brush(Colors::BackMedDarkGrey);
	dc.SetBackground(brush);
	Render(dc);

	evt.Skip(false);
}
void CurvePanel::PaintNow() {

	// get the drawing context
	wxClientDC dc(this);
	wxBufferedDC dcBuf(&dc);

	// set the background to grey (to prevent white flickering)
	wxBrush brush(Colors::BackMedDarkGrey);
	dcBuf.SetBackground(brush);
	Render(dcBuf);
}
void CurvePanel::Render(wxDC& dc) {

	// clear the drawing context
	dc.Clear();

	// define all of the colos
	wxColour black(0, 0, 0);
	wxColour white(255, 255, 255);
	wxColour red(255, 0, 0);
	wxColour green(0, 255, 0);
	wxColour blue(0, 0, 255);

	// create all of the brushes
	wxBrush greyBrush(Colors::BackMedDarkGrey);
	wxBrush whiteBrush(white);
	wxBrush redBrush(red);
	wxBrush greenBrush(green);
	wxBrush blueBrush(blue);

	// set the background to grey
	dc.SetBackground(greyBrush);

	// create the pen that will draw the spline
	wxPen CurvePen(white, 2, wxPENSTYLE_SOLID);

	// set the color of the spline according to the
	// color channel to be modified
	switch (channelColor) {

	case CURVE_CHANNEL_BRIGHT:
		CurvePen.SetColour(white);
		dc.SetBrush(whiteBrush);
		break;

	case CURVE_CHANNEL_RED:
		CurvePen.SetColour(red);
		dc.SetBrush(redBrush);
		break;

	case CURVE_CHANNEL_GREEN:
		CurvePen.SetColour(green);
		dc.SetBrush(greenBrush);
		break;

	case CURVE_CHANNEL_BLUE:
		CurvePen.SetColour(blue);
		dc.SetBrush(blueBrush);
		break;

	}

	// create the pen to draw the grid
	wxPen GridPen(Colors::BackLightGrey, 2, wxPENSTYLE_SOLID);

	// get the spline and control points
	wxVector<Point> SplinePoints = displayCurve->GetCurve(0.5, CATMULL_ROM_SPLINE, 300);
	wxVector<Point> ControlPoints = displayCurve->GetControlPoints();

	// clip to curve so it does not go off the panel
	SplinePoints = ClipCurve(SplinePoints);

	// calculate the points used to draw the grid
	// the grid has 3 vertical and 3 horizontal lines
	// evenly spaced
	wxSize Size = this->GetClientSize();

	int W14 = (int)(Size.GetWidth() / 4);
	int W24 = (int)(2 * Size.GetWidth() / 4);
	int W34 = (int)(3 * Size.GetWidth() / 4);
	int W44 = (int)(4 * Size.GetWidth() / 4);

	int H14 = (int)(Size.GetHeight() / 4);
	int H24 = (int)(2 * Size.GetHeight() / 4);
	int H34 = (int)(3 * Size.GetHeight() / 4);
	int H44 = (int)(4 * Size.GetHeight() / 4);

	dc.SetPen(GridPen);

	// draw the vertical lines
	dc.DrawLine(wxPoint(W14, 0), wxPoint(W14, H44));
	dc.DrawLine(wxPoint(W24, 0), wxPoint(W24, H44));
	dc.DrawLine(wxPoint(W34, 0), wxPoint(W34, H44));

	// draw the horizontal lines
	dc.DrawLine(wxPoint(0, H14), wxPoint(W44, H14));
	dc.DrawLine(wxPoint(0, H24), wxPoint(W44, H24));
	dc.DrawLine(wxPoint(0, H34), wxPoint(W44, H34));

	dc.SetPen(CurvePen);

	// draw the spline
	double x1 = 0.0;
	double x2 = 0.0;
	double y1 = 0.0;
	double y2 = 0.0;

	for (int i = 0; i < (int)SplinePoints.size() - 2; i++) {

		x1 = SplinePoints[i].x * Size.GetWidth();
		x2 = SplinePoints[i + 1].x * Size.GetWidth();

		y1 = (SplinePoints[i].y * Size.GetHeight());
		y2 = (SplinePoints[i + 1].y * Size.GetHeight());

		double position = (double)x2 / (double)Size.GetWidth();

		if (channelColor == CURVE_CHANNEL_BLUE_TO_YELLOW) {

			wxColour blueYellow(255 * position, 255 * position, (255 - (position * 255.0)));
			wxBrush blueYellowBrush(blueYellow);
			CurvePen.SetColour(blueYellow);
			dc.SetBrush(blueYellowBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_GREEN_TO_RED) {

			wxColour greenRed(255 * position, (255 - (position * 255.0)), 0);
			wxBrush  greenRedBrush(greenRed);
			CurvePen.SetColour(greenRed);
			dc.SetBrush(greenRedBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_GREY_TO_WHITE) {

			int grey = (position * 255.0 * 0.5) + 127;
			if (grey > 255) { grey = 255; }
			wxColour greyColor(grey, grey, grey);
			wxBrush  greyColorBrush(greyColor);
			CurvePen.SetColour(greyColor);
			dc.SetBrush(greyColorBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_HUE) {

			wxColour hueColor = this->GetRGBFromHue(position * 360.0);
			wxBrush  hueColorBrush(hueColor);
			CurvePen.SetColour(hueColor);
			dc.SetBrush(hueColorBrush);
			dc.SetPen(CurvePen);
		}

		dc.DrawLine(wxPoint((int)x1, (int)y1), wxPoint((int)x2, (int)y2));
	}

	// draw the control points (larger circles)
	double x1Circle = 0.0;
	double y1Circle = 0.0;

	for (int i = 0; i < (int)ControlPoints.size(); i++) {

		if (ControlPoints[i].x < 0.0) {
			ControlPoints[i].x = 0.0;
		}
		if (ControlPoints[i].x > 1.0) {
			ControlPoints[i].x = 1.0;
		}

		if (ControlPoints[i].y < 0.0) {
			ControlPoints[i].y = 0.0;
		}
		if (ControlPoints[i].y > 1.0) {
			ControlPoints[i].y = 1.0;
		}


		x1Circle = ControlPoints[i].x * Size.GetWidth();
		y1Circle = ControlPoints[i].y * Size.GetHeight();

		double position = (double)x1Circle / (double)Size.GetWidth();

		if (channelColor == CURVE_CHANNEL_BLUE_TO_YELLOW) {

			wxColour blueYellow(255 * position, 255 * position, (255 - (position * 255.0)));
			wxBrush blueYellowBrush(blueYellow);
			CurvePen.SetColour(blueYellow);
			dc.SetBrush(blueYellowBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_GREEN_TO_RED) {

			wxColour greenRed(255 * position, (255 - (position * 255.0)), 0);
			wxBrush  greenRedBrush(greenRed);
			CurvePen.SetColour(greenRed);
			dc.SetBrush(greenRedBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_GREY_TO_WHITE) {

			int grey = (position * 255.0 * 0.5) + 127;
			if (grey > 255) { grey = 255; }
			wxColour greyColor(grey, grey, grey);
			wxBrush  greyColorBrush(greyColor);
			CurvePen.SetColour(greyColor);
			dc.SetBrush(greyColorBrush);
			dc.SetPen(CurvePen);
		}

		if (channelColor == CURVE_CHANNEL_HUE) {

			wxColour hueColor = this->GetRGBFromHue(position * 360.0);
			wxBrush  hueColorBrush(hueColor);
			CurvePen.SetColour(hueColor);
			dc.SetBrush(hueColorBrush);
			dc.SetPen(CurvePen);
		}

		dc.DrawCircle(wxPoint((int)x1Circle, (int)y1Circle), 4);
	}
}
wxVector<Point> CurvePanel::ClipCurve(wxVector<Point> Points) {

	for (int i = 0; i < (int)Points.size() - 1; i++) {

		if (Points[i].x < 0.0) {
			Points[i].x = 0.0;
		}
		if (Points[i].x > 1.0) {
			Points[i].x = 1.0;
		}
		if (Points[i].y < 0.0) {
			Points[i].y = 0.0;
		}
		if (Points[i].y > 1.0) {
			Points[i].y = 1.0;
		}
	}
	return Points;
}
void CurvePanel::GetColorCurveMap(int numSteps, int * outCurve, float scale) {


	wxVector<Point> controlPoints = displayCurve->GetControlPoints();
	splineCurve = new Spline(numSteps * 2, true);

	for (int i = 0; i < (int)controlPoints.size(); i++) {
		splineCurve->AddPoint(controlPoints[i].x, controlPoints[i].y);
		lastPoints.push_back(controlPoints[i]);
	}

	wxVector<Point> SplinePoints = splineCurve->GetCurve(0.5, CATMULL_ROM_SPLINE);
	for (int i = 0; i < (int)SplinePoints.size(); i++) {
		SplinePoints[i].x *= scale;
		SplinePoints[i].y *= scale;

		SplinePoints[i].y = scale - SplinePoints[i].y;

		if (SplinePoints[i].y > scale) {
			SplinePoints[i].y = scale;
		}
	}

	int SplinePos = 0;
	float Bright = 0.0;

	for (int i = 0; i < numSteps; i++) {

		while (SplinePoints[SplinePos].x >= Bright && SplinePoints[SplinePos].x < Bright + 1.0) {

			outCurve[i] = SplinePoints[SplinePos].y;
			SplinePos += 1;
		}
		Bright += 1.0;
	}

	delete splineCurve;
}

wxVector<Point> CurvePanel::GetControlPoints() {
	return displayCurve->GetControlPoints();
}

void CurvePanel::SetControlPoints(wxVector<Point> newPoints) {
	delete displayCurve;
	displayCurve = NULL;
	displayCurve = new Spline(1000, true);

	for (size_t i = 0; i < newPoints.size(); i++) {
		displayCurve->AddPoint(newPoints[i].id, newPoints[i].x, newPoints[i].y);
	}

	this->PaintNow();
}

void CurvePanel::DestroySpline() {
	delete displayCurve;
	displayCurve = NULL;
}

bool CurvePanel::CheckForChanges() {

	if (lastPoints.size() != displayCurve->GetControlPoints().size()) {
		return true;
	}
	for (int i = 0; i < (int)lastPoints.size(); i++) {
		if (displayCurve->GetControlPoints()[i].x != lastPoints[i].x ||
			displayCurve->GetControlPoints()[i].y != lastPoints[i].y) {
			return true;
		}
	}
	return false;
}

wxColour CurvePanel::GetRGBFromHue(double degree) {

	double luminace = 0.5;
	double saturation = 1.0;
	double hue = degree;
	double temp1 = 0.0;
	double temp2 = 0.0;
	double tempR = 0.0;
	double tempG = 0.0;
	double tempB = 0.0;
	double tempRedHSL = 0.0;
	double tempGreenHSL = 0.0;
	double tempBlueHSL = 0.0;

	if (luminace < 0.5) { temp1 = luminace * (1.0 + saturation); }
	else { temp1 = (luminace + saturation) - (luminace * saturation); }

	temp2 = (2 * luminace) - temp1;

	// Convert 0-360 to 0-1
	hue /= 360.0;

	// Create temporary RGB from hue
	tempR = hue + (1.0 / 3.0);
	tempG = hue;
	tempB = hue - (1.0 / 3.0);

	// Adjust all temp RGB values to be between 0 and 1
	if (tempR < 0.0) { tempR += 1.0; }
	if (tempR > 1.0) { tempR -= 1.0; }
	if (tempG < 0.0) { tempG += 1.0; }
	if (tempG > 1.0) { tempG -= 1.0; }
	if (tempB < 0.0) { tempB += 1.0; }
	if (tempB > 1.0) { tempB -= 1.0; }

	// Calculate RGB Red from HSL
	if ((6.0 * tempR) < 1.0) { tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
	else if ((2.0 * tempR) < 1.0) { tempRedHSL = temp1; }
	else if ((3.0 * tempR) < 2.0) { tempRedHSL = temp2 + ((temp1 - temp2)*((2.0 / 3.0) - tempR)*6.0); }
	else { tempRedHSL = temp2; }

	// Calculate RGB Green from HSL
	if ((6.0 * tempG) < 1.0) { tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG); }
	else if ((2.0 * tempG) < 1.0) { tempGreenHSL = temp1; }
	else if ((3.0 * tempG) < 2.0) { tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0 / 3.0) - tempG)*6.0); }
	else { tempGreenHSL = temp2; }

	// Calculate RGB Blue from HSL
	if ((6.0 * tempB) < 1.0) { tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
	else if ((2.0 * tempB) < 1.0) { tempBlueHSL = temp1; }
	else if ((3.0 * tempB) < 2.0) { tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0 / 3.0) - tempB)*6.0); }
	else { tempBlueHSL = temp2; }

	// Scale to 0 - 255
	return wxColour(tempRedHSL * 255.0, tempGreenHSL * 255.0, tempBlueHSL * 255.0);
}