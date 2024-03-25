//Ryan Painter
//rapaint

/*
 * Program sample to warp an image using matrix-based warps
 * 
 * Command line parameters are as follows:
 *
 * warper infile.png [outfile.png] 
 *
 * Author: Ioannis Karamouzas, 10/20/19, modifed on 10/16/21

 */

#include "matrix.h"

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
Pixel **warped = NULL; // warped image
int pixformat; 			// the pixel format used to correctly  draw the image

string saveAs = "";

/*
Multiply M by a rotation matrix of angle theta
*/

void rotate(Matrix3D &M, float theta) {

	Matrix3D R;  // this initializes R to identity  
	double rad = PI * theta / 180.0; // convert degrees to radians

	//populate the rotation matrix
  R[0][0] = cos(rad);
  R[0][1] = -sin(rad);
  R[1][0] = sin(rad);
  R[1][1] = cos(rad);

	M = R * M; //append the rotation to your transformation matrix

}

void scale(Matrix3D &M, float sx, float sy) {
  Matrix3D S;

  S[0][0] = sx;
  S[1][1] = sy;

  M = S * M;
}

void translate(Matrix3D &M, float dx, float dy) {
  Matrix3D D;

  D[0][2] = dx;
  D[1][2] = dy;

  M = D * M;
}

void shear(Matrix3D &M, float hx, float hy) {
  Matrix3D H;

  H[0][1] = hx;
  H[1][0] = hy;

  M = H * M;
}

void flip(Matrix3D &M, float fx, float fy) {
  Matrix3D F;

  if(fx == 1) {
    F[0][0] = -1.0;
  }

  if(fy == 1) {
    F[1][1] = -1.0;
  }

  M = F * M;
}

void perspectiveWarp(Matrix3D &M, float px, float py) {
  Matrix3D P;

  P[2][0] = px;
  P[2][1] = py;

  M = P * M;
}

/*
Build a transformation matrix from input text
*/
void read_input(Matrix3D &M) {
	string cmd;
  float x, y;
	
	/* prompt for user input */
	do
	{
    cout << "enter a command (r, s, t, h, f, p, d)\n";
    x = 0.0;
    y = 0.0;
		cout << "> ";
		cin >> cmd;
		if (cmd.length() != 1){
			cout << "invalid command, enter r, s, t, h, f, p, d\n";
		}
		else {
			switch (cmd[0]) {
				case 'r':		/* Rotation, accept angle in degrees */
					float theta;
          cout << "input theta in degrees\n> ";
					cin >> theta;
					if (cin) {
						cout << "calling rotate\n";
						rotate(M, theta);
					}
					else {
						cerr << "invalid rotation angle\n";
						cin.clear();
					}						
					break;
				case 's':		/* scale, accept scale factors */
          cout << "input x factor\n> ";
          cin >> x;
          cout << "input y factor\n> ";
          cin >> y;
          if(cin) {
            cout << "calling scale\n";
            scale(M, x, y);
          }
          else {
            cerr << "invalid scale factor\n";
            cin.clear();
          }
					break;
				case 't':		/* Translation, accept translations */
          cout << "input delta x\n> ";
          cin >> x;
          cout << "input delta y\n> ";
          cin >> y;
          if(cin) {
            cout << "calling translate\n";
            translate(M, x, y);
          }
          else {
            cerr << "invalid delta\n";
            cin.clear();
          }
					break;
				case 'h':		/* Shear, accept shear factors */
          cout << "input x factor\n> ";
          cin >> x;
          cout << "input y factor\n> ";
          cin >> y;
          if(cin) {
            cout << "calling shear\n";
            shear(M, x, y);
          }
          else {
            cerr << "invalid shear factor\n";
            cin.clear();
          }
					break;
				case 'f':		/* Flip, accept flip factors */
          cout << "input flip x\n> ";
          cin >> x;
          cout << "input flip y\n> ";
          cin >> y;
          if(cin) {
            cout << "calling flip\n";
            flip(M, x, y);
          }
          else {
            cerr << "invalid flip factor\n";
            cin.clear();
          }
					break;
				case 'p':		/* Perspective, accept perspective factors */
          cout << "input x factor\n> ";
          cin >> x;
          cout << "input y factor\n> ";
          cin >> y;
          if(cin) {
            cout << "calling perspective warp\n";
            perspectiveWarp(M, x, y);
          }
          else {
            cerr << "invalid perspective warp factor\n";
            cin.clear();
          }
					break;		
				case 'd':		/* Done, that's all for now */
					break;
				default:
					cout << "invalid command, enter r, s, t, h, f, p, d\n";
			}
		}
	} while (cmd.compare("d")!=0);

}

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
  glDrawPixels(ImWidth, ImHeight, pixformat, GL_UNSIGNED_BYTE, pixmap[0]);
}


