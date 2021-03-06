// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "Processor.h"

wxDEFINE_EVENT(PROCESSOR_MESSAGE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(PROCESSOR_NUM_EVENT, wxCommandEvent);
wxDEFINE_EVENT(PROCESSOR_RAW_COMPLETE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(UPDATE_IMAGE_EVENT, wxCommandEvent);

double Processor::pi = 3.141592653589793238463;

typedef int (processor_callback)(void *callback_data,enum LibRaw_progress stage, int iteration, int expected);

bool Processor::forceStopRaw = false;
void * Processor::rawProcessorData = NULL;
enum LibRaw_progress Processor::rawProcessorProgress;
int Processor::rawIteration = 0;
int Processor::rawExpected = 0;

Processor::Processor(wxWindow * parent) {
	img = new Image();
	tempImage = NULL;
	originalImg = NULL;
	didUpdate = false;
	locked = false;
	parWindow = parent;
	rawImage = NULL;
	rawImageGood = false;
	doFitImage = false;
	processThread = NULL;
	forceStop = false;
	forceStopRaw = false;
	lockedRaw = false;
	multiThread = false;
	numThreadsToUse = 1;
	numEdits = 0;
	numStartedThreads = 0;
	isProcessing = false;
	hasEdits = false;
	copiedEdit = NULL;
	editListInternal = wxVector<ProcessorEdit*>();
	editListInternal.clear();

	restartRaw = true;
	fastEdit = false;

	colorSpace = ColorSpaceENUM::sRGB;
}

Processor::~Processor() {
	delete img;
	delete originalImg;
	this->DeleteEdits();
	rawPrcoessor.dcraw_clear_mem(rawImage);
	delete rawImage;
	if(copiedEdit != NULL){
		delete copiedEdit;
	}
}

void Processor::DestroyAll(){

	this->KillRawProcessing();
	this->KillCurrentProcessing();
	img->Destroy();
	originalImg->Destroy();
	this->DeleteEdits();
}

inline double Processor::FastTanH(double x){
	if(x < -3.0) { return -0.99999999; }
	if(x > 3.0){ return 0.99999999; }
	return x * ( 27 + x * x ) / ( 27 + 9 * x * x );
}

Image * Processor::GetImage() {
	return img;
}

Image * Processor::GetTempImage() {
	return tempImage;
}

void Processor::SetOriginalImage(Image *newOriginalImage) {

	if(originalImg != NULL){
		delete originalImg;
	}
	originalImg = NULL;
	originalImg = new Image(*newOriginalImage);
}

Image* Processor::GetOriginalImage() {
	return originalImg;
}

void  Processor::RevertToOriginalImage(bool skipUpdate) {

	if (originalImg == NULL) {
		return;
	}
	delete img;
	img = NULL;
	img = new Image(*originalImg);
	if(!skipUpdate){
		didUpdate = true;
	}
}

void Processor::AddEdit(ProcessorEdit * edit) {
	editListInternal.push_back(edit);
}

void Processor::DeleteEdits() {
	
	editListCritical.Enter();
	// Delete all edits in the internal vector
	for (size_t i = 0; i < editListInternal.size(); i++) {
		editListInternal.at(i)->ClearIntArray();
		editListInternal.at(i)->ClearDoubleArray();
		delete editListInternal.at(i);
	}

	// Clear the vector
	editListInternal.clear();

	editListCritical.Leave();
}

void Processor::SetHasEdits(bool doesHaveEdits){
	hasEdits = doesHaveEdits;
}

bool Processor::GetHasEdits(){
	return hasEdits;
}
wxVector<ProcessorEdit*> Processor::GetEditVector() {
	return editListInternal;
}

void Processor::Enable16Bit() {
	img->Enable16Bit();
	originalImg->Enable16Bit();
}

void Processor::Disable16Bit() {
	img->Disable16Bit();
	originalImg->Disable16Bit();
}

void Processor::SetColorSpace(int newColorSpace) {
	colorSpace = newColorSpace;
}

int Processor::GetColorSpace() {
	return colorSpace;
}

int Processor::ProcessEdits() {

	if(forceStop){
		return -1;
	}

	editListCritical.Enter();
	this->ProcessEdits(editListInternal);
	editListCritical.Leave();
	return 0;
}

void Processor::ProcessEdit(ProcessorEdit * edit) {

	numEdits = 1;

	forceStopCritical.Enter();
	this->forceStop = true;
	forceStopCritical.Leave();

	this->didUpdate = false;

	processThread = new ProcessThread(this, edit);
	processThread->Run();
}

void Processor::ProcessEdits(wxVector<ProcessorEdit*> editList) {

	numEdits = editList.size();
	this->didUpdate = false;

	// Start processing thread on edit list
	processThread = new ProcessThread(this, editList);
	processThread->Run();

	if(editList.size() < 1){ this->didUpdate = true;}
}

void Processor::KillCurrentProcessing(){

	if(this->GetLocked()){
		forceStopCritical.Enter();
		this->forceStop = true;
		forceStopCritical.Leave();
	}
}

void Processor::KillRawProcessing(){

	if (forceStop) { 
	}

	if(this->GetLockedRaw()){
		this->forceStopRaw = true;
	}
}

int Processor::ProcessRaw(){

	if (this->GetLockedRaw()) {
		return - 1;
	}
	forceStopCritical.Enter();
	this->forceStop = true;
	forceStopCritical.Leave();

	RawProcessThread * rawProcThread = new RawProcessThread(this);
	rawProcThread->Run();

	return 0;
}

int Processor::UnpackAndProcessRaw() {
	if (this->GetLockedRaw()) {
		return -1;
	}

	forceStopCritical.Enter();
	this->forceStop = true;
	forceStopCritical.Leave();

	RawProcessThread * rawProcThread = new RawProcessThread(this, true);
	rawProcThread->Run();

	return 0;
}

int Processor::GetLastNumEdits(){
	return numEdits;
}

void Processor::DeleteRawImage(){
	rawImageGood = false;
	rawPrcoessor.dcraw_clear_mem(rawImage);
	rawImage = NULL;
}

bool Processor::RawImageGood(){
	return rawImageGood;
}

int Processor::GetRawError(){
	return rawErrorCode;
}

void Processor::Lock() {

	lockCritical.Enter();
	locked = true;
	
}

void Processor::Unlock() {
	locked = false;
	lockCritical.Leave();
}

bool Processor::GetLocked() {
	return locked;
}

void Processor::LockRaw() {
	lockRawCritical.Enter();
	lockedRaw = true;
}

void Processor::UnlockRaw() {
	lockedRaw = false;
	lockRawCritical.Leave();
}

bool Processor::GetLockedRaw() {
	return lockedRaw;
}

void Processor::ResetForceStop() {
	forceStopCritical.Enter();
	forceStop = false;
	forceStopCritical.Leave();
}
void Processor::SetFilePath(wxString path) {
	filePath = path;
}

wxString Processor::GetFilePath() {
	return filePath;
}

void Processor::SetFileName(wxString name) {
	fileName = name;
}

wxString Processor::GetFileName() {
	return fileName;
}

void Processor::SetUpdated(bool updated) {
	didUpdate = updated;
}

bool Processor::GetUpdated() {
	return didUpdate;
}

void Processor::SetDoFitImage(bool fit) {
	doFitImage = fit;
}

bool Processor::GetDoFitImage() {
	return doFitImage;
}

void Processor::EnableFastEdit(){
	fastEdit = true;
}

void Processor::DisableFastEdit(){
	fastEdit = false;
}

bool Processor::GetFastEdit(){
	return fastEdit;
}

void Processor::SetMultithread(bool doMultiThread){
	multiThread = doMultiThread;
}

bool Processor::GetMultithread(){
	return multiThread;
}

void Processor::SetNumThreads(int numThreads){
	numThreadsToUse = numThreads;
}

int Processor::GetNumThreads(){
	return numThreadsToUse;
}

void Processor::SetParentWindow(wxWindow * parent){
	parWindow = parent;
}

wxWindow * Processor::GetParentWindow(){
	return parWindow;
}

void Processor::SendMessageToParent(wxString message) {

	// Send add edit event with edit ID to parent window (the edit panel)
	if (parWindow != NULL) {
		wxCommandEvent evt(PROCESSOR_MESSAGE_EVENT, ID_PROCESSOR_MESSAGE);
		evt.SetString(message);
		wxPostEvent(parWindow, evt);
	}

}

void Processor::SendProcessorEditNumToParent(int editNum){
	
	// Send add edit event with edit ID to parent window (the edit panel)
	if (parWindow != NULL) {
		wxCommandEvent evt(PROCESSOR_NUM_EVENT, ID_PROCESSOR_NUM);
		evt.SetInt(editNum);
		wxPostEvent(parWindow, evt);
	}
}

void Processor::SendRawComplete() {

	// Send add edit event with edit ID to parent window (the edit panel)
	if (parWindow != NULL) {
		wxCommandEvent evt(PROCESSOR_RAW_COMPLETE_EVENT, ID_RAW_COMPLETE);
		wxPostEvent(parWindow, evt);
	}
}

void Processor::StoreEditForCopyPaste(ProcessorEdit * edit){
	copiedEdit = edit;
}

ProcessorEdit * Processor::GetEditForCopyPaste(){
	return copiedEdit;
}

void Processor::StoreEditListForCopyPaste(wxVector<ProcessorEdit*> editList){
	copiedEditList.clear();

	for(size_t i = 0; i < editList.size(); i++){
		copiedEditList.push_back(new ProcessorEdit(*editList.at(i)));
	}
}

wxVector<ProcessorEdit*> Processor::GetEditListForCopyPaste(){
	return copiedEditList;
}

int Processor::RawCallback(void * data, enum LibRaw_progress p, int iteration, int expected){

	Processor::rawProcessorData = data;
	Processor::rawProcessorProgress = p;
	Processor::rawIteration = iteration;
	Processor::rawExpected = expected;

	if(forceStopRaw){ 
		return 1; 
	}
	else { return 0; }
}

void Processor::Get8BitHistrogram(uint32_t * outputHistogramRed, uint32_t * outputHistogramGreen, uint32_t * outputHistogramBlue, uint32_t * outputHistogramGrey) {
	
	if(img == NULL){ return; }
	int dataSize = img->GetWidth() * img->GetHeight();

	// Initialize the histrogram to 0
	for (int i = 0; i < 256; i++) {
		outputHistogramRed[i] = 0;
		outputHistogramGreen[i] = 0;
		outputHistogramBlue[i] = 0;
		outputHistogramGrey[i] = 0;
	}

	int grey = 0;

	// Compute the histogram for red, green, blue and grey

	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = 0; i < dataSize; i++) {

			if (forceStop) { return; }

			outputHistogramRed[redData8[i]] += 1;
			outputHistogramGreen[greenData8[i]] += 1;
			outputHistogramBlue[blueData8[i]] += 1;

			grey = (0.2126 * (redData8[i])) + (0.7152 * (greenData8[i])) + (0.0722 * (blueData8[i]));
			grey = grey > 255 ? 255 : grey;
			grey = grey < 0 ? 0 : grey;
			outputHistogramGrey[grey] += 1;
		}
	}
	else{

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();
		for (int i = 0; i < dataSize; i++) {

			if (forceStop) { return; }

			outputHistogramRed[(uint16_t)(redData16[i]/256.0)] += 1;
			outputHistogramGreen[(uint16_t)(greenData16[i]/256.0)] += 1;
			outputHistogramBlue[(uint16_t)(blueData16[i]/256.0)] += 1;

			grey = (0.2126 * (uint16_t)(redData16[i]/256.0)) + (0.7152 * (uint16_t)(greenData16[i]/256.0))+ (0.0722 * (uint16_t)(blueData16[i]/256.0));
			grey = grey > 255 ? 255 : grey;
			grey = grey < 0 ? 0 : grey;
			outputHistogramGrey[grey] += 1;
		}
	}
}

void Processor::Get16BitHistrogram(uint32_t * outputHistogramRed, uint32_t * outputHistogramGreen, uint32_t * outputHistogramBlue, uint32_t * outputHistogramGrey) {

	int dataSize = img->GetWidth() * img->GetHeight();

	// Get pointers to 16 bit data
	uint16_t * redData16 = img->Get16BitDataRed();
	uint16_t * greenData16 = img->Get16BitDataGreen();
	uint16_t * blueData16 = img->Get16BitDataBlue();

	// Initialize the histrogram to 0
	for (int i = 0; i < 65536; i++) {
		outputHistogramRed[i] = 0;
		outputHistogramGreen[i] = 0;
		outputHistogramBlue[i] = 0;
	}

	int grey = 0;

	// Compute the histogram for red, green, blue and grey
	for (int i = 0; i < dataSize; i++) {

		if (forceStop) { return; }
		outputHistogramRed[redData16[i]] += 1;
		outputHistogramGreen[greenData16[i]] += 1;
		outputHistogramBlue[blueData16[i]] += 1;

		// Compute grey scale level
		grey = (int)((redData16[i] * 0.2126) + (greenData16[i] * 0.7152) + (blueData16[i] * 0.0722));
		grey = grey > 255 ? 255 : grey;
		grey = grey < 0 ? 0 : grey;
		outputHistogramGrey[grey] += 1;
	}
}


void Processor::AdjustRGB(double all, double red, double green, double blue, int dataStart, int dataEnd) {
	
	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	// Create a temporary set of pixels that will allow handling of overflow
	int tempRed;
	int tempGreen;
	int tempBlue;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Calculate 8 bit shift values.  Need 16 bits because this is now signed
		int16_t all8 = (int16_t)(all * 255);
		int16_t red8 = (int16_t)(red * 255);
		int16_t green8 = (int16_t)(green * 255);
		int16_t blue8 = (int16_t)(blue * 255);

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }
			// Get red, green and blue temporary pixels
			tempRed = redData8[i];
			tempGreen = greenData8[i];
			tempBlue = blueData8[i];

			// add the 8 bit value to the pixels
			tempRed += (red8 + all8);
			tempGreen += (green8 + all8);
			tempBlue += (blue8 + all8);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Calculate 16 bit shift values.  Need 32 bits because this is now signed.
		int32_t all16 = (int32_t)(all * 65535.0);
		int32_t red16 = (int32_t)(red * 65535.0);
		int32_t green16 = (int32_t)(green * 65535.0);
		int32_t blue16 = (int32_t)(blue * 65535.0);

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get red, green and blue temporary pixels
			tempRed = redData16[i];
			tempGreen = greenData16[i];
			tempBlue = blueData16[i];

			// add the 16 bit value to the pixels
			tempRed += (red16 + all16);
			tempGreen += (green16 + all16);
			tempBlue += (blue16 + all16);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;

		}
	}
}

void Processor::AdjustHSL(double hShift, double sScale, double lScale, double rScale, double gScale, double bScale, int dataStart, int dataEnd) {

	if(hShift > 360.0) { hShift = 360.0; }
	if(hShift < 0.0) { hShift = 0.0; }
	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}
	
	// Create a temporary set of pixels that will allow handling of overflow
	int tempRed;
	int tempGreen;
	int tempBlue;

	double tempRedF;
	double tempGreenF;
	double tempBlueF;
	double min = 1.0;
	double max = 0.0;
	int maxColor = 0;

	double hue = 0.0;
	double saturation = 0.0;
	double luminace = 0.0;

	double tempRedHSL = 0.0;
	double tempGreenHSL = 0.0;
	double tempBlueHSL = 0.0;

	double temp1 = 0.0;
	double temp2 = 0.0;
	double tempR = 0.0;
	double tempG = 0.0;
	double tempB = 0.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get red, green and blue temporary pixels
			tempRed = redData8[i];
			tempGreen = greenData8[i];
			tempBlue = blueData8[i];

			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/255.0;
			tempGreenF = (double)tempGreen/255.0;
			tempBlueF = (double)tempBlue/255.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift and scale values
			hue += hShift;
			saturation *= sScale;
			luminace *= lScale;

			// Verify Hue is between 0 and 360
			if(hue > 360.0) { hue -= 360.0; }
			if(hue < 0.0){ hue += 360.0;}

			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG);}
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0); }
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 255
			tempRed = wxRound(((tempRedHSL * rScale) + (tempRedF * (1.0 - rScale))) * 255.0);
			tempGreen = wxRound(((tempGreenHSL * gScale) + (tempGreenF * (1.0 - gScale))) * 255.0);
			tempBlue = wxRound(((tempBlueHSL * bScale) + (tempBlueF * (1.0 - bScale))) * 255.0);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get red, green and blue temporary pixels
			tempRed = redData16[i];
			tempGreen = greenData16[i];
			tempBlue = blueData16[i];

			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/65535.0;
			tempGreenF = (double)tempGreen/65535.0;
			tempBlueF = (double)tempBlue/65535.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift and scale values
			hue += hShift;
			saturation *= sScale;
			luminace *= lScale;

			// Verify Hue is between 0 and 360
			if(hue > 360.0) { hue -= 360.0; }
			if(hue < 0.0){ hue += 360.0;}

			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG);}
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0); }
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 65535
			tempRed = wxRound(((tempRedHSL * rScale) + (tempRedF * (1.0 - rScale))) * 65535.0);
			tempGreen = wxRound(((tempGreenHSL * gScale) + (tempGreenF * (1.0 - gScale))) * 65535.0);
			tempBlue = wxRound(((tempBlueHSL * bScale) + (tempBlueF * (1.0 - bScale))) * 65535.0);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::AdjustLAB(double lScale, double aShift, double bShift, double rScale, double gScale, double bScale, int dataStart, int dataEnd) {
	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	RGB rgb;
	XYZ xyz;
	LAB lab;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	double tempRedOrig;
	double tempGreenOrig;
	double tempBlueOrig;

	// Process 8 bit data
	if (img->GetColorDepth() == 8) {

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) { return; }

			// Convert RGB to LAB color space
			rgb.R = (float)redData8[i] / 255.0;
			rgb.G = (float)greenData8[i] / 255.0;
			rgb.B = (float)blueData8[i] / 255.0;
			tempRedOrig = rgb.R;
			tempGreenOrig = rgb.G;
			tempBlueOrig = rgb.B;

			this->RGBtoXYZ(&rgb, &xyz, colorSpace);
			this->XYZtoLAB(&xyz, &lab);
			
			// Apply LAB adjustment
			lab.L = lab.L * lScale;
			lab.A = lab.A + aShift;
			lab.B = lab.B + bShift;

			// Convert LAB back to RGB color space
			this->LABtoXYZ(&lab, &xyz);
			this->XYZtoRGB(&xyz, &rgb, colorSpace);

			tempRed = wxRound(((rgb.R * rScale) + (tempRedOrig * (1.0 - rScale))) * 255.0);
			tempGreen = wxRound(((rgb.G * gScale) + (tempGreenOrig * (1.0 - gScale))) * 255.0);
			tempBlue = wxRound(((rgb.B * bScale) + (tempBlueOrig * (1.0 - bScale))) * 255.0);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) { return; }

			// Convert RGB to LAB color space
			rgb.R = (float)redData16[i] / 65535.0;
			rgb.G = (float)greenData16[i] / 65535.0;
			rgb.B = (float)blueData16[i] / 65535.0;
			tempRedOrig = rgb.R;
			tempGreenOrig = rgb.G;
			tempBlueOrig = rgb.B;

			this->RGBtoXYZ(&rgb, &xyz, colorSpace);
			this->XYZtoLAB(&xyz, &lab);

			// Apply LAB adjustment
			lab.L = lab.L * lScale;
			lab.A = lab.A + aShift;
			lab.B = lab.B + bShift;

			// Convert LAB back to RGB color space
			this->LABtoXYZ(&lab, &xyz);
			this->XYZtoRGB(&xyz, &rgb, colorSpace);

			tempRed = wxRound(((rgb.R * rScale) + (tempRedOrig * (1.0 - rScale))) * 65535.0);
			tempGreen = wxRound(((rgb.G * gScale) + (tempGreenOrig * (1.0 - gScale))) * 65535.0);
			tempBlue = wxRound(((rgb.B * bScale) + (tempBlueOrig * (1.0 - bScale))) * 65535.0);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::AdjustBrightness(double brightness, double detailsPreseve, double toneSetting, int tonePreservation, int dataStart, int dataEnd) {

	if(detailsPreseve < 0.0){ detailsPreseve = 0.0; }
	if(detailsPreseve > 1.0){ detailsPreseve = 1.0; }

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	// Create a temporary set of pixels that will allow handling of overflow
	int tempRed;
	int tempGreen;
	int tempBlue;

	double tempRedF;
	double tempGreenF;
	double tempBlueF;
	double min = 1.0;
	double max = 0.0;
	int maxColor = 0;

	double hue = 0.0;
	double saturation = 0.0;
	double luminace = 0.0;

	double tempRedHSL = 0.0;
	double tempGreenHSL = 0.0;
	double tempBlueHSL = 0.0;

	double temp1 = 0.0;
	double temp2 = 0.0;
	double tempR = 0.0;
	double tempG = 0.0;
	double tempB = 0.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get red, green and blue temporary pixels
			tempRed = redData8[i];
			tempGreen = greenData8[i];
			tempBlue = blueData8[i];

			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/255.0;
			tempGreenF = (double)tempGreen/255.0;
			tempBlueF = (double)tempBlue/255.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift values

			// Function to preserve highlights
			// plot 1.0 - tanh(pi * x) from x = 0 to x = 1
			if(tonePreservation == Processor::BrightnessPreservation::HIGHLIGHTS){
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve*(brightness*(1.0 - FastTanH(pi * luminace * (toneSetting * 3.0)))));
			}

			// Function to preserve Shadows (mirror of highlights function)
			// tanh(pi * x) from x = 0 to x = 1
			else if(tonePreservation == Processor::BrightnessPreservation::SHADOWS){
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve*(brightness*(FastTanH(pi * luminace * (toneSetting * 3.0)))));
			}
			else{
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve * brightness * 0.25 *(pow(4.0, (toneSetting*40.0)) * pow(luminace, ((toneSetting*40.0) - 1.0)) * pow((1.0 - luminace), ((toneSetting*40.0) - 1.0))));
			}
			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG); }
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0);	}
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 255
			tempRed = wxRound(tempRedHSL * 255.0);
			tempGreen =  wxRound(tempGreenHSL * 255.0);
			tempBlue =  wxRound(tempBlueHSL * 255.0);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }if(forceStop) { return; }

			// Get red, green and blue temporary pixels
			tempRed = redData16[i];
			tempGreen = greenData16[i];
			tempBlue = blueData16[i];

			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/65535.0;
			tempGreenF = (double)tempGreen/65535.0;
			tempBlueF = (double)tempBlue/65535.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift values

			// Function to preserve highlights
			// plot 1.0 - tanh(pi * x) from x = 0 to x = 1
			if(tonePreservation == Processor::BrightnessPreservation::HIGHLIGHTS){
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve*(brightness*(1.0 - FastTanH(pi * luminace * (toneSetting * 3.0)))));
			}

			// Function to preserve Shadows (mirror of highlights function)
			// tanh(pi * x) from x = 0 to x = 1
			else if(tonePreservation == Processor::BrightnessPreservation::SHADOWS){
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve*(brightness*(FastTanH(pi * luminace * (toneSetting * 3.0)))));
			}
			else{
				luminace += (brightness * (1.0 - detailsPreseve)) + (detailsPreseve * brightness * 0.25 *(pow(4.0, (toneSetting*40.0)) * pow(luminace, ((toneSetting*40.0) - 1.0)) * pow((1.0 - luminace), ((toneSetting*40.0) - 1.0))));
			}
			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG); }
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0);	}
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 65535
			tempRed = wxRound(tempRedHSL * 65535.0);
			tempGreen =  wxRound(tempGreenHSL * 65535.0);
			tempBlue =  wxRound(tempBlueHSL * 65535.0);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::AdjustContrast(double allContrast, double redContrast, double greenContrast, double blueContrast, double allCenter, double redCenter, double greenCenter, double blueCenter, int dataStart, int dataEnd) {

	if(allCenter < 0.0) {allCenter = 0.0;}
	if(allCenter > 1.0) {allCenter = 1.0;}
	if(redCenter < 0.0) {redCenter = 0.0;}
	if(redCenter > 1.0) {redCenter = 1.0;}
	if(greenCenter < 0.0) {greenCenter = 0.0;}
	if(greenCenter > 1.0) {greenCenter = 1.0;}
	if(blueCenter < 0.0) {blueCenter = 0.0;}
	if(blueCenter > 1.0) {blueCenter = 1.0;}

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	// Create a temporary set of pixels that will allow handling of overflow
	int tempRed;
	int tempGreen;
	int tempBlue;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform whole contrast on red, green and blue
			tempRed = (allContrast * (redData8[i] - (255.0 * allCenter))) + (255.0 * allCenter);
			tempGreen = (allContrast * (greenData8[i] - (255.0 * allCenter))) + (255.0 * allCenter);
			tempBlue = (allContrast * (blueData8[i] - (255.0 * allCenter))) + (255.0 * allCenter);

			// Perform individual contrast on red, green and blue
			tempRed = (redContrast * (tempRed - (255.0 * redCenter))) + (255.0 * redCenter);
			tempGreen = (greenContrast * (tempGreen - (255.0 * greenCenter))) + (255.0 * greenCenter);
			tempBlue = (blueContrast * (tempBlue - (255.0 * blueCenter))) + (255.0 * blueCenter);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}

	// Process 16 bit data
	else{

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform whole contrast on red, green and blue
			tempRed = (allContrast * (redData16[i] - (65535.0 * allCenter))) + (65535.0 * allCenter);
			tempGreen = (allContrast * (greenData16[i] - (65535.0 * allCenter))) + (65535.0 * allCenter);
			tempBlue = (allContrast * (blueData16[i] - (65535.0 * allCenter))) + (65535.0 * allCenter);

			// Perform individual contrast on red, green and blue
			tempRed = (redContrast * (tempRed - (65535.0 * redCenter))) + (65535.0 * redCenter);
			tempGreen = (greenContrast * (tempGreen - (65535.0 * greenCenter))) + (65535.0 * greenCenter);
			tempBlue = (blueContrast * (tempBlue - (65535.0 * blueCenter))) + (65535.0 * blueCenter);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::AdjustContrastCurve(double allContrast, double redContrast, double greenContrast, double blueContrast, double allCenter, double redCenter, double greenCenter, double blueCenter, int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	// Create a temporary set of pixels that will allow handling of overflow
	int tempRed;
	int tempGreen;
	int tempBlue;

	// Process 8 bit data
	if (img->GetColorDepth() == 8) {

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) { return; }

			tempRed = redData8[i];
			tempGreen = greenData8[i];
			tempBlue = blueData8[i];

			// Perform whole contrast on red, green and blue
			tempRed = (127.5*((FastTanH(allContrast * pi * ((tempRed/255.0)-allCenter))) + 1.0));
			tempGreen = (127.5*((FastTanH(allContrast * pi * ((tempGreen/255.0)-allCenter))) + 1.0));
			tempBlue = (127.5*((FastTanH(allContrast * pi * ((tempBlue/255.0)-allCenter))) + 1.0));

			// Perform individual contrast on red, green and blue
			tempRed = (127.5*((FastTanH(redContrast * pi * ((tempRed/255.0)-redCenter))) + 1.0));
			tempGreen = (127.5*((FastTanH(greenContrast * pi * ((tempGreen/255.0)-greenCenter))) + 1.0));
			tempBlue = (127.5*((FastTanH(blueContrast * pi * ((tempBlue/255.0)-blueCenter))) + 1.0));
			
			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) { return; }

			tempRed = redData16[i];
			tempGreen = greenData16[i];
			tempBlue = blueData16[i];

			// Perform whole contrast on red, green and blue
			tempRed = (32767.5*((FastTanH(allContrast * pi * ((tempRed/65535.0)-allCenter))) + 1.0));
			tempGreen = (32767.5*((FastTanH(allContrast * pi * ((tempGreen/65535.0)-allCenter))) + 1.0));
			tempBlue = (32767.5*((FastTanH(allContrast * pi * ((tempBlue/65535.0)-allCenter))) + 1.0));

			// Perform individual contrast on red, green and blue
			tempRed = (32767.5*((FastTanH(redContrast * pi * ((tempRed/65535.0)-redCenter))) + 1.0));
			tempGreen = (32767.5*((FastTanH(greenContrast * pi * ((tempGreen/65535.0)-greenCenter))) + 1.0));
			tempBlue = (32767.5*((FastTanH(blueContrast * pi * ((tempBlue/65535.0)-blueCenter))) + 1.0));
			
			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::ConvertGreyscale(double redScale, double greenScale, double blueScale, int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	// Create a temporary set of pixels that will allow handling of overflow
	int tempGrey;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform greyscale conversion
			tempGrey = (redScale * (redData8[i])) + (greenScale * (greenData8[i])) + (blueScale * (blueData8[i]));

			// handle overflow or underflow
			tempGrey = (tempGrey > 255) ? 255 : tempGrey;
			tempGrey = (tempGrey < 0) ? 0 : tempGrey;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempGrey;
			greenData8[i] = (uint8_t)tempGrey;
			blueData8[i] = (uint8_t)tempGrey;
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform greyscale conversion
			tempGrey = (redScale * (redData16[i])) + (greenScale * (greenData16[i])) + (blueScale * (blueData16[i]));

			// handle overflow or underflow
			tempGrey = (tempGrey > 65535) ? 65535 : tempGrey;
			tempGrey = (tempGrey < 0) ? 0 : tempGrey;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempGrey;
			greenData16[i] = (uint16_t)tempGrey;
			blueData16[i] = (uint16_t)tempGrey;
		}
	}
}

