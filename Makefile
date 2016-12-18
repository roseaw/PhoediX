WX_BUILD_PATH = /home/jacob/Development/wxWidgets-master/build-release/
LIBRAW_PATH = /home/jacob/Development/LibRaw-0.18.0-Beta2
LIBTIFF_PATH = /home/jacob/Development/tiff-4.0.7

# No need to modify any lines below.

LIBRAW_INCLUDE_PATH = $(LIBRAW_PATH)/libraw
LIBTIFF_INCLUDE_PATH = $(LIBTIFF_PATH)/libtiff

LIBRAW_LINK_PATH = $(LIBRAW_PATH)/lib/.libs
LIBTIFF_LINK_PATH = $(LIBTIFF_PATH)/libtiff/.libs

CC = g++
COMPILE_FLAG = -w -c -fopenmp

WX_CXXFLAGS = $(shell $(WX_BUILD_PATH)./wx-config --cxxflags)
WX_LFALGS = $(shell $(WX_BUILD_PATH)./wx-config --libs all)
BUILD_DIR = ./Build
OUT_DIR = $(BUILD_DIR)/Objects
INCLUDE_ALL = -I$(SRC) $(WX_CXXFLAGS) -I$(LIBRAW_INCLUDE_PATH) -I$(LIBTIFF_INCLUDE_PATH)

LL = g++
LINK_FLAG = -o
LINK_ALL = $(WX_LFALGS)	$(LIBRAW_LINK_PATH)/libraw.so $(LIBTIFF_LINK_PATH)/libtiff.so -lgomp

SRC=./Src

All:
	$(MAKE) clean
	$(MAKE) AllObjects
	$(MAKE) Link
#App
AppObjects = PhoediX.o
AppObjectsOut = $(OUT_DIR)/PhoediX.o
App : $(AppObjects)

SRC_APP = $(SRC)/App
PhoediX.o : $(SRC_APP)/PhoediX.h $(SRC_APP)/PhoediX.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/PhoediX.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_APP)/PhoediX.cpp

#GUI

#Main GUI
MainGUIObjects = AboutWindow.o AUIManager.o Colors.o ExportWindow.o HistogramDisplay.o Icons.o ImportImageDialog.o MainWindow.o PixelPeepWindow.o LibraryWindow.o LibraryImage.o DirectorySelections.o SettingsWindow.o SnapshotWindow.o SnapshotRenameDialog.o SupportedCamerasWindow.o
MainGUIObjectsOut = $(OUT_DIR)/AboutWindow.o $(OUT_DIR)/AUIManager.o $(OUT_DIR)/Colors.o $(OUT_DIR)/ExportWindow.o $(OUT_DIR)/HistogramDisplay.o $(OUT_DIR)/Icons.o $(OUT_DIR)/ImportImageDialog.o $(OUT_DIR)/MainWindow.o $(OUT_DIR)/PixelPeepWindow.o $(OUT_DIR)/LibraryWindow.o $(OUT_DIR)/LibraryImage.o $(OUT_DIR)/DirectorySelections.o $(OUT_DIR)/SettingsWindow.o $(OUT_DIR)/SnapshotWindow.o $(OUT_DIR)/SnapshotRenameDialog.o $(OUT_DIR)/SupportedCamerasWindow.o
MainGUI: $(MainGUIObjects)
SRC_GUI = $(SRC)/GUI

