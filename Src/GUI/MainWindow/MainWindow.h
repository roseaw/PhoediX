// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

// for compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

// for all others, include the necessary headers explicitly
#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/aui/aui.h"
#include "wx/thread.h"
#include "wx/timer.h"
#include "wx/msgdlg.h"

#include "Processing/Processor/Processor.h"
#include "Processing/ImageHandler/ImageHandler.h"
#include "GUI/ExportWindow/ExportWindow.h"
#include "GUI/PixelPeepWindow/PixelPeepWindow.h"
#include "GUI/SettingsWindow/SettingsWindow.h"
#include "GUI/ImageDisplay/ZoomImagePanel/ZoomImagePanel.h"
#include "GUI/EditList/EditListPanel/EditListPanel.h"
#include "GUI/SnapshotWindow/SnapshotWindow.h"
#include "GUI/AUIManager/AUIManager.h"
#include "GUI/Colors/Colors.h"
#include "GUI/HistogramDisplay/HistogramDisplay.h"
#include "GUI/AboutWindow/AboutWindow.h"
#include "GUI/LibraryWindow/LibraryWindow.h"
#include "GUI/SupportedCamerasWindow/SupportedCamerasWindow.h"
#include "GUI/Controls/ExifRead/ExifRead.h"
#include "GUI/GuidelinesWindow/GuidelinesWindow.h"
#include "Session/Session.h"
#include "Debugging/MemoryLeakCheck.h"

/**
	Main Window is the main display window of PhoediX.
*/
class MainWindow : public wxFrame {
	
public:
	/**
		Constructor for the main window.
	*/
	MainWindow(wxApp * application);
	bool OriginalImageDispalyed();
	void OpenFiles(wxArrayString files);
	void OnOpenWindow(wxCommandEvent& evt);
private:

	void SaveProject(wxCommandEvent& WXUNUSED(event));
	void CloseCurrentProject(wxCommandEvent& WXUNUSED(event));
	void CloseAllProjects(wxCommandEvent& WXUNUSED(event));
	void ShowLoadImage(wxCommandEvent& WXUNUSED(event));
	void ShowExport(wxCommandEvent& WXUNUSED(event));
	void ShowImage(wxCommandEvent& WXUNUSED(event));
	void ShowPixelPeep(wxCommandEvent& WXUNUSED(event));
	void ShowSnapshots(wxCommandEvent& WXUNUSED(event));
	void ShowGuidelines(wxCommandEvent& WXUNUSED(event));
	void ShowLibrary(wxCommandEvent& WXUNUSED(event));
	void ShowOriginal(wxCommandEvent& WXUNUSED(event));
	void ShowOriginalWindow(wxCommandEvent& WXUNUSED(event));
	void OnEnableFastEdit(wxCommandEvent& evt);
	void ShowSettings(wxCommandEvent& WXUNUSED(event));
	void ShowAbout(wxCommandEvent& WXUNUSED(event));
	void ShowSupportedCameras(wxCommandEvent& WXUNUSED(event));
	void ShowEditList(wxCommandEvent& evt);
	void ShowHistograms(wxCommandEvent& evt);
	void ShowImageInfo(wxCommandEvent& evt);
	void OnReprocessTimer(wxTimerEvent& evt);
	void OnImagePanelMouse(wxMouseEvent & evt);
	void OnReprocess(wxCommandEvent& WXUNUSED(event));
	void OnForceReprocess(wxCommandEvent& WXUNUSED(event));

	void CreateNewProject(wxString projectFile, bool rawProject);
	void OpenImage(wxString imagePath, bool checkForProject = true);
	void ShowImageRelatedWindows();
	void ReloadImage(wxCommandEvent& WXUNUSED(evt));
	void LoadProject(wxString projectPath);
	void OnImportImageNewProject(wxCommandEvent& evt);

	void OpenSession(PhoediXSession * session);
	void SaveCurrentSession();
	void CloseSession(PhoediXSession * session);
	void SetUniqueID(PhoediXSession * session);
	void CheckUncheckSession(int sessionID);
	void EnableDisableMenuItemsNoProject(bool enable);

	void SetSizeProperties();

	void SetMenuChecks();
	void OnPaneClose(wxAuiManagerEvent& evt);

	void SetStatusbarText(wxString text);
	void ClearStatusbarText(wxTimerEvent& WXUNUSED(evt));
	void OnClose(wxCloseEvent& WXUNUSED(evt));

	void RecieveMessageFromProcessor(wxCommandEvent& messageEvt);
	void RecieveNumFromProcessor(wxCommandEvent& numEvt);
	void RecieveRawComplete(wxCommandEvent& WXUNUSED(evt));
	void OnUpdateImage(wxCommandEvent& WXUNUSED(event));

	bool CheckSessionNeedsSaved(PhoediXSession * session);

	wxAuiManager * auiManager;

	wxMenuBar * menuBar;
	wxMenu * menuFile;
	wxMenu * menuCloseProjects;
	wxMenu * menuView;
	wxMenu * menuTools;
	wxMenu * menuWindow;
	wxMenu * menuHelp;
	wxStaticText * statusBarText;

	EditListPanel * editList;
	ExportWindow * exportWindow;
	SettingsWindow * settingsWindow;
	PixelPeepWindow * pixelPeepWindow;
	GuidelinesWindow * guidelinesWindow;
	LibraryWindow * libraryWindow;
	SnapshotWindow * snapshotWindow;
	SupportedCamerasWindow * supportedCamerasWindow;
	AboutWindow * aboutWindow;

	Processor * processor;

	ZoomImagePanel * imagePanel;
	ZoomImagePanel * originalImagePanel;
	wxImage * emptyImage;
	Image * emptyPhxImage;
	HistogramDisplay * histogramDisplay;
	ExifReadWindow * imageInfoPanel;

	PhoediXSession * currentSession;
	wxVector<PhoediXSession*> allSessions;

	int numnUnnamedProjectsOpen;

	wxTimer * clearStatusTimer;
	wxApp * app;

	enum MenuBar {

		// File
		ID_NEW_PROJECT = 1,
		ID_SHOW_LOAD_PROJECT,
		ID_QUICK_SAVE_PROJECT,
		ID_SHOW_SAVE_PROJECT,
		ID_CLOSE_CURRENT_PROJECT,
		ID_CLOSE_ALL_PROJECTS,
		ID_SHOW_LOAD_FILE,
		ID_SHOW_EXPORT,
		ID_SHOW_SETTINGS,
		ID_EXIT,

		// View
		ID_SHOW_IMAGE,
		ID_SHOW_EDIT_LIST,
		ID_SHOW_HISTOGRAMS,
		ID_SHOW_IMAGE_INFO,

		// Tools
		ID_ENABLE_FAST_EDIT,
		ID_FORCE_REPROCESS,
		ID_SHOW_ORIGINAL,
		ID_SHOW_ORIGINAL_WINDOW,
		ID_SHOW_SNAPSHOTS,
		ID_SHOW_PIXEL_PEEP,
		ID_SHOW_GUIDELINES,
		ID_SHOW_LIBRARY,

		// Help
		ID_SHOW_SUPPORTED_CAMERAS,
		ID_ABOUT
	};

	enum EVENT_IDS{
		EVT_CLEAR_STATUS_TIMER = 1
	};

	class ChangeImageThread : public wxThread {

		public:
			ChangeImageThread(ZoomImagePanel * imagePanel, Processor * processor, bool fitImage);

		protected:
			virtual wxThread::ExitCode Entry();

		private:
			ZoomImagePanel * imgPanel;
			Processor * proc;
			bool fit;
	};

};		
#endif
