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

//read the current image in the frame buffer and saves to the file given by the user
void writeImage(string outfilename){
  unsigned char pixels[4 * width * height];

  // create the oiio file handler for the image
  std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }
  
  //move each color channel into pixels to be saved
  int counter = 0;
  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      pixels[counter++] = pixmap[r][c].red;
      pixels[counter++] = pixmap[r][c].green;
      pixels[counter++] = pixmap[r][c].blue;
      pixels[counter++] = pixmap[r][c].alpha;
    }
  }

  // open a file for writing the image. The file header will indicate an image of
  // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
  // type unsigned char
  ImageSpec spec(width, height, 4, TypeDesc::UINT8);
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

//provided code for converting from rgb to hsv
//changed how to access the hsv values after running to fit better with my brain

/*
Input RGB colo r primary values : r , g, and b on scale 0 - 255
Output HSV colo rs : h on scale 0-360, s and v on scale -1
*/

#define maximum(x, y, z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x, y, z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

void RGBtoHSV(int r, int g, int b, double* ret){
  double red, green, blue;
  double h, s, v;
  double max, min, delta;
  red = r / 255.0; green = g / 255.0; blue = b / 255.0; /* r , g, b to 0 - 1 scale */
  max = maximum(red, green, blue);
  min = minimum(red, green, blue);
  v = max; /* value i s maximum of r , g, b */
  if (max == 0) { /* saturation and hue 0 i f value i s 0 */
    s = 0;
    h = 0;
  }
  else {
    s = (max - min) / max; /* saturation i s colo r pu ri ty on scale 0 - 1 */
    delta = max - min;

    if (delta == 0) { /* hue doesn't matter i f saturation i s 0 */
      h = 0;
    }
    else{
      if (red == max) { /* otherwise, determine hue on scale 0 - 360 */
        h = (green - blue) / delta;
      }
      else if (green == max) {
        h = 2.0 + (blue - red) / delta;
      }
      else {/* ( blue == max) */
        h = 4.0 + (red - green) / delta;
      }

      h = h * 60.0;
      if(h < 0) {
        h = h + 360.0;
      }
    }
  }

  ret[0] = h;
  ret[1] = s;
  ret[2] = v;
}

//these values determine the color to be masked
#define targetHue 120
#define hueVariance 55

#define targetSaturation 1
#define saturationVariance 0.65

#define targetValue 1
#define valueVariance 0.85

void mask() {
  double HSV[3];

  for (int r = 0; r < height; r++) {
    for (int c = 0; c < width; c++) {
      RGBtoHSV(pixmap[r][c].red, pixmap[r][c].green, pixmap[r][c].blue, HSV);

      //check to see if the color near the color we want to mask and set the alpha to 0 if it is
      if(HSV[0] < targetHue + hueVariance && HSV[0] > targetHue - hueVariance)
        if(HSV[1] < targetSaturation + saturationVariance && HSV[1] > targetSaturation - saturationVariance)
          if(HSV[2] < targetValue + valueVariance && HSV[2] > targetValue - valueVariance)
            pixmap[r][c].alpha = 0;
    }
  }
}

int main(int argc, char* argv[]) {
  if(argc != 3) {
    //throw wrong args error
    return -1;
  }

  readImage(argv[1]);
  mask();
  writeImage(argv[2]);
  
  return 0;
}