SRC_ABOUTWINDOW = $(SRC_GUI)/AboutWindow
SRC_AUIMANAGER = $(SRC_GUI)/AUIManager
SRC_COLORS = $(SRC_GUI)/Colors
SRC_EXPORTWINDOW = $(SRC_GUI)/ExportWindow
SRC_HISTOGRAMDISPLAY = $(SRC_GUI)/HistogramDisplay
SRC_ICONS = $(SRC_GUI)/Icons
SRC_IMPORTIMAGEDIALOG = $(SRC_GUI)/ImportImageDialog
SRC_MAINWINDOW = $(SRC_GUI)/MainWindow
SRC_PIXELPEEPWINDOW = $(SRC_GUI)/PixelPeepWindow
SRC_LIBRARYWINDOW = $(SRC_GUI)/LibraryWindow
SRC_LIBRARYWINDOW_LIBRARYIMAGE = $(SRC_LIBRARYWINDOW)/LibraryImage
SRC_LIBRARYWINDOW_DIRECTORYSELECTIONS = $(SRC_LIBRARYWINDOW)/DirectorySelections
SRC_SETTINGSWINDOW = $(SRC_GUI)/SettingsWindow
SRC_SNAPSHOTWINDOW = $(SRC_GUI)/SnapshotWindow
SRC_SNAPSHOTWINDOW_SNAPSHOTRENAMEDIALOG = $(SRC_SNAPSHOTWINDOW)/SnapshotRenameDialog
SRC_SUPPORTEDCAMERASWINDOW = $(SRC_GUI)/SupportedCamerasWindow

AboutWindow.o : $(SRC_ABOUTWINDOW)/AboutWindow.h $(SRC_ABOUTWINDOW)/AboutWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AboutWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_ABOUTWINDOW)/AboutWindow.cpp

AUIManager.o : $(SRC_AUIMANAGER)/AUIManager.h $(SRC_AUIMANAGER)/AUIManager.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AUIManager.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_AUIMANAGER)/AUIManager.cpp

Colors.o : $(SRC_COLORS)/Colors.h $(SRC_COLORS)/Colors.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/Colors.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_COLORS)/Colors.cpp

ExportWindow.o : $(SRC_EXPORTWINDOW)/ExportWindow.h $(SRC_EXPORTWINDOW)/ExportWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ExportWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EXPORTWINDOW)/ExportWindow.cpp

HistogramDisplay.o : $(SRC_HISTOGRAMDISPLAY)/HistogramDisplay.h $(SRC_HISTOGRAMDISPLAY)/HistogramDisplay.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/HistogramDisplay.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_HISTOGRAMDISPLAY)/HistogramDisplay.cpp

Icons.o : $(SRC_ICONS)/Icons.h $(SRC_ICONS)/Icons.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/Icons.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_ICONS)/Icons.cpp

ImportImageDialog.o : $(SRC_IMPORTIMAGEDIALOG)/ImportImageDialog.h $(SRC_IMPORTIMAGEDIALOG)/ImportImageDialog.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ImportImageDialog.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMPORTIMAGEDIALOG)/ImportImageDialog.cpp

MainWindow.o : $(SRC_MAINWINDOW)/MainWindow.h $(SRC_MAINWINDOW)/MainWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/MainWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_MAINWINDOW)/MainWindow.cpp

PixelPeepWindow.o : $(SRC_PIXELPEEPWINDOW)/PixelPeepWindow.h $(SRC_PIXELPEEPWINDOW)/PixelPeepWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/PixelPeepWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_PIXELPEEPWINDOW)/PixelPeepWindow.cpp

LibraryWindow.o : $(SRC_LIBRARYWINDOW)/LibraryWindow.h $(SRC_LIBRARYWINDOW)/LibraryWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/LibraryWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_LIBRARYWINDOW)/LibraryWindow.cpp

LibraryImage.o : $(SRC_LIBRARYWINDOW_LIBRARYIMAGE)/LibraryImage.h $(SRC_LIBRARYWINDOW_LIBRARYIMAGE)/LibraryImage.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/LibraryImage.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_LIBRARYWINDOW_LIBRARYIMAGE)/LibraryImage.cpp

DirectorySelections.o : $(SRC_LIBRARYWINDOW_DIRECTORYSELECTIONS)/DirectorySelections.h $(SRC_LIBRARYWINDOW_DIRECTORYSELECTIONS)/DirectorySelections.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/DirectorySelections.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_LIBRARYWINDOW_DIRECTORYSELECTIONS)/DirectorySelections.cpp