void Processor::ChannelScale(double redRedScale, double redGreenScale, double redBlueScale,
	double greenRedScale, double greenGreenScale, double greenBlueScale,
	double blueRedScale, double blueGreenScale, double blueBlueScale,
	int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform transform
			tempRed = (redRedScale * (redData8[i])) + (redGreenScale * (greenData8[i])) + (redBlueScale * (blueData8[i]));
			tempGreen = (greenRedScale * (redData8[i])) + (greenGreenScale * (greenData8[i])) + (greenBlueScale * (blueData8[i]));
			tempBlue = (blueRedScale * (redData8[i])) + (blueGreenScale * (greenData8[i])) + (blueBlueScale * (blueData8[i]));

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Perform transform
			tempRed = (redRedScale * (redData16[i])) + (redGreenScale * (greenData16[i])) + (redBlueScale * (blueData16[i]));
			tempGreen = (greenRedScale * (redData16[i])) + (greenGreenScale * (greenData16[i])) + (greenBlueScale * (blueData16[i]));
			tempBlue = (blueRedScale * (redData16[i])) + (blueGreenScale * (greenData16[i])) + (blueBlueScale * (blueData16[i]));

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
}

void Processor::RGBCurves(int * brightCurve8, int * redCurve8, int * greenCurve8, int * blueCurve8,
	int * brightCurve16, int * redCurve16, int * greenCurve16, int * blueCurve16,  int dataStart, int dataEnd) {

	// Need to copy curves incase thread ends and destroys original curve table while we are still in it
	// (but exiting soon)
	int numSteps8 = 256;
	int * brightCurve8Copy = new int[numSteps8];
	int * redCurve8Copy = new int[numSteps8];
	int * greenCurve8Copy = new int[numSteps8];
	int * blueCurve8Copy = new int[numSteps8];
	memcpy(brightCurve8Copy, brightCurve8, numSteps8 * sizeof(int));
	memcpy(redCurve8Copy, redCurve8, numSteps8 * sizeof(int));
	memcpy(greenCurve8Copy, greenCurve8, numSteps8 * sizeof(int));
	memcpy(blueCurve8Copy, blueCurve8, numSteps8 * sizeof(int));

	int numSteps16 = 65536;
	int * brightCurve16Copy = new int[numSteps16];
	int * redCurve16Copy = new int[numSteps16];
	int * greenCurve16Copy = new int[numSteps16];
	int * blueCurve16Copy = new int[numSteps16];
	memcpy(brightCurve16Copy, brightCurve16, numSteps16 * sizeof(int));
	memcpy(redCurve16Copy, redCurve16, numSteps16 * sizeof(int));
	memcpy(greenCurve16Copy, greenCurve16, numSteps16 * sizeof(int));
	memcpy(blueCurve16Copy, blueCurve16, numSteps16 * sizeof(int));

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) {
				delete[] redCurve8Copy;
				delete[] greenCurve8Copy;
				delete[] blueCurve8Copy;
				delete[] redCurve16Copy;
				delete[] greenCurve16Copy;
				delete[] blueCurve16Copy;
				return;
			}

			// Apply red curve and bright curve to red channel
			tempRed = redCurve8Copy[redData8[i]];
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempRed = brightCurve8Copy[tempRed];

			// Apply green curve and bright curve to green channel
			tempGreen = greenCurve8Copy[greenData8[i]];
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempGreen = brightCurve8Copy[tempGreen];

			// Apply blue curve and bright curve to blue channel
			tempBlue = blueCurve8Copy[blueData8[i]];
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;
			tempBlue = brightCurve8Copy[tempBlue];

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) {
				delete[] redCurve8Copy;
				delete[] greenCurve8Copy;
				delete[] blueCurve8Copy;
				delete[] redCurve16Copy;
				delete[] greenCurve16Copy;
				delete[] blueCurve16Copy;
				return;
			}

			// Apply red curve and bright curve to red channel
			tempRed = redCurve16Copy[redData16[i]];
			tempRed = (tempRed > 65535) ? 65535: tempRed;
			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempRed = brightCurve16Copy[tempRed];

			// Apply green curve and bright curve to green channel
			tempGreen = greenCurve16Copy[greenData16[i]];
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempGreen = brightCurve16Copy[tempGreen];

			// Apply blue curve and bright curve to blue channel
			tempBlue = blueCurve16Copy[blueData16[i]];
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;
			tempBlue = brightCurve16Copy[tempBlue];

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}

	delete[] brightCurve8Copy;
	delete[] redCurve8Copy;
	delete[] greenCurve8Copy;
	delete[] blueCurve8Copy;

	delete[] brightCurve16Copy;
	delete[] redCurve16Copy;
	delete[] greenCurve16Copy;
	delete[] blueCurve16Copy;

}

void Processor::LABCurves(int * lCurve16, int * aCurve16, int * bCurve16, double redScale, double greenScale, double blueScale, int dataStart, int dataEnd){

	// Need to copy curves incase thread ends and destroys original curve table while we are still in it
	// (but exiting soon)
	int numSteps = 65535;
	int * lCurve16Copy = new int[numSteps];
	int * aCurve16Copy = new int[numSteps];
	int * bCurve16Copy = new int[numSteps];
	memcpy(lCurve16Copy, lCurve16, numSteps * sizeof(int));
	memcpy(aCurve16Copy, aCurve16, numSteps * sizeof(int));
	memcpy(bCurve16Copy, bCurve16, numSteps * sizeof(int));

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	RGB rgb;
	XYZ xyz;
	LAB lab;

	int lScale;
	int aScale;
	int bScale;

	int newL;
	int newA;
	int newB;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	double tempRedOrig;
	double tempGreenOrig;
	double tempBlueOrig;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) {
				delete[] lCurve16Copy;
				delete[] aCurve16Copy;
				delete[] bCurve16Copy;
				return;
			}

			// Convert RGB to LAB color space
			rgb.R = (float)redData8[i] / 256.0;
			rgb.G = (float)greenData8[i] / 256.0;
			rgb.B = (float)blueData8[i] / 256.0;
			tempRedOrig = rgb.R;
			tempGreenOrig = rgb.G;
			tempBlueOrig = rgb.B;

			this->RGBtoXYZ(&rgb, &xyz, colorSpace);
			this->XYZtoLAB(&xyz, &lab);

			// Scale LAB to ints, 16 bits of precision
			lScale = (lab.L / 100.0f) * numSteps;
			aScale = ((lab.A + 128.0f)/256.0f)* numSteps;
			bScale = ((lab.B + 128.0f)/256.0f) * numSteps;

			// Apply LAB Curve
			//if(lCurve16 == NULL || aCurve16 == NULL || bCurve16 == NULL){ return; }
			newL = lCurve16Copy[lScale];
			newA = aCurve16Copy[aScale];
			newB = bCurve16Copy[bScale];

			// Scale LAB back
			lab.L = (float)(newL / (float)numSteps);
			lab.A = (float)(newA / (float)numSteps);
			lab.B = (float)(newB / (float)numSteps);

			lab.L *= 100.0;
			lab.A *= 256.0;
			lab.B *= 256.0;

			lab.A -= 128.0f;
			lab.B -= 128.0f;

			// Convert LAB back to RGB color space
			this->LABtoXYZ(&lab, &xyz);
			this->XYZtoRGB(&xyz, &rgb, colorSpace);
			
			// Scale to 0 - 255
			tempRed = wxRound(((rgb.R * redScale) + (tempRedOrig * (1.0 - redScale))) * 255.0);
			tempGreen = wxRound(((rgb.G * greenScale) + (tempGreenOrig * (1.0 - greenScale))) * 255.0);
			tempBlue = wxRound(((rgb.B * blueScale) + (tempBlueOrig * (1.0 - blueScale))) * 255.0);

			tempRed = (int32_t)(rgb.R * 256.0);
			tempGreen = (int32_t)(rgb.G * 256.0);
			tempBlue = (int32_t)(rgb.B * 256.0);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) {
				delete[] lCurve16Copy;
				delete[] aCurve16Copy;
				delete[] bCurve16Copy;
				return;
			}

			// Convert RGB to LAB color space
			rgb.R = (float)redData16[i] / 65536.0;
			rgb.G = (float)greenData16[i] / 65536.0;
			rgb.B = (float)blueData16[i] / 65536.0;
			tempRedOrig = rgb.R;
			tempGreenOrig = rgb.G;
			tempBlueOrig = rgb.B;

			this->RGBtoXYZ(&rgb, &xyz, colorSpace);
			this->XYZtoLAB(&xyz, &lab);

			// Scale LAB to ints, 16 bits of precision
			lScale = (lab.L / 100.0f) * numSteps;
			aScale = ((lab.A + 128.0f) / 256.0f)* numSteps;
			bScale = ((lab.B + 128.0f) / 256.0f) * numSteps;

			// Apply LAB Curve
			newL = lCurve16Copy[lScale];
			newA = aCurve16Copy[aScale];
			newB = bCurve16Copy[bScale];

			// Scale LAB back
			lab.L = (float)(newL / (float)numSteps);
			lab.A = (float)(newA / (float)numSteps);
			lab.B = (float)(newB / (float)numSteps);

			lab.L *= 100.0;
			lab.A *= 256.0;
			lab.B *= 256.0;

			lab.A -= 128.0f;
			lab.B -= 128.0f;

			// Convert LAB back to RGB color space
			this->LABtoXYZ(&lab, &xyz);
			this->XYZtoRGB(&xyz, &rgb, colorSpace);
			tempRed = wxRound(((rgb.R * redScale) + (tempRedOrig * (1.0 - redScale))) * 65535.0);
			tempGreen = wxRound(((rgb.G * greenScale) + (tempGreenOrig * (1.0 - greenScale))) * 65535.0);
			tempBlue = wxRound(((rgb.B * blueScale) + (tempBlueOrig * (1.0 - blueScale))) * 65535.0);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}

	delete[] lCurve16Copy;
	delete[] aCurve16Copy;
	delete[] bCurve16Copy;
}

void Processor::HSLCurves(int * hCurve16, int * sCurve16, int * lCurve16, double rScale, double gScale, double bScale, int dataStart, int dataEnd){

	// Need to copy curves incase thread ends and destroys original curve table while we are still in it
	// (but exiting soon)
	int numSteps = 65535;
	int * hCurve16Copy = new int[numSteps + 1];
	int * sCurve16Copy = new int[numSteps + 1];
	int * lCurve16Copy = new int[numSteps + 1];
	memcpy(hCurve16Copy, hCurve16, numSteps * sizeof(int));
	memcpy(sCurve16Copy, sCurve16, numSteps * sizeof(int));
	memcpy(lCurve16Copy, lCurve16, numSteps * sizeof(int));
	hCurve16Copy[numSteps] = hCurve16[numSteps];
	sCurve16Copy[numSteps] = sCurve16[numSteps];
	lCurve16Copy[numSteps] = lCurve16[numSteps];

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}
	
	int hScale;
	int sScale;
	int lScale;

	int newH;
	int newS;
	int newL;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	double tempRedF;
	double tempGreenF;
	double tempBlueF;
	double min = 1.0;
	double max = 0.0;
	int maxColor = 0;

	double hue = 0.0;
	double saturation = 0.0;
	double luminace = 0.0;

	double tempRedHSL = 0.0;
	double tempGreenHSL = 0.0;
	double tempBlueHSL = 0.0;

	double temp1 = 0.0;
	double temp2 = 0.0;
	double tempR = 0.0;
	double tempG = 0.0;
	double tempB = 0.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { 
				delete[] hCurve16Copy;
				delete[] sCurve16Copy;
				delete[] lCurve16Copy;
				return; 
			}

			tempRed = redData8[i];
			tempGreen = greenData8[i];
			tempBlue = blueData8[i];
	
			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/255.0;
			tempGreenF = (double)tempGreen/255.0;
			tempBlueF = (double)tempBlue/255.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift values

			// Scale HSL to ints, 16 bits of precision
			hScale = (hue/360.0) * numSteps;
			sScale = saturation * numSteps;
			lScale = luminace * numSteps;

			// Apply HSL Curve
			newH = hCurve16Copy[hScale];
			newS = sCurve16Copy[sScale];
			newL = lCurve16Copy[lScale];

			// Scale HSL back
			hue = (float)(newH / (float)numSteps) * 360.0f;
			saturation = (float)(newS / (float)numSteps);
			luminace = (float)(newL / (float)numSteps);
				
			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG); }
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0);	}
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 255
			tempRed = wxRound(((tempRedHSL * rScale) + (tempRedF * (1.0 - rScale))) * 255.0);
			tempGreen = wxRound(((tempGreenHSL * gScale) + (tempGreenF * (1.0 - gScale))) * 255.0);
			tempBlue = wxRound(((tempBlueHSL * bScale) + (tempBlueF * (1.0 - bScale))) * 255.0);

			// handle overflow or underflow
			tempRed = (tempRed > 255) ? 255 : tempRed;
			tempGreen = (tempGreen > 255) ? 255 : tempGreen;
			tempBlue = (tempBlue > 255) ? 255 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 8 bit data
			redData8[i] = (uint8_t)tempRed;
			greenData8[i] = (uint8_t)tempGreen;
			blueData8[i] = (uint8_t)tempBlue;
		}
	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if (forceStop) {
				delete[] hCurve16Copy;
				delete[] sCurve16Copy;
				delete[] lCurve16Copy;
				return;
			}

			// Get red, green and blue temporary pixels
			tempRed = redData16[i];
			tempGreen = greenData16[i];
			tempBlue = blueData16[i];

			// Convert to HSL

			// Scale to 0-1
			tempRedF = (double)tempRed/65535.0;
			tempGreenF = (double)tempGreen/65535.0;
			tempBlueF = (double)tempBlue/65535.0;

			min = 1.0;
			max = 0.0;

			// Find min and max
			if(tempRedF < min){ min = tempRedF; }
			if(tempGreenF < min){ min = tempGreenF; }
			if(tempBlueF < min){ min = tempBlueF; }
			if(tempRedF > max){ max = tempRedF; maxColor = 1; }
			if(tempGreenF > max){ max = tempGreenF; maxColor = 2; }
			if(tempBlueF > max){ max = tempBlueF; maxColor = 3;}

			luminace = (min + max) / 2.0;

			// No saturation if all rgb is same
			if(min == max){
				saturation = 0.0;
				hue = 0.0;
			}
			else{
				if(luminace < 0.5){ saturation = (max - min)/(max + min); }
				else{ saturation = (max - min)/(2.0 - max - min); }
		
				// Calculate hue
				//Red is maximum
				if(maxColor == 1){ hue = (tempGreenF - tempBlueF)/(max - min); }

				// Green in maximum
				else if(maxColor == 2){ hue = 2.0 + ((tempBlueF - tempRedF)/(max - min)); }

				// Blue is maximum
				else{ hue = 4.0 + ((tempRedF - tempGreenF)/(max - min));}

				// Convert hue to degrees
				hue *= 60.0;
				if(hue < 0.0){ hue += 360.0;}
			}

			// Shift values

			// Scale HSL to ints, 16 bits of precision
			hScale = (hue/360.0) * numSteps;
			sScale = saturation * numSteps;
			lScale = luminace * numSteps;

			// Apply HSL Curve
			newH = hCurve16Copy[hScale];
			newS = sCurve16Copy[sScale];
			newL = lCurve16Copy[lScale];

			// Scale HSL back
			hue = (float)(newH / (float)numSteps) * 360.0f;
			saturation = (float)(newS / (float)numSteps);
			luminace = (float)(newL / (float)numSteps);

			// Convert hsl back to rgb
			// Saturation is 0, this is greyscale
			if(saturation == 0.0){
				tempRedHSL =  luminace;
				tempGreenHSL = luminace;
				tempBlueHSL = luminace;
			}
			else{
				if(luminace < 0.5){ temp1 = luminace * (1.0 + saturation); }
				else{ temp1 = (luminace + saturation) - (luminace * saturation); }
		
				temp2 = (2 * luminace) - temp1;

				// Convert 0-360 to 0-1
				hue /=360.0;

				// Create temporary RGB from hue
				tempR = hue + (1.0/3.0);
				tempG = hue;
				tempB = hue - (1.0/3.0);

				// Adjust all temp RGB values to be between 0 and 1
				if(tempR < 0.0) {tempR += 1.0; }
				if(tempR > 1.0) {tempR -= 1.0; }
				if(tempG < 0.0) {tempG += 1.0; }
				if(tempG > 1.0) {tempG -= 1.0; }
				if(tempB < 0.0) {tempB += 1.0; }
				if(tempB > 1.0) {tempB -= 1.0; }

				// Calculate RGB Red from HSL
				if((6.0 * tempR) < 1.0){ tempRedHSL = temp2 + ((temp1 - temp2) * 6.0 * tempR); }
				else if((2.0 * tempR) < 1.0){ tempRedHSL = temp1; }
				else if((3.0 * tempR) < 2.0){ tempRedHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempR)*6.0); }
				else{ tempRedHSL = temp2; }

				// Calculate RGB Green from HSL
				if((6.0 * tempG) < 1.0){ tempGreenHSL = temp2 + ((temp1 - temp2) * 6.0 * tempG); }
				else if((2.0 * tempG) < 1.0){ tempGreenHSL = temp1; }
				else if((3.0 * tempG) < 2.0){ tempGreenHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempG)*6.0);	}
				else{ tempGreenHSL = temp2; }

				// Calculate RGB Blue from HSL
				if((6.0 * tempB) < 1.0){ tempBlueHSL = temp2 + ((temp1 - temp2) * 6.0 * tempB); }
				else if((2.0 * tempB) < 1.0){ tempBlueHSL = temp1; }
				else if((3.0 * tempB) < 2.0){ tempBlueHSL = temp2 + ((temp1 - temp2)*((2.0/3.0) - tempB)*6.0); }
				else{ tempBlueHSL = temp2; }
			}

			// Scale to 0 - 65535
			tempRed = wxRound(((tempRedHSL * rScale) + (tempRedF * (1.0 - rScale))) * 65535.0);
			tempGreen = wxRound(((tempGreenHSL * gScale) + (tempGreenF * (1.0 - gScale))) * 65535.0);
			tempBlue = wxRound(((tempBlueHSL * bScale) + (tempBlueF * (1.0 - bScale))) * 65535.0);

			// handle overflow or underflow
			tempRed = (tempRed > 65535) ? 65535 : tempRed;
			tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
			tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

			tempRed = (tempRed < 0) ? 0 : tempRed;
			tempGreen = (tempGreen < 0) ? 0 : tempGreen;
			tempBlue = (tempBlue < 0) ? 0 : tempBlue;

			// Set the new pixel to the 16 bit data
			redData16[i] = (uint16_t)tempRed;
			greenData16[i] = (uint16_t)tempGreen;
			blueData16[i] = (uint16_t)tempBlue;
		}
	}
	delete[] hCurve16Copy;
	delete[] sCurve16Copy;
	delete[] lCurve16Copy;
}

bool Processor::SetupRotation(int editID, double angleDegrees, int crop){

	// Copy image data to tmep image
	if(editID == ProcessorEdit::EditType::ROTATE_180){
		tempImage = new Image(*img);
		if (tempImage->GetErrorStr() != "") {
			return false;
		}
	}

	// Copy image data to tmep image.  Width and height will be swapped in cleanup
	else if (editID == ProcessorEdit::EditType::ROTATE_90_CW ||editID == ProcessorEdit::EditType::ROTATE_270_CW){
		tempImage = new Image(*img);
		if (tempImage->GetErrorStr() != "") {
			return false;
		}
	}

	// Create a blank temp image that will be filled in from rotation algorithms
	else if(editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC || 
		editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR ||
		editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST){

		tempImage = new Image();
		if(img->GetColorDepth() == 16){ 
			tempImage->Enable16Bit();
			if (tempImage->GetErrorStr() != "") {
				return false;
			}
		}

		// Set width and height to maximum size needed to fit whole image in rotation (with black surrounding borders)
		if (crop == Processor::RotationCropping::EXPAND) {
			tempImage->SetWidth(this->GetExpandedRotationWidth(angleDegrees, img->GetWidth(), img->GetHeight()));
			tempImage->SetHeight(this->GetExpandedRotationHeight(angleDegrees, img->GetWidth(), img->GetHeight()));
		}

		// Set width and height to minumum size needed to fit image with no border
		else if (crop == Processor::RotationCropping::FIT) {
			tempImage->SetWidth(this->GetFittedRotationWidth(angleDegrees, img->GetWidth(), img->GetHeight()));
			tempImage->SetHeight(this->GetFittedRotationHeight(angleDegrees, img->GetWidth(), img->GetHeight()));
		}

		// Rotated image is same size as current image
		else {
			tempImage->SetWidth(img->GetWidth());
			tempImage->SetHeight(img->GetHeight());
		}

		// Create blank image data
		tempImage->InitImage();
		if (tempImage->GetErrorStr() != "") {
			return false;
		}
	}
	return true;
}

