// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "EditSelection.h"
#include "GUI/AUIManager/AUIManager.h"

wxDEFINE_EVENT(EDIT_ADD_EVENT, wxCommandEvent);

EditSelection::EditSelection(wxWindow * parent) : wxScrolledWindow(parent) {

	parWindow = parent;
	mainSizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(mainSizer);

	editList = new wxListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
	editList->InsertColumn(0, "Edit Name");
	editList->InsertColumn(1, "Edit Description");

	addButton = new PhoediXButton(this, EditSelection::Buttons::ADD_EDIT, "Add Edit");

	this->SetBackgroundColour(Colors::BackDarkGrey);
	editList->SetBackgroundColour(Colors::BackDarkGrey);
	addButton->SetBackgroundColour(Colors::BackGrey);

	editList->SetForegroundColour(Colors::TextLightGrey);
	addButton->SetForegroundColour(Colors::TextLightGrey);
	
	this->GetSizer()->Add(editList, 1, wxEXPAND);
	this->GetSizer()->Add(addButton, 0, wxALIGN_RIGHT);

	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditSelection::OnAdd, this, EditSelection::Buttons::ADD_EDIT);
	this->Bind(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&EditSelection::OnClose, this);
	this->Bind(wxEVT_LIST_ITEM_ACTIVATED, (wxObjectEventFunction)&EditSelection::OnAdd, this);

	this->FitInside();
	this->SetScrollRate(5, 5);
}

void EditSelection::OnClose(wxCommandEvent& WXUNUSED(event)) {
	this->Hide();
}

void EditSelection::AddEditSelection(wxString editName, wxString editDescription) {

	// Insert new edit into the list
	long index = editList->InsertItem(editList->GetItemCount(), editName);
	editList->SetItem(index, 1, editDescription);

	// Size each column to the max size needed to show all the text
	editList->SetColumnWidth(0, wxLIST_AUTOSIZE);
	editList->SetColumnWidth(1, wxLIST_AUTOSIZE);
}

void EditSelection::OnAdd(wxCommandEvent& WXUNUSED(event)) {

	AvailableEdits available;
	wxVector<Edit> allEdits = available.GetAvailableEdits();

	for (int i = -1; i < editList->GetSelectedItemCount() - 1; i++) {

		// Get next selected edits name, and description
		long index = editList->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		wxString editName = editList->GetItemText(index, 0);
		wxString editDescription = editList->GetItemText(index, 1);
		
		// Het the edit ID from the edit name
		int editID = available.GetIDFromName(editName);

		// Send add edit event with edit ID to parent window (the edit panel)
		wxCommandEvent evt(EDIT_ADD_EVENT, ID_EDIT_ADD);
		evt.SetInt(editID);
		wxPostEvent(parWindow, evt);		
	}
}

void EditSelection::FitEdits() {

	this->FitInside();

	int editListWidth = editList->GetViewRect().GetWidth() + 25;
	int editListHeight = 0;
	for (int i = 0; i < editList->GetItemCount(); i++) {
		wxRect * itemRect = new wxRect();
		editListHeight += editList->GetItemRect(i, *itemRect);
		editListHeight += itemRect->GetHeight() * 1.5;
		delete itemRect;
	}

	if (editListWidth > 800) { editListWidth = 800; }
	if (editListHeight > 800) { editListHeight = 800; }

	this->SetClientSize(wxSize(editListWidth, editListHeight));
	PhoedixAUIManager::GetPhoedixAUIManager()->GetPane(this).FloatingSize(this->GetClientSize());
	PhoedixAUIManager::GetPhoedixAUIManager()->Update();
}