SettingsWindow.o : $(SRC_SETTINGSWINDOW)/SettingsWindow.h $(SRC_SETTINGSWINDOW)/SettingsWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/SettingsWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SETTINGSWINDOW)/SettingsWindow.cpp

SnapshotWindow.o : $(SRC_SNAPSHOTWINDOW)/SnapshotWindow.h $(SRC_SNAPSHOTWINDOW)/SnapshotWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/SnapshotWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SNAPSHOTWINDOW)/SnapshotWindow.cpp

SnapshotRenameDialog.o : $(SRC_SNAPSHOTWINDOW_SNAPSHOTRENAMEDIALOG)/SnapshotRenameDialog.h $(SRC_SNAPSHOTWINDOW_SNAPSHOTRENAMEDIALOG)/SnapshotRenameDialog.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/SnapshotRenameDialog.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SNAPSHOTWINDOW_SNAPSHOTRENAMEDIALOG)/SnapshotRenameDialog.cpp

SupportedCamerasWindow.o : $(SRC_SUPPORTEDCAMERASWINDOW)/SupportedCamerasWindow.h $(SRC_SUPPORTEDCAMERASWINDOW)/SupportedCamerasWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/SupportedCamerasWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SUPPORTEDCAMERASWINDOW)/SupportedCamerasWindow.cpp

#Edit List
EditListObjects = AvailableEdits.o EditListItem.o EditListPanel.o EditSelection.o EditWindow.o
EditListObjectsOut = $(OUT_DIR)/AvailableEdits.o $(OUT_DIR)/EditListItem.o $(OUT_DIR)/EditListPanel.o $(OUT_DIR)/EditSelection.o $(OUT_DIR)/EditWindow.o
EditList : $(EditListObjects)

SRC_EDITLIST = $(SRC_GUI)/EditList
SRC_EDITLIST_AVAILABLEEDITS = $(SRC_EDITLIST)/AvailableEdits
SRC_EDITLIST_EDITLISTITEM = $(SRC_EDITLIST)/EditListItem
SRC_EDITLIST_EDITLISTPANEL = $(SRC_EDITLIST)/EditListPanel
SRC_EDITLIST_EDITSELECTION = $(SRC_EDITLIST)/EditSelection
SRC_EDITLIST_EDITWINDOW = $(SRC_EDITLIST)/EditWindow

AvailableEdits.o : $(SRC_EDITLIST_AVAILABLEEDITS)/AvailableEdits.h $(SRC_EDITLIST_AVAILABLEEDITS)/AvailableEdits.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AvailableEdits.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITLIST_AVAILABLEEDITS)/AvailableEdits.cpp

EditListItem.o : $(SRC_EDITLIST_EDITLISTITEM)/EditListItem.h $(SRC_EDITLIST_EDITLISTITEM)/EditListItem.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/EditListItem.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITLIST_EDITLISTITEM)/EditListItem.cpp

EditListPanel.o : $(SRC_EDITLIST_EDITLISTPANEL)/EditListPanel.h $(SRC_EDITLIST_EDITLISTPANEL)/EditListPanel.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/EditListPanel.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITLIST_EDITLISTPANEL)/EditListPanel.cpp

EditSelection.o : $(SRC_EDITLIST_EDITSELECTION)/EditSelection.h $(SRC_EDITLIST_EDITSELECTION)/EditSelection.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/EditSelection.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITLIST_EDITSELECTION)/EditSelection.cpp

EditWindow.o : $(SRC_EDITLIST_EDITWINDOW)/EditWindow.h $(SRC_EDITLIST_EDITWINDOW)/EditWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/EditWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITLIST_EDITWINDOW)/EditWindow.cpp