void Processor::CleanupRotation(int editID){

	// Copy temp image to image if rotate custom was used
	if(editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC || 
		editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR ||
		editID ==ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST){
		delete img;
		img = new Image(*tempImage);
	}

	// Swap width and height for 90 or 270 rotation, 180 rotation is same width and height
	else if(editID == ProcessorEdit::EditType::ROTATE_270_CW || editID == ProcessorEdit::EditType::ROTATE_90_CW){
		img->SetWidth(tempImage->GetHeight());
		img->SetHeight(tempImage->GetWidth());
	}

	// Cleanup temp image
	if(tempImage != NULL){
		delete tempImage;
		tempImage = NULL;
	}
}


void Processor::Rotate90CW(int dataStart, int dataEnd) {

	int width = img->GetWidth();
	int height = img->GetHeight();

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int x = 0;
	int y = 0;
	int offset = 0;
	int newOffset = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;
			offset = height * x + y;
			newOffset = width * (height - 1 - y) + x;

			redData8[offset] = redData8Dup[newOffset];
			greenData8[offset] = greenData8Dup[newOffset];
			blueData8[offset] = blueData8Dup[newOffset];
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;
			offset = height * x + y;
			newOffset = width * (height - 1 - y) + x;

			redData16[offset] = redData16Dup[newOffset];
			greenData16[offset] = greenData16Dup[newOffset];
			blueData16[offset] = blueData16Dup[newOffset];
		}
	}
}

void Processor::Rotate180(int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int newOffset = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			newOffset = dataSize - i - 1;
			redData8[i] = redData8Dup[newOffset];
			greenData8[i] = greenData8Dup[newOffset];
			blueData8[i] = blueData8Dup[newOffset];
		}
	}
	
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			newOffset = dataSize - i - 1;
			redData16[i] = redData16Dup[newOffset];
			greenData16[i] = greenData16Dup[newOffset];
			blueData16[i] = blueData16Dup[newOffset];
		}
	}
}
void Processor::Rotate270CW(int dataStart, int dataEnd) {

	int width = img->GetWidth();
	int height = img->GetHeight();

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int x = 0;
	int y = 0;
	int newOffset = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;

			newOffset = dataSize - i - 1;

			x = newOffset % height;
			y = newOffset / height;
			newOffset = width * (height - 1 - x) + y;

			redData8[i] = redData8Dup[newOffset];
			greenData8[i] = greenData8Dup[newOffset];
			blueData8[i] = blueData8Dup[newOffset];
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;

			newOffset = dataSize - i - 1;

			x = newOffset % height;
			y = newOffset / height;
			newOffset = width * (height - 1 - x) + y;
		
			redData16[i] = redData16Dup[newOffset];
			greenData16[i] = greenData16Dup[newOffset];
			blueData16[i] = blueData16Dup[newOffset];
		}
	}
}

void Processor::RotateCustom(double angleDegrees, int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	angleDegrees *= -1.0;
	double angleSin = sin(angleDegrees * pi / 180.0);
	double angleCos = cos(angleDegrees * pi / 180.0);
	double pivotX = (double)img->GetWidth() / 2.0;
	double pivotY = (double)img->GetHeight() / 2.0;

	int width = img->GetWidth();
	int height = img->GetHeight();
	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int newWidth = 0;
	int newHeight = 0;
	int dWidth = 0;
	int dHeight = 0;

	newWidth = tempImage->GetWidth();
	newHeight = tempImage->GetHeight();
	dWidth = newWidth - width;
	dHeight = newHeight - height;

	int newI = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// Rotate point
			newX = (x * angleCos) - (y * angleSin);
			newY = (x * angleSin) + (y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Round double to int
			newX = wxRound(newX);
			newY = wxRound(newY);

			// Veirfy pixel location is in bounds of original image size
			if (newX > 0 && newX < width && newY > 0 && newY < height) {

				// Get new single dimmension array index from new x and y location
				newI = newY * width + newX;

				// Copy pixel from old location to new location
				redData8Dup[i] = redData8[newI];
				greenData8Dup[i] = greenData8[newI];
				blueData8Dup[i] = blueData8[newI];
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// rotate point
			newX = (x * angleCos) - (y * angleSin);
			newY = (x * angleSin) + (y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Round double to int
			newX = wxRound(newX);
			newY = wxRound(newY);

			// Veirfy pixel location is in bounds of original image size
			if (newX > 0 && newX < width && newY > 0 && newY < height) {

				// Get new single dimmension array index from new x and y location
				newI = newY * width + newX;

				// Copy pixel from old location to new location
				redData16Dup[i] = redData16[newI];
				greenData16Dup[i] = greenData16[newI];
				blueData16Dup[i] = blueData16[newI];
			}
		}
	}
}

void Processor::RotateCustomBilinear(double angleDegrees, int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	if(angleDegrees == 0.0){
		return;
	}

	double useNearestNeighborDistance = 0.0001;

	angleDegrees *= -1.0;
	double angleSin = sin(angleDegrees * pi / 180.0);
	double angleCos = cos(angleDegrees * pi / 180.0);
	double pivotX = (double)img->GetWidth() / 2.0;
	double pivotY = (double)img->GetHeight() / 2.0;

	int width = img->GetWidth();
	int height = img->GetHeight();
	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;
	
	int newWidth = 0;
	int newHeight = 0;
	int dWidth = 0;
	int dHeight = 0;
	
	newWidth = tempImage->GetWidth();
	newHeight = tempImage->GetHeight();
	newDataSize = newWidth * newHeight;
	dWidth = newWidth - width;
	dHeight = newHeight - height;

	// Neighbors of exact rotation location
	int point1 = 0;
	int point2 = 0;
	int point3 = 0;
	int point4 = 0;

	// Distances from exact point to nearest points to interpolate
	double distance1 = 0.0;
	double distance2 = 0.0;
	double distance3 = 0.0;
	double distance4 = 0.0;

	// Actual index of data of neighbors to interpoalte
	int newI1 = 0;
	int newI2 = 0;
	int newI3 = 0;
	int newI4 = 0;

	// Weights each neighbor will recieve based on distance
	double weight1 = 0.0;
	double weight2 = 0.0;
	double weight3 = 0.0;
	double weight4 = 0.0;
	double weightSum = 1.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// Rotate point
			newX = ((double)x * angleCos) - ((double)y * angleSin);
			newY = ((double)x * angleSin) + ((double)y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Veirfy pixel location is in bounds of original image size
			if (newX > 1 && newX < width - 1 && newY > 1 && newY < height - 1) {

				// new X is not on border of image
				if (newX > 0 && newX < width - 1) {
					point1 = wxRound(floor(newX));
					point2 = wxRound(ceil(newX));
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 0 && newY < height - 1) {
					point3 = wxRound(floor(newY));
					point4 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point3 = wxRound(newY);
					point4 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point3) * (newY - point3));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point3) * (newY - point3));
				distance3 = ((newX - point2) * (newX - point2)) + ((newY - point4) * (newY - point4));
				distance4 = ((newX - point1) * (newX - point1)) + ((newY - point4) * (newY - point4));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point3 * width + point1;
					redData8Dup[i] = redData8[newI1];
					greenData8Dup[i] = greenData8[newI1];
					blueData8Dup[i] = blueData8[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				if (distance2 < useNearestNeighborDistance) {

					newI2 = point3 * width + point2;
					redData8Dup[i] = redData8[newI2];
					greenData8Dup[i] = greenData8[newI2];
					blueData8Dup[i] = blueData8[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				if (distance3 < useNearestNeighborDistance) {

					newI3 = point4 * width + point1;
					redData8Dup[i] = redData8[newI3];
					greenData8Dup[i] = greenData8[newI3];
					blueData8Dup[i] = blueData8[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				if (distance4 < useNearestNeighborDistance) {

					newI4 = point4 * width + point1;
					redData8Dup[i] = redData8[newI4];
					greenData8Dup[i] = greenData8[newI4];
					blueData8Dup[i] = blueData8[newI4];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;
				weight2 = 1.0 / distance2;
				weight3 = 1.0 / distance3;
				weight4 = 1.0 / distance4;
				weightSum = weight1 + weight2 + weight3 + weight4;

				// Get new single dimmension array index from new x and y location
				newI1 = point3 * width + point1;
				newI2 = point3 * width + point2;
				newI3 = point4 * width + point1;
				newI4 = point4 * width + point2;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData8[newI1]) + (weight2 * redData8[newI2]) + (weight3 * redData8[newI3]) + (weight4 * redData8[newI4])) / weightSum;
				tempGreen = ((weight1 * greenData8[newI1]) + (weight2 * greenData8[newI2]) + (weight3 * greenData8[newI3]) + (weight4 * greenData8[newI4])) / weightSum;
				tempBlue = ((weight1 * blueData8[newI1]) + (weight2 * blueData8[newI2]) + (weight3 * blueData8[newI3]) + (weight4 * blueData8[newI4])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 255) ? 255 : tempRed;
				tempGreen = (tempGreen > 255) ? 255 : tempGreen;
				tempBlue = (tempBlue > 255) ? 255 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData8Dup[i] = (uint8_t)tempRed;
				greenData8Dup[i] = (uint8_t)tempGreen;
				blueData8Dup[i] = (uint8_t)tempBlue;
			}
		}

	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// Rotate point
			newX = ((double)x * angleCos) - ((double)y * angleSin);
			newY = ((double)x * angleSin) + ((double)y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Veirfy pixel location is in bounds of original image size
			if (newX > 1 && newX < width - 1 && newY > 1 && newY < height - 1) {

				// new X is not on border of image
				if (newX > 0 && newX < width - 1) {
					point1 = wxRound(floor(newX));
					point2 = wxRound(ceil(newX));
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 0 && newY < height - 1) {
					point3 = wxRound(floor(newY));
					point4 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point3 = wxRound(newY);
					point4 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point3) * (newY - point3));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point3) * (newY - point3));
				distance3 = ((newX - point2) * (newX - point2)) + ((newY - point4) * (newY - point4));
				distance4 = ((newX - point1) * (newX - point1)) + ((newY - point4) * (newY - point4));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point3 * width + point1;
					redData16Dup[i] = redData16[newI1];
					greenData16Dup[i] = greenData16[newI1];
					blueData16Dup[i] = blueData16[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				if (distance2 < useNearestNeighborDistance) {

					newI2 = point3 * width + point2;
					redData16Dup[i] = redData16[newI2];
					greenData16Dup[i] = greenData16[newI2];
					blueData16Dup[i] = blueData16[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				if (distance3 < useNearestNeighborDistance) {

					newI3 = point4 * width + point1;
					redData16Dup[i] = redData16[newI3];
					greenData16Dup[i] = greenData16[newI3];
					blueData16Dup[i] = blueData16[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				if (distance4 < useNearestNeighborDistance) {

					newI4 = point4 * width + point1;
					redData16Dup[i] = redData16[newI4];
					greenData16Dup[i] = greenData16[newI4];
					blueData16Dup[i] = blueData16[newI4];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;
				weight2 = 1.0 / distance2;
				weight3 = 1.0 / distance3;
				weight4 = 1.0 / distance4;
				weightSum = weight1 + weight2 + weight3 + weight4;

				// Get new single dimmension array index from new x and y location
				newI1 = point3 * width + point1;
				newI2 = point3 * width + point2;
				newI3 = point4 * width + point1;
				newI4 = point4 * width + point2;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData16[newI1]) + (weight2 * redData16[newI2]) + (weight3 * redData16[newI3]) + (weight4 * redData16[newI4])) / weightSum;
				tempGreen = ((weight1 * greenData16[newI1]) + (weight2 * greenData16[newI2]) + (weight3 * greenData16[newI3]) + (weight4 * greenData16[newI4])) / weightSum;
				tempBlue = ((weight1 * blueData16[newI1]) + (weight2 * blueData16[newI2]) + (weight3 * blueData16[newI3]) + (weight4 * blueData16[newI4])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 65535) ? 65535 : tempRed;
				tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
				tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData16Dup[i] = (uint16_t)tempRed;
				greenData16Dup[i] = (uint16_t)tempGreen;
				blueData16Dup[i] = (uint16_t)tempBlue;
			}
		}
	}
}

void Processor::RotateCustomBicubic(double angleDegrees, int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	if(angleDegrees == 0.0){
		return;
	}

	double useNearestNeighborDistance = 0.0001;

	angleDegrees *= -1.0;
	double angleSin = sin(angleDegrees * pi / 180.0);
	double angleCos = cos(angleDegrees * pi / 180.0);
	double pivotX = (double)img->GetWidth() / 2.0;
	double pivotY = (double)img->GetHeight() / 2.0;

	int width = img->GetWidth();
	int height = img->GetHeight();
	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	int newWidth = 0;
	int newHeight = 0;
	int dWidth = 0;
	int dHeight = 0;

	newWidth = tempImage->GetWidth();
	newHeight = tempImage->GetHeight();
	newDataSize = newWidth * newHeight;
	dWidth = newWidth - width;
	dHeight = newHeight - height;

	int point1 = 0; int point2 = 0; int point3 = 0; int point4 = 0;
	int point5 = 0; int point6 = 0; int point7 = 0; int point8 = 0;

	double distance1 = 0.0; double distance2 = 0.0; double distance3 = 0.0; double distance4 = 0.0;
	double distance5 = 0.0; double distance6 = 0.0; double distance7 = 0.0; double distance8 = 0.0;
	double distance9 = 0.0; double distance10 = 0.0; double distance11 = 0.0; double distance12 = 0.0;
	double distance13 = 0.0; double distance14 = 0.0; double distance15 = 0.0; double distance16 = 0.0;

	int newI1 = 0; int newI2 = 0; int newI3 = 0; int newI4 = 0;
	int newI5 = 0; int newI6 = 0; int newI7 = 0; int newI8 = 0;
	int newI9 = 0; int newI10 = 0; int newI11 = 0; int newI12 = 0;
	int newI13 = 0; int newI14 = 0; int newI15 = 0; int newI16 = 0;

	double weight1 = 0.0; double weight2 = 0.0; double weight3 = 0.0; double weight4 = 0.0;
	double weight5 = 0.0; double weight6 = 0.0; double weight7 = 0.0; double weight8 = 0.0;
	double weight9 = 0.0; double weight10 = 0.0; double weight11 = 0.0; double weight12 = 0.0;
	double weight13 = 0.0; double weight14 = 0.0; double weight15 = 0.0; double weight16 = 0.0;

	double weightSum = 1.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// Rotate point
			newX = ((double)x * angleCos) - ((double)y * angleSin);
			newY = ((double)x * angleSin) + ((double)y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Veirfy pixel location is in bounds of original image size
			if (newX > 2 && newX < width - 2 && newY > 2 && newY < height - 2) {

				// new X is not on border of image
				if (newX > 1 && newX < width - 2) {
					point1 = wxRound(floor(newX)) - 1;
					point2 = wxRound(floor(newX));
					point3 = wxRound(ceil(newX));
					point4 = wxRound(ceil(newX)) + 1;
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
					point3 = wxRound(newX);
					point4 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 1 && newY < height - 2) {
					point5 = wxRound(floor(newY)) - 1;
					point6 = wxRound(floor(newY));
					point7 = wxRound(ceil(newY));
					point8 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point5 = wxRound(newY);
					point6 = wxRound(newY);
					point7 = wxRound(newY);
					point8 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point5) * (newY - point5));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point5) * (newY - point5));
				distance3 = ((newX - point3) * (newX - point3)) + ((newY - point5) * (newY - point5));
				distance4 = ((newX - point4) * (newX - point4)) + ((newY - point5) * (newY - point5));

				distance5 = ((newX - point1) * (newX - point1)) + ((newY - point6) * (newY - point6));
				distance6 = ((newX - point2) * (newX - point2)) + ((newY - point6) * (newY - point6));
				distance7 = ((newX - point3) * (newX - point3)) + ((newY - point6) * (newY - point6));
				distance8 = ((newX - point4) * (newX - point4)) + ((newY - point6) * (newY - point6));

				distance9 = ((newX - point1) * (newX - point1)) + ((newY - point7) * (newY - point7));
				distance10 = ((newX - point2) * (newX - point2)) + ((newY - point7) * (newY - point7));
				distance11 = ((newX - point3) * (newX - point3)) + ((newY - point7) * (newY - point7));
				distance12 = ((newX - point4) * (newX - point4)) + ((newY - point7) * (newY - point7));

				distance13 = ((newX - point1) * (newX - point1)) + ((newY - point8) * (newY - point8));
				distance14 = ((newX - point2) * (newX - point2)) + ((newY - point8) * (newY - point8));
				distance15 = ((newX - point3) * (newX - point3)) + ((newY - point8) * (newY - point8));
				distance16 = ((newX - point4) * (newX - point4)) + ((newY - point8) * (newY - point8));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point5 * width + point1;
					redData8Dup[i] = redData8[newI1];
					greenData8Dup[i] = greenData8[newI1];
					blueData8Dup[i] = blueData8[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				else if (distance2 < useNearestNeighborDistance) {

					newI2 = point5 * width + point2;
					redData8Dup[i] = redData8[newI2];
					greenData8Dup[i] = greenData8[newI2];
					blueData8Dup[i] = blueData8[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				else if (distance3 < useNearestNeighborDistance) {

					newI3 = point5 * width + point3;
					redData8Dup[i] = redData8[newI3];
					greenData8Dup[i] = greenData8[newI3];
					blueData8Dup[i] = blueData8[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				else if (distance4 < useNearestNeighborDistance) {

					newI4 = point5 * width + point4;
					redData8Dup[i] = redData8[newI4];
					greenData8Dup[i] = greenData8[newI4];
					blueData8Dup[i] = blueData8[newI4];
					continue;
				}

				// If distance 5 is close enough to an actual point, use that closest point
				else if (distance5 < useNearestNeighborDistance) {

					newI5 = point6 * width + point1;
					redData8Dup[i] = redData8[newI5];
					greenData8Dup[i] = greenData8[newI5];
					blueData8Dup[i] = blueData8[newI5];
					continue;
				}

				// If distance 6 is close enough to an actual point, use that closest point
				else if (distance6 < useNearestNeighborDistance) {

					newI6 = point6 * width + point2;
					redData8Dup[i] = redData8[newI6];
					greenData8Dup[i] = greenData8[newI6];
					blueData8Dup[i] = blueData8[newI6];
					continue;
				}

				// If distance 7 is close enough to an actual point, use that closest point
				else if (distance7 < useNearestNeighborDistance) {

					newI7 = point6 * width + point3;
					redData8Dup[i] = redData8[newI7];
					greenData8Dup[i] = greenData8[newI7];
					blueData8Dup[i] = blueData8[newI7];
					continue;
				}

				// If distance 8 is close enough to an actual point, use that closest point
				else if (distance8 < useNearestNeighborDistance) {

					newI8 = point6 * width + point4;
					redData8Dup[i] = redData8[newI8];
					greenData8Dup[i] = greenData8[newI8];
					blueData8Dup[i] = blueData8[newI8];
					continue;
				}
				// If distance 9 is close enough to an actual point, use that closest point
				else if (distance9 < useNearestNeighborDistance) {

					newI9 = point7 * width + point1;
					redData8Dup[i] = redData8[newI9];
					greenData8Dup[i] = greenData8[newI9];
					blueData8Dup[i] = blueData8[newI9];
					continue;
				}

				// If distance 10 is close enough to an actual point, use that closest point
				else if (distance10 < useNearestNeighborDistance) {

					newI10 = point7 * width + point2;
					redData8Dup[i] = redData8[newI10];
					greenData8Dup[i] = greenData8[newI10];
					blueData8Dup[i] = blueData8[newI10];
					continue;
				}

				// If distance 11 is close enough to an actual point, use that closest point
				else if (distance11 < useNearestNeighborDistance) {

					newI11 = point7 * width + point3;
					redData8Dup[i] = redData8[newI11];
					greenData8Dup[i] = greenData8[newI11];
					blueData8Dup[i] = blueData8[newI11];
					continue;
				}

				// If distance 12 is close enough to an actual point, use that closest point
				else if (distance12 < useNearestNeighborDistance) {

					newI12 = point7 * width + point4;
					redData8Dup[i] = redData8[newI12];
					greenData8Dup[i] = greenData8[newI12];
					blueData8Dup[i] = blueData8[newI12];
					continue;
				}

				// If distance 13 is close enough to an actual point, use that closest point
				else if (distance13 < useNearestNeighborDistance) {

					newI13 = point8 * width + point1;
					redData8Dup[i] = redData8[newI13];
					greenData8Dup[i] = greenData8[newI13];
					blueData8Dup[i] = blueData8[newI13];
					continue;
				}

				// If distance 14 is close enough to an actual point, use that closest point
				else if (distance14 < useNearestNeighborDistance) {

					newI14 = point8 * width + point2;
					redData8Dup[i] = redData8[newI14];
					greenData8Dup[i] = greenData8[newI14];
					blueData8Dup[i] = blueData8[newI14];
					continue;
				}

				// If distance 15 is close enough to an actual point, use that closest point
				else if (distance15 < useNearestNeighborDistance) {

					newI15 = point8 * width + point3;
					redData8Dup[i] = redData8[newI15];
					greenData8Dup[i] = greenData8[newI15];
					blueData8Dup[i] = blueData8[newI15];
					continue;
				}

				// If distance 16 is close enough to an actual point, use that closest point
				else if (distance16 < useNearestNeighborDistance) {

					newI16 = point8 * width + point4;
					redData8Dup[i] = redData8[newI16];
					greenData8Dup[i] = greenData8[newI16];
					blueData8Dup[i] = blueData8[newI16];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;  weight2 = 1.0 / distance2;  weight3 = 1.0 / distance3;  weight4 = 1.0 / distance4;
				weight5 = 1.0 / distance5;  weight6 = 1.0 / distance6;  weight7 = 1.0 / distance7;  weight8 = 1.0 / distance8;
				weight9 = 1.0 / distance9;  weight10 = 1.0 / distance10;  weight11 = 1.0 / distance11;  weight12 = 1.0 / distance12;
				weight13 = 1.0 / distance13;  weight14 = 1.0 / distance14;  weight15 = 1.0 / distance15;  weight16 = 1.0 / distance16;

				weightSum = weight1 + weight2 + weight3 + weight4 + weight5 + weight6 + weight7 + weight8 +
					weight9 + weight10 + weight11 + weight12 + weight13 + weight14 + weight15 + weight16;

				// Get new single dimmension array index from new x and y location
				newI1 = point5 * width + point1;  newI2 = point5 * width + point2;  newI3 = point5 * width + point3;  newI4 = point5 * width + point4;
				newI5 = point6 * width + point1;  newI6 = point6 * width + point2;  newI7 = point6 * width + point3;  newI8 = point6 * width + point4;
				newI9 = point7 * width + point1;  newI10 = point7 * width + point2;  newI11 = point7 * width + point3;  newI12 = point7 * width + point4;
				newI13 = point8 * width + point1;  newI14 = point8 * width + point2;  newI15 = point8 * width + point3;  newI16 = point8 * width + point4;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData8[newI1]) + (weight2 * redData8[newI2]) + (weight3 * redData8[newI3]) + (weight4 * redData8[newI4]) +
					(weight5 * redData8[newI5]) + (weight6 * redData8[newI6]) + (weight7 * redData8[newI7]) + (weight8 * redData8[newI8]) +
					(weight9 * redData8[newI9]) + (weight10 * redData8[newI10]) + (weight11 * redData8[newI11]) + (weight12 * redData8[newI12]) +
					(weight13 * redData8[newI13]) + (weight14 * redData8[newI14]) + (weight15 * redData8[newI15]) + (weight16 * redData8[newI16])) / weightSum;

				tempGreen = ((weight1 * greenData8[newI1]) + (weight2 * greenData8[newI2]) + (weight3 * greenData8[newI3]) + (weight4 * greenData8[newI4]) +
					(weight5 * greenData8[newI5]) + (weight6 * greenData8[newI6]) + (weight7 * greenData8[newI7]) + (weight8 * greenData8[newI8]) +
					(weight9 * greenData8[newI9]) + (weight10 * greenData8[newI10]) + (weight11 * greenData8[newI11]) + (weight12 * greenData8[newI12]) +
					(weight13 * greenData8[newI13]) + (weight14 * greenData8[newI14]) + (weight15 * greenData8[newI15]) + (weight16 * greenData8[newI16])) / weightSum;

				tempBlue = ((weight1 * blueData8[newI1]) + (weight2 * blueData8[newI2]) + (weight3 * blueData8[newI3]) + (weight4 * blueData8[newI4]) +
					(weight5 * blueData8[newI5]) + (weight6 * blueData8[newI6]) + (weight7 * blueData8[newI7]) + (weight8 * blueData8[newI8]) +
					(weight9 * blueData8[newI9]) + (weight10 * blueData8[newI10]) + (weight11 * blueData8[newI11]) + (weight12 * blueData8[newI12]) +
					(weight13 * blueData8[newI13]) + (weight14 * blueData8[newI14]) + (weight15 * blueData8[newI15]) + (weight16 * blueData8[newI16])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 255) ? 255 : tempRed;
				tempGreen = (tempGreen > 255) ? 255 : tempGreen;
				tempBlue = (tempBlue > 255) ? 255 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData8Dup[i] = (uint8_t)tempRed;
				greenData8Dup[i] = (uint8_t)tempGreen;
				blueData8Dup[i] = (uint8_t)tempBlue;
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			x -= dWidth / 2.0;
			y -= dHeight / 2.0;

			// Shift by pivot point
			x -= pivotX;
			y -= pivotY;

			// Rotate point
			newX = ((double)x * angleCos) - ((double)y * angleSin);
			newY = ((double)x * angleSin) + ((double)y * angleCos);

			// Shift back
			newX += pivotX;
			newY += pivotY;

			// Veirfy pixel location is in bounds of original image size
			if (newX > 2 && newX < width - 2 && newY > 2 && newY < height - 2) {

				// new X is not on border of image
				if (newX > 1 && newX < width - 2) {
					point1 = wxRound(floor(newX)) - 1;
					point2 = wxRound(floor(newX));
					point3 = wxRound(ceil(newX));
					point4 = wxRound(ceil(newX)) + 1;
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
					point3 = wxRound(newX);
					point4 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 1 && newY < height - 2) {
					point5 = wxRound(floor(newY)) - 1;
					point6 = wxRound(floor(newY));
					point7 = wxRound(ceil(newY));
					point8 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point5 = wxRound(newY);
					point6 = wxRound(newY);
					point7 = wxRound(newY);
					point8 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point5) * (newY - point5));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point5) * (newY - point5));
				distance3 = ((newX - point3) * (newX - point3)) + ((newY - point5) * (newY - point5));
				distance4 = ((newX - point4) * (newX - point4)) + ((newY - point5) * (newY - point5));

				distance5 = ((newX - point1) * (newX - point1)) + ((newY - point6) * (newY - point6));
				distance6 = ((newX - point2) * (newX - point2)) + ((newY - point6) * (newY - point6));
				distance7 = ((newX - point3) * (newX - point3)) + ((newY - point6) * (newY - point6));
				distance8 = ((newX - point4) * (newX - point4)) + ((newY - point6) * (newY - point6));

				distance9 = ((newX - point1) * (newX - point1)) + ((newY - point7) * (newY - point7));
				distance10 = ((newX - point2) * (newX - point2)) + ((newY - point7) * (newY - point7));
				distance11 = ((newX - point3) * (newX - point3)) + ((newY - point7) * (newY - point7));
				distance12 = ((newX - point4) * (newX - point4)) + ((newY - point7) * (newY - point7));

				distance13 = ((newX - point1) * (newX - point1)) + ((newY - point8) * (newY - point8));
				distance14 = ((newX - point2) * (newX - point2)) + ((newY - point8) * (newY - point8));
				distance15 = ((newX - point3) * (newX - point3)) + ((newY - point8) * (newY - point8));
				distance16 = ((newX - point4) * (newX - point4)) + ((newY - point8) * (newY - point8));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point5 * width + point1;
					redData16Dup[i] = redData16[newI1];
					greenData16Dup[i] = greenData16[newI1];
					blueData16Dup[i] = blueData16[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				else if (distance2 < useNearestNeighborDistance) {

					newI2 = point5 * width + point2;
					redData16Dup[i] = redData16[newI2];
					greenData16Dup[i] = greenData16[newI2];
					blueData16Dup[i] = blueData16[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				else if (distance3 < useNearestNeighborDistance) {

					newI3 = point5 * width + point3;
					redData16Dup[i] = redData16[newI3];
					greenData16Dup[i] = greenData16[newI3];
					blueData16Dup[i] = blueData16[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				else if (distance4 < useNearestNeighborDistance) {

					newI4 = point5 * width + point4;
					redData16Dup[i] = redData16[newI4];
					greenData16Dup[i] = greenData16[newI4];
					blueData16Dup[i] = blueData16[newI4];
					continue;
				}

				// If distance 5 is close enough to an actual point, use that closest point
				else if (distance5 < useNearestNeighborDistance) {

					newI5 = point6 * width + point1;
					redData16Dup[i] = redData16[newI5];
					greenData16Dup[i] = greenData16[newI5];
					blueData16Dup[i] = blueData16[newI5];
					continue;
				}

				// If distance 6 is close enough to an actual point, use that closest point
				else if (distance6 < useNearestNeighborDistance) {

					newI6 = point6 * width + point2;
					redData16Dup[i] = redData16[newI6];
					greenData16Dup[i] = greenData16[newI6];
					blueData16Dup[i] = blueData16[newI6];
					continue;
				}

				// If distance 7 is close enough to an actual point, use that closest point
				else if (distance7 < useNearestNeighborDistance) {

					newI7 = point6 * width + point3;
					redData16Dup[i] = redData16[newI7];
					greenData16Dup[i] = greenData16[newI7];
					blueData16Dup[i] = blueData16[newI7];
					continue;
				}

				// If distance 8 is close enough to an actual point, use that closest point
				else if (distance8 < useNearestNeighborDistance) {

					newI8 = point6 * width + point4;
					redData16Dup[i] = redData16[newI8];
					greenData16Dup[i] = greenData16[newI8];
					blueData16Dup[i] = blueData16[newI8];
					continue;
				}

				// If distance 9 is close enough to an actual point, use that closest point
				else if (distance9 < useNearestNeighborDistance) {

					newI9 = point7 * width + point1;
					redData16Dup[i] = redData16[newI9];
					greenData16Dup[i] = greenData16[newI9];
					blueData16Dup[i] = blueData16[newI9];
					continue;
				}

				// If distance 10 is close enough to an actual point, use that closest point
				else if (distance10 < useNearestNeighborDistance) {

					newI10 = point7 * width + point2;
					redData16Dup[i] = redData16[newI10];
					greenData16Dup[i] = greenData16[newI10];
					blueData16Dup[i] = blueData16[newI10];
					continue;
				}

				// If distance 11 is close enough to an actual point, use that closest point
				else if (distance11 < useNearestNeighborDistance) {

					newI11 = point7 * width + point3;
					redData16Dup[i] = redData16[newI11];
					greenData16Dup[i] = greenData16[newI11];
					blueData16Dup[i] = blueData16[newI11];
					continue;
				}

				// If distance 12 is close enough to an actual point, use that closest point
				else if (distance12 < useNearestNeighborDistance) {

					newI12 = point7 * width + point4;
					redData16Dup[i] = redData16[newI12];
					greenData16Dup[i] = greenData16[newI12];
					blueData16Dup[i] = blueData16[newI12];
					continue;
				}

				// If distance 13 is close enough to an actual point, use that closest point
				else if (distance13 < useNearestNeighborDistance) {

					newI13 = point8 * width + point1;
					redData16Dup[i] = redData16[newI13];
					greenData16Dup[i] = greenData16[newI13];
					blueData16Dup[i] = blueData16[newI13];
					continue;
				}

				// If distance 14 is close enough to an actual point, use that closest point
				else if (distance14 < useNearestNeighborDistance) {

					newI14 = point8 * width + point2;
					redData16Dup[i] = redData16[newI14];
					greenData16Dup[i] = greenData16[newI14];
					blueData16Dup[i] = blueData16[newI14];
					continue;
				}

				// If distance 15 is close enough to an actual point, use that closest point
				else if (distance15 < useNearestNeighborDistance) {

					newI15 = point8 * width + point3;
					redData16Dup[i] = redData16[newI15];
					greenData16Dup[i] = greenData16[newI15];
					blueData16Dup[i] = blueData16[newI15];
					continue;
				}

				// If distance 16 is close enough to an actual point, use that closest point
				else if (distance16 < useNearestNeighborDistance) {

					newI16 = point8 * width + point4;
					redData16Dup[i] = redData16[newI16];
					greenData16Dup[i] = greenData16[newI16];
					blueData16Dup[i] = blueData16[newI16];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;  weight2 = 1.0 / distance2;  weight3 = 1.0 / distance3;  weight4 = 1.0 / distance4;
				weight5 = 1.0 / distance5;  weight6 = 1.0 / distance6;  weight7 = 1.0 / distance7;  weight8 = 1.0 / distance8;
				weight9 = 1.0 / distance9;  weight10 = 1.0 / distance10;  weight11 = 1.0 / distance11;  weight12 = 1.0 / distance12;
				weight13 = 1.0 / distance13;  weight14 = 1.0 / distance14;  weight15 = 1.0 / distance15;  weight16 = 1.0 / distance16;

				weightSum = weight1 + weight2 + weight3 + weight4 + weight5 + weight6 + weight7 + weight8 +
					weight9 + weight10 + weight11 + weight12 + weight13 + weight14 + weight15 + weight16;

				// Get new single dimmension array index from new x and y location
				newI1 = point5 * width + point1;  newI2 = point5 * width + point2;  newI3 = point5 * width + point3;  newI4 = point5 * width + point4;
				newI5 = point6 * width + point1;  newI6 = point6 * width + point2;  newI7 = point6 * width + point3;  newI8 = point6 * width + point4;
				newI9 = point7 * width + point1;  newI10 = point7 * width + point2;  newI11 = point7 * width + point3;  newI12 = point7 * width + point4;
				newI13 = point8 * width + point1;  newI14 = point8 * width + point2;  newI15 = point8 * width + point3;  newI16 = point8 * width + point4;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData16[newI1]) + (weight2 * redData16[newI2]) + (weight3 * redData16[newI3]) + (weight4 * redData16[newI4]) +
					(weight5 * redData16[newI5]) + (weight6 * redData16[newI6]) + (weight7 * redData16[newI7]) + (weight8 * redData16[newI8]) +
					(weight9 * redData16[newI9]) + (weight10 * redData16[newI10]) + (weight11 * redData16[newI11]) + (weight12 * redData16[newI12]) +
					(weight13 * redData16[newI13]) + (weight14 * redData16[newI14]) + (weight15 * redData16[newI15]) + (weight16 * redData16[newI16])) / weightSum;

				tempGreen = ((weight1 * greenData16[newI1]) + (weight2 * greenData16[newI2]) + (weight3 * greenData16[newI3]) + (weight4 * greenData16[newI4]) +
					(weight5 * greenData16[newI5]) + (weight6 * greenData16[newI6]) + (weight7 * greenData16[newI7]) + (weight8 * greenData16[newI8]) +
					(weight9 * greenData16[newI9]) + (weight10 * greenData16[newI10]) + (weight11 * greenData16[newI11]) + (weight12 * greenData16[newI12]) +
					(weight13 * greenData16[newI13]) + (weight14 * greenData16[newI14]) + (weight15 * greenData16[newI15]) + (weight16 * greenData16[newI16])) / weightSum;

				tempBlue = ((weight1 * blueData16[newI1]) + (weight2 * blueData16[newI2]) + (weight3 * blueData16[newI3]) + (weight4 * blueData16[newI4]) +
					(weight5 * blueData16[newI5]) + (weight6 * blueData16[newI6]) + (weight7 * blueData16[newI7]) + (weight8 * blueData16[newI8]) +
					(weight9 * blueData16[newI9]) + (weight10 * blueData16[newI10]) + (weight11 * blueData16[newI11]) + (weight12 * blueData16[newI12]) +
					(weight13 * blueData16[newI13]) + (weight14 * blueData16[newI14]) + (weight15 * blueData16[newI15]) + (weight16 * blueData16[newI16])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 65535) ? 65535 : tempRed;
				tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
				tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData16Dup[i] = (uint16_t)tempRed;
				greenData16Dup[i] = (uint16_t)tempGreen;
				blueData16Dup[i] = (uint16_t)tempBlue;
			}
		}
	}
}

int Processor::GetExpandedRotationWidth(double angleDegrees, int originalWidth, int originalHeight) {

	if (angleDegrees < 0.0) {
		angleDegrees += 180.0;
	}
	if ((angleDegrees < 90.0 && angleDegrees > 0.0) || (angleDegrees > -90.0 && angleDegrees < 0.0)) {
		return (int)(((double)originalWidth * cos(angleDegrees * pi / 180.0)) + ((double)originalHeight * sin(angleDegrees * pi / 180.0)));
	}
	else if ((angleDegrees > 90.0 && angleDegrees < 180.0) || (angleDegrees < -90.0 && angleDegrees > -180.0)) {
		angleDegrees -= 90.0;
		return (int)(((double)originalHeight * cos(angleDegrees * pi / 180.0)) + ((double)originalWidth * sin(angleDegrees * pi / 180.0)));
	}
	else if (angleDegrees == 90 || angleDegrees == -90) {
		return originalHeight;
	}
	else {
		return originalWidth;
	}
}

int Processor::GetExpandedRotationHeight(double angleDegrees, int originalWidth, int originalHeight) {

	if (angleDegrees < 0.0) {
		angleDegrees *= -1.0;
	}
	if ((angleDegrees < 90.0 && angleDegrees > 0.0) || (angleDegrees > -90.0 && angleDegrees < 0.0)) {
		return (int)(((double)originalWidth * sin(angleDegrees * pi / 180.0)) + ((double)originalHeight * cos(angleDegrees * pi / 180.0)));
	}
	else if ((angleDegrees > 90.0 && angleDegrees < 180.0) || (angleDegrees < -90.0 && angleDegrees > -180.0)) {
		angleDegrees -= 90.0;
		return (int)(((double)originalHeight * sin(angleDegrees * pi / 180.0)) + ((double)originalWidth * cos(angleDegrees * pi / 180.0)));
	}
	else if (angleDegrees == 90 || angleDegrees == -90) {
		return originalWidth;
	}
	else {
		return originalHeight;
	}
}

int Processor::GetFittedRotationWidth(double angleDegrees, int originalWidth, int originalHeight) {

	// Height stays the same if 180 degree rotation
	if (angleDegrees == 0.0 || angleDegrees == 180.0 || angleDegrees == -180.0) {
		return originalWidth;
	}

	// Height becomes width if 90 degree rotation
	else if (angleDegrees == 90.0 || angleDegrees == -90.0) {
		return originalHeight;
	}

	// Get absolute value of sin and cos of angle
	double absoluteSin = sin(angleDegrees * pi / 180.0);
	double absoluteCos = cos(angleDegrees * pi / 180.0);
	if (absoluteSin < 0.0) { absoluteSin *= -1.0; }
	if (absoluteCos < 0.0) { absoluteCos *= -1.0; }

	// Find length of shorter and longer sides
	int shorterSide = 0;
	int longerSide = 0;

	bool doSwap = false;
	if (originalWidth > originalHeight) {
		longerSide = originalWidth;
		shorterSide = originalHeight;
	}
	else {
		longerSide = originalHeight;
		shorterSide = originalWidth;
		doSwap = true;
	}

	if (shorterSide < 2.0 * absoluteSin * absoluteCos * longerSide) {
		if (!doSwap) {
			return (int)((0.5 * shorterSide) / absoluteSin);
		}
		else {
			return (int)((0.5 * shorterSide) / absoluteCos);
		}
	}
	else {

		double absoulteCos2 = absoluteCos * absoluteCos;
		double absoulteSin2 = absoluteSin * absoluteSin;
		double absoulteCos2MinAbsolulteSin2 = absoulteCos2 - absoulteSin2;

		return (int)(((originalWidth * absoluteCos) - (originalHeight * absoluteSin)) / absoulteCos2MinAbsolulteSin2);
	}
}

int Processor::GetFittedRotationHeight(double angleDegrees, int originalWidth, int originalHeight) {

	// Height stays the same if 180 degree rotation
	if (angleDegrees == 0.0 || angleDegrees == 180.0 || angleDegrees == -180.0) {
		return originalHeight;
	}

	// Height becomes width if 90 degree rotation
	else if (angleDegrees == 90.0 || angleDegrees == -90.0) {
		return originalWidth;
	}

	// Get absolute value of sin and cos of angle
	double absoluteSin = sin(angleDegrees * pi / 180.0);
	double absoluteCos = cos(angleDegrees * pi / 180.0);
	if (absoluteSin < 0.0) { absoluteSin *= -1.0; }
	if (absoluteCos < 0.0) { absoluteCos *= -1.0; }

	// Find length of shorter and longer sides
	int shorterSide = 0;
	int longerSide = 0;

	bool doSwap = false;
	if (originalWidth > originalHeight) {
		longerSide = originalWidth;
		shorterSide = originalHeight;
	}
	else {
		longerSide = originalHeight;
		shorterSide = originalWidth;
		doSwap = true;
	}

	if (shorterSide < 2.0 * absoluteSin * absoluteCos * longerSide) {
		if (!doSwap) {
			return (int)((0.5 * shorterSide) / absoluteCos);
		}
		else {
			return (int)((0.5 * shorterSide) / absoluteSin);
		}
	}
	else {

		double absoulteCos2 = absoluteCos * absoluteCos;
		double absoulteSin2 = absoluteSin * absoluteSin;
		double absoulteCos2MinAbsolulteSin2 = absoulteCos2 - absoulteSin2;

		return (int)(((originalHeight * absoluteCos) - (originalWidth * absoluteSin)) / absoulteCos2MinAbsolulteSin2);
	}
}

bool Processor::SetupScale(int newWidth, int newHeight){

	// Create temp image with new scale size
	tempImage = new Image();
	tempImage->SetWidth(newWidth);
	tempImage->SetHeight(newHeight);
	if (this->GetImage()->GetColorDepth() == 16) { tempImage->Enable16Bit(); }
	tempImage->InitImage();

	if (tempImage->GetErrorStr() != "") {
		return false;
	}
	return true;
}

void Processor::CleanupScale(){

	// Copy temp image to image
	delete img;
	img = new Image(*tempImage);

	// Cleanup temp image
	if(tempImage != NULL){
		delete tempImage;
		tempImage = NULL;
	}
}

void Processor::ScaleNearest(int dataStart, int dataEnd){

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	int width = img->GetWidth();
	int height = img->GetHeight();

	int newWidth = tempImage->GetWidth();
	int newHeight = tempImage->GetHeight();

	double xRatio = (double)width / (double)newWidth;
	double yRatio = (double)height / (double)newHeight;

	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int newI = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;
			
			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;

			// Round double to int
			newX = wxRound(newX);
			newY = wxRound(newY);

			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width && newY >= 0 && newY < height) {

				// Get new single dimmension array index from new x and y location
				newI = newY * width + newX;

				// Copy pixel from old location to new location
				redData8Dup[i] = redData8[newI];
				greenData8Dup[i] = greenData8[newI];
				blueData8Dup[i] = blueData8[newI];
			}	
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;
			
			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;

			// Round double to int
			newX = wxRound(newX);
			newY = wxRound(newY);

			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width && newY >= 0 && newY < height) {

				// Get new single dimmension array index from new x and y location
				newI = newY * width + newX;

				// Copy pixel from old location to new location
				redData16Dup[i] = redData16[newI];
				greenData16Dup[i] = greenData16[newI];
				blueData16Dup[i] = blueData16[newI];
			}	
		}
	}
}

