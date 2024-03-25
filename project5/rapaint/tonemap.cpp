// Ryan Painter CPSC 4040
// This program reads and displays image files. Read images can be color inverted, noisified, and saved.
#include <OpenImageIO/imageio.h>
#include <iostream>
#include <math.h>

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
  float r;
  float g;
  float b;
  float a;
};

bool displayOriginal = false;
struct pixel** pixmap;
struct pixel** original;
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
  vector<float> pixels (width*height*channels);
  in->read_image (TypeDesc::FLOAT, &pixels[0]);
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
      pixmap[r][c].r = pixels[i++];

      //if image is greyscale, copy red value into blue and green to maintain color
      if(channels == 1) {
        pixmap[r][c].g = pixmap[r][c].r;
        pixmap[r][c].b = pixmap[r][c].r;
      }
      //else read the green and blue channels
      else {
        pixmap[r][c].g = pixels[i++];
        pixmap[r][c].b = pixels[i++];
      }

      //if image has alpha channel read it
      if(channels == 4) {
        pixmap[r][c].a = pixels[i++];
      }
      //else set to max oppacity
      else {
        pixmap[r][c].a = 255;
      }
    }
  }

  original = new pixel * [height];
  original[0] = new pixel[width * height];

  for (int i = 1; i < height; i++) {
    original[i] = original[i-1] + width;
  }

  for(int r = 0; r < height; r++) {
    for(int c = 0; c < width; c++) {
      original[r][c].r = pixmap[r][c].r;
      original[r][c].g = pixmap[r][c].g;
      original[r][c].b = pixmap[r][c].b;
      original[r][c].a = pixmap[r][c].a;
    }
  }
}

void resetImage() {
  for(int r = 0; r < height; r++) {
    for(int c = 0; c < width; c++) {
      pixmap[r][c].r = original[r][c].r;
      pixmap[r][c].g = original[r][c].g;
      pixmap[r][c].b = original[r][c].b;
      pixmap[r][c].a = original[r][c].a;
    }
  }
}

void baselineToneMapping() {
  float luminance = 0;
  float displayLuminance = 0;

  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      //find luminance for each pixel using the luma function from Y'UV space and use it find compressed luminance
      luminance = (0.299 * pixmap[r][c].r) + (0.587 * pixmap[r][c].g) + (0.114 * pixmap[r][c].b);
      displayLuminance = luminance / (luminance + 1);

      pixmap[r][c].r = (displayLuminance/luminance) * pixmap[r][c].r;
      pixmap[r][c].g = (displayLuminance/luminance) * pixmap[r][c].g;
      pixmap[r][c].b = (displayLuminance/luminance) * pixmap[r][c].b;
    }
  }
}

void gammaCompression() {
  float gamma;
  char response;

  //prompt user to either take a gamma from them or use a default
  bool run = true;
  while(run) {
    cout << "would you like to use a custom gamma value? (Y/N): ";
    cin >> response;
    switch(response) {
      case 'y':
      case 'Y':
        run = false;
        cout << "enter a gamma value: ";
        cin >> gamma;
        break;
      case 'n':
      case 'N':
        run = false;
        //good approximate for gamma correciton. could be better if program could read the monitors gamma display range.
        gamma = 1.0 / 2.2;
        break;
      default:
        cout << "enter a valid response" << endl;
        break;
    }
  }

  float luminance = 0;
  float displayLuminance = 0;

  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      //find luminance for each pixel using the luma function from Y'UV space and use it find compressed luminance
      luminance = (0.299 * pixmap[r][c].r) + (0.587 * pixmap[r][c].g) + (0.114 * pixmap[r][c].b);
      displayLuminance = pow(luminance, gamma);

      pixmap[r][c].r = (displayLuminance/luminance) * pixmap[r][c].r;
      pixmap[r][c].g = (displayLuminance/luminance) * pixmap[r][c].g;
      pixmap[r][c].b = (displayLuminance/luminance) * pixmap[r][c].b;
    }
  }
}

