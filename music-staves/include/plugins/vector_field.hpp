/*
 * Copyright (C) 2005,2006 Thomas Karsten, Christoph Dalitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef VECTORFIELD_TOOM
#define VECTORFIELD_TOOM

#define DEBUG 0

using namespace Gamera;
using namespace std;

#include <gamera.hpp>

#include <math.h>
#include <vector>

struct angle_t {
  double alpha;
  double tan_alpha;
};

/*
 * prototype declaration
 */
template<class T>
ImageList* compute_longest_chord_vectors(const T& image,
    const int path_length, double resolution=3.0,
    ProgressBar progress_bar=ProgressBar());

template<class T>
RGBImageView* angles_to_RGB(const T& image, const ImageVector v_images,
   size_t limit=100);

//private module functions
template<class T>
static double __get_angle_of_longest_chord(const T& image, size_t col,
    size_t row, size_t limit, vector<struct angle_t> angles,
    size_t* length, size_t* thickness);
template<class T>
static unsigned int __runlength(const T& image, size_t x, size_t y,
    struct angle_t, size_t limit);
static Rgb<GreyScalePixel> __set_Hsv(FloatPixel h, FloatPixel s=1.0,
    FloatPixel v=1.0);
template<class T>
static void __create_Rgb_1(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors);
template<class T>
static void __create_Rgb_2(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors, const FloatImageView* lengths,
    size_t limit);
template<class T>
static void __create_Rgb_3(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors, const FloatImageView* lengths,
    const FloatImageView* thicknesses, size_t limit);


/*****************************************************************************
 *
 *
 * toom, 2005-12-06
 ****************************************************************************/
template<class T>
ImageList* compute_longest_chord_vectors(const T& image,
    const int path_length, double resolution, ProgressBar progress_bar)
{
  FloatImageData* v_image_data=new FloatImageData(
      Dim(image.ncols(), image.nrows()),
      Point(image.offset_x(), image.offset_y()));
  FloatImageView* v_image=new FloatImageView(*v_image_data);

  FloatImageData* lengths_data=new FloatImageData(
      Dim(image.ncols(), image.nrows()),
      Point(image.offset_x(), image.offset_y()));
  FloatImageView* lengths=new FloatImageView(*lengths_data);

  FloatImageData* thicknesses_data=new FloatImageData(
      Dim(image.ncols(), image.nrows()),
      Point(image.offset_x(), image.offset_y()));
  FloatImageView* thicknesses=new FloatImageView(*thicknesses_data);

  typename T::value_type black_value=black(image);

  size_t length;
  size_t thickness;
  double d_alpha;
  double min_alpha;

  vector<struct angle_t> angles;
  struct angle_t pre_calc_angle;

  // 'resolution' is checked by the call in python
  // (must be > 0.0)!!!
  d_alpha=M_PI*resolution/180.;

  /*
   * precalculate angles and their tangents
   */
  for (double a=-M_PI_2; a < M_PI_2; a+=d_alpha) {
    pre_calc_angle.alpha=a;
    pre_calc_angle.tan_alpha=tan(pre_calc_angle.alpha);
    angles.push_back(pre_calc_angle);
  }

  progress_bar.set_length(image.nrows());

  /*
   * go through all the angles and compute the
   * runlength for this pixel
   */
  for (size_t row=0; row < image.nrows(); row++) {
    for (size_t col=0; col < image.ncols(); col++) {

      // ignore white pixels from here on
      if (image.get(Point(col, row)) != black_value)
        continue;

      min_alpha=__get_angle_of_longest_chord(image, col, row, path_length,
          angles, &length, &thickness);

      v_image->set(Point(col, row), min_alpha);
      lengths->set(Point(col, row), length);
      thicknesses->set(Point(col, row), thickness);
    }
    progress_bar.step();
#if DEBUG
    std::cerr << "\rrow " << row+1 << "/" << image.nrows();
    //std::cerr << std::endl;
#endif
  }
#if DEBUG
  std::cerr << std::endl;
#endif

  ImageList* result=new ImageList();
  result->push_back(v_image);
  result->push_back(lengths);
  result->push_back(thicknesses);

  return result;
}

