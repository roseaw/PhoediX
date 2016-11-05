// Copyright 2016 Jacob Chesley

#include "EditListItem.h"

wxDEFINE_EVENT(EDIT_UP_EVENT, wxCommandEvent);
wxDEFINE_EVENT(EDIT_DOWN_EVENT, wxCommandEvent);
wxDEFINE_EVENT(EDIT_DELETE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(EDIT_DISABLE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(EDIT_COPY_EVENT, wxCommandEvent);
wxDEFINE_EVENT(EDIT_PASTE_EVENT, wxCommandEvent);

EditListItem::EditListItem(wxWindow * parent, wxString title, int Sequence, EditWindow * editWindow, bool disableButtons) : wxPanel(parent) {

	parWindow = parent;

	sizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(sizer);
	this->SetBackgroundColour(Colors::BackGrey);

	textSizer = new wxBoxSizer(wxVERTICAL);

	// Add title text
	titleText = new wxButton(this, EditListItem::Buttons::OPEN_EDIT_BUTTON, title, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	titleText->SetBackgroundColour(this->GetBackgroundColour());
	titleText->SetForegroundColour(Colors::TextWhite);
	titleText->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	titleText->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditListItem::OnRightClick), NULL, this);
	textSizer->AddSpacer(5);
	textSizer->Add(titleText, 0, wxALIGN_CENTER, 1);
	textSizer->AddSpacer(5);
		
	// Add control buttons when there is an edit that must remain in the exact place (RAW edit must be at top)
	if(!disableButtons){

		// Add size to keep up and down button in
		upDownButtonSizer = new wxBoxSizer(wxVERTICAL);

		// Layout is:
		//                                  [Up Button]
		// [disable button]  [TitleText]                 [Delete Button]
		//                                 [Down Button]
		disableButton = new wxButton(this, EditListItem::Buttons::DISABLE_BUTTON, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		upButton = new wxButton(this, EditListItem::Buttons::UP_BUTTON, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		downButton = new wxButton(this, EditListItem::Buttons::DOWN_BUTTON, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
		deleteButton = new wxButton(this, EditListItem::Buttons::DELETE_BUTTON, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

		// Background of buttons
		upButton->SetBackgroundColour(this->GetBackgroundColour());
		downButton->SetBackgroundColour(this->GetBackgroundColour());
		deleteButton->SetBackgroundColour(this->GetBackgroundColour());
		disableButton->SetBackgroundColour(this->GetBackgroundColour());

		// Add icons to buttons
		Icons icons;
		upButton->SetBitmap(icons.UpButton.Rescale(17, 17, wxIMAGE_QUALITY_HIGH));
		downButton->SetBitmap(icons.DownButton.Rescale(17, 17, wxIMAGE_QUALITY_HIGH));
		deleteButton->SetBitmap(icons.DeleteButton.Rescale(22, 22, wxIMAGE_QUALITY_HIGH));
		disableButton->SetBitmap(icons.DisableButton.Rescale(22, 22, wxIMAGE_QUALITY_HIGH));

		// Size buttons
		upButton->SetMinSize(wxSize(17, 17));
		downButton->SetMinSize(wxSize(17, 17));
		deleteButton->SetMinSize(wxSize(22, 22));
		disableButton->SetMinSize(wxSize(22, 22));

		// Add up and down button to its own sizer
		upDownButtonSizer->Add(upButton);
		upDownButtonSizer->Add(downButton);
	}

	// Add disable button if buttons are allowed
	if (!disableButtons) {
		this->GetSizer()->Add(disableButton, 0, wxEXPAND);
	}

	// Always add title text
	this->GetSizer()->Add(textSizer, 1, wxEXPAND);

	// Add up and down buttons, and delete button, is buttons are allowed
	if(!disableButtons){
		this->GetSizer()->Add(upDownButtonSizer, 0, wxEXPAND);
		this->GetSizer()->AddSpacer(12);
		this->GetSizer()->Add(deleteButton, 0, wxEXPAND);
	}

	// Add connected edit window
	editWin = NULL;
	if (editWindow != NULL) {
		editWin = editWindow;
	}

	// Bind all events needed
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditListItem::OnOpenEdit, this, EditListItem::Buttons::OPEN_EDIT_BUTTON);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditListItem::OnUp, this, EditListItem::Buttons::UP_BUTTON);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditListItem::OnDown, this, EditListItem::Buttons::DOWN_BUTTON);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditListItem::OnDelete, this, EditListItem::Buttons::DELETE_BUTTON);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&EditListItem::OnDisable, this, EditListItem::Buttons::DISABLE_BUTTON);
	this->Bind(wxEVT_RIGHT_DOWN, (wxMouseEventFunction)&EditListItem::OnRightClick, this);
	seq = Sequence;
	isDisabled = false;
}

