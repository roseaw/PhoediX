// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "ExportWindow.h"

wxDEFINE_EVENT(UPDATE_EXPORT_PROGRESS_NUM_EVENT, wxCommandEvent);
wxDEFINE_EVENT(UPDATE_EXPORT_PROGRESS_MSG_EVENT, wxCommandEvent);
wxDEFINE_EVENT(SAVE_PROJECT_EVENT, wxCommandEvent);

ExportWindow::ExportWindow(wxWindow * parent, Processor * processor, EditListPanel * panel) : wxScrolledWindow(parent){

	editList = panel;
	proc = processor;

	this->SetBackgroundColour(parent->GetBackgroundColour());

	mainSizer = new wxFlexGridSizer(3, 15, 15);
	this->SetSizer(mainSizer);

	fileTypeLabel = new wxStaticText(this, -1, "Image Type");
	fileTypeLabel->SetForegroundColour(Colors::TextLightGrey);
	fileTypeControl = new PhoediXComboBox(this, -1);
	fileTypeControl->AppendString("JPEG");
	fileTypeControl->AppendString("PNG");
	fileTypeControl->AppendString("TIFF 8 Bit");
	fileTypeControl->AppendString("TIFF 16 Bit");
	fileTypeControl->AppendString("BMP");
	fileTypeControl->SetSelection(0);

	fileTypeControl->SetBackgroundColour(Colors::BackDarkDarkGrey);
	fileTypeControl->SetForegroundColour(Colors::TextLightGrey);

	this->GetSizer()->Add(fileTypeLabel);
	this->GetSizer()->Add(fileTypeControl);
	this->GetSizer()->Add(new wxPanel(this));

	jpegQualityLabel = new wxStaticText(this, -1, "JPEG Quality");
	jpegQualityLabel->SetForegroundColour(Colors::TextLightGrey);
	jpegQualitySlide = new DoubleSlider(this, 90.0, 0.0, 100.0, 100, 0);
	jpegQualitySlide->SetValuePosition(DoubleSlider::VALUE_INLINE_RIGHT);

	jpegQualitySlide->SetBackgroundColour(this->GetBackgroundColour());
	jpegQualitySlide->SetForegroundColour(Colors::TextLightGrey);
	this->GetSizer()->Add(jpegQualityLabel);
	this->GetSizer()->Add(jpegQualitySlide);
	this->GetSizer()->Add(new wxPanel(this));

	outputFolderLabel = new wxStaticText(this, -1, "Export Folder");
	outputFolderLabel->SetForegroundColour(Colors::TextLightGrey);
	outputFolderText = new wxTextCtrl(this, -1);
	outputFolderText->SetBackgroundColour(this->GetBackgroundColour());
	outputFolderText->SetForegroundColour(Colors::TextLightGrey);

	outputFolderButton = new PhoediXButton(this, ExportWindow::ExportActions::ID_BROWSE, "Browse");
	outputFolderButton->SetForegroundColour(Colors::TextLightGrey);
	outputFolderButton->SetBackgroundColour(Colors::BackGrey);
	this->GetSizer()->Add(outputFolderLabel);
	this->GetSizer()->Add(outputFolderText,0, wxEXPAND);
	this->GetSizer()->Add(outputFolderButton);

	outputNameLabel = new wxStaticText(this, -1, "Export Image Name");
	outputNameLabel->SetForegroundColour(Colors::TextLightGrey);
	outputNameText = new wxTextCtrl(this, -1);
	outputNameText->SetBackgroundColour(this->GetBackgroundColour());
	outputNameText->SetForegroundColour(Colors::TextLightGrey);
	this->GetSizer()->Add(outputNameLabel);
	this->GetSizer()->Add(outputNameText,0, wxEXPAND);
	this->GetSizer()->Add(new wxPanel(this));

	exportButton = new PhoediXButton(this, ExportWindow::ExportActions::ID_EXPORT, "Export");
	exportButton->SetForegroundColour(Colors::TextLightGrey);
	exportButton->SetBackgroundColour(Colors::BackGrey);
	exportButton->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	
	this->GetSizer()->Add(exportButton);
	this->GetSizer()->Add(new wxPanel(this));
	this->GetSizer()->Add(new wxPanel(this));

	this->FitInside();
	this->SetScrollRate(5, 5);
	this->SetClientSize(this->GetVirtualSize());

	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&ExportWindow::OnBrowse, this, ExportWindow::ExportActions::ID_BROWSE);
	this->Bind(wxEVT_BUTTON, (wxObjectEventFunction)&ExportWindow::OnExport, this, ExportWindow::ExportActions::ID_EXPORT);
	this->Bind(wxEVT_COMBOBOX, (wxObjectEventFunction)&ExportWindow::OnCombo, this);
	this->Bind(wxEVT_SCROLL_THUMBRELEASE, (wxObjectEventFunction)&ExportWindow::OnSlide, this);
	this->Bind(wxEVT_TEXT, (wxObjectEventFunction)&ExportWindow::OnText, this);
	this->Bind(UPDATE_EXPORT_PROGRESS_MSG_EVENT, (wxObjectEventFunction)&ExportWindow::SetProgressEditMsg, this, ID_UPDATE_EXPORT_PROGRESS_MSG_EVENT);
	this->Bind(UPDATE_EXPORT_PROGRESS_NUM_EVENT, (wxObjectEventFunction)&ExportWindow::SetProgressEditNum, this, ID_UPDATE_EXPORT_PROGRESS_NUM_EVENT);

	exportStarted = false;
	rawImageLoaded = false;
	progressSize = 0;
	currentEditNumber = 0;
	currentEditString = "";	
}