void Processor::ScaleBilinear(int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	double useNearestNeighborDistance = 0.0001;

	int width = img->GetWidth();
	int height = img->GetHeight();

	int newWidth = tempImage->GetWidth();
	int newHeight = tempImage->GetHeight();

	double xRatio = (double)width / (double)newWidth;
	double yRatio = (double)height / (double)newHeight;

	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	// Neighbors of exact rotation location
	int point1 = 0;
	int point2 = 0;
	int point3 = 0;
	int point4 = 0;

	// Distances from exact point to nearest points to interpolate
	double distance1 = 0.0;
	double distance2 = 0.0;
	double distance3 = 0.0;
	double distance4 = 0.0;

	// Actual index of data of neighbors to interpoalte
	int newI1 = 0;
	int newI2 = 0;
	int newI3 = 0;
	int newI4 = 0;

	// Weights each neighbor will recieve based on distance
	double weight1 = 0.0;
	double weight2 = 0.0;
	double weight3 = 0.0;
	double weight4 = 0.0;
	double weightSum = 1.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;
		
			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width  && newY >= 0 && newY < height) {

				// new X is not on border of image
				if (newX > 0 && newX < width - 1) {
					point1 = wxRound(floor(newX));
					point2 = wxRound(ceil(newX));
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 0 && newY < height - 1) {
					point3 = wxRound(floor(newY));
					point4 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point3 = wxRound(newY);
					point4 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point3) * (newY - point3));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point3) * (newY - point3));
				distance3 = ((newX - point2) * (newX - point2)) + ((newY - point4) * (newY - point4));
				distance4 = ((newX - point1) * (newX - point1)) + ((newY - point4) * (newY - point4));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point3 * width + point1;
					redData8Dup[i] = redData8[newI1];
					greenData8Dup[i] = greenData8[newI1];
					blueData8Dup[i] = blueData8[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				if (distance2 < useNearestNeighborDistance) {

					newI2 = point3 * width + point2;
					redData8Dup[i] = redData8[newI2];
					greenData8Dup[i] = greenData8[newI2];
					blueData8Dup[i] = blueData8[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				if (distance3 < useNearestNeighborDistance) {

					newI3 = point4 * width + point1;
					redData8Dup[i] = redData8[newI3];
					greenData8Dup[i] = greenData8[newI3];
					blueData8Dup[i] = blueData8[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				if (distance4 < useNearestNeighborDistance) {

					newI4 = point4 * width + point1;
					redData8Dup[i] = redData8[newI4];
					greenData8Dup[i] = greenData8[newI4];
					blueData8Dup[i] = blueData8[newI4];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;
				weight2 = 1.0 / distance2;
				weight3 = 1.0 / distance3;
				weight4 = 1.0 / distance4;
				weightSum = weight1 + weight2 + weight3 + weight4;

				// Get new single dimmension array index from new x and y location
				newI1 = point3 * width + point1;
				newI2 = point3 * width + point2;
				newI3 = point4 * width + point1;
				newI4 = point4 * width + point2;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData8[newI1]) + (weight2 * redData8[newI2]) + (weight3 * redData8[newI3]) + (weight4 * redData8[newI4])) / weightSum;
				tempGreen = ((weight1 * greenData8[newI1]) + (weight2 * greenData8[newI2]) + (weight3 * greenData8[newI3]) + (weight4 * greenData8[newI4])) / weightSum;
				tempBlue = ((weight1 * blueData8[newI1]) + (weight2 * blueData8[newI2]) + (weight3 * blueData8[newI3]) + (weight4 * blueData8[newI4])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 255) ? 255 : tempRed;
				tempGreen = (tempGreen > 255) ? 255 : tempGreen;
				tempBlue = (tempBlue > 255) ? 255 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData8Dup[i] = (uint8_t)tempRed;
				greenData8Dup[i] = (uint8_t)tempGreen;
				blueData8Dup[i] = (uint8_t)tempBlue;
			}
		}

	}
	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;

			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width  && newY >= 0 && newY < height) {

				// new X is not on border of image
				if (newX > 0 && newX < width - 1) {
					point1 = wxRound(floor(newX));
					point2 = wxRound(ceil(newX));
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 0 && newY < height - 1) {
					point3 = wxRound(floor(newY));
					point4 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point3 = wxRound(newY);
					point4 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point3) * (newY - point3));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point3) * (newY - point3));
				distance3 = ((newX - point2) * (newX - point2)) + ((newY - point4) * (newY - point4));
				distance4 = ((newX - point1) * (newX - point1)) + ((newY - point4) * (newY - point4));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point3 * width + point1;
					redData16Dup[i] = redData16[newI1];
					greenData16Dup[i] = greenData16[newI1];
					blueData16Dup[i] = blueData16[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				if (distance2 < useNearestNeighborDistance) {

					newI2 = point3 * width + point2;
					redData16Dup[i] = redData16[newI2];
					greenData16Dup[i] = greenData16[newI2];
					blueData16Dup[i] = blueData16[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				if (distance3 < useNearestNeighborDistance) {

					newI3 = point4 * width + point1;
					redData16Dup[i] = redData16[newI3];
					greenData16Dup[i] = greenData16[newI3];
					blueData16Dup[i] = blueData16[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				if (distance4 < useNearestNeighborDistance) {

					newI4 = point4 * width + point1;
					redData16Dup[i] = redData16[newI4];
					greenData16Dup[i] = greenData16[newI4];
					blueData16Dup[i] = blueData16[newI4];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;
				weight2 = 1.0 / distance2;
				weight3 = 1.0 / distance3;
				weight4 = 1.0 / distance4;
				weightSum = weight1 + weight2 + weight3 + weight4;

				// Get new single dimmension array index from new x and y location
				newI1 = point3 * width + point1;
				newI2 = point3 * width + point2;
				newI3 = point4 * width + point1;
				newI4 = point4 * width + point2;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData16[newI1]) + (weight2 * redData16[newI2]) + (weight3 * redData16[newI3]) + (weight4 * redData16[newI4])) / weightSum;
				tempGreen = ((weight1 * greenData16[newI1]) + (weight2 * greenData16[newI2]) + (weight3 * greenData16[newI3]) + (weight4 * greenData16[newI4])) / weightSum;
				tempBlue = ((weight1 * blueData16[newI1]) + (weight2 * blueData16[newI2]) + (weight3 * blueData16[newI3]) + (weight4 * blueData16[newI4])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 65535) ? 65535 : tempRed;
				tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
				tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData16Dup[i] = (uint16_t)tempRed;
				greenData16Dup[i] = (uint16_t)tempGreen;
				blueData16Dup[i] = (uint16_t)tempBlue;
			}
		}
	}
}

