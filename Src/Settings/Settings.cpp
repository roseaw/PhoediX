// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#include "Settings.h"

int PhoedixSettings::bitDepth = 16;
int PhoedixSettings::colorSpace = 0;
int PhoedixSettings::numThreads = 0;
bool PhoedixSettings::libraryImageUseRaw = false;

int PhoedixSettings::GetBitDepth() {
	return bitDepth;
}
void PhoedixSettings::SetBitDepth(int newBitDepth) {
	bitDepth = newBitDepth;
}

int PhoedixSettings::GetColorSpace() {
	return colorSpace;
}
void PhoedixSettings::SetColorSpace(int newColorSpace) {
	colorSpace = newColorSpace;
}

int PhoedixSettings::GetNumThreads() {
	return numThreads;
}
void PhoedixSettings::SetNumThreads(int newNumThreads) {
	numThreads = newNumThreads;
}

bool PhoedixSettings::GetLibraryImageUseRaw() {
	return libraryImageUseRaw;
}
void PhoedixSettings::SetLibraryImageUseRaw(bool useRawImage) {
	libraryImageUseRaw = useRawImage;
}