void ExportWindow::RawImageLoaded(bool isRawImageLoaded){
	rawImageLoaded = isRawImageLoaded;
}

void ExportWindow::OnCombo(wxCommandEvent & WXUNUSED(event)){

	if(fileTypeControl->GetSelection() == 0){
		jpegQualityLabel->Show();
		jpegQualitySlide->Show();
	}
	else{
		jpegQualityLabel->Hide();
		jpegQualitySlide->Hide();
	}
	this->Layout();
	this->SaveProject();
}

void ExportWindow::OnSlide(wxCommandEvent & WXUNUSED(event)) {
	this->SaveProject();
}
void ExportWindow::OnText(wxCommandEvent & WXUNUSED(event)) {
	this->SaveProject();
}

void ExportWindow::OnBrowse(wxCommandEvent & WXUNUSED(event)){

	// Browse for directory to store export in
	wxDirDialog openDirDialog(this, "Export Folder");
	if (openDirDialog.ShowModal() == wxID_CANCEL) {
		this->SaveProject();
		return;
	}
	outputFolderText->SetValue(openDirDialog.GetPath());
	this->SaveProject();
}

void ExportWindow::OnExport(wxCommandEvent & WXUNUSED(event)){

	wxString extension = "";
	int fileType = ImageHandler::SaveType::JPEG;
	if (fileTypeControl->GetSelection() == 0) { fileType = ImageHandler::SaveType::JPEG; extension = ".jpg"; }
	else if (fileTypeControl->GetSelection() == 1) { fileType = ImageHandler::SaveType::PNG; extension = ".png"; }
	else if (fileTypeControl->GetSelection() == 2) { fileType = ImageHandler::SaveType::TIFF8; extension = ".tif"; }
	else if (fileTypeControl->GetSelection() == 3) { fileType = ImageHandler::SaveType::TIFF16; extension = ".tif"; }
	else if (fileTypeControl->GetSelection() == 4) { fileType = ImageHandler::SaveType::BMP; extension = ".bmp"; }
	else {}

	wxString exportFilename = outputFolderText->GetValue() + wxFileName::GetPathSeparator() + outputNameText->GetValue() + extension;
	if (wxFileExists(exportFilename)) {
		if (wxMessageBox(_(outputNameText->GetValue() + extension + " Already exists.  File will be overwritten.  Is this okay?"), _("File Exists"), wxICON_QUESTION | wxYES_NO, this) == wxNO) {
			return;
		}
	}

	exportStarted = true;

	// Get flag for fast edit, and force disable
	fastEditFlag = proc->GetFastEdit();
	proc->DisableFastEdit();
	
	// Process both raw image (if it has one) and image itself
	editList->ReprocessImageRaw();
	
	wxGenericProgressDialog progressDialog("Export Status", "", proc->GetEditVector().size(), this, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH);
	progressDialog.SetRange(proc->GetEditVector().size());
	currentEditNumber = 0;
	currentEditString = "";

	while (exportStarted) {

		wxMilliSleep(10);
		progressMutex.Lock();
		progressDialog.Update(currentEditNumber, currentEditString);
		progressMutex.Unlock();
		wxSafeYield();
	}
}

