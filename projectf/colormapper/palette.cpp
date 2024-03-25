/*
  Palette swapping program
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <GL/glut.h>
#include <OpenImageIO/imageio.h>

using namespace std;
using std::string;
OIIO_NAMESPACE_USING

static int icolor = 0;

struct Pixel{ // defines a pixel structure
	unsigned char r,g,b,a;
}; 

bool operator==(const Pixel& a, const Pixel& b)
{
  return  a.r == b.r &&
          a.g == b.g &&
          a.b == b.b &&
          a.a == b.a;
}

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

Pixel **pixmap = NULL;  // the image pixmap for the loaded image
Pixel **paletteSwappedPixmap = NULL; // the palette swapped pixmap used for OpenGL display
vector<Pixel> palette;
vector<Pixel> generatedPalette;
bool useGeneratedPalette = false;
int pixformat; 			// the pixel format used to correctly  draw the image

void reset() {
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      paletteSwappedPixmap[r][c].r = pixmap[r][c].r;
      paletteSwappedPixmap[r][c].g = pixmap[r][c].g;
      paletteSwappedPixmap[r][c].b = pixmap[r][c].b;
      paletteSwappedPixmap[r][c].a = pixmap[r][c].a;
    }
  }
}

void medianCut(vector<Pixel> bucket, int depth) {
  int size = bucket.size();

  if(depth == 0) {
    int rAverage, gAverage, bAverage;
    rAverage = gAverage = bAverage = 0;

    for(int i = 0; i < size; i++) {
      rAverage += bucket[i].r;
      gAverage += bucket[i].g;
      bAverage += bucket[i].b;
    }

    Pixel tempPixel;
    tempPixel.r = rAverage / size;
    tempPixel.g = gAverage / size;
    tempPixel.b = bAverage / size;

    generatedPalette.push_back(tempPixel);
  }
  else {
    int rLow, rHigh, gLow, gHigh, bLow, bHigh;
    rLow = rHigh = bucket[0].r;
    gLow = gHigh = bucket[0].g;
    bLow = bHigh = bucket[0].b;

    for(int i = 1; i < size; i++) {
      if(bucket[i].r < rLow) rLow = bucket[i].r;
      else if(bucket[i].r > rHigh) rHigh = bucket[i].r;

      if(bucket[i].r < gLow) gLow = bucket[i].g;
      else if(bucket[i].r > gHigh) gHigh = bucket[i].g;

      if(bucket[i].r < bLow) bLow = bucket[i].b;
      else if(bucket[i].r > bHigh) bHigh = bucket[i].b;
    }

    int rRange = rHigh - rLow;
    int gRange = gHigh - gLow;
    int bRange = bHigh - bLow;

    int pos;
    for(int i = 0; i < size; i++) {
      pos = i;

      for(int j = i + 1; j < size; j++) {
        if(rRange > gRange && rRange > bRange) {
          if(bucket[j].r < bucket[pos].r) {
            pos = j;
          }
        }
        else if(gRange > bRange) {
          if(bucket[j].g < bucket[pos].g) {
            pos = j;
          }
        }
        else {
          if(bucket[j].b < bucket[pos].b) {
            pos = j;
          }
        }
      }

      if(pos != i) {
        iter_swap(bucket.begin() + pos, bucket.begin() + i);
      }
    }

    vector<Pixel> firstHalf;
    vector<Pixel> secondHalf;
    for(int i = 0; i < size; i++) {
      if(i < size / 2) {
        firstHalf.push_back(bucket[i]);
      }
      else {
        secondHalf.push_back(bucket[i]);
      }
    }
    medianCut(firstHalf, depth - 1);
    medianCut(secondHalf, depth - 1);
  }
}

void generatePalette() {
  generatedPalette.clear();
  cout << "Input color depth (must be a power of 2): ";
  int depth;
  cin >> depth;
  cout << "Generating... (This may take several minutes)\n";
  vector<Pixel> bucket;
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      bucket.push_back(pixmap[r][c]);
    }
  }

  medianCut(bucket, (int)log2(depth));
  cout << "Palette generated.\n";
}

Pixel findClosestColor(const Pixel& a) {
  int dif, minDif, pos;
  minDif = 255 * 4;

  if(useGeneratedPalette) {
    for(int i = 0; i < generatedPalette.size(); i++) {
      dif = sqrt(pow(a.r - generatedPalette[i].r, 2) + pow(a.g - generatedPalette[i].g, 2) + pow(a.b - generatedPalette[i].b, 2));

      if(dif < minDif) {
        minDif = dif;
        pos = i;
      }
    }

    return generatedPalette[pos];
  }
  else {
    for(int i = 0; i < palette.size(); i++) {
      dif = sqrt(pow(a.r - palette[i].r, 2) + pow(a.g - palette[i].g, 2) + pow(a.b - palette[i].b, 2));

      if(dif < minDif) {
        minDif = dif;
        pos = i;
      }
    }

    return palette[pos];
  }

  return a;
}

void simplePalette() {
  reset();
  Pixel tempPixel;  
  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      tempPixel = findClosestColor(paletteSwappedPixmap[r][c]);
      paletteSwappedPixmap[r][c] = tempPixel;
    }
  }
}

//Floyd steinberg dithering
//Difference in new and old pixel colors is distributed among neighboring pixels using the following matrix
//0 0 0
//0 X 7
//3 5 1
void ditheringPalette() {
  reset();

  Pixel oldPixel, newPixel, error;

  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      oldPixel.r = paletteSwappedPixmap[r][c].r;
      oldPixel.g = paletteSwappedPixmap[r][c].g;
      oldPixel.b = paletteSwappedPixmap[r][c].b;

      newPixel = findClosestColor(oldPixel);
      paletteSwappedPixmap[r][c].r = newPixel.r;
      paletteSwappedPixmap[r][c].g = newPixel.g;
      paletteSwappedPixmap[r][c].b = newPixel.b;

      error.r = oldPixel.r - newPixel.r;
      error.g = oldPixel.g - newPixel.g;
      error.b = oldPixel.b - newPixel.b;

      //skip applying error to out of bounds pixels
      if(c < ImWidth - 1) {
        paletteSwappedPixmap[r][c + 1].r += error.r * (7.0 / 16.0);
        paletteSwappedPixmap[r][c + 1].g += error.g * (7.0 / 16.0);
        paletteSwappedPixmap[r][c + 1].b += error.b * (7.0 / 16.0);
      }
      if(r < ImHeight - 1) {        
        if(c > 0) {
          paletteSwappedPixmap[r + 1][c - 1].r += error.r * (3.0 / 16.0);
          paletteSwappedPixmap[r + 1][c - 1].g += error.g * (3.0 / 16.0);
          paletteSwappedPixmap[r + 1][c - 1].b += error.b * (3.0 / 16.0);
        }

        paletteSwappedPixmap[r + 1][c].r += error.r * (5.0 / 16.0);
        paletteSwappedPixmap[r + 1][c].g += error.g * (5.0 / 16.0);
        paletteSwappedPixmap[r + 1][c].b += error.b * (5.0 / 16.0);

        if(c < ImWidth - 1) {
          paletteSwappedPixmap[r + 1][c + 1].r += error.r / 16.0;
          paletteSwappedPixmap[r + 1][c + 1].g += error.g / 16.0;
          paletteSwappedPixmap[r + 1][c + 1].b += error.b / 16.0;
        }
      }

      // if(paletteSwappedPixmap[r][c].r > 255) paletteSwappedPixmap[r][c].r = 255;
      // else if(paletteSwappedPixmap[r][c].r < 0) paletteSwappedPixmap[r][c].r = 0;

      // if(paletteSwappedPixmap[r][c].g > 255) paletteSwappedPixmap[r][c].g = 255;
      // else if(paletteSwappedPixmap[r][c].g < 0) paletteSwappedPixmap[r][c].g = 0;
      
      // if(paletteSwappedPixmap[r][c].b > 255) paletteSwappedPixmap[r][c].b = 255;
      // else if(paletteSwappedPixmap[r][c].b < 0) paletteSwappedPixmap[r][c].b = 0;

      // if(c < ImWidth - 1) {
      //   paletteSwappedPixmap[r][c + 1].r = min(paletteSwappedPixmap[r][c + 1].r + (error.r * (7.0 / 16.0)), 255.0);
      //   paletteSwappedPixmap[r][c + 1].g = min(paletteSwappedPixmap[r][c + 1].g + (error.g * (7.0 / 16.0)), 255.0);
      //   paletteSwappedPixmap[r][c + 1].b = min(paletteSwappedPixmap[r][c + 1].b + (error.b * (7.0 / 16.0)), 255.0);
      // }
      // if(r < ImHeight - 1) {        
      //   if(c > 0) {
      //     paletteSwappedPixmap[r + 1][c - 1].r = min(paletteSwappedPixmap[r + 1][c - 1].r + (error.r * (3.0 / 16.0)), 255.0);
      //     paletteSwappedPixmap[r + 1][c - 1].g = min(paletteSwappedPixmap[r + 1][c - 1].g + (error.g * (3.0 / 16.0)), 255.0);
      //     paletteSwappedPixmap[r + 1][c - 1].b = min(paletteSwappedPixmap[r + 1][c - 1].b + (error.b * (3.0 / 16.0)), 255.0);
      //   }

      //   paletteSwappedPixmap[r + 1][c].r = min(paletteSwappedPixmap[r + 1][c].r + (error.r * (5.0 / 16.0)), 255.0);
      //   paletteSwappedPixmap[r + 1][c].g = min(paletteSwappedPixmap[r + 1][c].g + (error.g * (5.0 / 16.0)), 255.0);
      //   paletteSwappedPixmap[r + 1][c].b = min(paletteSwappedPixmap[r + 1][c].b + (error.b * (5.0 / 16.0)), 255.0);

      //   if(c < ImWidth - 1) {
      //     paletteSwappedPixmap[r + 1][c + 1].r = min(paletteSwappedPixmap[r + 1][c + 1].r + (error.r * (1.0 / 16.0)), 255.0);
      //     paletteSwappedPixmap[r + 1][c + 1].g = min(paletteSwappedPixmap[r + 1][c + 1].g + (error.g * (1.0 / 16.0)), 255.0);
      //     paletteSwappedPixmap[r + 1][c + 1].b = min(paletteSwappedPixmap[r + 1][c + 1].b + (error.b * (1.0 / 16.0)), 255.0);
      //   }
      // }
    }
  }
}

void readPalette(string filename) {
  std::unique_ptr<ImageInput> infile = ImageInput::open(filename);
  //read palette as a text file
  if(!infile) {
    fstream infile(filename);
    string hexcode;
    Pixel tempPixel;

    while(getline(infile, hexcode)) {
      transform(hexcode.begin(), hexcode.end(), hexcode.begin(), ::toupper);
      tempPixel.r = stoi(hexcode.substr(0, 2), 0, 16);
      tempPixel.g = stoi(hexcode.substr(2, 2), 0, 16);
      tempPixel.b = stoi(hexcode.substr(4, 2), 0, 16);
      tempPixel.a = 255;

      if(!(find(palette.begin(), palette.end(), tempPixel) != palette.end())) {
          palette.push_back(tempPixel);
      }
    }
  }
  //read palette as an image
  else {
    //Record image width, height and number of channels into local variable
    int width = infile->spec().width;
    int height = infile->spec().height;
    int channels = infile->spec().nchannels;

  
    // allocate temporary structure to read the image 
    unsigned char tmp_pixels[width * height * channels];

    // read the image into the tmp_pixels from the input file, flipping it upside down using negative y-stride,
    // since OpenGL pixmaps have the bottom scanline first, and 
    // oiio expects the top scanline first in the image file.
    int scanlinesize = width * channels * sizeof(unsigned char);
    if(!infile->read_image(TypeDesc::UINT8, &tmp_pixels[0] + (height - 1) * scanlinesize, AutoStride, -scanlinesize)){
      cerr << "Could not read image from " << filename << ", error = " << geterror() << endl;
      return;
    }
  
    //  assign the read pixels to the the data structure
    int index;
    Pixel tempPixel;
    for(int row = 0; row < height; ++row) {
      for(int col = 0; col < width; ++col) {
        index = (row*width+col)*channels;

        if (ImChannels==1) {

        tempPixel.r = tmp_pixels[index];
        tempPixel.g = tmp_pixels[index];
        tempPixel.b = tmp_pixels[index];
        tempPixel.a = 255;
        }
        else {
          tempPixel.r = tmp_pixels[index];
          tempPixel.g = tmp_pixels[index+1];
          tempPixel.b = tmp_pixels[index+2];			
          if (ImChannels <4) // no alpha value is present so set it to 255
            tempPixel.a = 255; 
          else // read the alpha value
            tempPixel.a = tmp_pixels[index+3];			
        }

        if(!(find(palette.begin(), palette.end(), tempPixel) != palette.end())) {
          palette.push_back(tempPixel);
        }
      }
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
  paletteSwappedPixmap = new Pixel*[ImHeight];
  if(paletteSwappedPixmap != NULL)
	paletteSwappedPixmap[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	paletteSwappedPixmap[i] = paletteSwappedPixmap[i - 1] + ImWidth;

  for(int r = 0; r < ImHeight; r++) {
    for(int c = 0; c < ImWidth; c++) {
      paletteSwappedPixmap[r][c].r = pixmap[r][c].r;
      paletteSwappedPixmap[r][c].g = pixmap[r][c].g;
      paletteSwappedPixmap[r][c].b = pixmap[r][c].b;
      paletteSwappedPixmap[r][c].a = pixmap[r][c].a;
    }
  }

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
  glDrawPixels(ImWidth, ImHeight, pixformat, GL_UNSIGNED_BYTE, paletteSwappedPixmap[0]);
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
      reset();
      glutPostRedisplay();
      break;

    case 's':
    case 'S':
      simplePalette();
      glutPostRedisplay();
      break;

    case 'g':
    case 'G':
      generatePalette();
      break;

    case 'd':
    case 'D':
      ditheringPalette();
      glutPostRedisplay();
      break;

    case 'c':
    case 'C':
      if(palette.empty()) {
        cout << "No palette was provided at startup. Please restart the program to load a custom palette, or continue use with the computer generated palette.\n";
      }
      else {
        if(generatedPalette.empty()) {
          cout << "Please press g to generate a palette.\n"; 
        }
        else {
          useGeneratedPalette = !useGeneratedPalette;
          if(useGeneratedPalette) {
            glutSetWindowTitle("Palette Swapper (Generated Palette)");
          }
          else {
            glutSetWindowTitle("Palette Swapper (Custom Palette)");
          }
        }
      }
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
	if(argc == 2) {
    useGeneratedPalette = true;
  }
  else if(argc == 3) {
    readPalette(argv[2]);
  }
  else {
    cerr << "incorrect usage. correct usage is: \"./palette in.ext [palette.ext]\"" << endl;
		exit(-1);
  }

	//your code to read in the input image
	readImage(argv[1]);

  

	// start up the glut utilities
	glutInit(&argc, argv);
	
	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(ImWidth, ImHeight);
	glutCreateWindow("Palette Swapper");

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