#Edit Windows
EditWindowObjects = AvailableEditWindows.o AdjustBrightnessWindow.o AdjustHSLWindow.o ChannelMixerWindow.o ContrastWindow.o CropWindow.o ConvertGreyscaleWindow.o HSLCurvesWindow.o LABCurvesWindow.o MirrorWindow.o RawWindow.o RGBCurvesWindow.o RotationWindow.o ScaleWindow.o ShiftRGBWindow.o
EditWindowObjectsOut = $(OUT_DIR)/AvailableEditWindows.o $(OUT_DIR)/AdjustBrightnessWindow.o $(OUT_DIR)/AdjustHSLWindow.o $(OUT_DIR)/ChannelMixerWindow.o $(OUT_DIR)/ContrastWindow.o $(OUT_DIR)/CropWindow.o $(OUT_DIR)/ConvertGreyscaleWindow.o $(OUT_DIR)/HSLCurvesWindow.o $(OUT_DIR)/LABCurvesWindow.o $(OUT_DIR)/MirrorWindow.o $(OUT_DIR)/RawWindow.o $(OUT_DIR)/RGBCurvesWindow.o $(OUT_DIR)/RotationWindow.o $(OUT_DIR)/ScaleWindow.o $(OUT_DIR)/ShiftRGBWindow.o
EditWindows : $(EditWindowObjects)

SRC_EDITLIST = $(SRC_GUI)/EditList
SRC_AVAILABLEEDITWINDOWS = $(SRC_EDITLIST)/AvailableEditWindows
SRC_EDITWINDOW_ADJUSTBRIGHTNESSWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/AdjustBrightnessWindow
SRC_EDITWINDOW_ADJUSTHSLWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/AdjustHSLWindow
SRC_EDITWINDOW_CHANNELMIXERWINDOW= $(SRC_AVAILABLEEDITWINDOWS)/ChannelMixerWindow
SRC_EDITWINDOW_CONTRASTWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/ContrastWindow
SRC_EDITWINDOW_CONVERTGREYSCALEWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/ConvertGreyscaleWindow
SRC_EDITWINDOW_CROPWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/CropWindow
SRC_EDITWINDOW_HSLCURVESWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/HSLCurvesWindow
SRC_EDITWINDOW_LABCURVESWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/LABCurvesWindow
SRC_EDITWINDOW_MIRRORWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/MirrorWindow
SRC_EDITWINDOW_RAWWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/RawWindow
SRC_EDITWINDOW_RGBCURVESWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/RGBCurvesWindow
SRC_EDITWINDOW_ROTATIONWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/RotationWindow
SRC_EDITWINDOW_SCALEWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/ScaleWindow
SRC_EDITWINDOW_SHIFTRGBWINDOW = $(SRC_AVAILABLEEDITWINDOWS)/ShiftRGBWindow

AvailableEditWindows.o : $(SRC_AVAILABLEEDITWINDOWS)/AvailableEditWindows.h $(SRC_AVAILABLEEDITWINDOWS)/AvailableEditWindows.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AvailableEditWindows.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_AVAILABLEEDITWINDOWS)/AvailableEditWindows.cpp

AdjustBrightnessWindow.o : $(SRC_EDITWINDOW_ADJUSTBRIGHTNESSWINDOW)/AdjustBrightnessWindow.h $(SRC_EDITWINDOW_ADJUSTBRIGHTNESSWINDOW)/AdjustBrightnessWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AdjustBrightnessWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_ADJUSTBRIGHTNESSWINDOW)/AdjustBrightnessWindow.cpp

AdjustHSLWindow.o : $(SRC_EDITWINDOW_ADJUSTHSLWINDOW)/AdjustHSLWindow.h $(SRC_EDITWINDOW_ADJUSTHSLWINDOW)/AdjustHSLWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/AdjustHSLWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_ADJUSTHSLWINDOW)/AdjustHSLWindow.cpp

ChannelMixerWindow.o : $(SRC_EDITWINDOW_CHANNELMIXERWINDOW)/ChannelMixerWindow.h $(SRC_EDITWINDOW_CHANNELMIXERWINDOW)/ChannelMixerWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ChannelMixerWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_CHANNELMIXERWINDOW)/ChannelMixerWindow.cpp

