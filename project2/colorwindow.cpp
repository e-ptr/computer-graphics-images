// colorwindow.cpp
// Cem Yuksel

#include "colorwindow.h"
#include "math.h"

#define min(x1,x2)  ((x1)<(x2)?(x1):(x2))
#define max(x1,x2)  ((x1)>(x2)?(x1):(x2))


void RGBtoHSV (unsigned char r, unsigned char g, unsigned char b, float &h, float &s, float &v) {

	float red, green, blue;
	float maxc, minc, delta;

	// r, g, b to 0 - 1 scale
	red = r / 255.0; green = g / 255.0; blue = b / 255.0;  

	maxc = max(max(red, green), blue);
	minc = min(min(red, green), blue);

	v = maxc;        // value is maximum of r, g, b

	if(maxc == 0){    // saturation and hue 0 if value is 0
		s = 0;
		h = 0;
	} else {
		s = (maxc - minc) / maxc; 	// saturation is color purity on scale 0 - 1

		delta = maxc - minc;
		if(delta == 0)           // hue doesn't matter if saturation is 0 
			h = 0;
		else{
			if(red == maxc)       // otherwise, determine hue on scale 0 - 360
				h = (green - blue) / delta;
			else if(green == maxc)
				h = 2.0 + (blue - red) / delta;
			else // (blue == maxc)
				h = 4.0 + (red - green) / delta;
			h = h * 60.0;
			if(h < 0)
				h = h + 360.0;
		}
	}
}

void HSVtoRGB ( float h, float s, float v, unsigned char &r, unsigned char &g, unsigned char &b )
{
	int i;
	float f, p, q, t, red, green, blue;

	if (s == 0) {
		// grey
		red = green = blue = v;
	} else {
		h /= 60.0;
		i = (int) floor(h);
		f = h - i;
		p = v * (1-s);
		q = v * (1-s*f);
		t = v * (1 - s * (1 - f));

		switch (i) {
			case 0:
				red = v;
				green = t;
				blue = p;
				break;
			case 1:
				red = q;
				green = v;
				blue = p;
				break;
			case 2:
				red = p;
				green = v;
				blue = t;
				break;
			case 3:
				red = p;
				green = q;
				blue = v;
				break;
			case 4:
				red = t;
				green = p;
				blue = v;
				break;
			default:
				red = v;
				green = p;
				blue = q;
				break;
		}
	}

	r = (unsigned char) (red*255.0 + .5);
	g = (unsigned char) (green*255.0 + .5);
	b = (unsigned char) (blue*255.0 + .5);
}