// Destroy the edit window
EditListItem::~EditListItem() {
	if (editWin != NULL) {
		editWin->StopWatchdog();
		editWin->Destroy();
	}
}

// Destroy this and edit window
void EditListItem::DestroyItem() {
	if (editWin != NULL) {
		editWin->StopWatchdog();
		editWin->Destroy();
		editWin = NULL;
	}

	this->Destroy();
}

EditWindow* EditListItem::GetEditWindow() {
	return editWin;
}

bool EditListItem::GetDisabled() {
	return isDisabled;
}

void EditListItem::SetDisabled(bool disabled){

	// Darken the title text to show this is disabled
	if (disabled) {
		titleText->SetForegroundColour(Colors::TextGrey);
		isDisabled = true;
	}

	// Lighten the title text to show this is enabled
	else {
		titleText->SetForegroundColour(Colors::TextWhite);
		isDisabled = false;
	}
}

void EditListItem::OnOpenEdit(wxCommandEvent& WXUNUSED(event)) {

	// Show the edit window
	if (editWin != NULL) {
		PhoedixAUIManager::GetPhoedixAUIManager()->GetPane(editWin).Show();
		PhoedixAUIManager::GetPhoedixAUIManager()->Update();
	}
}

void EditListItem::OnRightClick(wxMouseEvent& WXUNUSED(event)) {

	// Display a popup menu of options
	wxMenu popupMenu;
	popupMenu.Append(EditListItem::PopupMenuActions::COPY_EDIT_PARAMS, "Copy Edit Parameters");

	if(this->GetEditWindow()->CheckCopiedParamsAndFlags()){
		popupMenu.Append(EditListItem::PopupMenuActions::PASTE_EDIT_PARAMS, "Paste Edit Parameters");
	}
	popupMenu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(EditListItem::OnPopupMenuClick), NULL, this);
	this->PopupMenu(&popupMenu);
}

void EditListItem::OnPopupMenuClick(wxCommandEvent& inEvt) {

	int id = inEvt.GetId();

	switch(id){

		// Copy the edit parameters
		case EditListItem::PopupMenuActions::COPY_EDIT_PARAMS:{
			// Tell the Edit Layer Panel to copy parameters of this edit
			wxCommandEvent evt(EDIT_COPY_EVENT, ID_EDIT_COPY);
			evt.SetInt(seq);
			wxPostEvent(parWindow, evt);
			break;
		}

		case EditListItem::PopupMenuActions::PASTE_EDIT_PARAMS:{
			// Tell the Edit Layer Panel to copy parameters of this edit
			wxCommandEvent evt(EDIT_PASTE_EVENT, ID_EDIT_PASTE);
			evt.SetInt(seq);
			wxPostEvent(parWindow, evt);
			break;
		}
	}
}

void EditListItem::OnUp(wxCommandEvent& WXUNUSED(event)) {

	// Tell the Edit Layer Panel to move this edit up
	wxCommandEvent evt(EDIT_UP_EVENT, ID_EDIT_UP);
	evt.SetInt(seq);
	wxPostEvent(parWindow, evt);
}

void EditListItem::OnDown(wxCommandEvent& WXUNUSED(event)) {

	// Tell the Edit Layer Panel to move this edit down
	wxCommandEvent evt(EDIT_DOWN_EVENT, ID_EDIT_DOWN);
	evt.SetInt(seq);
	wxPostEvent(parWindow, evt);
}

void EditListItem::OnDelete(wxCommandEvent& WXUNUSED(event)) {

	// Tell the Edit Layer Panel to delete this edit
	wxCommandEvent evt(EDIT_DELETE_EVENT, ID_EDIT_DELETE);
	evt.SetInt(seq);
	wxPostEvent(parWindow, evt);
}

void EditListItem::OnDisable(wxCommandEvent& WXUNUSED(event)) {

	// Enable if this is already disabled
	if (isDisabled) {
		this->SetDisabled(false);
	}

	// Disable if this is enabled
	else {
		this->SetDisabled(true);
	}

	// Tell parent window if this is disabled or enabled
	wxCommandEvent evt(EDIT_DISABLE_EVENT, ID_EDIT_DISABLE);
	wxPostEvent(parWindow, evt);
}

wxString EditListItem::GetTitle() {
	return titleText->GetLabel();
}

void EditListItem::SetTitle(wxString title) {
	return titleText->SetLabel(title);
}

int EditListItem::GetSequence() {
	return seq;
}

void EditListItem::SetSequence(size_t sequence) {
	seq = sequence;
}