/*****************************************************************************
 * Finds the longest chord that the pixel (col, row) is part of. Therefor,
 * each angle in 'angles' is checked and traced (in both directions). The
 * function will trace a maximum of 'limit' pixels (one direction). The
 * computed length of the chord will be stored in 'length', the computed
 * thickness will be stored in 'thickness' (thickness is the length of the
 * chord, that is perpendicular to the longest chord).
 *
 * toom, 2005-12-15
 ****************************************************************************/
template<class T>
inline double __get_angle_of_longest_chord(const T& image, size_t col,
    size_t row, size_t limit, vector<struct angle_t> angles,
    size_t* length, size_t* thickness)
{
  unsigned int tmp_length;
  double min_alpha;
  vector<struct angle_t>::iterator iter;

  size_t angles_size=angles.size();
  vector<size_t> chord_length(angles_size);

  size_t i=0;
  size_t max_index=0;
 
  *length=0;
  min_alpha=2*M_PI;
  for (iter=angles.begin(); iter != angles.end(); iter++, i++) {

    // get the runlength for this angle
    tmp_length=__runlength(image, col, row, *iter, limit);

    chord_length[i]=tmp_length;

    // store both the maximum runlength and the
    // angle with the smallest value, remember the
    // index of the maximum runlength (used for the thickness)
    if (tmp_length > *length) {
      *length=tmp_length;
      min_alpha=iter->alpha;
      max_index=i;
    } else if (tmp_length == *length) {
      if (fabs(iter->alpha) < fabs(min_alpha)) {
        min_alpha=iter->alpha;
        max_index=i;
      }
    }
  }

  // thickness is the length of the chord
  // perpendicular to the longest chord
  *thickness=chord_length[(max_index+angles_size/2)%angles_size];

  return min_alpha;
}

/*****************************************************************************
 * Computes the black runlength for the pixel (center_x, center_y) with the
 * given angle 'alpha'. The function will check a runlength, until either
 * 'limit' pixels are checked or a white pixel occured.
 *
 * 'alpha' is given in radian and must be -2*M_PI <= alpha <= 2*M_PI.
 *
 * toom, 2005-12-06
 ****************************************************************************/
template<class T>
unsigned int __runlength(const T& image, size_t center_x, size_t center_y,
    struct angle_t angle, size_t limit)
{
  typename T::value_type black_value=black(image);

  size_t x, y;
  float __x, __y;
  float one_step;
  int length_factor;
  int limit_factor;
  size_t ncols, nrows;
  float dx, dy;

  /*
   * compute directions where to go
   */
  if (angle.alpha < 0.)
    angle.alpha+=2*M_PI;

  if (angle.alpha <= M_PI_4 || angle.alpha > 7*M_PI_4) {
    dx=1.;
    dy=angle.tan_alpha;
  } else if (angle.alpha <= 3*M_PI_4) {
    if (fabs(angle.alpha-M_PI_2) < 0.0001)
      dx=0.;
    else
      dx=1./angle.tan_alpha;
    dy=1;
  } else if (angle.alpha <= 5*M_PI_4) {
    dx=-1.;
    dy=-angle.tan_alpha;
  } else {
    if (fabs(angle.alpha-3*M_PI_2) < 0.0001)
      dx=0.;
    else
      dx=-1./angle.tan_alpha;
    dy=-1.;
  }

  ncols=image.ncols();
  nrows=image.nrows();

  /*
   * trace the pixels along the line (forward)
   */

  // start with the adjacent pixel
  __x=center_x+dx+0.5f;        // adding 0.5 saves us from rounding
  __y=center_y-dy+0.5f;        // the values in every loop iteration
  x=(size_t)(__x);
  y=(size_t)(__y);
  one_step=sqrt(dx*dx+dy*dy);

  // GO!
  length_factor=1;
  limit_factor=(int)::round(limit/one_step);
  while (length_factor < limit_factor && (size_t)x < ncols &&
      (size_t)y < nrows &&
      image.get(Point((size_t)x, (size_t)y)) == black_value) {
    __x+=dx;
    __y-=dy;
    x=(size_t)(__x);
    y=(size_t)(__y);
    length_factor++;
  }

  /*
   * trace the pixels along the line (backward)
   */

  // compute the adjacent pixel
  __x=center_x-dx+0.5f;
  __y=center_y+dy+0.5f;
  x=(size_t)(__x);
  y=(size_t)(__y);