void Processor::ScaleBicubic(int dataStart, int dataEnd) {

	// Get number of pixels for the image
	int newDataSize = tempImage->GetWidth() * tempImage->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	double useNearestNeighborDistance = 0.0001;

	int width = img->GetWidth();
	int height = img->GetHeight();

	int newWidth = tempImage->GetWidth();
	int newHeight = tempImage->GetHeight();

	double xRatio = (double)width / (double)newWidth;
	double yRatio = (double)height / (double)newHeight;

	int x = 0;
	int y = 0;
	double newX = 0;
	double newY = 0;

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;
	int point1 = 0; int point2 = 0; int point3 = 0; int point4 = 0;
	int point5 = 0; int point6 = 0; int point7 = 0; int point8 = 0;

	double distance1 = 0.0; double distance2 = 0.0; double distance3 = 0.0; double distance4 = 0.0;
	double distance5 = 0.0; double distance6 = 0.0; double distance7 = 0.0; double distance8 = 0.0;
	double distance9 = 0.0; double distance10 = 0.0; double distance11 = 0.0; double distance12 = 0.0;
	double distance13 = 0.0; double distance14 = 0.0; double distance15 = 0.0; double distance16 = 0.0;

	int newI1 = 0; int newI2 = 0; int newI3 = 0; int newI4 = 0;
	int newI5 = 0; int newI6 = 0; int newI7 = 0; int newI8 = 0;
	int newI9 = 0; int newI10 = 0; int newI11 = 0; int newI12 = 0;
	int newI13 = 0; int newI14 = 0; int newI15 = 0; int newI16 = 0;

	double weight1 = 0.0; double weight2 = 0.0; double weight3 = 0.0; double weight4 = 0.0;
	double weight5 = 0.0; double weight6 = 0.0; double weight7 = 0.0; double weight8 = 0.0;
	double weight9 = 0.0; double weight10 = 0.0; double weight11 = 0.0; double weight12 = 0.0;
	double weight13 = 0.0; double weight14 = 0.0; double weight15 = 0.0; double weight16 = 0.0;

	double weightSum = 1.0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) {	return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;

			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width && newY >= 0 && newY < height) {

				// new X is not on border of image
				if (newX > 1 && newX < width - 2) {
					point1 = wxRound(floor(newX)) - 1;
					point2 = wxRound(floor(newX));
					point3 = wxRound(ceil(newX));
					point4 = wxRound(ceil(newX)) + 1;
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
					point3 = wxRound(newX);
					point4 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 1 && newY < height - 2) {
					point5 = wxRound(floor(newY)) - 1;
					point6 = wxRound(floor(newY));
					point7 = wxRound(ceil(newY));
					point8 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point5 = wxRound(newY);
					point6 = wxRound(newY);
					point7 = wxRound(newY);
					point8 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point5) * (newY - point5));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point5) * (newY - point5));
				distance3 = ((newX - point3) * (newX - point3)) + ((newY - point5) * (newY - point5));
				distance4 = ((newX - point4) * (newX - point4)) + ((newY - point5) * (newY - point5));

				distance5 = ((newX - point1) * (newX - point1)) + ((newY - point6) * (newY - point6));
				distance6 = ((newX - point2) * (newX - point2)) + ((newY - point6) * (newY - point6));
				distance7 = ((newX - point3) * (newX - point3)) + ((newY - point6) * (newY - point6));
				distance8 = ((newX - point4) * (newX - point4)) + ((newY - point6) * (newY - point6));

				distance9 = ((newX - point1) * (newX - point1)) + ((newY - point7) * (newY - point7));
				distance10 = ((newX - point2) * (newX - point2)) + ((newY - point7) * (newY - point7));
				distance11 = ((newX - point3) * (newX - point3)) + ((newY - point7) * (newY - point7));
				distance12 = ((newX - point4) * (newX - point4)) + ((newY - point7) * (newY - point7));

				distance13 = ((newX - point1) * (newX - point1)) + ((newY - point8) * (newY - point8));
				distance14 = ((newX - point2) * (newX - point2)) + ((newY - point8) * (newY - point8));
				distance15 = ((newX - point3) * (newX - point3)) + ((newY - point8) * (newY - point8));
				distance16 = ((newX - point4) * (newX - point4)) + ((newY - point8) * (newY - point8));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point5 * width + point1;
					redData8Dup[i] = redData8[newI1];
					greenData8Dup[i] = greenData8[newI1];
					blueData8Dup[i] = blueData8[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				else if (distance2 < useNearestNeighborDistance) {

					newI2 = point5 * width + point2;
					redData8Dup[i] = redData8[newI2];
					greenData8Dup[i] = greenData8[newI2];
					blueData8Dup[i] = blueData8[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				else if (distance3 < useNearestNeighborDistance) {

					newI3 = point5 * width + point3;
					redData8Dup[i] = redData8[newI3];
					greenData8Dup[i] = greenData8[newI3];
					blueData8Dup[i] = blueData8[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				else if (distance4 < useNearestNeighborDistance) {

					newI4 = point5 * width + point4;
					redData8Dup[i] = redData8[newI4];
					greenData8Dup[i] = greenData8[newI4];
					blueData8Dup[i] = blueData8[newI4];
					continue;
				}

				// If distance 5 is close enough to an actual point, use that closest point
				else if (distance5 < useNearestNeighborDistance) {

					newI5 = point6 * width + point1;
					redData8Dup[i] = redData8[newI5];
					greenData8Dup[i] = greenData8[newI5];
					blueData8Dup[i] = blueData8[newI5];
					continue;
				}

				// If distance 6 is close enough to an actual point, use that closest point
				else if (distance6 < useNearestNeighborDistance) {

					newI6 = point6 * width + point2;
					redData8Dup[i] = redData8[newI6];
					greenData8Dup[i] = greenData8[newI6];
					blueData8Dup[i] = blueData8[newI6];
					continue;
				}

				// If distance 7 is close enough to an actual point, use that closest point
				else if (distance7 < useNearestNeighborDistance) {

					newI7 = point6 * width + point3;
					redData8Dup[i] = redData8[newI7];
					greenData8Dup[i] = greenData8[newI7];
					blueData8Dup[i] = blueData8[newI7];
					continue;
				}

				// If distance 8 is close enough to an actual point, use that closest point
				else if (distance8 < useNearestNeighborDistance) {

					newI8 = point6 * width + point4;
					redData8Dup[i] = redData8[newI8];
					greenData8Dup[i] = greenData8[newI8];
					blueData8Dup[i] = blueData8[newI8];
					continue;
				}
				// If distance 9 is close enough to an actual point, use that closest point
				else if (distance9 < useNearestNeighborDistance) {

					newI9 = point7 * width + point1;
					redData8Dup[i] = redData8[newI9];
					greenData8Dup[i] = greenData8[newI9];
					blueData8Dup[i] = blueData8[newI9];
					continue;
				}

				// If distance 10 is close enough to an actual point, use that closest point
				else if (distance10 < useNearestNeighborDistance) {

					newI10 = point7 * width + point2;
					redData8Dup[i] = redData8[newI10];
					greenData8Dup[i] = greenData8[newI10];
					blueData8Dup[i] = blueData8[newI10];
					continue;
				}

				// If distance 11 is close enough to an actual point, use that closest point
				else if (distance11 < useNearestNeighborDistance) {

					newI11 = point7 * width + point3;
					redData8Dup[i] = redData8[newI11];
					greenData8Dup[i] = greenData8[newI11];
					blueData8Dup[i] = blueData8[newI11];
					continue;
				}

				// If distance 12 is close enough to an actual point, use that closest point
				else if (distance12 < useNearestNeighborDistance) {

					newI12 = point7 * width + point4;
					redData8Dup[i] = redData8[newI12];
					greenData8Dup[i] = greenData8[newI12];
					blueData8Dup[i] = blueData8[newI12];
					continue;
				}

				// If distance 13 is close enough to an actual point, use that closest point
				else if (distance13 < useNearestNeighborDistance) {

					newI13 = point8 * width + point1;
					redData8Dup[i] = redData8[newI13];
					greenData8Dup[i] = greenData8[newI13];
					blueData8Dup[i] = blueData8[newI13];
					continue;
				}

				// If distance 14 is close enough to an actual point, use that closest point
				else if (distance14 < useNearestNeighborDistance) {

					newI14 = point8 * width + point2;
					redData8Dup[i] = redData8[newI14];
					greenData8Dup[i] = greenData8[newI14];
					blueData8Dup[i] = blueData8[newI14];
					continue;
				}

				// If distance 15 is close enough to an actual point, use that closest point
				else if (distance15 < useNearestNeighborDistance) {

					newI15 = point8 * width + point3;
					redData8Dup[i] = redData8[newI15];
					greenData8Dup[i] = greenData8[newI15];
					blueData8Dup[i] = blueData8[newI15];
					continue;
				}

				// If distance 16 is close enough to an actual point, use that closest point
				else if (distance16 < useNearestNeighborDistance) {

					newI16 = point8 * width + point4;
					redData8Dup[i] = redData8[newI16];
					greenData8Dup[i] = greenData8[newI16];
					blueData8Dup[i] = blueData8[newI16];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;  weight2 = 1.0 / distance2;  weight3 = 1.0 / distance3;  weight4 = 1.0 / distance4;
				weight5 = 1.0 / distance5;  weight6 = 1.0 / distance6;  weight7 = 1.0 / distance7;  weight8 = 1.0 / distance8;
				weight9 = 1.0 / distance9;  weight10 = 1.0 / distance10;  weight11 = 1.0 / distance11;  weight12 = 1.0 / distance12;
				weight13 = 1.0 / distance13;  weight14 = 1.0 / distance14;  weight15 = 1.0 / distance15;  weight16 = 1.0 / distance16;

				weightSum = weight1 + weight2 + weight3 + weight4 + weight5 + weight6 + weight7 + weight8 +
					weight9 + weight10 + weight11 + weight12 + weight13 + weight14 + weight15 + weight16;

				// Get new single dimmension array index from new x and y location
				newI1 = point5 * width + point1;  newI2 = point5 * width + point2;  newI3 = point5 * width + point3;  newI4 = point5 * width + point4;
				newI5 = point6 * width + point1;  newI6 = point6 * width + point2;  newI7 = point6 * width + point3;  newI8 = point6 * width + point4;
				newI9 = point7 * width + point1;  newI10 = point7 * width + point2;  newI11 = point7 * width + point3;  newI12 = point7 * width + point4;
				newI13 = point8 * width + point1;  newI14 = point8 * width + point2;  newI15 = point8 * width + point3;  newI16 = point8 * width + point4;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData8[newI1]) + (weight2 * redData8[newI2]) + (weight3 * redData8[newI3]) + (weight4 * redData8[newI4]) +
					(weight5 * redData8[newI5]) + (weight6 * redData8[newI6]) + (weight7 * redData8[newI7]) + (weight8 * redData8[newI8]) +
					(weight9 * redData8[newI9]) + (weight10 * redData8[newI10]) + (weight11 * redData8[newI11]) + (weight12 * redData8[newI12]) +
					(weight13 * redData8[newI13]) + (weight14 * redData8[newI14]) + (weight15 * redData8[newI15]) + (weight16 * redData8[newI16])) / weightSum;

				tempGreen = ((weight1 * greenData8[newI1]) + (weight2 * greenData8[newI2]) + (weight3 * greenData8[newI3]) + (weight4 * greenData8[newI4]) +
					(weight5 * greenData8[newI5]) + (weight6 * greenData8[newI6]) + (weight7 * greenData8[newI7]) + (weight8 * greenData8[newI8]) +
					(weight9 * greenData8[newI9]) + (weight10 * greenData8[newI10]) + (weight11 * greenData8[newI11]) + (weight12 * greenData8[newI12]) +
					(weight13 * greenData8[newI13]) + (weight14 * greenData8[newI14]) + (weight15 * greenData8[newI15]) + (weight16 * greenData8[newI16])) / weightSum;

				tempBlue = ((weight1 * blueData8[newI1]) + (weight2 * blueData8[newI2]) + (weight3 * blueData8[newI3]) + (weight4 * blueData8[newI4]) +
					(weight5 * blueData8[newI5]) + (weight6 * blueData8[newI6]) + (weight7 * blueData8[newI7]) + (weight8 * blueData8[newI8]) +
					(weight9 * blueData8[newI9]) + (weight10 * blueData8[newI10]) + (weight11 * blueData8[newI11]) + (weight12 * blueData8[newI12]) +
					(weight13 * blueData8[newI13]) + (weight14 * blueData8[newI14]) + (weight15 * blueData8[newI15]) + (weight16 * blueData8[newI16])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 255) ? 255 : tempRed;
				tempGreen = (tempGreen > 255) ? 255 : tempGreen;
				tempBlue = (tempBlue > 255) ? 255 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData8Dup[i] = (uint8_t)tempRed;
				greenData8Dup[i] = (uint8_t)tempGreen;
				blueData8Dup[i] = (uint8_t)tempBlue;
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % newWidth;
			y = i / newWidth;

			// Scale point
			newX = x * xRatio;
			newY = y * yRatio;

			// Veirfy pixel location is in bounds of original image size
			if (newX >= 0 && newX < width && newY >= 0 && newY < height) {

				// new X is not on border of image
				if (newX > 1 && newX < width - 2) {
					point1 = wxRound(floor(newX)) - 1;
					point2 = wxRound(floor(newX));
					point3 = wxRound(ceil(newX));
					point4 = wxRound(ceil(newX)) + 1;
				}

				// new X is on border.
				else {
					point1 = wxRound(newX);
					point2 = wxRound(newX);
					point3 = wxRound(newX);
					point4 = wxRound(newX);
				}

				// new Y is not on border of image
				if (newY > 1 && newY < height - 2) {
					point5 = wxRound(floor(newY)) - 1;
					point6 = wxRound(floor(newY));
					point7 = wxRound(ceil(newY));
					point8 = wxRound(ceil(newY));
				}

				// new Y is on border.
				else {
					point5 = wxRound(newY);
					point6 = wxRound(newY);
					point7 = wxRound(newY);
					point8 = wxRound(newY);
				}

				// Get distances between actual point and rounded points
				distance1 = ((newX - point1) * (newX - point1)) + ((newY - point5) * (newY - point5));
				distance2 = ((newX - point2) * (newX - point2)) + ((newY - point5) * (newY - point5));
				distance3 = ((newX - point3) * (newX - point3)) + ((newY - point5) * (newY - point5));
				distance4 = ((newX - point4) * (newX - point4)) + ((newY - point5) * (newY - point5));

				distance5 = ((newX - point1) * (newX - point1)) + ((newY - point6) * (newY - point6));
				distance6 = ((newX - point2) * (newX - point2)) + ((newY - point6) * (newY - point6));
				distance7 = ((newX - point3) * (newX - point3)) + ((newY - point6) * (newY - point6));
				distance8 = ((newX - point4) * (newX - point4)) + ((newY - point6) * (newY - point6));

				distance9 = ((newX - point1) * (newX - point1)) + ((newY - point7) * (newY - point7));
				distance10 = ((newX - point2) * (newX - point2)) + ((newY - point7) * (newY - point7));
				distance11 = ((newX - point3) * (newX - point3)) + ((newY - point7) * (newY - point7));
				distance12 = ((newX - point4) * (newX - point4)) + ((newY - point7) * (newY - point7));

				distance13 = ((newX - point1) * (newX - point1)) + ((newY - point8) * (newY - point8));
				distance14 = ((newX - point2) * (newX - point2)) + ((newY - point8) * (newY - point8));
				distance15 = ((newX - point3) * (newX - point3)) + ((newY - point8) * (newY - point8));
				distance16 = ((newX - point4) * (newX - point4)) + ((newY - point8) * (newY - point8));

				// If distance 1 is close enough to an actual point, use that closest point
				if (distance1 < useNearestNeighborDistance) {

					newI1 = point5 * width + point1;
					redData16Dup[i] = redData16[newI1];
					greenData16Dup[i] = greenData16[newI1];
					blueData16Dup[i] = blueData16[newI1];
					continue;
				}

				// If distance 2 is close enough to an actual point, use that closest point
				else if (distance2 < useNearestNeighborDistance) {

					newI2 = point5 * width + point2;
					redData16Dup[i] = redData16[newI2];
					greenData16Dup[i] = greenData16[newI2];
					blueData16Dup[i] = blueData16[newI2];
					continue;
				}

				// If distance 3 is close enough to an actual point, use that closest point
				else if (distance3 < useNearestNeighborDistance) {

					newI3 = point5 * width + point3;
					redData16Dup[i] = redData16[newI3];
					greenData16Dup[i] = greenData16[newI3];
					blueData16Dup[i] = blueData16[newI3];
					continue;
				}

				// If distance 4 is close enough to an actual point, use that closest point
				else if (distance4 < useNearestNeighborDistance) {

					newI4 = point5 * width + point4;
					redData16Dup[i] = redData16[newI4];
					greenData16Dup[i] = greenData16[newI4];
					blueData16Dup[i] = blueData16[newI4];
					continue;
				}

				// If distance 5 is close enough to an actual point, use that closest point
				else if (distance5 < useNearestNeighborDistance) {

					newI5 = point6 * width + point1;
					redData16Dup[i] = redData16[newI5];
					greenData16Dup[i] = greenData16[newI5];
					blueData16Dup[i] = blueData16[newI5];
					continue;
				}

				// If distance 6 is close enough to an actual point, use that closest point
				else if (distance6 < useNearestNeighborDistance) {

					newI6 = point6 * width + point2;
					redData16Dup[i] = redData16[newI6];
					greenData16Dup[i] = greenData16[newI6];
					blueData16Dup[i] = blueData16[newI6];
					continue;
				}

				// If distance 7 is close enough to an actual point, use that closest point
				else if (distance7 < useNearestNeighborDistance) {

					newI7 = point6 * width + point3;
					redData16Dup[i] = redData16[newI7];
					greenData16Dup[i] = greenData16[newI7];
					blueData16Dup[i] = blueData16[newI7];
					continue;
				}

				// If distance 8 is close enough to an actual point, use that closest point
				else if (distance8 < useNearestNeighborDistance) {

					newI8 = point6 * width + point4;
					redData16Dup[i] = redData16[newI8];
					greenData16Dup[i] = greenData16[newI8];
					blueData16Dup[i] = blueData16[newI8];
					continue;
				}
				// If distance 9 is close enough to an actual point, use that closest point
				else if (distance9 < useNearestNeighborDistance) {

					newI9 = point7 * width + point1;
					redData16Dup[i] = redData16[newI9];
					greenData16Dup[i] = greenData16[newI9];
					blueData16Dup[i] = blueData16[newI9];
					continue;
				}

				// If distance 10 is close enough to an actual point, use that closest point
				else if (distance10 < useNearestNeighborDistance) {

					newI10 = point7 * width + point2;
					redData16Dup[i] = redData16[newI10];
					greenData16Dup[i] = greenData16[newI10];
					blueData16Dup[i] = blueData16[newI10];
					continue;
				}

				// If distance 11 is close enough to an actual point, use that closest point
				else if (distance11 < useNearestNeighborDistance) {

					newI11 = point7 * width + point3;
					redData16Dup[i] = redData16[newI11];
					greenData16Dup[i] = greenData16[newI11];
					blueData16Dup[i] = blueData16[newI11];
					continue;
				}

				// If distance 12 is close enough to an actual point, use that closest point
				else if (distance12 < useNearestNeighborDistance) {

					newI12 = point7 * width + point4;
					redData16Dup[i] = redData16[newI12];
					greenData16Dup[i] = greenData16[newI12];
					blueData16Dup[i] = blueData16[newI12];
					continue;
				}

				// If distance 13 is close enough to an actual point, use that closest point
				else if (distance13 < useNearestNeighborDistance) {

					newI13 = point8 * width + point1;
					redData16Dup[i] = redData16[newI13];
					greenData16Dup[i] = greenData16[newI13];
					blueData16Dup[i] = blueData16[newI13];
					continue;
				}

				// If distance 14 is close enough to an actual point, use that closest point
				else if (distance14 < useNearestNeighborDistance) {

					newI14 = point8 * width + point2;
					redData16Dup[i] = redData16[newI14];
					greenData16Dup[i] = greenData16[newI14];
					blueData16Dup[i] = blueData16[newI14];
					continue;
				}

				// If distance 15 is close enough to an actual point, use that closest point
				else if (distance15 < useNearestNeighborDistance) {

					newI15 = point8 * width + point3;
					redData16Dup[i] = redData16[newI15];
					greenData16Dup[i] = greenData16[newI15];
					blueData16Dup[i] = blueData16[newI15];
					continue;
				}

				// If distance 16 is close enough to an actual point, use that closest point
				else if (distance16 < useNearestNeighborDistance) {

					newI16 = point8 * width + point4;
					redData16Dup[i] = redData16[newI16];
					greenData16Dup[i] = greenData16[newI16];
					blueData16Dup[i] = blueData16[newI16];
					continue;
				}

				// Calculate weights to scale data 
				weight1 = 1.0 / distance1;  weight2 = 1.0 / distance2;  weight3 = 1.0 / distance3;  weight4 = 1.0 / distance4;
				weight5 = 1.0 / distance5;  weight6 = 1.0 / distance6;  weight7 = 1.0 / distance7;  weight8 = 1.0 / distance8;
				weight9 = 1.0 / distance9;  weight10 = 1.0 / distance10;  weight11 = 1.0 / distance11;  weight12 = 1.0 / distance12;
				weight13 = 1.0 / distance13;  weight14 = 1.0 / distance14;  weight15 = 1.0 / distance15;  weight16 = 1.0 / distance16;

				weightSum = weight1 + weight2 + weight3 + weight4 + weight5 + weight6 + weight7 + weight8 +
					weight9 + weight10 + weight11 + weight12 + weight13 + weight14 + weight15 + weight16;

				// Get new single dimmension array index from new x and y location
				newI1 = point5 * width + point1;  newI2 = point5 * width + point2;  newI3 = point5 * width + point3;  newI4 = point5 * width + point4;
				newI5 = point6 * width + point1;  newI6 = point6 * width + point2;  newI7 = point6 * width + point3;  newI8 = point6 * width + point4;
				newI9 = point7 * width + point1;  newI10 = point7 * width + point2;  newI11 = point7 * width + point3;  newI12 = point7 * width + point4;
				newI13 = point8 * width + point1;  newI14 = point8 * width + point2;  newI15 = point8 * width + point3;  newI16 = point8 * width + point4;

				// Interpolate the data using weights calculated from distances above
				tempRed = ((weight1 * redData16[newI1]) + (weight2 * redData16[newI2]) + (weight3 * redData16[newI3]) + (weight4 * redData16[newI4]) +
					(weight5 * redData16[newI5]) + (weight6 * redData16[newI6]) + (weight7 * redData16[newI7]) + (weight8 * redData16[newI8]) +
					(weight9 * redData16[newI9]) + (weight10 * redData16[newI10]) + (weight11 * redData16[newI11]) + (weight12 * redData16[newI12]) +
					(weight13 * redData16[newI13]) + (weight14 * redData16[newI14]) + (weight15 * redData16[newI15]) + (weight16 * redData16[newI16])) / weightSum;

				tempGreen = ((weight1 * greenData16[newI1]) + (weight2 * greenData16[newI2]) + (weight3 * greenData16[newI3]) + (weight4 * greenData16[newI4]) +
					(weight5 * greenData16[newI5]) + (weight6 * greenData16[newI6]) + (weight7 * greenData16[newI7]) + (weight8 * greenData16[newI8]) +
					(weight9 * greenData16[newI9]) + (weight10 * greenData16[newI10]) + (weight11 * greenData16[newI11]) + (weight12 * greenData16[newI12]) +
					(weight13 * greenData16[newI13]) + (weight14 * greenData16[newI14]) + (weight15 * greenData16[newI15]) + (weight16 * greenData16[newI16])) / weightSum;

				tempBlue = ((weight1 * blueData16[newI1]) + (weight2 * blueData16[newI2]) + (weight3 * blueData16[newI3]) + (weight4 * blueData16[newI4]) +
					(weight5 * blueData16[newI5]) + (weight6 * blueData16[newI6]) + (weight7 * blueData16[newI7]) + (weight8 * blueData16[newI8]) +
					(weight9 * blueData16[newI9]) + (weight10 * blueData16[newI10]) + (weight11 * blueData16[newI11]) + (weight12 * blueData16[newI12]) +
					(weight13 * blueData16[newI13]) + (weight14 * blueData16[newI14]) + (weight15 * blueData16[newI15]) + (weight16 * blueData16[newI16])) / weightSum;

				// handle overflow or underflow
				tempRed = (tempRed > 65535) ? 65535 : tempRed;
				tempGreen = (tempGreen > 65535) ? 65535 : tempGreen;
				tempBlue = (tempBlue > 65535) ? 65535 : tempBlue;

				tempRed = (tempRed < 0) ? 0 : tempRed;
				tempGreen = (tempGreen < 0) ? 0 : tempGreen;
				tempBlue = (tempBlue < 0) ? 0 : tempBlue;

				// Set the new pixel to the 8 bit data
				redData16Dup[i] = (uint16_t)tempRed;
				greenData16Dup[i] = (uint16_t)tempGreen;
				blueData16Dup[i] = (uint16_t)tempBlue;
			}
		}
	}
}

void Processor::SetupCrop(double newWidth, double newHeight){

	// Create temp image with new scale size
	tempImage = new Image();
	tempImage->SetWidth((int)(newWidth * (double)img->GetWidth()));
	tempImage->SetHeight((int)(newHeight * (double)img->GetHeight()));
	if (this->GetImage()->GetColorDepth() == 16) { tempImage->Enable16Bit(); }
	tempImage->InitImage();
}

void Processor::CleanupCrop(){

	// Copy temp image to image
	delete img;
	img = new Image(*tempImage);

	// Cleanup temp image
	if(tempImage != NULL){
		delete tempImage;
		tempImage = NULL;
	}
}

void Processor::Crop(double startPointX, double startPointY, int dataStart, int dataEnd){

	// Get number of pixels for the image
	int newDataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = newDataSize;
	}

	int width = img->GetWidth();
	int height = img->GetHeight();
	int startX = (int)(startPointX * (double)width);
	int startY = (int)(startPointY * (double)height);

	int newWidth = tempImage->GetWidth();
	int newHeight = tempImage->GetHeight();

	int x = 0;
	int y = 0;

	int newI = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;
			
			// Veirfy pixel location is in bounds of new image size
			if (x >= startX && x < (newWidth + startX) && y >= startY && y < (newHeight + startY)) {

				// Get new single dimmension array index from new x and y location
				newI = (y - startY) * newWidth + (x-startX);

				// Copy pixel from old location to new location
				redData8Dup[newI] = redData8[i];
				greenData8Dup[newI] = greenData8[i];
				blueData8Dup[newI] = blueData8[i];
			}	
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// Get x and y coordinates from current index of data
			x = i % width;
			y = i / width;
			
			// Veirfy pixel location is in bounds of new image size
			if (x >= startX && x < (newWidth + startX) && y >= startY && y < (newHeight + startY)) {

				// Get new single dimmension array index from new x and y location
				newI = (y - startY) * newWidth + (x-startX);

				// Copy pixel from old location to new location
				redData16Dup[newI] = redData16[i];
				greenData16Dup[newI] = greenData16[i];
				blueData16Dup[newI] = blueData16[i];
			}		
		}
	}
}