//handles the rendering of images to the viewport
void renderImage(){

  //copy the 2d array of pixels structs into a 1d array of float
  float pixels[width * height * 4];
  int i = 0;
  if(displayOriginal) {
    for (int r = 0; r < height; r++) {
      for (int c = 0; c < width; c++) {
        pixels[i++] = original[r][c].r;
        pixels[i++] = original[r][c].g;
        pixels[i++] = original[r][c].b;
        pixels[i++] = original[r][c].a;
      }
    }
  }
  else {
    for (int r = 0; r < height; r++) {
      for (int c = 0; c < width; c++) {
        pixels[i++] = pixmap[r][c].r;
        pixels[i++] = pixmap[r][c].g;
        pixels[i++] = pixmap[r][c].b;
        pixels[i++] = pixmap[r][c].a;
      }
    }
  }

  //clear the buffer
  glClear(GL_COLOR_BUFFER_BIT);
  //flip the image upright
  glPixelZoom(1, -1);
  //shift the image up
  glRasterPos2d(0, height);
  //draw the image
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, pixels);
  //flush the buffer to the viewport
  glFlush();
  //resize window to fit image
  glutReshapeWindow(width, height);
}

//read the current image in the frame buffer and saves to the file given by the user
void writeImage(){
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  float pixels[4 * w * h];
  string outfilename;

  // get a filename for the image. The file suffix should indicate the image file
  // type. For example: output.png to create a png image file named output
  cout << "enter output image filename: ";
  cin >> outfilename;

  outfilename = outfilename + ".png";

  // create the oiio file handler for the image
  std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  // get the current pixels from the OpenGL framebuffer and store in pixmap
  glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, pixels);

  //flip image
  //allocation for temp pixmap
  pixel **temp = new pixel * [height];
  temp[0] = new pixel[width * height];
  for (int i = 1; i < height; i++) {
    temp[i] = temp[i-1] + width;
  }

  //copy read pixels to temp
  int i = 0;
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      temp[r][c].r = pixels[i++];
      temp[r][c].g = pixels[i++];
      temp[r][c].b = pixels[i++];
      temp[r][c].a = pixels[i++];
    }
  }

  //flip temp across x axis
  float red, green, blue, alpha;
  for(int r = 0; r < height / 2; r++) {
    for(int c = 0; c < width; c++) {
      red = temp[r][c].r;
      green = temp[r][c].g;
      blue = temp[r][c].b;
      alpha = temp[r][c].a;

      temp[r][c].r = temp[height - 1 - r][c].r;
      temp[r][c].g = temp[height - 1 - r][c].g;
      temp[r][c].b = temp[height - 1 - r][c].b;
      temp[r][c].a = temp[height - 1 - r][c].a;

      temp[height - 1 - r][c].r = red;
      temp[height - 1 - r][c].g = green;
      temp[height - 1 - r][c].b = blue;
      temp[height - 1 - r][c].a = alpha;
    }
  }

  //move flipped pixmap back into pixels
  i = 0;
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      pixels[i++] = temp[r][c].r;
      pixels[i++] = temp[r][c].g;
      pixels[i++] = temp[r][c].b;
      pixels[i++] = temp[r][c].a;
    }
  }

  // open a file for writing the image. The file header will indicate an image of
  // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
  // type float
  ImageSpec spec(w, h, 4, TypeDesc::FLOAT);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    return;
  }

  // write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars
  if(!outfile->write_image(TypeDesc::FLOAT, pixels)){
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

/*
   Keyboard Callback Routine: 
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
  string fileName;

  switch(key){     
    case 'r': // r - reset image
    case 'R':
      resetImage();
      glutPostRedisplay();
      break;

    case 'w': // w - write image
    case 'W':
      writeImage();
      glutPostRedisplay();
      break;

    case 'b': //b - apply baseline tone mapping
    case 'B':
      baselineToneMapping();
      glutPostRedisplay();
      break;

    case 'g': //g - apply gamma compression
    case 'G':
      gammaCompression();
      glutPostRedisplay();
      break;

    case 't': //t - toggle between edited and unedited image
    case 'T':
      displayOriginal = !displayOriginal;
      if(displayOriginal) {
        glutSetWindowTitle("Tone Mapper (original)");
      }
      else {
        glutSetWindowTitle("Tone Mapper (edit)");
      }
      glutPostRedisplay();
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
  if(argc != 2) {
    cerr << "incorrect usage. correct usage is: \"./tonemap <filename>\"" << endl;
    exit(-1);
  }

  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Tone Mapper (edit)");

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