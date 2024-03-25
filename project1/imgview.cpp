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

struct pixel** pixmap;
unsigned int width;
unsigned int height;


//read from an image file and convert it to a pixmap with red, green, blue, and alpha channels
void readImage(string fileName) {
  
  //code from https://openimageio.readthedocs.io/en/release-2.1.20.0/imageinput.html
  auto in = ImageInput::open (fileName);
  if (! in)
    return;
  const ImageSpec &spec = in->spec();
  width = spec.width;
  height = spec.height;
  int channels = spec.nchannels;
  vector<unsigned char> pixels (width*height*channels);
  in->read_image (TypeDesc::UINT8, &pixels[0]);
  in->close ();

  //allocation for pixmap
  pixmap = new pixel * [height];
  pixmap[0] = new pixel[width * height];

  for (int i = 1; i < height; i++) {
    pixmap[i] = pixmap[i-1] + width;
  }

  //iterates through the pixmap and pixels vector copying all color values into their corresponding pixel in pixmap
  int i = 0;
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      pixmap[r][c].red = pixels[i++];

      //if image is greyscale, copy red value into blue and green to maintain color
      if(channels == 1) {
        pixmap[r][c].green = pixmap[r][c].red;
        pixmap[r][c].blue = pixmap[r][c].red;
      }
      //else read the green and blue channels
      else {
        pixmap[r][c].green = pixels[i++];
        pixmap[r][c].blue = pixels[i++];
      }

      //if image has alpha channel read it
      if(channels == 4) {
        pixmap[r][c].alpha = pixels[i++];
      }
      //else set to max oppacity
      else {
        pixmap[r][c].alpha = 255;
      }
    }
  }
}

//inverts the color of each pixel
void invertColors() {
  //iterate through pixmap and invert each color channel for each pixel
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      pixmap[r][c].red = 255 - pixmap[r][c].red;
      pixmap[r][c].green = 255 - pixmap[r][c].green;
      pixmap[r][c].blue = 255 - pixmap[r][c].blue;
    }
  }
}

//randomly sets 20% of pixels to black
void noisify() {
  //iterate through pixmap and for each pixel generate a number 1 - 10, if that number is 1 or 2 set the pixel to black
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      if((rand()/((RAND_MAX + 1u)/10)) <= 2) {
        pixmap[r][c].red = 0;
        pixmap[r][c].green = 0;
        pixmap[r][c].blue = 0;
      }
    }
  }
}

//read the current image in the frame buffer and saves to the file given by the user
void writeImage(){
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  unsigned char pixels[4 * w * h];
  string outfilename;

  // get a filename for the image. The file suffix should indicate the image file
  // type. For example: output.png to create a png image file named output
  cout << "enter output image filename: ";
  cin >> outfilename;

  // create the oiio file handler for the image
  std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // get the current pixels from the OpenGL framebuffer and store in pixmap
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // open a file for writing the image. The file header will indicate an image of
  // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
  // type unsigned char
  ImageSpec spec(w, h, 4, TypeDesc::UINT8);
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
  unsigned char pixels[width * height * 4];
  int i = 0;
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      pixels[i++] = pixmap[r][c].red;
      pixels[i++] = pixmap[r][c].green;
      pixels[i++] = pixmap[r][c].blue;
      pixels[i++] = pixmap[r][c].alpha;
    }
  }

  //clear the buffer
  glClear(GL_COLOR_BUFFER_BIT);
  //flip the image upright
  glPixelZoom(1, -1);
  //shift the image up
  glRasterPos2d(0, height);
  //draw the image
  glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  //flush the buffer to the viewport
  glFlush();
  //resize window to fit image
  glutReshapeWindow(width, height);
}

/*
   Keyboard Callback Routine: 
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
  string fileName;

  switch(key){     
    case 'r': // r - read image
    case 'R':
      cout << "enter input image filename: ";
      cin >> fileName;
      readImage(fileName);
      renderImage();
      break;

    case 'w': // w - write image
    case 'W':
      writeImage();
      break;

    case 'i': // i - invert image colors
    case 'I':
      invertColors();
      renderImage();
      break;

    case 'n': // n - noisify image (set random 20% of pixels to black)
    case 'N':
      noisify();
      renderImage();
      break;

    case 'q':		// q - quit
    case 'Q':
    case 27:		// esc - quit
      exit(0);

    default:		// not a valid key -- just ignore it
      return;
  }
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
  glutInitWindowSize(width, height);
  glutCreateWindow("Image viewer");

  // set up the callback routines to be called when glutMainLoop() detects
  // an event
  glutDisplayFunc(renderImage);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback

  if(argc == 2) {
    readImage(argv[1]);
    renderImage();
  }
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  return 0;
}