void Processor::MirrorHorizontal(int dataStart, int dataEnd) {

	int width = img->GetWidth();

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	int x = 0;
	int y = 0;

	int widthOver2 = width / 2;

	int newI = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;
			newI = (y * width) + (width - x);

			if (x < widthOver2 && newI < dataSize) {

				tempRed = redData8[i];
				redData8[i] = redData8[newI];
				redData8[newI] = tempRed;

				tempGreen = greenData8[i];
				greenData8[i] = greenData8[newI];
				greenData8[newI] = tempGreen;

				tempBlue = blueData8[i];
				blueData8[i] = blueData8[newI];
				blueData8[newI] = tempBlue;
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;
			newI = (y * width) + (width - x);

			if (x < widthOver2  && newI < dataSize) {

				tempRed = redData16[i];
				redData16[i] = redData16[newI];
				redData16[newI] = tempRed;

				tempGreen = greenData16[i];
				greenData16[i] = greenData16[newI];
				greenData16[newI] = tempGreen;

				tempBlue = blueData16[i];
				blueData16[i] = blueData16[newI];
				blueData16[newI] = tempBlue;
			}
		}
	}
}

void Processor::MirrorVertical(int dataStart, int dataEnd) {

	int width = img->GetWidth();
	int height = img->GetHeight();

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();
	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int32_t tempRed;
	int32_t tempGreen;
	int32_t tempBlue;

	int x = 0;
	int y = 0;

	int heightOver2 = height / 2;

	int newI = 0;

	// Process 8 bit data
	if(img->GetColorDepth() == 8){

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;
			newI = (width *(height - y)) + x;

			if (y < heightOver2  && newI < dataSize) {

				tempRed = redData8[i];
				redData8[i] = redData8[newI];
				redData8[newI] = tempRed;

				tempGreen = greenData8[i];
				greenData8[i] = greenData8[newI];
				greenData8[newI] = tempGreen;

				tempBlue = blueData8[i];
				blueData8[i] = blueData8[newI];
				blueData8[newI] = tempBlue;
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;
			newI = (width *(height - y)) + x;

			if (y < heightOver2 && newI < dataSize) {

				tempRed = redData16[i];
				redData16[i] = redData16[newI];
				redData16[newI] = tempRed;

				tempGreen = greenData16[i];
				greenData16[i] = greenData16[newI];
				greenData16[newI] = tempGreen;

				tempBlue = blueData16[i];
				blueData16[i] = blueData16[newI];
				blueData16[newI] = tempBlue;
			}
		}
	}
}

void Processor::SetupBlur(){

	// Create temp image with same size as current image
	tempImage = new Image();
	tempImage->SetWidth(img->GetWidth());
	tempImage->SetHeight(img->GetHeight());
	if (this->GetImage()->GetColorDepth() == 16) { tempImage->Enable16Bit(); }
	tempImage->InitImage();
}

void Processor::CleanupBlur(){
	// Copy temp image to image
	delete img;
	img = new Image(*tempImage);

	// Cleanup temp image
	if(tempImage != NULL){
		delete tempImage;
		tempImage = NULL;
	}
}

double Processor::CalculateBlurSize(double inputBlurSize){

	if(img->GetWidth() > img->GetHeight()){
		return inputBlurSize * (double)img->GetWidth();
	}
	else{
		return inputBlurSize * (double)img->GetHeight();
	}
}
void Processor::BoxBlurHorizontal(double blurSize, int dataStart, int dataEnd) {

	int width = img->GetWidth();

	int pixelBlurSize = blurSize;
	
	if(pixelBlurSize > img->GetWidth()/2.0){
		pixelBlurSize = img->GetWidth()/2.0;
	}
	if(pixelBlurSize < 1){
		pixelBlurSize = 1;
	}

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();

	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int64_t tempRed = 0;
	int64_t tempGreen = 0;
	int64_t tempBlue = 0;
	int64_t tempRedScale = 0;
	int64_t tempGreenScale = 0;
	int64_t tempBlueScale = 0;

	int x = 0;
	int y = 0;
	int idx = 0;

	int xPlus = 0;
	int xMin = 0;
	int iMapPlus = 0;
	int iMapMin = 0;
	double scalar = 1.0/((2.0 * (pixelBlurSize))+1.0);

	// Process 8 bit data
	if (img->GetColorDepth() == 8) {

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;

			// First pixel in row
			if(x == 0){

				tempRed = 0;
				tempGreen = 0;
				tempBlue = 0;

				for(int j = 0; j < (pixelBlurSize * 2) + 1; j++){

					idx = (width * y) + j;
					tempRed += redData8[idx];
					tempGreen += greenData8[idx];
					tempBlue += blueData8[idx];
				}
			}

			// Standard box blur
			else{
				xPlus = x + pixelBlurSize + 1;
				xMin = x - pixelBlurSize;

				if(xPlus <= width - 1 && xMin >= 0){

					iMapPlus = (y * width) + xPlus;
					iMapMin = (y * width) + xMin;
					scalar = 1.0 / ((2.0 * pixelBlurSize) + 1.0);

					tempRed += redData8[iMapPlus];
					tempRed -= redData8[iMapMin];
					tempGreen+= greenData8[iMapPlus];
					tempGreen -= greenData8[iMapMin];
					tempBlue += blueData8[iMapPlus];
					tempBlue -= blueData8[iMapMin];
				}
			}
						
			// Scale values
			tempRedScale = tempRed * scalar;
			tempGreenScale = tempGreen * scalar;
			tempBlueScale = tempBlue * scalar;

			// handle overflow or underflow
			tempRedScale = (tempRedScale > 255) ? 255 : tempRedScale;
			tempGreenScale = (tempGreenScale > 255) ? 255 : tempGreenScale;
			tempBlueScale = (tempBlueScale  > 255) ? 255 : tempBlueScale;

			tempRedScale = (tempRedScale  < 0) ? 0 : tempRedScale;
			tempGreenScale = (tempGreenScale  < 0) ? 0 : tempGreenScale;
			tempBlueScale = (tempBlueScale  < 0) ? 0 : tempBlueScale;
				
			redData8Dup[i] = tempRedScale;
			greenData8Dup[i] = tempGreenScale;
			blueData8Dup[i] = tempBlueScale;
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			x = i % width;
			y = i / width;

			// First pixel in row
			if(x == 0){

				tempRed = 0;
				tempGreen = 0;
				tempBlue = 0;

				for(int j = 0; j < (pixelBlurSize * 2) + 1; j++){

					idx = (width * y) + j;
					tempRed += redData16[idx];
					tempGreen += greenData16[idx];
					tempBlue += blueData16[idx];
				}
			}

			// Standard box blur
			else{
				xPlus = x + pixelBlurSize + 1;
				xMin = x - pixelBlurSize;

				if(xPlus < width && xMin >= 0){

					iMapPlus = (y * width) + xPlus;
					iMapMin = (y * width) + xMin;

					tempRed += redData16[iMapPlus];
					tempRed -= redData16[iMapMin];
					tempGreen+= greenData16[iMapPlus];
					tempGreen -= greenData16[iMapMin];
					tempBlue += blueData16[iMapPlus];
					tempBlue -= blueData16[iMapMin];
				}
			}

			// Scale values
			tempRedScale = tempRed * scalar;
			tempGreenScale = tempGreen * scalar;
			tempBlueScale = tempBlue* scalar;
			
			// handle overflow or underflow
			tempRedScale = (tempRedScale > 65535) ? 65535 : tempRedScale;
			tempGreenScale = (tempGreenScale  > 65535) ? 65535 : tempGreenScale;
			tempBlueScale = (tempBlueScale  > 65535) ? 65535 : tempBlueScale;

			tempRedScale = (tempRedScale  < 0) ? 0 : tempRedScale;
			tempGreenScale = (tempGreenScale  < 0) ? 0 : tempGreenScale;
			tempBlueScale = (tempBlueScale  < 0) ? 0 : tempBlueScale;
		
			redData16Dup[i] = tempRedScale;
			greenData16Dup[i] = tempGreenScale;
			blueData16Dup[i] = tempBlueScale;
			
		}
	}
}

void Processor::BoxBlurVertical(double blurSize, int dataStart, int dataEnd) {

	int width = img->GetWidth();
	int height = img->GetHeight();

	int pixelBlurSize = blurSize;
	if(pixelBlurSize > img->GetHeight()){
		pixelBlurSize = img->GetHeight();
	}
	if(pixelBlurSize < 1){
		pixelBlurSize = 1;
	}

	// Get number of pixels for the image
	int dataSize = img->GetWidth() * img->GetHeight();

	if (dataStart < 0 || dataEnd < 0) {
		dataStart = 0;
		dataEnd = dataSize;
	}

	int64_t tempRed = 0;
	int64_t tempGreen = 0;
	int64_t tempBlue = 0;
	int64_t tempRedScale = 0;
	int64_t tempGreenScale = 0;
	int64_t tempBlueScale = 0;

	int x = dataStart;
	int y = 0;

	int idx = 0;
	int iMapPlus = 0;
	int iMapMin = 0;
	int yPlus = 0;
	int yMin = 0;
	double scalar = 1.0/((2.0 * pixelBlurSize)+1.0);

	// Process 8 bit data
	if (img->GetColorDepth() == 8) {

		// Get pointers to 8 bit data
		uint8_t * redData8 = img->Get8BitDataRed();
		uint8_t * greenData8 = img->Get8BitDataGreen();
		uint8_t * blueData8 = img->Get8BitDataBlue();

		uint8_t * redData8Dup = tempImage->Get8BitDataRed();
		uint8_t * greenData8Dup = tempImage->Get8BitDataGreen();
		uint8_t * blueData8Dup = tempImage->Get8BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// First pixel in column
			if(y == 0){

				tempRed = 0;
				tempGreen = 0;
				tempBlue = 0;

				// Blur first pixel in column
				for(int j = 0; j < pixelBlurSize * 2 + 1; j++){

					idx = (width * j) + x;
					tempRed += redData8[idx];
					tempGreen += greenData8[idx];
					tempBlue += blueData8[idx];
				}
			}

			else{
				yPlus = y + pixelBlurSize + 1;
				yMin = y - pixelBlurSize;

				if(yPlus <= height - 1 && yMin >= 0){

					iMapPlus = (yPlus * width) + x;
					iMapMin = (yMin * width) + x;

					tempRed += redData8[iMapPlus];
					tempRed -= redData8[iMapMin];
					tempGreen += greenData8[iMapPlus];
					tempGreen -= greenData8[iMapMin];
					tempBlue += blueData8[iMapPlus];
					tempBlue -= blueData8[iMapMin];
				}
			}

			// Scale values
			tempRedScale = tempRed * scalar;
			tempGreenScale = tempGreen * scalar;
			tempBlueScale = tempBlue* scalar;
		
			// handle overflow or underflow
			tempRedScale = (tempRedScale > 255) ? 255 : tempRedScale;
			tempGreenScale = (tempGreenScale > 255) ? 255 : tempGreenScale;
			tempBlueScale = (tempBlueScale  > 255) ? 255 : tempBlueScale;

			tempRedScale = (tempRedScale  < 0) ? 0 : tempRedScale;
			tempGreenScale = (tempGreenScale  < 0) ? 0 : tempGreenScale;
			tempBlueScale = (tempBlueScale  < 0) ? 0 : tempBlueScale;

			idx = (width * y) + x;
			redData8Dup[idx] = tempRedScale;
			greenData8Dup[idx] = tempGreenScale;
			blueData8Dup[idx] = tempBlueScale;

			y += 1;
			if(y % height == 0){
				y = 0;
				x += 1;
			}
		}
	}

	// Process 16 bit data
	else {

		// Get pointers to 16 bit data
		uint16_t * redData16 = img->Get16BitDataRed();
		uint16_t * greenData16 = img->Get16BitDataGreen();
		uint16_t * blueData16 = img->Get16BitDataBlue();

		uint16_t * redData16Dup = tempImage->Get16BitDataRed();
		uint16_t * greenData16Dup = tempImage->Get16BitDataGreen();
		uint16_t * blueData16Dup = tempImage->Get16BitDataBlue();

		for (int i = dataStart; i < dataEnd; i++) {

			if(forceStop) { return; }

			// First pixel in column
			if(y == 0){

				tempRed = 0;
				tempGreen = 0;
				tempBlue = 0;

				// Blur first pixel in column
				for(int j = 0; j < pixelBlurSize * 2 + 1; j++){

					idx = (width * j) + x;
					tempRed += redData16[idx];
					tempGreen += greenData16[idx];
					tempBlue += blueData16[idx];
				}
			}

			else{
				yPlus = y + pixelBlurSize + 1;
				yMin = y - pixelBlurSize;

				if(yPlus < height && yMin >= 0){
			
					iMapPlus = (yPlus * width) + x;
					iMapMin = (yMin * width) + x;

					tempRed += redData16[iMapPlus];
					tempRed -= redData16[iMapMin];
					tempGreen += greenData16[iMapPlus];
					tempGreen -= greenData16[iMapMin];
					tempBlue += blueData16[iMapPlus];
					tempBlue -= blueData16[iMapMin];
				}
			}

			// Scale values
			tempRedScale = tempRed * scalar;
			tempGreenScale = tempGreen * scalar;
			tempBlueScale = tempBlue* scalar;
		
			// handle overflow or underflow
			tempRedScale = (tempRedScale > 65535) ? 65535 : tempRedScale;
			tempGreenScale = (tempGreenScale > 65535) ? 65535 : tempGreenScale;
			tempBlueScale = (tempBlueScale  > 65535) ? 65535 : tempBlueScale;

			tempRedScale = (tempRedScale  < 0) ? 0 : tempRedScale;
			tempGreenScale = (tempGreenScale  < 0) ? 0 : tempGreenScale;
			tempBlueScale = (tempBlueScale  < 0) ? 0 : tempBlueScale;

			idx = (width * y) + x;
			redData16Dup[idx] = tempRedScale;
			greenData16Dup[idx] = tempGreenScale;
			blueData16Dup[idx] = tempBlueScale;

			y += 1;
			if(y % height == 0){
				y = 0;
				x += 1;
			}
		}
	}
}

void Processor::RGBtoXYZ(RGB * rgb, XYZ * xyz, int colorSpaceToUse) {

	float tempR = rgb->R;
	float tempG = rgb->G;
	float tempB = rgb->B;

	if (tempR > 0.04045f) {
		tempR = pow(((tempR + 0.055f) / 1.055f), 2.4f);
	}
	else {
		tempR = tempR / 12.92f;
	}
	if (tempG > 0.04045f) {
		tempG = pow(((tempG + 0.055f) / 1.055f), 2.4f);
	}
	else {
		tempG = tempG / 12.92f;
	}
	if (tempB > 0.04045f) {
		tempB = pow(((tempB + 0.055f) / 1.055f), 2.4f);
	}
	else {
		tempB = tempB / 12.92f;
	}

	tempR *= 100.0f;
	tempG *= 100.0f;
	tempB *= 100.0f;

	switch (colorSpaceToUse) {

	case ADOBE_RGB:
		xyz->X = (tempR * 0.5767309f) + (tempG * 0.1855540f) + (tempB * 0.1881852f);
		xyz->Y = (tempR * 0.2973769f) + (tempG * 0.6273491f) + (tempB * 0.0752741f);
		xyz->Z = (tempR * 0.0270343f) + (tempG * 0.0706872f) + (tempB * 0.9911085f);
		break;

	case PROPHOTO_RGB:
		xyz->X = (tempR * 0.7976749f) + (tempG * 0.1351917f) + (tempB * 0.0313534f);
		xyz->Y = (tempR * 0.2880402f) + (tempG * 0.7118741f) + (tempB * 0.0000857f);
		xyz->Z = (tempR * 0.0000000f) + (tempG * 0.0000000f) + (tempB * 0.8252100f);
		break;

	case sRGB:
		xyz->X = (tempR * 0.4124564f) + (tempG * 0.3575761f) + (tempB * 0.1804375f);
		xyz->Y = (tempR * 0.2126729f) + (tempG * 0.7151522f) + (tempB * 0.0721750f);
		xyz->Z = (tempR * 0.0193339f) + (tempG * 0.1191920f) + (tempB * 0.9503041f);
		break;

	case WIDE_GAMUT_RGB:
		xyz->X = (tempR * 0.7161046f) + (tempG * 0.1009296f) + (tempB * 0.1471858f);
		xyz->Y = (tempR * 0.2581874f) + (tempG * 0.7249378f) + (tempB * 0.0168748f);
		xyz->Z = (tempR * 0.0000000f) + (tempG * 0.0517813f) + (tempB * 0.7734287f);
		break;
	}
}

void Processor::XYZtoRGB(XYZ * xyz, RGB * rgb, int colorSpaceToUse) {

	float tempX = (float)xyz->X / 100.0f;
	float tempY = (float)xyz->Y / 100.0f;
	float tempZ = (float)xyz->Z / 100.0f;

	float tempR = 0.0f;
	float tempG = 0.0f;
	float tempB = 0.0f;

	switch (colorSpaceToUse) {

	case ADOBE_RGB:
		tempR = (tempX *  2.0413690f) + (tempY * -0.5649464f) + (tempZ * -0.3446944f);
		tempG = (tempX * -0.9692660f) + (tempY *  1.8760108f) + (tempZ *  0.0415560f);
		tempB = (tempX *  0.0134474f) + (tempY * -0.1183897f) + (tempZ *  1.0154096f);
		break;

	case PROPHOTO_RGB:
		tempR = (tempX *  1.3459433f) + (tempY * -0.2556075f) + (tempZ * -0.0511118f);
		tempG = (tempX * -0.5445989f) + (tempY *  1.5081673f) + (tempZ *  0.0205351f);
		tempB = (tempX *  0.0000000f) + (tempY *  0.0000000f) + (tempZ *  1.2118128f);
		break;

	case sRGB:
		tempR = (tempX *  3.2404542f) + (tempY * -1.5371385f) + (tempZ * -0.4985314f);
		tempG = (tempX * -0.9692660f) + (tempY *  1.8760108f) + (tempZ *  0.0415560f);
		tempB = (tempX *  0.0556434f) + (tempY * -0.2040259f) + (tempZ *  1.0572252f);
		break;

	case WIDE_GAMUT_RGB:
		tempR = (tempX *  1.4628067f) + (tempY * -0.1840623f) + (tempZ * -0.2743606f);
		tempG = (tempX * -0.5217933f) + (tempY *  1.4472381f) + (tempZ *  0.0677227f);
		tempB = (tempX *  0.0349342f) + (tempY * -0.0968930f) + (tempZ *  1.2884099f);
		break;
	}

	if (tempR > 0.0031308f) {
		tempR = 1.055f * pow(tempR, 1.0f / 2.4f) - 0.055f;
	}
	else {
		tempR = 12.92f * tempR;
	}
	if (tempG > 0.0031308f) {
		tempG = 1.055f * pow(tempG, 1.0f / 2.4f) - 0.055f;
	}
	else {
		tempG = 12.92f * tempG;
	}
	if (tempB > 0.0031308f) {
		tempB = 1.055f * pow(tempB, 1.0f / 2.4f) - 0.055f;
	}
	else {
		tempB = 12.92f * tempB;
	}

	rgb->R = tempR;
	rgb->G = tempG;
	rgb->B = tempB;
}

void Processor::XYZtoLAB(XYZ * xyz, LAB * lab) {

	float tempX = xyz->X / 95.047f;
	float tempY = xyz->Y / 100.000f;
	float tempZ = xyz->Z / 108.883f;

	if (tempX > 0.008856f) {
		tempX = pow(tempX, 1.0f / 3.0f);
	}
	else {
		tempX = (7.787f * tempX) + (16.0f / 116.0f);
	}
	if (tempY  > 0.008856f) {
		tempY = pow(tempY, 1.0f / 3.0f);
	}
	else {
		tempY = (7.787f * tempY) + (16.0f / 116.0f);
	}
	if (tempZ > 0.008856f) {
		tempZ = pow(tempZ, 1.0f / 3.0f);
	}
	else {
		tempZ = (7.787f * tempZ) + (16.0f / 116.0f);
	}

	lab->L = (116.0f * tempY) - 16.0f;
	lab->A = 500.0f  * (tempX - tempY);
	lab->B = 200.0f  * (tempY - tempZ);

}

void Processor::LABtoXYZ(LAB * lab, XYZ * xyz) {

	float tempY = (lab->L + 16.0f) / 116.0f;
	float tempX = (lab->A / 500.0f) + tempY;
	float tempZ = tempY - (lab->B / 200.0F);

	if (tempY*tempY*tempY > 0.008856) {
		tempY = tempY*tempY*tempY;
	}
	else {
		tempY = (tempY - (16.0f / 116.0f)) / 7.787f;
	}
	if (tempX*tempX*tempX > 0.008856) {
		tempX = tempX*tempX*tempX;
	}
	else {
		tempX = (tempX - (16.0f / 116.0f)) / 7.787f;
	}
	if (tempZ*tempZ*tempZ > 0.008856) {
		tempZ = tempZ*tempZ*tempZ;
	}
	else {
		tempZ = (tempZ - (16.0f / 116.0f)) / 7.787f;
	}

	xyz->X = tempX * 95.047f;
	xyz->Y = tempY * 100.0f;
	xyz->Z = tempZ * 108.883f;
}

