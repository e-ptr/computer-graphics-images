Ryan Painter rapaint@clemson.edu

alphamask:

	This program offers basic chromakeying.

	1) run the "make mask" command
	2) run "./alphamask <file to open> <file to save as>"

	To change the color being masked go to line 183. Here there are places to specify the HSV of 	the color to target, as well as thresholds for each channel.

	I used the same HSV and thresholds for all 3 images and found numbers that worked best across all 3.

	Target:
	H - 120
	S - 1
	V - 1

	Variance:
	H - 55
	S - 0.65
	V - 0.85

compose:

	This program composites images using A over B.

	1) run the "make compose" command
	2) run "./compose <image A> <image B> <file to save as>"
		<file to save as> is optional
		IMPORTANT: Image B must be larger than image A in both dimensions.
	3) the composited image is displayed, simply close the window to run another composite
	
	Issues:
	If images are too large to program may segfault.
