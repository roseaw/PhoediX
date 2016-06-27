# PhoediX #
PhoediX is a non destructive image editing application created by Jacob Chesley.  This is an application currently under development, and will be capable of loading, editing and saving image files.  An image is loaded into PhoediX, and a list of edits the user select and modifies are completed on the image.  The edit list along with each edits parameters will be able to be saved, and used on multiple images.  This allows the original image to remain as is, and a render of a new image after the edits are completed on the original can be saved as a new image.  

The current list of image edits PhoediX is capable of are:

* **Brightness Adjustment** (scalar/multiplicative and Additive) (All Color Channels and Per Channel).
* **Contrast Adjustmen**t (All Color Channels and Per Channel).
* **Grayscale Conversion** (Average channels, human eyesight multipliers, and custom channel multipliers).
* **Channel Transform** (Each channel is created by all channels using multipliers.  Example - Sepia Tone).
* **Image Rotation** (90, 180, and 270 Clockwise.  Nearest Neighbor, Bilinear and Bicubic interpolation use for custom angles)
* **RGB Curves** (A brightness curves tool for brightness, red, green and blue channels)
* **LAB Curves** (A curves tool for LAB color space)

Edits Coming Soon:

* **Raw File Processing using LibRaw**

General PhoediX capabilites Coming Soon:

* Save Edit List (PhoediX project?)
* Open Edit List (PhoediX project?)
* Open a Raw Image - using Libraw
* Edit a Raw Image - using LibRaw
* Save Raw Image edit parameters (PhoediX project?)

Head on over to the Wiki to learn how to build PhoediX from source!