ContrastWindow.o : $(SRC_EDITWINDOW_CONTRASTWINDOW)/ContrastWindow.h $(SRC_EDITWINDOW_CONTRASTWINDOW)/ContrastWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ContrastWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_CONTRASTWINDOW)/ContrastWindow.cpp

CropWindow.o : $(SRC_EDITWINDOW_CROPWINDOW)/CropWindow.h $(SRC_EDITWINDOW_CROPWINDOW)/CropWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/CropWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_CROPWINDOW)/CropWindow.cpp

ConvertGreyscaleWindow.o : $(SRC_EDITWINDOW_CONVERTGREYSCALEWINDOW)/ConvertGreyscaleWindow.h $(SRC_EDITWINDOW_CONVERTGREYSCALEWINDOW)/ConvertGreyscaleWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ConvertGreyscaleWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_CONVERTGREYSCALEWINDOW)/ConvertGreyscaleWindow.cpp

HSLCurvesWindow.o : $(SRC_EDITWINDOW_HSLCURVESWINDOW)/HSLCurvesWindow.h $(SRC_EDITWINDOW_HSLCURVESWINDOW)/HSLCurvesWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/HSLCurvesWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_HSLCURVESWINDOW)/HSLCurvesWindow.cpp

LABCurvesWindow.o : $(SRC_EDITWINDOW_LABCURVESWINDOW)/LABCurvesWindow.h $(SRC_EDITWINDOW_LABCURVESWINDOW)/LABCurvesWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/LABCurvesWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_LABCURVESWINDOW)/LABCurvesWindow.cpp

MirrorWindow.o : $(SRC_EDITWINDOW_MIRRORWINDOW)/MirrorWindow.h $(SRC_EDITWINDOW_MIRRORWINDOW)/MirrorWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/MirrorWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_MIRRORWINDOW)/MirrorWindow.cpp

RawWindow.o : $(SRC_EDITWINDOW_RAWWINDOW)/RawWindow.h $(SRC_EDITWINDOW_RAWWINDOW)/RawWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/RawWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_RAWWINDOW)/RawWindow.cpp

RGBCurvesWindow.o : $(SRC_EDITWINDOW_RGBCURVESWINDOW)/RGBCurvesWindow.h $(SRC_EDITWINDOW_RGBCURVESWINDOW)/RGBCurvesWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/RGBCurvesWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_RGBCURVESWINDOW)/RGBCurvesWindow.cpp

RotationWindow.o : $(SRC_EDITWINDOW_ROTATIONWINDOW)/RotationWindow.h $(SRC_EDITWINDOW_ROTATIONWINDOW)/RotationWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/RotationWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_ROTATIONWINDOW)/RotationWindow.cpp

ScaleWindow.o : $(SRC_EDITWINDOW_SCALEWINDOW)/ScaleWindow.h $(SRC_EDITWINDOW_SCALEWINDOW)/ScaleWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ScaleWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_SCALEWINDOW)/ScaleWindow.cpp

ShiftRGBWindow.o : $(SRC_EDITWINDOW_SHIFTRGBWINDOW)/ShiftRGBWindow.h $(SRC_EDITWINDOW_SHIFTRGBWINDOW)/ShiftRGBWindow.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ShiftRGBWindow.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_EDITWINDOW_SHIFTRGBWINDOW)/ShiftRGBWindow.cpp

#Controls
ControlsObjects = CollapsiblePane.o CurvesPanel.o DoubleSlider.o 
ControlsObjectsOut = $(OUT_DIR)/CollapsiblePane.o $(OUT_DIR)/CurvesPanel.o $(OUT_DIR)/DoubleSlider.o 
Controls: $(ControlsObjects)

SRC_CONTROLS = $(SRC_GUI)/Controls
SRC_CONTROLS_COLLAPSIBLEPANE = $(SRC_CONTROLS)/CollapsiblePane
SRC_CONTROLS_CURVESPANEL = $(SRC_CONTROLS)/CurvesPanel
SRC_CONTROLS_DOUBLESLIDER = $(SRC_CONTROLS)/DoubleSlider

