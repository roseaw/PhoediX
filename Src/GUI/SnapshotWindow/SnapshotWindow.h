// Copyright 2016 Jacob Chesley

#ifndef SNAPSHOT_WINDOW_H
#define SNAPSHOT_WINDOW_H

// for compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

// for all others, include the necessary headers explicitly
#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif

#include "wx/dataview.h"
#include "wx\wrapsizer.h"

#include "Processing\Snapshot\Snapshot.h"
#include "GUI/AUI Manager/AUIManager.h"
#include "GUI\EditList\EditListPanel\EditListPanel.h"
#include "GUI\Colors\Colors.h"
#include "Debugging\MemoryLeakCheck.h"

class SnapshotWindow : public wxScrolledWindow {

public:
	SnapshotWindow(wxWindow * parent, EditListPanel * editListPanel, Processor * processor);
	void AddSnapshots(wxVector<Snapshot*> snapshotsToLoad);
	void DeleteAllSnapshots();
	wxVector<Snapshot*> GetSnapshots();

private:

	void OnResize(wxSizeEvent& WXUNUSED(evt));

	void OnRemoveSnapshot(wxCommandEvent & WXUNUSED(evt));
	void OnRestoreSnapshot(wxCommandEvent & WXUNUSED(evt));
	void OnTakeSnapshot(wxCommandEvent & WXUNUSED(evt));

	wxString GetUniqueName(wxString tryName);

	wxVector<Snapshot*> snapshots;

	wxBoxSizer * mainSizer;
	wxDataViewListCtrl * snapshotList;

	wxWrapSizer * buttonSizer;

	wxButton * removeSnapshot;
	wxButton * restoreSnapshot;
	wxButton * takeSnapshot;

	int curID;

	enum Buttons{
		ID_REMOVE_SNAPSHOT,
		ID_RESTORE_SNAPSHOT,
		ID_TAKE_SNAPSHOT
	};

	EditListPanel * editPanel;
	Processor * proc;
};

#endif