// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "PhoediX.h"

//#define COMMAND_LOG

IMPLEMENT_APP(PhoediX)

// 'Main program' equivalent: the program execution "starts" here
bool PhoediX::OnInit(){

#ifdef CHECK_MEMORY_LEAK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (!wxApp::OnInit()){
		return false;
	}

#ifdef COMMAND_LOG
	Logger::SetApp(this);
	Logger::SetLogFileName();
	Logger::SetLogLevel(Logger::LogLevel::LOG_VERBOSE);
	Logger::LogToConsole(true);
	Logger::LogToFile(true);
#endif
	wxInitAllImageHandlers();
	Image::InitExif();

    // Create and show the main window
	Logger::Log("PhoediX App - Creating and Showing Main Window", Logger::LogLevel::LOG_VERBOSE);
	MainWindow * mainWindow = new MainWindow(this);
	mainWindow->Show();
	mainWindow->OpenFiles(filesToOpen);
    return true;
}

void PhoediX::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(cmdLineDesc);
	// must refuse '/' as parameter starter or cannot use "/path" style paths
	parser.SetSwitchChars(wxT("-"));
}

bool PhoediX::OnCmdLineParsed(wxCmdLineParser& parser) {
	for (int i = 0; i < parser.GetParamCount(); i++){
		filesToOpen.Add(parser.GetParam(i));
	}
	return true;
}