#include "MirrorWindow.h"

MirrorWindow::MirrorWindow(wxWindow * parent, wxString editName, Processor * processor) : EditWindow(parent, editName) {

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

	processButton = new wxButton(this, EditWindow::ID_PROCESS_EDITS, "Process Edits");

	mirrorMethod->SetBackgroundColour(this->GetBackgroundColour());
	mirrorMethod->SetForegroundColour(Colors::TextLightGrey);

	gridSizer->Add(mirrorMethodLabel);
	gridSizer->Add(mirrorMethod);

	mainSizer->Add(editLabel);
	mainSizer->AddSpacer(10);
	mainSizer->Add(gridSizer);
	mainSizer->AddSpacer(15);
	mainSizer->Add(processButton, 0, wxALIGN_LEFT);

	proc = processor;
	parWindow = parent;

	//this->Bind(wxEVT_SCROLL_CHANGED, (wxObjectEventFunction)&ConvertGreyscaleWindow::Process, this);
	//this->Bind(wxEVT_TEXT_ENTER, (wxObjectEventFunction)&ConvertGreyscaleWindow::Process, this);
	//this->Bind(wxEVT_TEXT, (wxObjectEventFunction)&ConvertGreyscaleWindow::Process, this);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&MirrorWindow::Process, this, EditWindow::ID_PROCESS_EDITS);

	this->SetSizer(mainSizer);
	this->FitInside();
	this->SetScrollRate(5, 5);

	this->SetClientSize(this->GetVirtualSize());

	wxCommandEvent comboEvt(wxEVT_COMBOBOX, 0);
	wxPostEvent(this, comboEvt);
}

void MirrorWindow::Process(wxCommandEvent& WXUNUSED(event)) {

	wxCommandEvent evt(REPROCESS_IMAGE_EVENT, ID_REPROCESS_IMAGE);
	wxPostEvent(parWindow, evt);
}

void MirrorWindow::AddEditToProcessor() {

	int mirrorSelection = mirrorMethod->GetSelection();

	if (mirrorSelection == 0) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_NONE);
		mirrorEdit->AddFlag(mirrorSelection);
		proc->AddEdit(mirrorEdit);
	}

	else if (mirrorSelection == 1) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_HORIZONTAL);
		mirrorEdit->AddFlag(mirrorSelection);
		proc->AddEdit(mirrorEdit);
	}

	else if (mirrorSelection == 2) {
		ProcessorEdit * mirrorEdit = new ProcessorEdit(ProcessorEdit::EditType::MIRROR_VERTICAL);
		mirrorEdit->AddFlag(mirrorSelection);
		proc->AddEdit(mirrorEdit);
	}
}

void MirrorWindow::SetParamsAndFlags(ProcessorEdit * edit) {

	// Choose method based on edit loaded
	if (edit->GetFlagsSize() == 1 && 
		(edit->GetEditType() == ProcessorEdit::EditType::MIRROR_HORIZONTAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_VERTICAL ||
		edit->GetEditType() == ProcessorEdit::EditType::MIRROR_NONE)) {

		mirrorMethod->SetSelection(edit->GetFlag(0));
	}

	
}