  // GO!
  while (length_factor < limit_factor && (size_t)x < ncols &&
      (size_t)y < nrows &&
      image.get(Point((size_t)x, (size_t)y)) == black_value) {
    __x-=dx;
    __y+=dy;
    x=(size_t)(__x);
    y=(size_t)(__y);
    length_factor++;
  }
  return (size_t)(one_step*length_factor);
}

/*****************************************************************************
 * Converts a Float image into an RGB image, where the information of the
 * Float image is treated as angles in the range of -2*M_PI <= alpha <=
 * 2*M_PI (otherwise the method will throw an exception).
 *
 * An angle of 0 degree will produce the RGB values (127, 255, 0), an angle of
 * 90 degrees will return the RGB values (255, 0, 0).
 *
 * Input images are the image containing the angles ('v_image') and the
 * original image ('image').
 *
 * toom, 2005-12-12
 ****************************************************************************/
template<class T>
RGBImageView* angles_to_RGB(const T& image, const ImageVector v_images,
		size_t limit)
{
  ImageVector::const_iterator v_iter;
  const Image* img;
  size_t n_images;

  /*
   * test the images included in the list
   * for their size and their type
   */
  n_images=v_images.size();
  if (n_images > 3)
    throw std::runtime_error("The list must not include more than"
        " 3 images.");
  for (v_iter=v_images.begin(); v_iter != v_images.end(); v_iter++) {
    img=v_iter->first;
    if (img->nrows() != image.nrows() || img->ncols() != image.ncols())
      throw std::runtime_error("An image included in the list has different"
          " dimensions than the original image.");
    if (v_iter->second != FLOATIMAGEVIEW)
      throw std::runtime_error("An image included in the list is not of type"
          " FloatImage.");
  }

  RGBImageData* rgb_image_data=new RGBImageData(
      Dim(image.ncols(), image.nrows()),
      Point(image.offset_x(), image.offset_y()));
  RGBImageView* rgb_image=new RGBImageView(*rgb_image_data);

#if 1
  switch (n_images) {
    case 1:
      __create_Rgb_1(image, rgb_image, (FloatImageView*)(v_images[0].first));
      break;
    case 2:
      __create_Rgb_2(image, rgb_image, (FloatImageView*)(v_images[0].first),
          (FloatImageView*)(v_images[1].first), limit);
      break;
    case 3:
      __create_Rgb_3(image, rgb_image, (FloatImageView*)(v_images[0].first),
          (FloatImageView*)(v_images[1].first),
          (FloatImageView*)(v_images[2].first), limit);
      break;
  }
#else
  double alpha;
  unsigned char r, g, b;
  size_t nrows=image.nrows();
  size_t ncols=image.ncols();

  typename T::value_type white_pix=white(image);
  typename RGBImageView::value_type white_rgb=white(*rgb_image);

  r=g=b=0;

  for (size_t row=0; row < nrows; row++)
    for (size_t col=0; col < ncols; col++) {
      
      if (image.get(Point(col, row)) == white_pix) {
        rgb_image->set(Point(col, row), white_rgb);
        continue;
      }

      alpha=((FloatImageView*)(v_images[0].first))->get(Point(col, row));
      //alpha=0.;

      if (fabs(alpha) > 2*M_PI)
        throw std::runtime_error("Angle value out of range");

      /*
       * normalise alpha and start with RGB (127, 255, 0) as 0 degree,
       * rather than starting with RGB (255, 0, 0)
       */
      if (alpha < 0.0)
        alpha+=2*M_PI;
      alpha/=2*M_PI;
      alpha-=0.25;
      if (alpha < 0.)
        alpha+=1.;

      /*
       * compute the RGB values for the given angle (assume
       * both saturation and intensity set to 1.0)
       */
      if (alpha < 0.5)
        std::cerr << "Angle too low." << std::endl;

      // alpha < -1/3*M_PI
      else if (alpha < 7./12) {
        r=255;
        g=0;
        b=(unsigned char)((alpha-6./12)*12*255);

      // alpha < -1/6*M_PI
      } else if (alpha < 8./12) {
        r=(unsigned char)((1-(alpha-7./12)*12*255));
        g=0;
        b=255;

      // alpha < 0
      } else if (alpha < 9./12) {
        r=0;
        g=(unsigned char)((alpha-8./12)*12*255);
        b=255;

      // alpha < 1/6*M_PI
      } else if (alpha < 10./12) {
        r=0;
        g=255;
        b=(unsigned char)((1-(alpha-9./12)*12)*255);

      // alpha < 1/3*M_PI
      } else if (alpha < 11./12) {
        r=(unsigned char)((alpha-10./12)*12*255);
        g=255;
        b=0;

      // alpha <= 1/2*M_PI
      } else {
        r=255;
        g=(unsigned char)((1-(alpha-11./12)*12)*255);
        b=0;
      }
      rgb_image->set(Point(col, row), RGBPixel(r, g, b));
    }
#endif

  return rgb_image;
}