CollapsiblePane.o : $(SRC_CONTROLS_COLLAPSIBLEPANE)/CollapsiblePane.h $(SRC_CONTROLS_COLLAPSIBLEPANE)/CollapsiblePane.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/CollapsiblePane.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_CONTROLS_COLLAPSIBLEPANE)/CollapsiblePane.cpp

CurvesPanel.o : $(SRC_CONTROLS_CURVESPANEL)/CurvesPanel.h $(SRC_CONTROLS_CURVESPANEL)/CurvesPanel.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/CurvesPanel.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_CONTROLS_CURVESPANEL)/CurvesPanel.cpp

DoubleSlider.o : $(SRC_CONTROLS_DOUBLESLIDER)/DoubleSlider.h $(SRC_CONTROLS_DOUBLESLIDER)/DoubleSlider.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/DoubleSlider.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_CONTROLS_DOUBLESLIDER)/DoubleSlider.cpp

#Image Display
ImageDisplayObjects = ImagePanel.o wxImagePanel.o ZoomImageFrame.o ZoomImagePanel.o 
ImageDisplayObjectsOut = $(OUT_DIR)/ImagePanel.o $(OUT_DIR)/wxImagePanel.o $(OUT_DIR)/ZoomImageFrame.o $(OUT_DIR)/ZoomImagePanel.o 
ImageDisplay: $(ImageDisplayObjects)

SRC_IMAGEDISPLAY = $(SRC_GUI)/ImageDisplay
SRC_IMAGEDISPLAY_IMAGEPANEL = $(SRC_IMAGEDISPLAY)/ImagePanel
SRC_IMAGEDISPLAY_WXIMAGEPANEL = $(SRC_IMAGEDISPLAY)/wxImagePanel
SRC_IMAGEDISPLAY_ZOOMIMAGEFRAME = $(SRC_IMAGEDISPLAY)/ZoomImageFrame
SRC_IMAGEDISPLAY_ZOOMIMAGEPANEL = $(SRC_IMAGEDISPLAY)/ZoomImagePanel

ImagePanel.o : $(SRC_IMAGEDISPLAY_IMAGEPANEL)/ImagePanel.h $(SRC_IMAGEDISPLAY_IMAGEPANEL)/ImagePanel.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ImagePanel.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGEDISPLAY_IMAGEPANEL)/ImagePanel.cpp

wxImagePanel.o : $(SRC_IMAGEDISPLAY_WXIMAGEPANEL)/wxImagePanel.h $(SRC_IMAGEDISPLAY_WXIMAGEPANEL)/wxImagePanel.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/wxImagePanel.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGEDISPLAY_WXIMAGEPANEL)/wxImagePanel.cpp

ZoomImageFrame.o : $(SRC_IMAGEDISPLAY_ZOOMIMAGEFRAME)/ZoomImageFrame.h $(SRC_IMAGEDISPLAY_ZOOMIMAGEFRAME)/ZoomImageFrame.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ZoomImageFrame.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGEDISPLAY_ZOOMIMAGEFRAME)/ZoomImageFrame.cpp

ZoomImagePanel.o : $(SRC_IMAGEDISPLAY_ZOOMIMAGEPANEL)/ZoomImagePanel.h $(SRC_IMAGEDISPLAY_ZOOMIMAGEPANEL)/ZoomImagePanel.cpp $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/ZoomImagePanel.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGEDISPLAY_ZOOMIMAGEPANEL)/ZoomImagePanel.cpp

AllGUIObjects = $(MainGUIObjects) $(EditListObjects) $(EditWindowObjects) $(ControlsObjects) $(ImageDisplayObjects)
AllGUIObjectsOut = $(MainGUIObjectsOut) $(EditListObjectsOut) $(EditWindowObjectsOut) $(ControlsObjectsOut) $(ImageDisplayObjectsOut)
AllGUI : $(AllGUIObjects)

