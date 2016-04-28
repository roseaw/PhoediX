#ifndef EDIT_LIST_PANEL_H
#define EDIT_LIST_PANEL_H

// for compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

// for all others, include the necessary headers explicitly
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "GUI\EditList\AvailableEdits\AvailableEdits.h"
#include "GUI\EditList\EditListItem\EditListItem.h"
#include "GUI\EditList\EditSelection\EditSelection.h"
#include "GUI\Colors\Colors.h"
#include "Debugging\MemoryLeakCheck.h"

class EditListPanel : public wxPanel {
public:

	EditListPanel(wxWindow * parent);
	wxVector<Edit> edits;

private:

	void InitializeEdits();
	void OnAddEdit(wxCommandEvent& WXUNUSED(event));

	void AddEditToPanel(wxCommandEvent& addEvt);
	void MoveEditUp(wxCommandEvent& upEvt);
	void MoveEditDown(wxCommandEvent& downEvt);
	void DeleteEdit(wxCommandEvent& deleteEvt);

	wxStaticText * titleText;
	wxBoxSizer * mainSizer;
	wxBoxSizer * bottomButtonsSizer;
	wxButton * addEditButton;

	EditSelection * editSelection;

	enum Buttons {
		ADD_EDIT_BUTTON = 100
	};

	class EditListScroll : public wxScrolledWindow {
	public:
		EditListScroll(wxWindow * parent);
		void AddEdit(EditListItem * item);
		void DeleteEdit(size_t index);
		void MoveEditUp(size_t index);
		void MoveEditDown(size_t index);
		int GetNextID();

	private:

		void RedrawEdits();
		wxBoxSizer * sizer;
		wxVector<EditListItem*> editList;
	};
	
	EditListScroll * scroller;

};
#endif