void Processor::CalculateWidthHeightRotation(ProcessorEdit * rotationEdit, int origWidth, int origHeight, int * width, int * height) {

	// No rotation and 180 degree rotation have same width and height as original image
	if (rotationEdit->GetEditType()== ProcessorEdit::EditType::ROTATE_180 || rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_NONE) {
		*width = origWidth;
		*height = origHeight;
	}

	// 90 degree and 270 degree rotation cause width and height to flip
	else if (rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_90_CW || rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_270_CW) {
		*width = origHeight;
		*height = origWidth;
	}

	// Calculate width and height based on angle, and crop flag
	else if (rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC ||
		rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR ||
		rotationEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST) {

		if (!rotationEdit->CheckForParameter(PHOEDIX_PARAMETER_ROTATE_ANGLE) || !rotationEdit->CheckForFlag(PHOEDIX_FLAG_ROTATE_CROP)) { return; }
		double angleDegrees = rotationEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
		int cropFlag = rotationEdit->GetFlag(PHOEDIX_FLAG_ROTATE_CROP);

		// Set width and height to maximum size needed to fit whole image in rotation (with black surrounding borders)
		if (cropFlag == Processor::RotationCropping::EXPAND) {
			*width = this->GetExpandedRotationWidth(angleDegrees, origWidth, origHeight);
			*height = this->GetExpandedRotationHeight(angleDegrees, origWidth, origHeight);
		}

		// Set width and height to minumum size needed to fit image with no border
		else if (cropFlag == Processor::RotationCropping::FIT) {
			*width = this->GetFittedRotationWidth(angleDegrees, origWidth, origHeight);
			*height = this->GetFittedRotationHeight(angleDegrees, origWidth, origHeight);
		}

		// Rotated image is same size as current image
		else {
			*width = origWidth;
			*height = origHeight;
		}
	}
}

void Processor::CalcualteWidthHeightEdits(wxVector<ProcessorEdit*> edits, int * width, int * height) {

	// Start with original image dimmensions
	*width = this->GetOriginalImage()->GetWidth();
	*height = this->GetOriginalImage()->GetHeight();

	ProcessorEdit * curEdit;

	for (size_t i = 0; i < edits.size(); i++) {
		curEdit = edits.at(i);

		if (curEdit->GetEditType() == ProcessorEdit::EditType::SCALE_NEAREST ||
			curEdit->GetEditType() == ProcessorEdit::EditType::SCALE_BILINEAR ||
			curEdit->GetEditType() == ProcessorEdit::EditType::SCALE_BICUBIC) {

			// Set width and height to scale width and height
				if (curEdit->CheckForParameter(PHOEDIX_PARAMETER_SCALE_WIDTH) && curEdit->CheckForParameter(PHOEDIX_PARAMETER_SCALE_HEIGHT) ) {
				*width = (int)curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_WIDTH);
				*height = (int)curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_HEIGHT);
			}
		}

		if (curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_90_CW ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_180 ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_270_CW ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_NONE ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR ||
			curEdit->GetEditType() == ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC) {

			this->CalculateWidthHeightRotation(curEdit, *width, *height, width, height);

		}

		if (curEdit->GetEditType() == ProcessorEdit::EditType::CROP) {
			
			// Set width and height to crop width and height
			if (curEdit->CheckForParameter(PHOEDIX_PARAMETER_CROP_WIDTH) && curEdit->CheckForParameter(PHOEDIX_PARAMETER_CROP_WIDTH)) {

				// Params 0 and 1 are x and y start points
				*width = (int)(curEdit->GetParam(PHOEDIX_PARAMETER_CROP_WIDTH) * (double)*width);
				*height = (int)(curEdit->GetParam(PHOEDIX_PARAMETER_CROP_WIDTH) * (double)*height);
			}			
		}
	}
}

Processor::ProcessThread::ProcessThread(Processor * processorPar, ProcessorEdit * edit) : wxThread(wxTHREAD_DETACHED) {
	processor = processorPar;
	editVec = wxVector<ProcessorEdit*>();
	editVec.clear();
	editVec.push_back(edit);
	terminated = false;
}

Processor::ProcessThread::ProcessThread(Processor * processorPar, wxVector<ProcessorEdit*> edits) : wxThread(wxTHREAD_DETACHED) {
	processor = processorPar;
	
	editVec = wxVector<ProcessorEdit*>();
	editVec.clear();

	// Copy edit vector to threads edit vector
	for (size_t i = 0; i < edits.size(); i++) {
		editVec.push_back(new ProcessorEdit(*edits.at(i)));
	}

	terminated = false;
}

void Processor::ProcessThread::Terminate(){
	terminated = true;
	this->DeleteEditVector();
}

void Processor::ProcessThread::DeleteEditVector() {

	// Clean up copied edits
	for (size_t i = 0; i < editVec.size(); i++) {
		editVec.at(i)->ClearIntArray();
		editVec.at(i)->ClearDoubleArray();
		delete editVec.at(i);
	}
	editVec.clear();
}

void Processor::ProcessThread::Multithread(ProcessorEdit * edit, int maxDataSize) {

	int numThreads = processor->GetNumThreads();
	int dataSize = 0;
	if (maxDataSize > -1) {
		dataSize = maxDataSize;
	}
	else {
		dataSize = processor->GetImage()->GetWidth() * processor->GetImage()->GetHeight();
	}

	int chunkSize = dataSize / numThreads;
	wxMutex mutexLock;
	wxCondition wait(mutexLock);
	int threadComplete = 0;

	mutexLock.Lock();

	// Horizontal blur must have data seperated into full rows
	if(edit->GetEditType()== ProcessorEdit::HORIZONTAL_BLUR){

		if(chunkSize % processor->GetImage()->GetWidth() != 0){

			for(int i = 0; i < processor->GetImage()->GetWidth(); i++){

				chunkSize += 1;
				if(chunkSize % processor->GetImage()->GetWidth() == 0){ break;}
			}
		}
	}

	// Vertical blur must have data seperated into full columns
	if(edit->GetEditType()== ProcessorEdit::VERTICAL_BLUR){

		// Get Chunk Size
		chunkSize = (processor->GetImage()->GetWidth() / numThreads) * processor->GetImage()->GetHeight();

		int chunkStart = 0;
		int dataRemain = processor->GetImage()->GetWidth() * processor->GetImage()->GetHeight();
		for (int thread = 0; thread < numThreads; thread++) {

			if (thread != numThreads - 1) {
				EditThread * editWorker = new EditThread(processor, edit, chunkStart, chunkSize + chunkStart, &mutexLock, &wait, numThreads, &threadComplete);
				editWorker->Run();
				chunkStart += (processor->GetImage()->GetWidth() / numThreads);
				dataRemain -= chunkSize;
			}

			// Go all the way to end of data (incase of rounding error)
			else {
				EditThread * editWorker = new EditThread(processor, edit, chunkStart, dataRemain + chunkStart, &mutexLock, &wait, numThreads, &threadComplete);
				editWorker->Run();
			}
		}
	}

	// Multithreaded chunks split into horizontal rows (almost all edits except vertical blur).
	else{
		for (int thread = 0; thread < numThreads; thread++) {

			// Process current chunk of data
			if (thread != numThreads - 1) {
				EditThread * editWorker = new EditThread(processor, edit, chunkSize * thread, chunkSize * (thread + 1), &mutexLock, &wait, numThreads, &threadComplete);
				editWorker->Run();
			}

			// Go all the way to end of data (incase of rounding error)
			else {
				EditThread * editWorker = new EditThread(processor, edit, chunkSize * thread, dataSize, &mutexLock, &wait, numThreads, &threadComplete);
				editWorker->Run();
			}
		}
	}
	// Wait for all worker threads to complete
	wait.Wait();
	mutexLock.Unlock();
	processor->SetUpdated(true);
}

