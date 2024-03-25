/*
  Example inverse warps
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <GL/glut.h>
#include <OpenImageIO/imageio.h>

using namespace std;
using std::string;
OIIO_NAMESPACE_USING

static int icolor = 0;

struct Pixel{ // defines a pixel structure
	unsigned char r,g,b,a;
}; 

//
// Global variables and constants
//
const int DEFAULTWIDTH = 600;	// default window dimensions if no image
const int DEFAULTHEIGHT = 600;

int WinWidth, WinHeight;	// window width and height
int ImWidth, ImHeight;		// image width and height
int ImChannels;           // number of channels per image pixel

int VpWidth, VpHeight;		// viewport width and height
int Xoffset, Yoffset;     // viewport offset from lower left corner of window

Pixel **pixmap = NULL;  // the image pixmap used for OpenGL display
Pixel **warpedPixmap = NULL;
int pixformat; 			// the pixel format used to correctly  draw the image

/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
void inv_map(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = x/2;
  v = y/2; 
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}

void inv_map2(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = 0.5 * (x * x * x * x + sqrt(sqrt(y)));
  v = 0.5 * (sqrt(sqrt(x)) + y * y * y * y);
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}

void repair() {
  //repair into temp
  float u, v, a, b;
  int j, k;
  Pixel tempPixel, topLeft, topRight, bottomLeft, bottomRight;
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      inv_map(r, c, u, v, ImHeight, ImWidth, ImHeight, ImWidth);

      j = (int)(u);
      k = (int)(v);
      a = u - j;
      b = v - k;

      topLeft = pixmap[j][k];
      topRight = pixmap[j + 1][k];
      bottomLeft = pixmap[j][k + 1];
      bottomRight = pixmap[j + 1][k + 1];

      tempPixel.r = ((((1 - b) * topLeft.r) + (b * bottomLeft.r)) * (1 - a)) + ((((1 - b) * topRight.r) + (b * bottomRight.r)) * a);
      tempPixel.g = ((((1 - b) * topLeft.g) + (b * bottomLeft.g)) * (1 - a)) + ((((1 - b) * topRight.g) + (b * bottomRight.g)) * a);
      tempPixel.b = ((((1 - b) * topLeft.b) + (b * bottomLeft.b)) * (1 - a)) + ((((1 - b) * topRight.b) + (b * bottomRight.b)) * a);
      tempPixel.a = ((((1 - b) * topLeft.a) + (b * bottomLeft.a)) * (1 - a)) + ((((1 - b) * topRight.a) + (b * bottomRight.a)) * a);

      // if(r == 300 && c == 300) {
      //   cout << "u = " << u << "\nv = " << v <<"\nj = " << j << "\nk = " << k << "\na = " << a << "\nb = " << b << "\n";
      //   cout << "topLeft = " << topLeft.r << " " << topLeft.g << " " << topLeft.b << " " << topLeft.a << "\n";
      //   cout << "topRight = " << topRight.r << " " << topRight.g << " " << topRight.b << " " << topRight.a << "\n";
      //   cout << "bottomLeft = " << bottomLeft.r << " " << bottomLeft.g << " " << bottomLeft.b << " " << bottomLeft.a << "\n";
      //   cout << "bottomRight = " << bottomRight.r << " " << bottomRight.g << " " << bottomRight.b << " " << bottomRight.a << "\n";
      //   cout << "tempPixel = " << tempPixel.r << " " << tempPixel.g << " " << tempPixel.b << " " << tempPixel.a << "\n";
      // }
      
      warpedPixmap[r][c] = tempPixel;
    }
  }
}

#pragma region display
//
//  Routine to cleanup the memory.   
//
void destroy(){
 if (pixmap){
     delete pixmap[0];
	 delete pixmap;  
  }
}


//
//  Routine to read an image file and store in a pixmap
//  returns the size of the image in pixels if correctly read, or 0 if failure
//
int readImage(string infilename){
  // Create the oiio file handler for the image, and open the file for reading the image.
  // Once open, the file spec will indicate the width, height and number of channels.
  std::unique_ptr<ImageInput> infile = ImageInput::open(infilename);
  if(!infile){
    cerr << "Could not input image file " << infilename << ", error = " << geterror() << endl;
    return 0;
  }

  // Record image width, height and number of channels in global variables
  ImWidth = infile->spec().width;
  ImHeight = infile->spec().height;
  ImChannels = infile->spec().nchannels;

 
  // allocate temporary structure to read the image 
  unsigned char tmp_pixels[ImWidth * ImHeight * ImChannels];

  // read the image into the tmp_pixels from the input file, flipping it upside down using negative y-stride,
  // since OpenGL pixmaps have the bottom scanline first, and 
  // oiio expects the top scanline first in the image file.
  int scanlinesize = ImWidth * ImChannels * sizeof(unsigned char);
  if(!infile->read_image(TypeDesc::UINT8, &tmp_pixels[0] + (ImHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not read image from " << infilename << ", error = " << geterror() << endl;
    return 0;
  }
 
 // get rid of the old pixmap and make a new one of the new size
  destroy();
  
 // allocate space for the Pixmap (contiguous approach, 2d style access)
  pixmap = new Pixel*[ImHeight];
  if(pixmap != NULL)
	pixmap[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	pixmap[i] = pixmap[i - 1] + ImWidth;
 
 //  assign the read pixels to the the data structure
 int index;
  for(int row = 0; row < ImHeight; ++row) {
    for(int col = 0; col < ImWidth; ++col) {
      index = (row*ImWidth+col)*ImChannels;
      
      if (ImChannels==1){ 
        pixmap[row][col].r = tmp_pixels[index];
        pixmap[row][col].g = tmp_pixels[index];
        pixmap[row][col].b = tmp_pixels[index];
        pixmap[row][col].a = 255;
      }
      else{
        pixmap[row][col].r = tmp_pixels[index];
        pixmap[row][col].g = tmp_pixels[index+1];
        pixmap[row][col].b = tmp_pixels[index+2];			
        if (ImChannels <4) // no alpha value is present so set it to 255
          pixmap[row][col].a = 255; 
        else // read the alpha value
          pixmap[row][col].a = tmp_pixels[index+3];			
      }
    }
  }
 
  // close the image file after reading, and free up space for the oiio file handler
  infile->close();
  
  // set the pixel format to GL_RGBA and fix the # channels to 4  
  pixformat = GL_RGBA;  
  ImChannels = 4;

  //initialize warped pixmap to be same size as pixmap
  warpedPixmap = new Pixel*[ImHeight];
  if(warpedPixmap != NULL)
	warpedPixmap[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	warpedPixmap[i] = warpedPixmap[i - 1] + ImWidth;

  // return image size in pixels
  return ImWidth * ImHeight;
}

//
// Routine to display a pixmap in the current window
//
void displayImage(){
  // if the window is smaller than the image, scale it down, otherwise do not scale
  if(WinWidth < ImWidth  || WinHeight < ImHeight)
    glPixelZoom(float(VpWidth) / ImWidth, float(VpHeight) / ImHeight);
  else
    glPixelZoom(1.0, 1.0);
  
  // display starting at the lower lefthand corner of the viewport
  glRasterPos2i(0, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(ImWidth, ImHeight, pixformat, GL_UNSIGNED_BYTE, warpedPixmap[0]);
}


//
// Routine to write the current framebuffer to an image file
//
void writeImage(){
  string outfilename;
  cout << "Please input a filename: ";
  cin >> outfilename;

  // make a pixmap that is the size of the window and grab OpenGL framebuffer into it
  // alternatively, you can read the pixmap into a 1d array and export this 
   unsigned char local_pixmap[WinWidth * WinHeight * ImChannels];
   glReadPixels(0, 0, WinWidth, WinHeight, pixformat, GL_UNSIGNED_BYTE, local_pixmap);
  
  // create the oiio file handler for the image
  std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // Open a file for writing the image. The file header will indicate an image of
  // width WinWidth, height WinHeight, and ImChannels channels per pixel.
  // All channels will be of type unsigned char
  ImageSpec spec(WinWidth, WinHeight, ImChannels, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // Write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars. While writing, flip the image upside down by using negative y stride, 
  // since OpenGL pixmaps have the bottom scanline first, and oiio writes the top scanline first in the image file.
  int scanlinesize = WinWidth * ImChannels * sizeof(unsigned char);
  if(!outfile->write_image(TypeDesc::UINT8, local_pixmap + (WinHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // close the image file after the image is written and free up space for the
  // oiio file handler
  outfile->close();
}

//
//   Display Callback Routine: clear the screen and draw the current image
//
void handleDisplay(){
  // specify window clear (background) color to be opaque black
  glClearColor(0, 0, 0, 1);
  // clear window to background color
  glClear(GL_COLOR_BUFFER_BIT);  
  
  // only draw the image if it is of a valid size
  if(ImWidth > 0 && ImHeight > 0)
    displayImage();
  
  // flush the OpenGL pipeline to the viewport
  glFlush();
}

//
//  Keyboard Callback Routine: 'r' - read and display a new image,
//  'w' - write the current window to an image file, 'q' or ESC - quit
//
void handleKey(unsigned char key, int x, int y){
  switch(key){	
    case 'w':
    case 'W':
      writeImage();
      break;

    case 'r':
    case 'R':
      repair();
      glutPostRedisplay();
      break;

	  case 'q':		// q or ESC - quit
    case 'Q':
    case 27:
      destroy();
      exit(0);
      
    default:		// not a valid key -- just ignore it
      return;
  }
}


///
//  Reshape Callback Routine: If the window is too small to fit the image,
//  make a viewport of the maximum size that maintains the image proportions.
//  Otherwise, size the viewport to match the image size. In either case, the
//  viewport is centered in the window.
//
void handleReshape(int w, int h){
  float imageaspect = (float)ImWidth / (float)ImHeight;	// aspect ratio of image
  float newaspect = (float)w / (float)h; // new aspect ratio of window
  
  // record the new window size in global variables for easy access
  WinWidth = w;
  WinHeight = h;
  
  // if the image fits in the window, viewport is the same size as the image
  if(w >= ImWidth && h >= ImHeight){
    Xoffset = (w - ImWidth) / 2;
    Yoffset = (h - ImHeight) / 2;
    VpWidth = ImWidth;
    VpHeight = ImHeight;
  }
  // if the window is wider than the image, use the full window height
  // and size the width to match the image aspect ratio
  else if(newaspect > imageaspect){
    VpHeight = h;
    VpWidth = int(imageaspect * VpHeight);
    Xoffset = int((w - VpWidth) / 2);
    Yoffset = 0;
  }
  // if the window is narrower than the image, use the full window width
  // and size the height to match the image aspect ratio
  else{
    VpWidth = w;
    VpHeight = int(VpWidth / imageaspect);
    Yoffset = int((h - VpHeight) / 2);
    Xoffset = 0;
  }
  
  // center the viewport in the window
  glViewport(Xoffset, Yoffset, VpWidth, VpHeight);
  
  // viewport coordinates are simply pixel coordinates
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, VpWidth, 0, VpHeight);
  glMatrixMode(GL_MODELVIEW);
}
#pragma endregion

/*
   Main program to read an image file, then ask the user
   for transform information, transform the image and display
   it using the appropriate warp.  Optionally save the transformed
   images in  files.
*/
int main(int argc, char *argv[]){
	if(argc != 2) {
		cerr << "incorrect usage. correct usage is: \"./okwarp in.ext\"" << endl;
		exit(-1);
  }

	//your code to read in the input image
	readImage(argv[1]);

  //inverse map
  float u, v;
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      inv_map(r, c, u, v, ImHeight, ImWidth, ImHeight, ImWidth);

      warpedPixmap[r][c] = pixmap[(int)(u)][(int)(v)];
    }
  }

	// start up the glut utilities
	glutInit(&argc, argv);
	
	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(ImWidth, ImHeight);
	glutCreateWindow("okwarp");

	// set up the callback routines to be called when glutMainLoop() detects
	// an event
	glutDisplayFunc(handleDisplay);	  	// display callback
	glutKeyboardFunc(handleKey);	  	// keyboard callback
	glutReshapeFunc(handleReshape); 	// window resize callback

	// Routine that loops forever looking for events. It calls the registered
	// callback routine to handle each event that is detected
	glutMainLoop();

	return 0;
}