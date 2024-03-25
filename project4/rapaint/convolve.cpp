//   Author: Ioannis Karamouzas Date: 09/15/2021  
//   Sample Solution to Project 1: Get the Picture!
//   OpenGL/GLUT Program using OpenImageIO to Read and Write Images, and OpenGL to display
//
//   The program responds to the following keyboard commands:
//    r or R: prompt for an input image file name, read the image into
//	    an appropriately sized pixmap, resize the window, and display
//
//    w or W: prompt for an output image file name, read the display into a pixmap,
//	    and write from the pixmap to the file.
//
//    i or I: invert the colors of the displayed image.
//
//    q, Q or ESC: quit.
//
//   When the window is resized by the user: If the size of the window becomes bigger than
//   the image, the image is centered in the window. If the size of the window becomes
//   smaller than the image, the image is uniformly scaled down to the largest size that
//   fits in the window.
//


#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <GL/glut.h>

using namespace std;
OIIO_NAMESPACE_USING


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
Pixel **original = NULL; // original image pixmap
int pixformat; 			// the pixel format used to correctly  draw the image

vector<vector<float>> kernel; //kernel
vector<vector<float>> normalizedKernel; //normalized kernel

string saveAs = ""; //name of the saved file


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

  //allocate space for a copy of the original
  original = new Pixel*[ImHeight];
  if(original != NULL)
  original[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
  original[i] = original[i - 1] + ImWidth;

  //copy over pixels for original
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      original[r][c].r = pixmap[r][c].r;
      original[r][c].g = pixmap[r][c].g;
      original[r][c].b = pixmap[r][c].b;
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

void readFilter(string fileName) {
  //open specified filter
  ifstream file;
  file.open(fileName);
  if(!file) {
    cerr << "Could not input filter file " << fileName << ", error = " << geterror() << endl;
    return;
  }

  //store size
  int N;
  file >> N;

  float tempInt;
  vector<float> tempVect;

  //read in values storing them in 2d vector kernel
  for(int r = 0; r < N; r++) {
    for(int c = 0; c < N; c++) {
      file >> tempInt;
      tempVect.push_back(tempInt);
    }
    kernel.push_back(tempVect);
    tempVect.clear();
  }
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

//reflects kernel across x and y axis
void reflectKernel() {
  //reflect inner vectors
  for(int r = 0; r < kernel.size(); r++) {
    reverse(kernel[r].begin(), kernel[r].end());
  }
  //reflect outer vector
  reverse(kernel.begin(), kernel.end());
}

//calculates rescale factor for kernel
void calculateRescale(){
  float rescaleFactor;
  float negSum = 0;
  float posSum = 0;


  //find magnitude of positive and negative weight
  for(int i = 0; i < kernel.size(); i++) {
    for(int j = 0; j < kernel.size(); j++) {
      if(kernel[i][j] < 0) {
        negSum -= kernel[i][j];
      }
      else {
        posSum += kernel[i][j];
      }
    }
  }


  //calculate rescale based on largest sum
  if(negSum > posSum) {
    rescaleFactor = 1.0 / negSum;
  }
  else {
    rescaleFactor = 1.0 / posSum;
  }

  //normalize kernel
  vector<float> tempVect;
  for(int i = 0; i < kernel.size(); i++) {
    for(int j = 0; j < kernel.size(); j++) {
      tempVect.push_back(rescaleFactor * kernel[i][j]);
    }
    normalizedKernel.push_back(tempVect);
    tempVect.clear();
  }
}

//convolves image and filter
void convolvesImage(){
  int N = normalizedKernel.size();
  int sumR;
  int sumG;
  int sumB;
  int currR;
  int currC;

  //allocate space for temporary image copy
  Pixel **temp;
  temp = new Pixel*[ImHeight];
  if(temp != NULL)
	temp[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	temp[i] = temp[i - 1] + ImWidth;

  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      temp[r][c].r = pixmap[r][c].r;
      temp[r][c].g = pixmap[r][c].g;
      temp[r][c].b = pixmap[r][c].b;
    }
  }

  //convolution
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      sumR = 0;
      sumG = 0;
      sumB = 0;

      for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
          currR = r + i - (N / 2);
          currC = c + j - (N / 2);

          if(currR >= 0 && currR < ImHeight && currC >= 0 && currC < ImWidth) {
            sumR += temp[currR][currC].r * normalizedKernel[i][j];
            sumG += temp[currR][currC].g * normalizedKernel[i][j];
            sumB += temp[currR][currC].b * normalizedKernel[i][j];
          }
        }
      }

      //clamp sums to 0 to 255
      sumR = clamp(sumR, 0, 255);
      sumG = clamp(sumG, 0, 255);
      sumB = clamp(sumB, 0, 255);

      pixmap[r][c].r = sumR;
      pixmap[r][c].g = sumG;
      pixmap[r][c].b = sumB; 
	  }
  }
}

//reload original image
void reloadImage() {
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      pixmap[r][c].r = original[r][c].r;
      pixmap[r][c].g = original[r][c].g;
      pixmap[r][c].b = original[r][c].b;
    }
  }
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
  string infilename, outfilename;
  int ok;
  
  switch(key){
    case 'r':		// 'r' - read an image from a file
    case 'R':
      reloadImage();
      glutPostRedisplay();
      break;
      
    case 'w':		// 'w' - write the image to a file
    case 'W':
      if(saveAs != "") {
        writeImage(saveAs);
      }
      break;
      
    case 'c':		// 'i' - invert the image
    case 'C':
      convolvesImage();       
	    glutPostRedisplay();
      break;
    
	
	  case 'q':		// q or ESC - quit
    case 'Q':
    case 27:
      destroy();
      exit(0);

    case 't':
    case 'T':
      glutPostRedisplay();
      break;
      
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

//
// Main program to scan the commandline, set up GLUT and OpenGL, and start Main Loop
//
int main(int argc, char* argv[]){
  // scan command line and process
  // only one parameter allowed, an optional image filename and extension
  if(argc != 3 && argc != 4){
    cout << "usage: convolve filter.filt in.ext [out.ext]" << endl;
    exit(1);
  }

  if(argc == 4) {
    saveAs = argv[3];
  }

  readFilter(argv[1]);
  reflectKernel();
  calculateRescale();
  readImage(argv[2]);
  
  // set up the default window and empty pixmap if no image or image fails to load
  WinWidth = ImWidth;
  WinHeight = ImHeight;
  
  // start up GLUT
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(WinWidth, WinHeight);
  glutCreateWindow("Image Convolver");
  
  // set up the callback routines
  glutDisplayFunc(handleDisplay); // display update callback
  glutKeyboardFunc(handleKey);	  // keyboard key press callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  
  // Enter GLUT's event loop
  glutMainLoop();
  return 0;
}
