// Ryan Painter CPSC 4040
// This program reads and displays image files. Read images can be color inverted, noisified, and saved.
#include <OpenImageIO/imageio.h>
#include <iostream>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
OIIO_NAMESPACE_USING

static int icolor = 0;

//struct that stores the red, green, blue, and alpha channel of a pixel
struct pixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

struct pixel** pixmapA;
unsigned int widthA;
unsigned int heightA;

struct pixel** pixmapB;
unsigned int widthB;
unsigned int heightB;

//read from an image file and convert it to a pixmap with red, green, blue, and alpha channels
//this is awful i'm sorry
void readImages(string fileNameA, string fileNameB) {
  
  //code from https://openimageio.readthedocs.io/en/release-2.1.20.0/imageinput.html
  auto in = ImageInput::open (fileNameA);
  if (! in)
    return;
  const ImageSpec &specA = in->spec();
  widthA = specA.width;
  heightA = specA.height;
  int channels = specA.nchannels;
  vector<unsigned char> pixels (widthA*heightA*channels);
  in->read_image (TypeDesc::UINT8, &pixels[0]);
  in->close ();

  //allocation for pixmap
  pixmapA = new pixel * [heightA];
  pixmapA[0] = new pixel[widthA * heightA];

  for (int i = 1; i < heightA; i++) {
    pixmapA[i] = pixmapA[i-1] + widthA;
  }

  //iterates through the pixmap and pixels vector copying all color values into their corresponding pixel in pixmap
  int i = 0;
  for (int r = 0; r < heightA; r++) {
    for (int c = 0; c < widthA; c++) {
      pixmapA[r][c].red = pixels[i++];

      //if image is greyscale, copy red value into blue and green to maintain color
      if(channels == 1) {
        pixmapA[r][c].green = pixmapA[r][c].red;
        pixmapA[r][c].blue = pixmapA[r][c].red;
      }
      //else read the green and blue channels
      else {
        pixmapA[r][c].green = pixels[i++];
        pixmapA[r][c].blue = pixels[i++];
      }

      //if image has alpha channel read it
      if(channels == 4) {
        pixmapA[r][c].alpha = pixels[i++];
      }
      //else set to max oppacity
      else {
        pixmapA[r][c].alpha = 255;
      }
    }
  }
  cout << "read A" << endl;

  in = ImageInput::open (fileNameB);
  if (! in)
    return;
  const ImageSpec &specB = in->spec();
  widthB = specB.width;
  heightB = specB.height;
  channels = specB.nchannels;
  in->read_image (TypeDesc::UINT8, &pixels[0]);
  in->close ();

  //allocation for pixmap
  pixmapB = new pixel * [heightB];
  pixmapB[0] = new pixel[widthB * heightB];

  for (int i = 1; i < heightB; i++) {
    pixmapB[i] = pixmapB[i-1] + widthB;
  }

  //iterates through the pixmap and pixels vector copying all color values into their corresponding pixel in pixmap
  i = 0;
  for (int r = 0; r < heightB; r++) {
    for (int c = 0; c < widthB; c++) {
      pixmapB[r][c].red = pixels[i++];

      //if image is greyscale, copy red value into blue and green to maintain color
      if(channels == 1) {
        pixmapB[r][c].green = pixmapB[r][c].red;
        pixmapB[r][c].blue = pixmapB[r][c].red;
      }
      //else read the green and blue channels
      else {
        pixmapB[r][c].green = pixels[i++];
        pixmapB[r][c].blue = pixels[i++];
      }

      //if image has alpha channel read it
      if(channels == 4) {
        pixmapB[r][c].alpha = pixels[i++];
      }
      //else set to max oppacity
      else {
        pixmapB[r][c].alpha = 255;
      }
    }
  }

  cout << "read B" << endl;
}