//
// Routine to write the current framebuffer to an image file
//
void writeImage(string outfilename){
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

  if(saveAs != "") {
    writeImage(saveAs);
    saveAs = "";
  }
}

//
//  Keyboard Callback Routine: 'r' - read and display a new image,
//  'w' - write the current window to an image file, 'q' or ESC - quit
//
void handleKey(unsigned char key, int x, int y){
  switch(key){	
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



/*
   Main program to read an image file, then ask the user
   for transform information, transform the image and display
   it using the appropriate warp.  Optionally save the transformed
   images in  files.
*/
int main(int argc, char *argv[]){
	if(argc != 2 && argc != 3) {
		cerr << "incorrect usage. correct usage is: \"./warper in.ext [out.ext]\"" << endl;
		exit(-1);
  }

  if(argc == 3) {
    saveAs = argv[2];
  }

	// initialize transformation matrix to identity
	Matrix3D M;

	//your code to read in the input image
	readImage(argv[1]);

	//build the transformation matrix based on user input
	read_input(M);

	// printout the final matrix
	// cout << "Accumulated Matrix: " << endl;
	M.print();

	// your code to perform inverse mapping (4 steps)

  //find new corners
  // cout << "find new corners\n";
  Vector3D topLeft(0.0, 0.0, 0.0);
  Vector3D topRight(ImWidth, 0.0, 0.0);
  Vector3D bottomLeft(0.0, ImHeight, 0.0);
  Vector3D bottomRight(ImWidth, ImHeight, 0.0);

  Vector3D warpTopLeft = M * topLeft;
  Vector3D warpTopRight = M * topRight;
  Vector3D warpBottomLeft = M * bottomLeft;
  Vector3D warpBottomRight = M * bottomRight;

  //find new size
  // cout << "find new size\n";
  double xs[4] = {warpTopLeft.x, warpTopRight.x, warpBottomLeft.x, warpBottomRight.x};
  double right = *max_element(xs, xs + 4);
  double left = *min_element(xs, xs + 4);
  int newWidth = abs(right - left);
  double ys[4] = {warpTopLeft.y, warpTopRight.y, warpBottomLeft.y, warpBottomRight.y};
  double top = *max_element(ys, ys + 4);
  double bottom = *min_element(ys, ys + 4);
  int newHeight = abs(top - bottom);
  // cout << "top: " << top << "\n";
  // cout << "bottom: " << bottom << "\n";
  // cout << "left: " << left << "\n";
  // cout << "right: " << right << "\n";
  // cout << newHeight << " x " << newWidth << " (H x W)\n";

  //allocate space for pixmap of new size
  // cout << "allocate\n";
  warped = new Pixel*[newHeight];
  if(warped != NULL)
	warped[0] = new Pixel[newWidth * newHeight];
  for(int i = 1; i < newHeight; i++)
	warped[i] = warped[i - 1] + newWidth;

  Matrix3D invM = M.inverse();
  int u, v;

  //warp
  // cout << "warp\n";
  // cout << "origin: (" << left << ", " << bottom << ")\n";
  for(int y = 0; y < newHeight; y++) {
    for(int x = 0; x < newWidth; x++) {
      //map pixel coords
      //current coords + origin
      Vector3D outPixel(x + left, y + bottom, 1);
      Vector3D inPixel = invM * outPixel;

      //normalize
      u = inPixel.x / inPixel.z;
      v = inPixel.y / inPixel.z;

      //only copy the pixel if it exists in pixmap.
      if(u >= 0 && u < ImWidth && v >= 0 && v < ImHeight) {
        warped[y][x] = pixmap[v][u];
      }
    }
  }
  // cout << "origin: (" << left << ", " << bottom << ")\n";

  pixmap = warped;
  ImHeight = newHeight;
  ImWidth = newWidth;

	// start up the glut utilities
	glutInit(&argc, argv);
	
	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(ImWidth, ImHeight);
	glutCreateWindow("Warper");

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