wxThread::ExitCode Processor::ProcessThread::Entry() {
	
	// Wait for raw image to process (must process raw before image data)
	while (processor->GetLockedRaw()) {
		this->Sleep(20);
	}

	if (processor->GetLocked()) {
		this->DeleteEditVector();
		return 0;
	}

	// Immediatly return if there is no vaild image
	if(processor->GetImage() == NULL){
		this->DeleteEditVector();
		processor->forceStopCritical.Enter();
		processor->forceStop = false;
		processor->forceStopCritical.Leave();
		return 0;
	}
	if(processor->GetImage()->GetWidth() < 1 || processor->GetImage()->GetHeight() < 1){
		this->DeleteEditVector();
		processor->forceStopCritical.Enter();
		processor->forceStop = false;
		processor->forceStopCritical.Leave();
		return 0;
	}


	if(processor->forceStop){
		this->DeleteEditVector();
		processor->forceStopCritical.Enter();
		processor->forceStop = false;
		processor->forceStopCritical.Leave();
		return 0;
	}

	// Lock and revert to original image, so we have a clean slate
	processor->Lock();
	processor->RevertToOriginalImage(true);

	// Get number of edits and set to a string to display in UI
	size_t numberOfEdits = editVec.size();

	int subtractEdits = 0;
	if (editVec.size() > 0) {
		if (editVec.at(0)->GetEditType() == ProcessorEdit::EditType::RAW) {
			numberOfEdits -= 1;
			subtractEdits = 1;
		}
	}

	wxString numEditsStr;
	numEditsStr << numberOfEdits;

	// FAST EDIT - Scale image to half size
	int firstEditType = ProcessorEdit::EditType::UNDEFINED;
	if(editVec.size() > 0){ firstEditType = editVec.at(0)->GetEditType(); }

	if(processor->GetFastEdit() && firstEditType != ProcessorEdit::EditType::RAW){
		processor->SendMessageToParent("Scaling image for fast processing");

		// Perform an edit on the data through the processor
		int halfWidth = processor->GetImage()->GetWidth() / 2.0;
		int halfHeight = processor->GetImage()->GetHeight() / 2.0;
				
		if(halfWidth > 0 && halfHeight > 0.0){
			
			ProcessorEdit halfSizeEdit(ProcessorEdit::EditType::SCALE_NEAREST);
			halfSizeEdit.AddParam(PHOEDIX_PARAMETER_SCALE_WIDTH, halfWidth);
			halfSizeEdit.AddParam(PHOEDIX_PARAMETER_SCALE_HEIGHT, halfHeight);

			// Multithread if needed
			if(processor->GetMultithread()){
				if (processor->SetupScale(halfWidth, halfHeight)){
					int dataSize = halfWidth * halfHeight;
					this->Multithread(&halfSizeEdit, dataSize);
					processor->CleanupScale();
				}
			}

			// Single thread
			else{
				// Perform an edit on the data through the processor
				if (processor->SetupScale(halfWidth, halfHeight)){
					processor->ScaleNearest();
					processor->CleanupScale();
				}
			}
		}
		processor->SetUpdated(true);
	}
	
	// Go through each edit one by one.  Each of these will invoke at least 1 child thread for the edit itself
	for (size_t editIndex = 0; editIndex < editVec.size(); editIndex++) {
		
		if(processor->forceStop){
			this->DeleteEditVector();
			processor->forceStopCritical.Enter();
			processor->forceStop = false;
			processor->forceStopCritical.Leave();
			processor->SendMessageToParent("");
			processor->Unlock();
			return 0;
		}

		// Get the next edit to take place
		ProcessorEdit * curEdit = editVec.at(editIndex);
		if (curEdit == NULL) {
			continue;
		}
		// Get the type of edit to perform
		int editToComplete = curEdit->GetEditType();
		
		// Skip disabled edits
		if (curEdit->GetDisabled()) {
			processor->SetUpdated(true);
			continue;
		}

		wxString curEditStr;
		curEditStr << (editIndex + 1 - subtractEdits);

		wxString fullEditNumStr = " (" + curEditStr + "/" + numEditsStr + ")";
		
		switch (editToComplete) {

			case ProcessorEdit::EditType::RAW: {
				processor->SetUpdated(true);
				processor->SendProcessorEditNumToParent(editIndex + 1);
				continue;
			}

			// Peform a Brightness Adjustment edit
			case ProcessorEdit::EditType::ADJUST_BRIGHTNESS: {

				processor->SendMessageToParent("Processing Adjust Brightness Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double brighnessAmount = curEdit->GetParam(PHOEDIX_PARAMETER_BRIGHTNESS);
				double detailsPreservation = curEdit->GetParam(PHOEDIX_PARAMETER_PRESERVATION);
				double toneSetting = curEdit->GetParam(PHOEDIX_PARAMETER_TONE);

				int toneFlag = curEdit->GetFlag(PHOEDIX_FLAG_PRESERVATION_TYPE);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->AdjustBrightness(brighnessAmount, detailsPreservation, toneSetting, toneFlag);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform an HSL adjustment
			case ProcessorEdit::EditType::ADJUST_HSL: {

				processor->SendMessageToParent("Processing Adjust HSL Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double hueShift = curEdit->GetParam(PHOEDIX_PARAMETER_HUE);
				double saturationScale = curEdit->GetParam(PHOEDIX_PARAMETER_SATURATION);
				double luminaceScale = curEdit->GetParam(PHOEDIX_PARAMETER_LUMINACE);
				double rScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
				double gScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
				double bScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->AdjustHSL(hueShift, saturationScale, luminaceScale, rScale, gScale, bScale);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a LAB adjustment
			case ProcessorEdit::EditType::ADJUST_LAB: {

				processor->SendMessageToParent("Processing Adjust LAB Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double luminaceScale = curEdit->GetParam(PHOEDIX_PARAMETER_LUMINACE);
				double aShift = curEdit->GetParam(PHOEDIX_PARAMETER_A);
				double bShift = curEdit->GetParam(PHOEDIX_PARAMETER_B);
				double rScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
				double gScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
				double bScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

				// Multithread if needed
				if (processor->GetMultithread()) {
					this->Multithread(curEdit);
				}

				// Single thread
				else {
					// Perform an edit on the data through the processor
					processor->AdjustLAB(luminaceScale, aShift, bShift, rScale, gScale, bScale);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a Shift RGB edit
			case ProcessorEdit::EditType::ADJUST_RGB: {

				processor->SendMessageToParent("Processing Shift RGB Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double allBrightShift = curEdit->GetParam(PHOEDIX_PARAMETER_ALL);
				double redBrightShift = curEdit->GetParam(PHOEDIX_PARAMETER_RED);
				double greenBrightShift = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN);
				double blueBrightShift = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE);

				// Multithread if needed
				if (processor->GetMultithread()) {
					this->Multithread(curEdit);
				}

				// Single thread
				else {
					// Perform an edit on the data through the processor
					processor->AdjustRGB(allBrightShift, redBrightShift, greenBrightShift, blueBrightShift);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform Blur edit
			case ProcessorEdit::EditType::BLUR: {

				processor->SendMessageToParent("Processing Blur" + fullEditNumStr);

				double blurSize = curEdit->GetParam(PHOEDIX_PARAMETER_BLURSIZE);
				int numPasses = (int)curEdit->GetParam(PHOEDIX_PARAMETER_NUM_PASSES);

				// No blur, radius too small
				if (blurSize == 0.0) { break; }

				// Multithread if needed
				if (processor->GetMultithread()) {
					for (int i = 0; i < numPasses; i++) {

						// Setup temporary edits to break down blur into horizontal and vertical blur edits
						ProcessorEdit * horizontalBlur = new ProcessorEdit(ProcessorEdit::HORIZONTAL_BLUR);
						ProcessorEdit * verticalBlur = new ProcessorEdit(ProcessorEdit::VERTICAL_BLUR);
						horizontalBlur->AddParam(PHOEDIX_PARAMETER_BLURSIZE, blurSize);
						verticalBlur->AddParam(PHOEDIX_PARAMETER_BLURSIZE, blurSize);

						// Blur horiztonally 
						processor->SetupBlur();
						this->Multithread(horizontalBlur);
						processor->CleanupBlur();
						if (processor->forceStop) { break; }

						// Blur vertically
						processor->SetupBlur();
						this->Multithread(verticalBlur);
						processor->CleanupBlur();
						if (processor->forceStop) { break; }

						delete horizontalBlur;
						delete verticalBlur;
					}
				}

				// Single thread
				else {

					// Perform an edit on the data through the processor

					for (int i = 0; i < numPasses; i++) {

						// Blur horizontally
						processor->SetupBlur();
						processor->BoxBlurHorizontal(processor->CalculateBlurSize(blurSize));
						processor->CleanupBlur();
						if (processor->forceStop) { break; }

						// Blur vertically
						processor->SetupBlur();
						processor->BoxBlurVertical(processor->CalculateBlurSize(blurSize));
						processor->CleanupBlur();
						if (processor->forceStop) { break; }
					}
				}
			}
			break;

			// Peform Horizontal Blur edit
			case ProcessorEdit::EditType::HORIZONTAL_BLUR: {

				processor->SendMessageToParent("Processing Horizontal Blur" + fullEditNumStr);

				double blurSize = curEdit->GetParam(PHOEDIX_PARAMETER_BLURSIZE);
				int numPasses = (int)curEdit->GetParam(PHOEDIX_PARAMETER_NUM_PASSES);

				// No blur, radius too small
				if (blurSize * processor->GetImage()->GetWidth() < 1.0) { break; }

				if (processor->GetMultithread()) {
					// Multithread if needed

					for (int i = 0; i < numPasses; i++) {
						processor->SetupBlur();
						this->Multithread(curEdit);
						processor->CleanupBlur();
						if (processor->forceStop) { break; }
					}
				}

				// Single thread
				else {

					// Blur Horizontally
					for (int i = 0; i < numPasses; i++) {
						processor->SetupBlur();
						processor->BoxBlurHorizontal(processor->CalculateBlurSize(blurSize));
						processor->CleanupBlur();
						if (processor->forceStop) { break; }
					}
				}
			}
			break;

			// Peform Blur edit
			case ProcessorEdit::EditType::VERTICAL_BLUR: {

				processor->SendMessageToParent("Processing Vertical Blur" + fullEditNumStr);

				double blurSize = curEdit->GetParam(PHOEDIX_PARAMETER_BLURSIZE);
				int numPasses = (int)curEdit->GetParam(PHOEDIX_PARAMETER_NUM_PASSES);

				// No blur, radius too small
				if (blurSize * processor->GetImage()->GetHeight() < 1.0) { break; }

				// Multithread if needed
				if (processor->GetMultithread()) {

					// Perform an edit on the data through the processor
					for (int i = 0; i < numPasses; i++) {

						// Blur image vertically
						processor->SetupBlur();
						this->Multithread(curEdit);
						processor->CleanupBlur();
						if (processor->forceStop) { break; }
					}
				}

				// Single thread
				else {

					// Perform an edit on the data through the processor
					for (int i = 0; i < numPasses; i++) {

						// Blur rotated image horizontally
						processor->SetupBlur();
						processor->BoxBlurVertical(processor->CalculateBlurSize(blurSize));
						processor->CleanupBlur();
						if(processor->forceStop){ break; }
					}

					processor->SetUpdated(true);
				}
			}
			break;

			// Peform a channel scale
			case ProcessorEdit::EditType::CHANNEL_MIXER: {

				processor->SendMessageToParent("Processing Channel Mixer Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double redRedScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_RED);
				double redGreenScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_GREEN);
				double redBlueScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_BLUE);
				double greenRedScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_RED);
				double greenGreenScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_GREEN);
				double greenBlueScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_BLUE);
				double blueRedScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_RED);
				double blueGreenScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_GREEN);
				double blueBlueScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_BLUE);

				// Multithread if needed
				if (processor->GetMultithread()) {
					this->Multithread(curEdit);
				}
				// Single thread
				else {
					// Perform an edit on the data through the processor
					processor->ChannelScale(redRedScale, redGreenScale, redBlueScale,
						greenRedScale, greenGreenScale, greenBlueScale,
						blueRedScale, blueGreenScale, blueBlueScale);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform an Adjust Contrast edit
			case ProcessorEdit::EditType::ADJUST_CONTRAST: {

				processor->SendMessageToParent("Processing Adjust Contrast Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double allContrast = curEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST);
				double redContrast = curEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST);
				double greenContrast = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST);
				double blueContrast = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST);
				double allCenter = curEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST_CENTER);
				double redCenter = curEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST_CENTER);
				double greenCenter = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST_CENTER);
				double blueCenter = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST_CENTER);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->AdjustContrast(allContrast, redContrast, greenContrast, blueContrast, allCenter, redCenter, greenCenter, blueCenter);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform an Adjust Contrast edit
			case ProcessorEdit::EditType::ADJUST_CONTRAST_CURVE: {

				processor->SendMessageToParent("Processing Adjust Contrast Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double allContrast = curEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST);
				double redContrast = curEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST);
				double greenContrast = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST);
				double blueContrast = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST);
				double allCenter = curEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST_CENTER);
				double redCenter = curEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST_CENTER);
				double greenCenter = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST_CENTER);
				double blueCenter = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST_CENTER);

				// Multithread if needed
				if (processor->GetMultithread()) {
					this->Multithread(curEdit);
				}

				// Single thread
				else {
					// Perform an edit on the data through the processor
					processor->AdjustContrastCurve(allContrast, redContrast, greenContrast, blueContrast, 
						allCenter, redCenter, greenCenter, blueCenter);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform Crop edit
			case ProcessorEdit::EditType::CROP: {

				if (curEdit->GetFlag(PHOEDIX_FLAG_CROP_ENABLED) == 0) { break; }

				processor->SendMessageToParent("Processing crop" + fullEditNumStr);

				// Get crop dimmensions
				double startX = curEdit->GetParam(PHOEDIX_PARAMETER_STARTX);
				double startY = curEdit->GetParam(PHOEDIX_PARAMETER_STARTY);
				double newWidth = curEdit->GetParam(PHOEDIX_PARAMETER_CROP_WIDTH);
				double newHeight = curEdit->GetParam(PHOEDIX_PARAMETER_CROP_HEIGHT);

				// Multithread if needed
				if (processor->GetMultithread()) {
					processor->SetupCrop(newWidth, newHeight);
					this->Multithread(curEdit);
					processor->CleanupCrop();
				}

				// Single thread
				else {
					// Perform an edit on the data through the processor
					processor->SetupCrop(newWidth, newHeight);
					processor->Crop(startX, startY);
					processor->CleanupCrop();
				}
			}
			break;

			// Peform a greyscale conversion, averaging RGB values
			case ProcessorEdit::EditType::CONVERT_GREYSCALE_AVG: {

				processor->SendMessageToParent("Processing Greyscale (Average) Edit" + fullEditNumStr);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->ConvertGreyscale((1.0 / 3.0), (1.0 / 3.0), (1.0 / 3.0));
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a greyscale conversion, using humany eyesight values
			case ProcessorEdit::EditType::CONVERT_GREYSCALE_EYE: {

				processor->SendMessageToParent("Processing Greyscale (Human Eyesight) Edit" + fullEditNumStr);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->ConvertGreyscale(0.2126, 0.7152, 0.0722);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a greyscale conversion, using custom scalars
			case ProcessorEdit::EditType::CONVERT_GREYSCALE_CUSTOM: {

				processor->SendMessageToParent("Processing Greyscale (Custom) Edit" + fullEditNumStr);

				// Get all parameters from the edit
				double redScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
				double greenScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
				double blueScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->ConvertGreyscale(redScale, greenScale, blueScale);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform HSL Curves edit
			case ProcessorEdit::EditType::HSL_CURVES: {
			
				if (curEdit->GetNumIntArrays() == 3 && curEdit->GetParamsSize() == 3) {
					processor->SendMessageToParent("Processing HSL Curves Edit" + fullEditNumStr);

					// Get LAB curve data
					int * hCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_H_CURVE);
					int * sCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_S_CURVE);
					int * lCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_L_CURVE);

					double rScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
					double gScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
					double bScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

					// Multithread if needed
					if(processor->GetMultithread()){
						this->Multithread(curEdit);
					}

					// Single thread
					else{
						// Perform an edit on the data through the processor
						processor->HSLCurves(hCurve16, sCurve16, lCurve16, rScale, gScale, bScale);
					}
					processor->SetUpdated(true);
			
				}
			}
			break;

			// Peform LAB Curves edit
			case ProcessorEdit::EditType::LAB_CURVES: {
			
				if (curEdit->GetNumIntArrays() == 3) {
					processor->SendMessageToParent("Processing LAB Curves Edit" + fullEditNumStr);

					// Get LAB curve data
					int * lCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_L_CURVE);
					int * aCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_A_CURVE);
					int * bCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE);

					double rScale = curEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
					double gScale = curEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
					double bScale = curEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

					// Multithread if needed
					if(processor->GetMultithread()){
						this->Multithread(curEdit);
					}

					// Single thread
					else{
						// Perform an edit on the data through the processor
						processor->LABCurves(lCurve16, aCurve16, bCurve16, rScale, gScale, bScale);
					}
					processor->SetUpdated(true);
			
				}
			}
			break;

			// Peform a horizontal image flip
			case ProcessorEdit::EditType::MIRROR_HORIZONTAL: {

				processor->SendMessageToParent("Processing Mirror Edit" + fullEditNumStr);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->MirrorHorizontal();
				}
				processor->SetUpdated(true);
			}
			break;

			 // Peform a vertical image flip
			case ProcessorEdit::EditType::MIRROR_VERTICAL: {

				processor->SendMessageToParent("Processing Mirror Edit" + fullEditNumStr);

				// Multithread if needed
				if(processor->GetMultithread()){
					this->Multithread(curEdit);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					processor->MirrorVertical();
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform RGB Curves edit
			case ProcessorEdit::EditType::RGB_CURVES: {

				if (curEdit->GetNumIntArrays() == 8) {
					processor->SendMessageToParent("Processing RGB Curves Edit" + fullEditNumStr);

					// Get 8 bit curve data
					int * brightCurve8 = curEdit->GetIntArray(PHOEDIX_PARAMETER_BRIGHT_CURVE);
					int * redCurve8 = curEdit->GetIntArray(PHOEDIX_PARAMETER_R_CURVE);
					int * greenCurve8 = curEdit->GetIntArray(PHOEDIX_PARAMETER_G_CURVE);
					int * blueCurve8 = curEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE);

					// Get 16 bit curve data
					int * brightCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_BRIGHT_CURVE_16);
					int * redCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_R_CURVE_16);
					int * greenCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_G_CURVE_16);
					int * blueCurve16 = curEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE_16);

					// Multithread if needed
					if(processor->GetMultithread()){
						this->Multithread(curEdit);
					}

					// Single thread
					else{
						// Perform an edit on the data through the processor
						processor->RGBCurves(brightCurve8, redCurve8, greenCurve8, blueCurve8,
						brightCurve16, redCurve16, greenCurve16, blueCurve16);
					}
					processor->SetUpdated(true);
				}
			}
			break;

			// Peform a 90 degree clockwise roctation
			case ProcessorEdit::EditType::ROTATE_90_CW: {

				// Perform an edit on the data through the processor
				processor->SetupRotation(editToComplete, 90.0, 0);
				processor->Rotate90CW();
				processor->CleanupRotation(editToComplete);
				
				processor->SetUpdated(true);
			}
			break;


			// Peform a 180 degree clockwise roctation
			// Peform a 90 degree clockwise roctation
			case ProcessorEdit::EditType::ROTATE_180: {

				processor->SendMessageToParent("Processing Rotation Edit" + fullEditNumStr);
			
				// Perform an edit on the data through the processor
				processor->SetupRotation(editToComplete, 180.0, 0);
				processor->Rotate180();
				processor->CleanupRotation(editToComplete);
				processor->SetUpdated(true);
			}
			break;

			// Peform a 270 degree clockwise roctation (90 counter clockwise)
			case ProcessorEdit::EditType::ROTATE_270_CW: {
				
				// Perform an edit on the data through the processor
				processor->SetupRotation(editToComplete, 270.0, 0);
				processor->Rotate270CW();
				processor->CleanupRotation(editToComplete);
				processor->SetUpdated(true);
			}
			break;

			// Peform a custom angle clockwise roctation
			case ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST: {

				processor->SendMessageToParent("Processing Rotation Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				double angle = curEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
				int cropFlag = curEdit->GetFlag(PHOEDIX_FLAG_ROTATE_CROP);

				if(angle == 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupRotation(editToComplete);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					processor->RotateCustom(angle);
					processor->CleanupRotation(editToComplete);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a custom angle clockwise roctation using bilinear interpolation
			case ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR: {

				processor->SendMessageToParent("Processing Rotation Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				double angle = curEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
				int cropFlag = curEdit->GetFlag(PHOEDIX_FLAG_ROTATE_CROP);

				if(angle == 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupRotation(editToComplete);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					processor->RotateCustomBilinear(angle);
					processor->CleanupRotation(editToComplete);
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a custom angle clockwise roctation using bicubic interpolation
			case ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC: {

				processor->SendMessageToParent("Processing Rotation Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				double angle = curEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
				int cropFlag = curEdit->GetFlag(PHOEDIX_FLAG_ROTATE_CROP);

				if(angle == 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupRotation(editToComplete);
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupRotation(editToComplete, angle, cropFlag)) { break; }
					processor->RotateCustomBicubic(angle);
					processor->CleanupRotation(editToComplete);
				}
				processor->SetUpdated(true);
			}
			break;
	
			// Peform a sclaing of the image using nearest neighbor
			case ProcessorEdit::EditType::SCALE_NEAREST: {

				processor->SendMessageToParent("Processing Scale Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				int width = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_WIDTH);
				int height = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_HEIGHT);
				
				// Fast edit, half size
				if(processor->GetFastEdit()){
					width/= 2.0;
					height /= 2.0;
				}

				if(width < 0 || height < 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupScale(width, height)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupScale();
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupScale(width, height)) { break; }
					processor->ScaleNearest();
					processor->CleanupScale();
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a sclaing of the image using bilinear interpolation
			case ProcessorEdit::EditType::SCALE_BILINEAR: {

				processor->SendMessageToParent("Processing Scale Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				int width = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_WIDTH);
				int height = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_HEIGHT);
				
				// Fast edit, half size
				if(processor->GetFastEdit()){
					width/= 2.0;
					height /= 2.0;
				}

				if(width < 0 || height < 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupScale(width, height)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupScale();
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupScale(width, height)) { break; }
					processor->ScaleBilinear();
					processor->CleanupScale();
				}
				processor->SetUpdated(true);
			}
			break;

			// Peform a sclaing of the image using bicubic interpolation
			case ProcessorEdit::EditType::SCALE_BICUBIC: {

				processor->SendMessageToParent("Processing Scale Edit" + fullEditNumStr);

				// Perform an edit on the data through the processor
				int width = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_WIDTH);
				int height = (int) curEdit->GetParam(PHOEDIX_PARAMETER_SCALE_HEIGHT);
				
				// Fast edit, half size
				if(processor->GetFastEdit()){
					width/= 2.0;
					height /= 2.0;
				}

				if(width < 0 || height < 0.0){ break; }

				// Multithread if needed
				if(processor->GetMultithread()){
					if (!processor->SetupScale(width, height)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupScale();
				}

				// Single thread
				else{
					// Perform an edit on the data through the processor
					if (!processor->SetupScale(width, height)) { break; }
					int dataSize = processor->GetTempImage()->GetWidth() * processor->GetTempImage()->GetHeight();
					this->Multithread(curEdit, dataSize);
					processor->CleanupScale();
				}
				processor->SetUpdated(true);
			}
			break;
		}
		processor->SendProcessorEditNumToParent(editIndex + 1);
	}
	
	// Send update image event to parent window, to get latest image from processor
	if (!processor->forceStop) {
		wxCommandEvent evt(UPDATE_IMAGE_EVENT, ID_UPDATE_IMAGE);
		wxPostEvent(processor->GetParentWindow(), evt);
	}

	processor->forceStopCritical.Enter();
	processor->forceStop = false;
	processor->forceStopCritical.Leave();
	this->DeleteEditVector();

	processor->SendMessageToParent("");
	processor->Unlock();
	return (wxThread::ExitCode)0;
}

Processor::EditThread::EditThread(Processor * processorPar, ProcessorEdit * edit, int dataStart, int dataEnd, wxMutex * mutLockIn,  wxCondition * condition, int numThreads, int * threadsComplete) : wxThread(wxTHREAD_DETACHED) {
	processor = processorPar;
	procEdit = edit;
	start = dataStart;
	end = dataEnd;
	mutLock = mutLockIn;
	cond = condition;
	threads = numThreads;
	complete = threadsComplete;
}

wxThread::ExitCode Processor::EditThread::Entry() {

	switch (procEdit->GetEditType()) {

		case ProcessorEdit::EditType::RAW: {
			return (wxThread::ExitCode) 0;
		}

		// Peform a Brightness Adjustment edit
		case ProcessorEdit::EditType::ADJUST_BRIGHTNESS: {

			// Get all parameters from the edit
			double brighnessAmount = procEdit->GetParam(PHOEDIX_PARAMETER_BRIGHTNESS);
			double detailsPreservation = procEdit->GetParam(PHOEDIX_PARAMETER_PRESERVATION);
			double toneSetting = procEdit->GetParam(PHOEDIX_PARAMETER_TONE);

			int toneFlag = procEdit->GetFlag(PHOEDIX_FLAG_PRESERVATION_TYPE);
			processor->AdjustBrightness(brighnessAmount, detailsPreservation, toneSetting, toneFlag, start, end);
		}
		 break;

		// Peform an HSL adjustment
		case ProcessorEdit::EditType::ADJUST_HSL: {

			// Get all parameters from the edit
			double hueShift = procEdit->GetParam(PHOEDIX_PARAMETER_HUE);
			double saturationScale = procEdit->GetParam(PHOEDIX_PARAMETER_SATURATION);
			double luminaceScale = procEdit->GetParam(PHOEDIX_PARAMETER_LUMINACE);
			double rScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
			double gScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
			double bScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

			processor->AdjustHSL(hueShift, saturationScale, luminaceScale, rScale, gScale, bScale, start, end);

		}
		break;

		// Peform an HSL adjustment
		case ProcessorEdit::EditType::ADJUST_LAB: {

			// Get all parameters from the edit
			double luminaceScale = procEdit->GetParam(PHOEDIX_PARAMETER_LUMINACE);
			double aShift = procEdit->GetParam(PHOEDIX_PARAMETER_A);
			double bShift = procEdit->GetParam(PHOEDIX_PARAMETER_B);
			double rScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
			double gScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
			double bScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

			processor->AdjustLAB(luminaceScale, aShift, bShift, rScale, gScale, bScale, start, end);

		}
		break;

		// Peform a Shift RGB edit
		case ProcessorEdit::EditType::ADJUST_RGB: {

			// Get all parameters from the edit
			double allBrightShift = procEdit->GetParam(PHOEDIX_PARAMETER_ALL);
			double redBrightShift = procEdit->GetParam(PHOEDIX_PARAMETER_RED);
			double greenBrightShift = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN);
			double blueBrightShift = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE);

			processor->AdjustRGB(allBrightShift, redBrightShift, greenBrightShift, blueBrightShift, start, end);
		}
		break;

		// Peform Horizontal Blur edit
		case ProcessorEdit::EditType::HORIZONTAL_BLUR: {

			double blurSize = procEdit->GetParam(PHOEDIX_PARAMETER_BLURSIZE);
			processor->BoxBlurHorizontal(processor->CalculateBlurSize(blurSize), start, end);
		}
		break;

		// Peform Vertical Blur edit
		case ProcessorEdit::EditType::VERTICAL_BLUR: {
			double blurSize = procEdit->GetParam(PHOEDIX_PARAMETER_BLURSIZE);
			processor->BoxBlurVertical(processor->CalculateBlurSize(blurSize), start, end);
		}
		break;

		// Peform a channel scale
		case ProcessorEdit::EditType::CHANNEL_MIXER: {

			// Get all parameters from the edit
			double redRedScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_RED);
			double redGreenScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_GREEN);
			double redBlueScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_BLUE);
			double greenRedScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_RED);
			double greenGreenScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_GREEN);
			double greenBlueScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_BLUE);
			double blueRedScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_RED);
			double blueGreenScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_GREEN);
			double blueBlueScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_BLUE);

			processor->ChannelScale(redRedScale, redGreenScale, redBlueScale,
				greenRedScale, greenGreenScale, greenBlueScale,
				blueRedScale, blueGreenScale, blueBlueScale, start, end);
		}
		break;

		// Peform an Adjust Contrast edit
		case ProcessorEdit::EditType::ADJUST_CONTRAST: {

			// Get all parameters from the edit
			double allContrast = procEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST);
			double redContrast = procEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST);
			double greenContrast = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST);
			double blueContrast = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST);
			double allCenter = procEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST_CENTER);
			double redCenter = procEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST_CENTER);
			double greenCenter = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST_CENTER);
			double blueCenter = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST_CENTER);


			processor->AdjustContrast(allContrast, redContrast, greenContrast, blueContrast,
				allCenter, redCenter, greenCenter, blueCenter, start, end);
		}
		break;

		// Peform an Adjust Contrast edit
		case ProcessorEdit::EditType::ADJUST_CONTRAST_CURVE: {

			// Get all parameters from the edit
			double allContrast = procEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST);
			double redContrast = procEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST);
			double greenContrast = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST);
			double blueContrast = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST);
			double allCenter = procEdit->GetParam(PHOEDIX_PARAMETER_ALL_CONTRAST_CENTER);
			double redCenter = procEdit->GetParam(PHOEDIX_PARAMETER_RED_CONTRAST_CENTER);
			double greenCenter = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_CONTRAST_CENTER);
			double blueCenter = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_CONTRAST_CENTER);

			processor->AdjustContrastCurve(allContrast, redContrast, greenContrast, blueContrast,
				allCenter, redCenter, greenCenter, blueCenter, start, end);
		}
		break;

		// Peform Crop edit
		case ProcessorEdit::EditType::CROP: {

			// Get crop dimmensions
			double startX = procEdit->GetParam(PHOEDIX_PARAMETER_STARTX);
			double startY = procEdit->GetParam(PHOEDIX_PARAMETER_STARTY);

			processor->Crop(startX, startY, start, end);
		}
		break;

		 // Peform a greyscale conversion, averaging RGB values
		case ProcessorEdit::EditType::CONVERT_GREYSCALE_AVG: {

			processor->ConvertGreyscale((1.0 / 3.0), (1.0 / 3.0), (1.0 / 3.0), start, end);
		}
		break;

		// Peform a greyscale conversion, using humany eyesight values
		case ProcessorEdit::EditType::CONVERT_GREYSCALE_EYE: {

			processor->ConvertGreyscale(0.2126, 0.7152, 0.0722, start, end);
		}
		break;

		// Peform a greyscale conversion, using custom scalars
		case ProcessorEdit::EditType::CONVERT_GREYSCALE_CUSTOM: {

			// Get all parameters from the edit
			double redScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
			double greenScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE);
			double blueScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

			processor->ConvertGreyscale(redScale, greenScale, blueScale, start, end);
		}
		break;

		// Peform HSL Curves edit
		case ProcessorEdit::EditType::HSL_CURVES: {

			if (procEdit->GetNumIntArrays() == 3 && procEdit->GetParamsSize() == 3) {

				// Get LAB curve data
				int * hCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_H_CURVE);
				int * sCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_S_CURVE);
				int * lCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_L_CURVE);

				double rScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
				double gScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE );
				double bScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

				processor->HSLCurves(hCurve16, sCurve16, lCurve16, rScale, gScale, bScale, start, end);
			}
		}
		break;

		// Peform LAB Curves edit
		case ProcessorEdit::EditType::LAB_CURVES: {

			if (procEdit->GetNumIntArrays() == 3 && procEdit->GetParamsSize() == 3) {

				// Get LAB curve data
				int * lCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_L_CURVE);
				int * aCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_A_CURVE);
				int * bCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE);

				double rScale = procEdit->GetParam(PHOEDIX_PARAMETER_RED_SCALE);
				double gScale = procEdit->GetParam(PHOEDIX_PARAMETER_GREEN_SCALE );
				double bScale = procEdit->GetParam(PHOEDIX_PARAMETER_BLUE_SCALE);

				processor->LABCurves(lCurve16, aCurve16, bCurve16, rScale, gScale, bScale, start, end);
			}
		}
		break;

		// Peform a horizontal image flip
		case ProcessorEdit::EditType::MIRROR_HORIZONTAL: {

			processor->MirrorHorizontal(start, end);
		}
		break;

		// Peform a vertical image flip
		case ProcessorEdit::EditType::MIRROR_VERTICAL: {

			processor->MirrorVertical(start, end);

		}
		break;

		// Peform a 90 degree clockwise roctation
		case ProcessorEdit::EditType::ROTATE_90_CW: {
			processor->Rotate90CW(start, end);
		}
		break;


		// Peform a 180 degree clockwise roctation (not multithreaded)
		case ProcessorEdit::EditType::ROTATE_180: {
			processor->Rotate180(start, end);
		}
		break;

		// Peform a 270 degree clockwise roctation (not multithreaded)
		case ProcessorEdit::EditType::ROTATE_270_CW: {
			processor->Rotate270CW(start, end);
		}
		break;

		// Peform a custom angle clockwise roctation
		case ProcessorEdit::EditType::ROTATE_CUSTOM_NEAREST: {

			double angle = procEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
			processor->RotateCustom(angle, start, end);
		}
		break;

		// Peform a custom angle clockwise roctation using bilinear interpolation
		case ProcessorEdit::EditType::ROTATE_CUSTOM_BILINEAR: {

			double angle = procEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
			processor->RotateCustomBilinear(angle, start, end);
		}
		break;

		// Peform a custom angle clockwise roctation using bicubic interpolation
		case ProcessorEdit::EditType::ROTATE_CUSTOM_BICUBIC: {

			// Perform an edit on the data through the processor
			double angle = procEdit->GetParam(PHOEDIX_PARAMETER_ROTATE_ANGLE);
			processor->RotateCustomBicubic(angle, start, end);

		}

		break;

		// Peform a sclaing of the image using nearest neighbor
		case ProcessorEdit::EditType::SCALE_NEAREST: {
			processor->ScaleNearest(start, end);
		}
		break;

		// Peform a sclaing of the image using bilinear interpolation
		case ProcessorEdit::EditType::SCALE_BILINEAR: {

			processor->ScaleBilinear(start, end);
		}
		break;

		// Peform a sclaing of the image using bicubic interpolation
		case ProcessorEdit::EditType::SCALE_BICUBIC: {

			processor->ScaleBicubic(start, end);
		}
		break;

		// Peform RGB Curves edit
		case ProcessorEdit::EditType::RGB_CURVES: {

			if (procEdit->GetNumIntArrays() == 8) {

				// Get 8 bit curve data
				int * brightCurve8 = procEdit->GetIntArray(PHOEDIX_PARAMETER_BRIGHT_CURVE);
				int * redCurve8 = procEdit->GetIntArray(PHOEDIX_PARAMETER_R_CURVE);
				int * greenCurve8 = procEdit->GetIntArray(PHOEDIX_PARAMETER_G_CURVE);
				int * blueCurve8 = procEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE);

				// Get 16 bit curve data
				int * brightCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_BRIGHT_CURVE_16);
				int * redCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_R_CURVE_16);
				int * greenCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_G_CURVE_16);
				int * blueCurve16 = procEdit->GetIntArray(PHOEDIX_PARAMETER_B_CURVE_16);

				processor->RGBCurves(brightCurve8, redCurve8, greenCurve8, blueCurve8,
					brightCurve16, redCurve16, greenCurve16, blueCurve16, start, end);
			}
		}
		break;
	}

	mutLock->Lock();
	*complete += 1;
	mutLock->Unlock();

	// All worker threads have finished, signal condition to continue
	if (*complete == threads && cond->IsOk()) {
		cond->Broadcast();
	}

	return (wxThread::ExitCode)0;
}

Processor::RawProcessThread::RawProcessThread(Processor * processorPar, bool unpackAndProcess) : wxThread(wxTHREAD_DETACHED) {

	processor = processorPar;
	unpackProcess = unpackAndProcess;
	bool fastProcessRaw = processor->GetFastEdit();
	
	// If fast edit, set half size and turn off denoising for performance
	if(fastProcessRaw){
		processor->rawPrcoessor.imgdata.params.half_size = 1;
		processor->rawPrcoessor.imgdata.params.threshold = 0.0;
	}
	else {
		processor->rawPrcoessor.imgdata.params.half_size = 0;
	}

	// Open and unpack data if there is a change in fast process.
	if (fastProcessRaw != processor->lastRawFastProcess) {
		unpackProcess = true;
	}
	
	processor->lastRawFastProcess = processor->GetFastEdit();
}

wxThread::ExitCode Processor::RawProcessThread::Entry() {

	if (unpackProcess) {

		while (processor->GetLockedRaw()) {
			this->Sleep(20);
		}

		processor->LockRaw();

		processor->SendMessageToParent("Opening Raw File");
		processor->rawPrcoessor.recycle();

		#ifdef __WXMSW__
			processor->rawErrorCode = processor->rawPrcoessor.open_file(processor->GetFilePath().wc_str());
		#else
			processor->rawErrorCode = processor->rawPrcoessor.open_file(processor->GetFilePath().c_str());
		#endif
			// Open failed, present error and return
			if(processor->rawErrorCode != 0){

				wxString errorName = RawError::GetStringFromError(processor->rawErrorCode);
				processor->SendMessageToParent("RAW Image open failed: " + errorName);
				processor->UnlockRaw();
				return 0;
			}

			processor->SendMessageToParent("Unpacking Raw File");
			processor->rawErrorCode = processor->rawPrcoessor.unpack();

			// Unpack failed, present error and return
			if(processor->rawErrorCode != 0){

				wxString errorName = RawError::GetStringFromError(processor->rawErrorCode);
				processor->SendMessageToParent("RAW Image unpack failed: " + errorName);
				processor->UnlockRaw();
				return 0;
			}

		processor->UnlockRaw();
	}

	if (processor->GetLockedRaw()) {
		return 0;
	}

	processor->LockRaw();

	// process the raw image
	processor->SendMessageToParent("RAW Image Processing");
	processor->rawPrcoessor.set_progress_handler(&Processor::RawCallback, NULL);
	processor->rawErrorCode = processor->rawPrcoessor.dcraw_process();
	processor->rawPrcoessor.set_progress_handler(NULL, NULL);
	processor->SendMessageToParent("RAW Image Done Processing");

	// Exit method before creating raw image from bad data
	if(processor->rawErrorCode != LIBRAW_SUCCESS){

		if(processor->rawErrorCode == LIBRAW_CANCELLED_BY_CALLBACK){

			processor->SendMessageToParent("RAW Image processing canceled");
			processor->forceStopRaw = false;

			#ifdef __WXMSW__
				processor->rawErrorCode = processor->rawPrcoessor.open_file(processor->GetFilePath().wc_str());
			#else
				processor->rawErrorCode = processor->rawPrcoessor.open_file(processor->GetFilePath().c_str());
			#endif
			// Open failed, present error and return
			if(processor->rawErrorCode != 0){

				wxString errorName = RawError::GetStringFromError(processor->rawErrorCode);
				processor->SendMessageToParent("RAW Image open failed: " + errorName);
				processor->UnlockRaw();
				return 0;
			}

			processor->rawErrorCode = processor->rawPrcoessor.unpack();
			// Unpack failed, present error and return
			if(processor->rawErrorCode != 0){

				wxString errorName = RawError::GetStringFromError(processor->rawErrorCode);
				processor->SendMessageToParent("RAW Image unpack failed: " + errorName);
				processor->UnlockRaw();
				return 0;
			}

			// Set progress handler to processors raw callback
			if(processor->restartRaw){

				processor->UnlockRaw();
				processor->ProcessRaw();
				return 0;
			}
		}
		wxString errorName = RawError::GetStringFromError(processor->rawErrorCode);
		processor->SendMessageToParent("RAW Image processing failed: " + errorName);
		processor->UnlockRaw();
		return 0;
	}

	// Create the raw image
	processor->SendMessageToParent("Creating RAW Image for display");
	processor->rawImage = processor->rawPrcoessor.dcraw_make_mem_image(&processor->rawErrorCode);

	// Set rawImageGood on processor if success
	if(processor->rawErrorCode == LIBRAW_SUCCESS){

		processor->SendMessageToParent("Updating Display");
		processor->rawImageGood = true;
		
		while(processor->GetLocked()){
			this->Sleep(20);
		}

		processor->Lock();

		ImageHandler::CopyImageFromRaw(processor->rawImage, processor->GetImage());
		if(processor->GetOriginalImage() != NULL){
			processor->GetOriginalImage()->Destroy();
		}
		processor->SetOriginalImage(processor->GetImage());

		// If there are no edits after raw procesing, set updated.  We don't want to show the raw image without
		// following edits, if those edits exist
		if(!processor->GetHasEdits()){
			//processor->SetUpdated(true);
			wxCommandEvent evt(UPDATE_IMAGE_EVENT, ID_UPDATE_IMAGE);
			wxPostEvent(processor->GetParentWindow(), evt);
		}

		// Populate image exif information
		ImageHandler::ReadExifFromRaw(&processor->rawPrcoessor, processor->GetImage());

		processor->Unlock();
		processor->ProcessEdits();
		processor->DeleteRawImage();
		processor->SendMessageToParent("");
		processor->SendRawComplete();
	}
	else{
		
	}

	processor->UnlockRaw();
	return (wxThread::ExitCode)0;
}