//composites image A and B and stores the result into pixmapA
void compose() {
  //iterate through pixmapA and do the over operation
  double rA, gA, bA, aA, rB, gB, bB, aB;
  for (int r = 0; r < heightA; r++) {
    for (int c = 0; c < widthA; c++) {
      //convert to premultiplied
      aA = pixmapA[r][c].alpha / 255.0;
      rA = pixmapA[r][c].red * aA;
      gA = pixmapA[r][c].green * aA;
      bA = pixmapA[r][c].blue * aA;
      aB = pixmapB[r][c].alpha / 255.0;
      rB = pixmapB[r][c].red * aB;
      gB = pixmapB[r][c].green * aB;
      bB = pixmapB[r][c].blue * aB;

      //A over B formula
      pixmapA[r][c].red = (rA + (1 - aA) * rB);
      pixmapA[r][c].green = (gA + (1 - aA) * gB);
      pixmapA[r][c].blue = (bA + (1 - aA) * bB);
      pixmapA[r][c].alpha = (aA + (1 - aA) * aB) * 255;
    }
  }
}


//read the current image in the frame buffer and saves to the file given by the user
void writeImage(string outfilename){
  unsigned char pixels[4 * widthA * heightA];

  // create the oiio file handler for the image
  std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  //move each color channel into pixels to be saved
  int counter = 0;
  for (int r = 0; r < heightA; r++) {
    for (int c = 0; c < widthA; c++) {
      pixels[counter++] = pixmapA[r][c].red;
      pixels[counter++] = pixmapA[r][c].green;
      pixels[counter++] = pixmapA[r][c].blue;
      pixels[counter++] = pixmapA[r][c].alpha;
    }
  }

  // open a file for writing the image. The file header will indicate an image of
  // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
  // type unsigned char
  ImageSpec spec(widthA, heightA, 4, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    return;
  }

  // write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars
  if(!outfile->write_image(TypeDesc::UINT8, pixels)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  else
    cout << "File saved" << endl;
  
  // close the image file after the image is written
  if(!outfile->close()){
    cerr << "Could not close " << outfilename << ", error = " << geterror() << endl;
    return;
  }
}

//handles the rendering of images to the viewport
void renderImage(){

  //copy the 2d array of pixels structs into a 1d array of unsigned char
  unsigned char pixels[widthA * heightA * 4];
  int i = 0;
  for (int r = 0; r < heightA; r++) {
    for (int c = 0; c < widthA; c++) {
      pixels[i++] = pixmapA[r][c].red;
      pixels[i++] = pixmapA[r][c].green;
      pixels[i++] = pixmapA[r][c].blue;
      pixels[i++] = pixmapA[r][c].alpha;
    }
  }

  //clear the buffer
  glClear(GL_COLOR_BUFFER_BIT);
  //flip the image upright
  glPixelZoom(1, -1);
  //shift the image up
  glRasterPos2d(0, heightA);
  //draw the image
  glDrawPixels(widthA, heightA, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  //flush the buffer to the viewport
  glFlush();
  //resize window to fit image
  glutReshapeWindow(widthA, heightA);
}

/*
   Reshape Callback Routine: sets up the viewport and drawing coordinates
   This routine is called when the window is created and every time the window
   is resized, by the program or by the user
*/
void handleReshape(int w, int h){
  // set the viewport to be the entire window
  glViewport(0, 0, w, h);
  
  // define the drawing coordinate system on the viewport
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
}

/*
   Main program to draw the square, change colors, and wait for quit
*/
int main(int argc, char* argv[]){
  
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(widthA, heightA);
  glutCreateWindow("Compose");

  // set up the callback routines to be called when glutMainLoop() detects
  // an event
  glutDisplayFunc(renderImage);	  // display callback
  glutReshapeFunc(handleReshape); // window resize callback

  if(argc == 3 || argc == 4) {
    cout << "start" << endl;
    readImages(argv[1], argv[2]);
    cout << "read" << endl;
    if(heightA > heightB || widthA > widthB) {
      cout << "Image B must be larger than Image A" << endl;
      //image B must be at least as big as image A
      return -1;
    }

    compose();
    cout << "composed" << endl;
    renderImage();
    cout << "rendered" << endl;

    if(argc == 4) {
      writeImage(argv[3]);
    }
  }
  else {
    cout << "Incorrect number of arguments" << endl;
    return -1;
  }
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  return 0;
}