void ExportWindow::ProcessingComplete(){

	if(exportStarted){

		exportStarted = false;

		// Enable or disable fast edit flag after export.
		if (fastEditFlag) { proc->EnableFastEdit(); }
		else { proc->DisableFastEdit(); }
		
		// Make sure progress bar is including saving image step
		this->SetMessage("Saving Image");
		wxString extension = "";
		int fileType = ImageHandler::SaveType::JPEG;
		if(fileTypeControl->GetSelection() == 0){ fileType = ImageHandler::SaveType::JPEG; extension = ".jpg"; }
		else if(fileTypeControl->GetSelection() == 1){ fileType = ImageHandler::SaveType::PNG; extension = ".png"; }
		else if(fileTypeControl->GetSelection() == 2){ fileType = ImageHandler::SaveType::TIFF8; extension = ".tif"; }
		else if(fileTypeControl->GetSelection() == 3){ fileType = ImageHandler::SaveType::TIFF16; extension = ".tif"; }
		else if(fileTypeControl->GetSelection() == 4){ fileType = ImageHandler::SaveType::BMP; extension = ".bmp"; }
		else{}
		int jpegQual = (int)jpegQualitySlide->GetValue();

		wxString fileName = outputFolderText->GetValue() + wxFileName::GetPathSeparator() + outputNameText->GetValue() + extension;
		
		// Save the image
		ImageHandler::SaveImageToFile(fileName, proc->GetImage(), fileType, jpegQual);

	}
}

void ExportWindow::SetProgressEditMsg(wxCommandEvent & evt){

	// Not right event type, return
	if(evt.GetEventType() != UPDATE_EXPORT_PROGRESS_MSG_EVENT){
		return;
	}

	progressMutex.Lock();
	currentEditString = evt.GetString();
	progressMutex.Unlock();
}

void ExportWindow::SetProgressEditNum(wxCommandEvent & evt){
	
	// Not right event type, return
	if(evt.GetEventType() != UPDATE_EXPORT_PROGRESS_NUM_EVENT){
		return;
	}

	progressMutex.Lock();
	currentEditNumber = evt.GetInt();
	progressMutex.Unlock();
}

void ExportWindow::SetEditNum(int editNum){

	// Fire event with int to update to
	wxCommandEvent evt(UPDATE_EXPORT_PROGRESS_NUM_EVENT, ID_UPDATE_EXPORT_PROGRESS_NUM_EVENT);
	evt.SetInt(editNum);
	wxPostEvent(this, evt);
}

void ExportWindow::SetMessage(wxString message){

	// Fire event with string to update message to
	wxCommandEvent evt(UPDATE_EXPORT_PROGRESS_MSG_EVENT, ID_UPDATE_EXPORT_PROGRESS_MSG_EVENT);
	evt.SetString(message);
	wxPostEvent(this, evt);
}

int ExportWindow::GetImageType() {
	return fileTypeControl->GetSelection();
}

void ExportWindow::SetImageType(int type) {
	fileTypeControl->SetSelection(type);

	if (fileTypeControl->GetSelection() == 0) {
		jpegQualityLabel->Show();
		jpegQualitySlide->Show();
	}
	else {
		jpegQualityLabel->Hide();
		jpegQualitySlide->Hide();
	}
	this->Layout();
}

int ExportWindow::GetJPEGQuality() {
	return (int)jpegQualitySlide->GetValue();
}

void ExportWindow::SetJPEGQuality(int quality) {
	jpegQualitySlide->SetValue((double)quality);
}

wxString ExportWindow::GetExportFolder() {
	return outputFolderText->GetValue();
}
void ExportWindow::SetExportFolder(wxString folder) {
	outputFolderText->SetValue(folder);
}

wxString ExportWindow::GetExportName() {
	return outputNameText->GetValue();
}

void ExportWindow::SetExportName(wxString name) {
	outputNameText->SetValue(name);
}

void ExportWindow::SaveProject() {

	wxCommandEvent evt(SAVE_PROJECT_EVENT, ID_SAVE_PROJECT);
	wxPostEvent(this->GetParent(), evt);
}