/*****************************************************************************
 * keep_tall_vectorfield_runs
 *
 * see the documentation of the plugin for how to use this function.
 *
 * chris, 2005-12-23
 ****************************************************************************/
template<class T> 
typename ImageFactory<T>::view_type* 
keep_vectorfield_runs(T& image, PointVector* points,
                           int height, char* directionstr,
                           int maxlength, int numangles)
{
  typedef typename ImageFactory<T>::data_type data_t;
  typedef typename ImageFactory<T>::view_type view_t;
  int direction; // +1 for down, -1 for up
  PointVector::iterator p;
  double angle, delta, yoff, dy, xoff, dx;
  size_t length, thickness;
  Point curp;
  vector<struct angle_t> angles;
  struct angle_t aa;
  int blackvalue = 1;

  // compute sample angles
  delta = M_PI / numangles;
  for (double a=M_PI_2; a>-M_PI_2; a-=delta) {
    aa.alpha=a;
    if (fabs(a-M_PI_2) > 0.001) aa.tan_alpha = tan(a); else aa.tan_alpha = 0;
    angles.push_back(aa);
  }

  // image objects
  data_t* dest=new data_t(image.size(), Point(image.offset_x(),
                                              image.offset_y()));
  view_t* dest_view=new view_t(*dest);


  // return an empty image in case of errenous parameters
  if (((size_t)height > dest_view->nrows()/2) || (height < 1) ||
      (points->size() == 0))
    return dest_view;

  // set direction
  if (0==strcmp(directionstr,"up"))
    direction = -1;
  else
    direction = +1;

  // for each point compute vector field angle 
  // and follow run in its direction
  for (p=points->begin(); p!=points->end(); p++) {
    if (is_white(image.get(*p))) continue;
    angle = __get_angle_of_longest_chord(image, p->x(), p->y(), maxlength,
                                         angles, &length, &thickness);
    // horizontal walk up to height would be infinite
    if (fabs(angle) < 0.001) continue;
    //printf("angle at (%d,%d): %.4f\n", (int)p->x(), (int)p->y(), angle);
    // we must distinguish four quadrants
    if (direction > 0) {
      if (fabs(angle) < M_PI_4) {
        // rather horizontal
        if (angle > 0) dx = -1; else dx = +1;
        dy = tan(fabs(angle));
      } else {
        // rather vertical
        dy = +1;
        dx = tan(M_PI_2 + angle); // == -cot(angle)
      }
      xoff = yoff = 0.0;
      curp = Point(p->x(), p->y());
      //printf("dx=%.4f dy=%.4f\n", dx, dy);
      while ((yoff < height) && (curp.y() < image.nrows())) {
        curp = Point((int)(p->x() + xoff + 0.5),(int)(p->y() + yoff + 0.5));
        if (is_white(image.get(curp))) break;
        dest_view->set(curp, blackvalue);
        xoff += dx; yoff += dy;
      }
    } 
    else { // direction < 0
      if (fabs(angle) < M_PI_4) {
        // rather horizontal
        if (angle > 0) dx = +1; else dx = -1;
        dy = tan(fabs(angle));
      } else {
        // rather vertical
        dy = +1;
        dx = -tan(M_PI_2 + angle); // == cot(angle)
      }
      xoff = yoff = 0.0;
      curp = Point(p->x(), p->y());
      //printf("dx=%.4f dy=%.4f\n", dx, dy);
      while ((yoff < height) && (curp.y() > 0)) {
        curp = Point((int)(p->x() + xoff + 0.5),(int)(p->y() - yoff - 0.5));
        if (is_white(image.get(curp))) break;
        dest_view->set(curp, blackvalue);
        xoff += dx; yoff += dy;
      }
    }
  }

  return dest_view;
}


