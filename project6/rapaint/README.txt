Ryan Painter rapaint@clemson.edu

warper:
	
	This progam provides some basic ways to warp and transform images
	
	1) run the "make" command
	2) run "./warper in.ext <out.ext>"
		<> indicates optional arg
	3) follow the command line instructions. descriptions of each command are below.
		r - rotate image counter clockwise by theta in degrees
		s - scales the image by sx and sy
		t - translate the image by dx and dy
		h - shears the image by hx and hy
		f - flips the image. fx = 1 to flip horizontally and fy = 1 to flip vertically
		p - perspective warp by px and py
		d - done. runs the transformations
	4) once done viewing the new image press q to close the window.
		
	Issues:
		images that have a perspective warp done on them do not fill the full allocated space. I tried accounting for this when the new width and height are calculated, but it lead to strange results.
