#ifndef CHANNEL_TRANSFORM_WINDOW_H
#define CHANNEL_TRANSFORM_WINDOW_H

#include "GUI\EditList\EditWindow\EditWindow.h"
#include "GUI\Controls\DoubleSlider\DoubleSlider.h"
#include "GUI\Colors\Colors.h"
#include "Processing\Processor\Processor.h"
#include "Debugging\MemoryLeakCheck.h"

class ChannelTransformWindow : public EditWindow {
public:
	ChannelTransformWindow(wxWindow * parent, wxString editName, Processor * processor);
	void AddEditToProcessor();
	void Process(wxCommandEvent& WXUNUSED(event));

private:

	class ChannelTransformPreset;

	void PopulateIntialPresets();
	void CreateComboPresetList();
	ChannelTransformPreset GetChannelTransformPresetByName(wxString name);
	void SetValuesFromPreset(ChannelTransformPreset preset);
	void PresetChange(wxCommandEvent& WXUNUSED(event));

	wxWindow * parWindow;

	wxBoxSizer * mainSizer;

	wxBoxSizer * presetSizer;
	wxFlexGridSizer * redGridSizer;
	wxFlexGridSizer * greenGridSizer;
	wxFlexGridSizer * blueGridSizer;

	wxStaticText * editLabel;

	wxStaticText * presetLabel;
	wxComboBox * presetBox;

	wxStaticText * redRedLabel;
	wxStaticText * redGreenLabel;
	wxStaticText * redBlueLabel;

	wxStaticText * greenRedLabel;
	wxStaticText * greenGreenLabel;
	wxStaticText * greenBlueLabel;

	wxStaticText * blueRedLabel;
	wxStaticText * blueGreenLabel;
	wxStaticText * blueBlueLabel;

	DoubleSlider * redRedSlider;
	DoubleSlider * redGreenSlider;
	DoubleSlider * redBlueSlider;

	DoubleSlider * greenRedSlider;
	DoubleSlider * greenGreenSlider;
	DoubleSlider * greenBlueSlider;

	DoubleSlider * blueRedSlider;
	DoubleSlider * blueGreenSlider;
	DoubleSlider * blueBlueSlider;

	wxButton * processButton;

	Processor * proc;

	wxVector<ChannelTransformPreset> presetList;

	class ChannelTransformPreset {

	public:
		ChannelTransformPreset(wxString name, double redRedScale, double redGreenScale, double redBlueScale,
			double greenRedScale, double greenGreenScale, double greenBlueScale,
			double blueRedScale, double blueGreenScale, double blueBlueScale);

		void SetName(wxString name);
		wxString GetName();
		
		void SetRedRedScale(double newRedRedScale);
		void SetRedGreenScale(double newRedGreenScale);
		void SetRedBlueScale(double newRedBlueScale);
		void SetGreenRedScale(double newGreenRedScale);
		void SetGreenGreenScale(double newGreenGreenScale);
		void SetGreenBlueScale(double newGreenBlueScale);
		void SetBlueRedScale(double newBlueRedScale);
		void SetBlueGreenScale(double newBlueGreenScale);
		void SetBlueBlueScale(double newBlueBlueScale);

		double GetRedRedScale();
		double GetRedGreenScale();
		double GetRedBlueScale();
		double GetGreenRedScale();
		double GetGreenGreenScale();
		double GetGreenBlueScale();
		double GetBlueRedScale();
		double GetBlueGreenScale();
		double GetBlueBlueScale();

	private:

		wxString presetName;
		double redRed;
		double redGreen;
		double redBlue;
		double greenRed;
		double greenGreen;
		double greenBlue;
		double blueRed;
		double blueGreen;
		double blueBlue;
	};
};
#endif