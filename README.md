# PhoediX #
PhoediX is a non destructive image editing application created by Jacob Chesley.

Visit www.phoedix.com for more information.

The current list of image edits PhoediX is capable of are:

* **Brightness Adjustment** Adjust the brightness of the image with options to preserve details in shadows, highlights, or both.
* **Contrast Adjustment** Adjust the contrast using a linear or built in S curve function, to give the image a more vivid or flat look.
* **Adjust HSL** Adjust the Hue, Saturtation, and Luminance (HSL) channels of the image.
* **Adjust LAB** Adjust the Luminance, A, and B (LAB) channels of the image.
* **Adjust RGB** Adjust the Red, Green, and Blue (RGB) channels of the image.
* **HSL Curves** Adjust the Hue, Saturtation, and Luminance (HSL) channels of the image using custom defined curves.
* **LAB Curves** Adjust the Luminance, A, and B (LAB) channels of the image using custom defined curves.
* **RGB Curves** Adjust the Red, Green, and Blue (RGB) channels of the image using custom defined curves.
* **Grayscale Conversion** Convert the image to greyscale, with three different options. Options include Average of RGB channels, human eyesight scale, or custom scale.
* **Channel Mixer** Mix RGB master channels of the image with RGB subchannels for each channel.
* **Rotate Image** 	Rotate the image 90, 180, or 270 degrees. Custom angles and cropping available as well, using nearest neighbor, bilinear, or bicubic interpolation.
* **Mirror Image** 	Mirror the image vertically or horizontally.
* **Scale Image** Scale and resize the image to any size (with or without preserving the aspect ratio).  Choose nearest neighbor, bilinear, or bicubic interpolation.
* **Crop Image** Crop the image (with or without preserving the aspect ratio).
* **Blur Image** Use Fast Blur to blur the image horizontally, vertically, or in both directions. Specify the blur size and number of iterations.

Head on over to the Wiki to learn how to build PhoediX from source!

Jakes Informations above. Mines Below.

This version of PhoediX includes all necessary repositories as submodules. After cloning this repository it is necessary to run "git submodule update --init --recursive".

For Windows based builds everything can be built from the VS Solution.
* Will need to create wxWidgets\include\wxWidgets\setup.h manually as it cannot be checked into the repository.
* After opening the solution run Build manually on LibRaw_make, libtiff_make, and wxWidgetsBuild to perform setup.
* You may need to move the LibRaw\bin\libraw.dll manually to the output directory for the application to launch (Working on fix).