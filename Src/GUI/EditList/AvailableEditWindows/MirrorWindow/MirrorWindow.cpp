// Copyright 2016 Jacob Chesley

#include "MirrorWindow.h"

MirrorWindow::MirrorWindow(wxWindow * parent, wxString editName, Processor * processor) : EditWindow(parent, editName, processor) {

	this->SetBackgroundColour(parent->GetBackgroundColour());

	mainSizer = new wxBoxSizer(wxVERTICAL);

	// 2 Columns, 15 pixel vertical gap, 5 pixel horizontal gap
	gridSizer = new wxFlexGridSizer(2, 15, 5);

	editLabel = new wxStaticText(this, -1, editName);
	editLabel->SetForegroundColour(Colors::TextWhite);
	editLabel->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

	mirrorMethodLabel = new wxStaticText(this, -1, "Rotation Type");

	mirrorMethodLabel->SetForegroundColour(Colors::TextLightGrey);

	mirrorMethod = new wxComboBox(this, -1);
	mirrorMethod->AppendString("No Mirror");
	mirrorMethod->AppendString("Mirror Horizontal");
	mirrorMethod->AppendString("Mirror Vertical");
	mirrorMethod->SetSelection(0);

	mirrorMethod->SetBackgroundColour(this->GetBackgroundColour());
	mirrorMethod->SetForegroundColour(Colors::TextLightGrey);

	gridSizer->Add(mirrorMethodLabel);
	gridSizer->Add(mirrorMethod);

	mainSizer->Add(editLabel);
	mainSizer->AddSpacer(10);
	mainSizer->Add(gridSizer);

	proc = processor;
	parWindow = parent;

	this->SetSizer(mainSizer);
	this->FitInside();
	this->SetScrollRate(5, 5);

	this->SetClientSize(this->GetVirtualSize());

	wxCommandEvent comboEvt(wxEVT_COMBOBOX, 0);
	wxPostEvent(this, comboEvt);
}

void MirrorWindow::SetParamsAndFlags(ProcessorEdit * edit) {

	if(edit == NULL){ return; }

	// Choose method based on edit loaded
	if (edit->GetFlagsSize() == 1 && 
		(edit->GetEditType() == ProcessorEdit::EditType::MIRROR_HORIZONTAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_VERTICAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_NONE)) {

		mirrorMethod->SetSelection(edit->GetFlag(0));
	}
}

ProcessorEdit * MirrorWindow::GetParamsAndFlags(){

	int mirrorSelection = mirrorMethod->GetSelection();

	if (mirrorSelection == 0) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_NONE);
		mirrorEdit->AddFlag(mirrorSelection);

		// Set enabled / disabled
		mirrorEdit->SetDisabled(isDisabled);

		return mirrorEdit;
	}

	else if (mirrorSelection == 1) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_HORIZONTAL);
		mirrorEdit->AddFlag(mirrorSelection);

		// Set enabled / disabled
		mirrorEdit->SetDisabled(isDisabled);

		return mirrorEdit;
	}

	else if (mirrorSelection == 2) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_VERTICAL);
		mirrorEdit->AddFlag(mirrorSelection);

		// Set enabled / disabled
		mirrorEdit->SetDisabled(isDisabled);

		return mirrorEdit;
	}

	return NULL;
}

bool MirrorWindow::CheckCopiedParamsAndFlags(){

	ProcessorEdit * edit = proc->GetEditForCopyPaste();
	if(edit == NULL){ return false; }

	if (edit->GetFlagsSize() == 1 && 
		(edit->GetEditType() == ProcessorEdit::EditType::MIRROR_HORIZONTAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_VERTICAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_NONE)) {
		return true;
		
	}
	return false;
}