/*****************************************************************************
 * toom, 2005-12-21
 ****************************************************************************/
template<class T>
void __create_Rgb_1(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors)
{
  typename T::value_type white_pix=white(image);

  Rgb<GreyScalePixel> white(255, 255, 255);
  Rgb<GreyScalePixel> pix;

  FloatPixel hue;
 
  for (size_t col=0; col < vectors->ncols(); col++)
    for (size_t row=0; row < vectors->nrows(); row++) {
      if (image.get(Point(col, row)) == white_pix) {
        rgb_image->set(Point(col, row), white);
      } else {
        hue=vectors->get(Point(col, row));
        hue=1.0-(hue+M_PI_2)/M_PI;
        pix=__set_Hsv(hue);
        rgb_image->set(Point(col, row), pix);
      }
    }
}

/*****************************************************************************
 * toom, 2005-12-21
 ****************************************************************************/
template<class T>
void __create_Rgb_2(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors, const FloatImageView* lengths,
    size_t limit)
{
  typename T::value_type white_pix=white(image);

  Rgb<GreyScalePixel> white(255, 255, 255);
  Rgb<GreyScalePixel> pix;

  FloatPixel hue, sat;

  for (size_t col=0; col < vectors->ncols(); col++)
    for (size_t row=0; row < vectors->nrows(); row++) {
      if (image.get(Point(col, row)) == white_pix) {
        rgb_image->set(Point(col, row), white);
      } else {
        hue=vectors->get(Point(col, row));
        hue=1.0-(hue+M_PI_2)/M_PI;
        sat=lengths->get(Point(col, row))/limit;
        if (sat > 1.0)
          sat=1.0;
        pix=__set_Hsv(hue, sat);
        rgb_image->set(Point(col, row), pix);
      }
    }
}

/*****************************************************************************
 * toom, 2005-12-21
 ****************************************************************************/
template<class T>
void __create_Rgb_3(const T& image, RGBImageView* rgb_image,
    const FloatImageView* vectors, const FloatImageView* lengths,
    const FloatImageView* thicknesses, size_t limit)
{
  typename T::value_type white_pix=white(image);

  Rgb<GreyScalePixel> white(255, 255, 255);
  Rgb<GreyScalePixel> pix;

  FloatPixel hue, sat, val;
 
  for (size_t col=0; col < vectors->ncols(); col++)
    for (size_t row=0; row < vectors->nrows(); row++) {
      if (image.get(Point(col, row)) == white_pix) {
        rgb_image->set(Point(col, row), white);
      } else {
        hue=vectors->get(Point(col, row));
        hue=1.0-(hue+M_PI_2)/M_PI;
        sat=lengths->get(Point(col, row))/limit;
        val=1.0-thicknesses->get(Point(col, row))/limit;
        if (sat > 1.0)
          sat=1.0;
        if (val > 1.0)
          val=1.0;
        pix=__set_Hsv(hue, sat, val);
        rgb_image->set(Point(col, row), pix);
      }
    }
}

/*****************************************************************************
 * 
 *
 * toom, 2005-12-21
 ****************************************************************************/
Rgb<GreyScalePixel> __set_Hsv(FloatPixel h, FloatPixel s, FloatPixel v)
{
  int sector;
  double f;
  double p, q, t;

  double r, g, b;

  r=g=b=0.0;
  
  if (s == 0.0) {
    r=g=b=v;
  } else {
    h*=6.;
    sector=(int)(h)%6;
    f=h-sector;

    p=v*(1-s);
    q=v*(1-s*f);
    t=v*(1-s*(1-f));

    switch (sector) {
      case 0:
        r=v; g=t; b=p;
        break;
      case 1:
        r=q; g=v; b=p;
        break;
      case 2:
        r=p; g=v; b=t;
        break;
      case 3:
        r=p; g=q; b=v;
        break;
      case 4:
        r=t; g=p; b=v;
        break;
      case 5:
        r=v; g=p; b=q;
        break;
    }
  }
  return Rgb<GreyScalePixel>((GreyScalePixel)(r*255), (GreyScalePixel)(g*255),
      (GreyScalePixel)(b*255));
}

#endif