#Processing
ProcessingObjects = Image.o ImageHandler.o Processor.o ProcessorEdit.o RawError.o
ProcessingObjectsOut = $(OUT_DIR)/Image.o $(OUT_DIR)/ImageHandler.o $(OUT_DIR)/Processor.o $(OUT_DIR)/ProcessorEdit.o $(OUT_DIR)/RawError.o
Processing : $(ProcessingObjects)

SRC_PROCESSING = $(SRC)/Processing
SRC_IMAGE = $(SRC_PROCESSING)/Image
SRC_IMAGE_HANDLER = $(SRC_PROCESSING)/ImageHandler
SRC_PROCESSOR = $(SRC_PROCESSING)/Processor
SRC_PROCESSOR_EDIT = $(SRC_PROCESSING)/ProcessorEdit
SRC_RAWERROR = $(SRC_PROCESSING)/RawErrorCodes

Image.o : $(SRC_IMAGE)/Image.h $(SRC_IMAGE)/Image.cpp  $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/Image.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGE)/Image.cpp

ImageHandler.o : $(SRC_IMAGE_HANDLER)/ImageHandler.h $(SRC_IMAGE_HANDLER)/ImageHandler.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/ImageHandler.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_IMAGE_HANDLER)/ImageHandler.cpp

Processor.o : $(SRC_PROCESSOR)/Processor.h $(SRC_PROCESSOR)/Processor.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/Processor.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_PROCESSOR)/Processor.cpp

ProcessorEdit.o : $(SRC_PROCESSOR_EDIT)/ProcessorEdit.h $(SRC_PROCESSOR_EDIT)/ProcessorEdit.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/ProcessorEdit.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_PROCESSOR_EDIT)/ProcessorEdit.cpp

RawError.o : $(SRC_RAWERROR)/RawError.h $(SRC_RAWERROR)/RawError.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/RawError.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_RAWERROR)/RawError.cpp

#Session
SessionObjects = Session.o SessionEditList.o
SessionObjectsOut = $(OUT_DIR)/Session.o $(OUT_DIR)/SessionEditList.o
Sessions : $(SessionObjects)

SRC_SESSION = $(SRC)/Session
SRC_SESSION_EDIT_LIST = $(SRC_SESSION)/SessionEditList

Session.o : $(SRC_SESSION)/Session.h $(SRC_SESSION)/Session.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/Session.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SESSION)/Session.cpp

SessionEditList.o : $(SRC_SESSION_EDIT_LIST)/SessionEditList.h $(SRC_SESSION_EDIT_LIST)/SessionEditList.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/SessionEditList.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SESSION_EDIT_LIST)/SessionEditList.cpp

#Spline
SplineObjects = Spline.o
SplineObjectsOut = $(OUT_DIR)/Spline.o
Splines : $(SplineObjects)

SRC_SPLINE = $(SRC)/Spline

Spline.o : $(SRC_SPLINE)/Spline.h $(SRC_SPLINE)/Spline.cpp $(OUT_DIR) 
	$(CC) -o $(OUT_DIR)/Spline.o $(COMPILE_FLAG) $(INCLUDE_ALL) $(SRC_SPLINE)/Spline.cpp


AllObjectsStr = $(AppObjects) $(AllGUIObjects) $(ProcessingObjects) $(SessionObjects) $(SplineObjects)
AllObjects : $(AllObjectsStr)

AllObjectsOutStr = $(AppObjectsOut) $(AllGUIObjectsOut) $(ProcessingObjectsOut) $(SessionObjectsOut) $(SplineObjectsOut)

Link :
	$(LL) $(LINK_FLAG) PhoediX $(AllObjectsOutStr) $(LINK_ALL)

clean :
	rm -rf $(BUILD_DIR)
	mkdir $(BUILD_DIR)
	mkdir $(OUT_DIR)