/*
 * Copyright (C) 2006-2010 Christoph Dalitz
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

#ifndef _MusicStaves_Deformation_HPP_
#define _MusicStaves_Deformation_HPP_

#include <gamera.hpp>
#include <plugins/morphology.hpp>
#include <plugins/arithmetic.hpp>
#include <stdlib.h>

using namespace Gamera;
using namespace std;



/*
 * Image degradation after Kanungo et al.
 */
template<class T>
typename ImageFactory<T>::view_type* degrade_kanungo_single_image(const T &src, float eta, float a0, float a, float b0, float b, int k, int random_seed = 0)
{
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;
  typedef typename T::value_type value_type;
  int d;
  double randval;

  FloatImageView *dt_fore, *dt_back;
  typename T::const_vec_iterator p;
  typename view_type::vec_iterator q;
  FloatImageView::vec_iterator df,db;
  value_type blackval = black(src);
  value_type whiteval = white(src);

  data_type* dest_data = new data_type(src.size(), src.origin());
  view_type* dest = new view_type(*dest_data);
  
  // compute distance transform of foreground and background
  // as dest is not yet needed we abuse it for storing the inverted image
  dt_fore = (FloatImageView*)distance_transform(src, 0);
  for (p=src.vec_begin(), q=dest->vec_begin(); p != src.vec_end(); p++, q++) {
    if (is_black(*p)) *q = whiteval;
    else *q = blackval;
  }
  dt_back = (FloatImageView*)distance_transform(*dest, 0);

  // precompute probabilities (maximum distance 32 should be enough)
  double P_foreground_flip[32];
  double P_background_flip[32];
  for (d=0; d<32; d++) {
    P_foreground_flip[d] = a0*exp(-a*(d+1)*(d+1)) + eta;
    P_background_flip[d] = b0*exp(-b*(d+1)*(d+1)) + eta;
  }

  // flip pixels randomly based on their distance from border
  srand(random_seed);
  for (q=dest->vec_begin(), df=dt_fore->vec_begin(), db=dt_back->vec_begin();
       q != dest->vec_end(); q++, df++, db++) {
    randval = ((double)rand()) / RAND_MAX;
    // note that dest is still inverted => black is background!!
    if (is_black(*q)) {
      d = (int)(*db + 0.5);
      if ((d > 32) || (randval > P_background_flip[d-1]))
        *q = whiteval;
    } else {
      d = (int)(*df + 0.5);
      if ((d > 32) || (randval > P_foreground_flip[d-1]))
        *q = blackval;
    }
  }

  // do a morphological closing
  if (k>1) {
    // build structuring element
    data_type* se_data = new data_type(Dim(k,k), Point(0,0));
    view_type* se = new view_type(*se_data);
    for (q=se->vec_begin(); q!=se->vec_end(); q++)
      *q = blackval;
    view_type* dilated = dilate_with_structure(*dest, *se, Point(k/2,k/2));
    view_type* eroded = erode_with_structure(*dilated, *se, Point(k/2,k/2));
    delete dilated->data(); delete dilated;
    delete dest->data(); delete dest;
    delete se_data; delete se;
    dest = eroded;
  }

  // clean up
  delete dt_fore->data(); delete dt_fore;
  delete dt_back->data(); delete dt_back;

  return dest;
}

/*
 * add white speckles in onebit image
 * returns only the two images deformed from src and staffonly
 */
template<class T, class U>
ImageList* white_speckles_parallel_noskel(const T &src, const U &staffonly, float p0, int n, int k, int connectivity = 2, int random_seed = 0)
{
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;
  typedef typename T::value_type value_type;
  double randval;
  size_t x,y;
  size_t maxx = src.ncols() - 1;
  size_t maxy = src.nrows() - 1;
  int i;

  value_type blackval = black(src);
  value_type whiteval = white(src);

  data_type* speckles_data = new data_type(src.size(), src.origin());
  view_type* speckles = new view_type(*speckles_data);

  // create random walk data
  for (y=0; y <= maxy; y++) {
    for (x=0; x <= maxx; x++) {
      Point p(x,y);
      if (is_black(src.get(p)) && (((double)rand()) / RAND_MAX < p0)) {
        speckles->set(p,blackval);
        for (i=0; i<n; i++) {
          if (p.x() == 0 || p.x() == maxx || p.y() == 0 || p.y() == maxy)
            break;
          randval = ((double)rand()) / RAND_MAX;
          if (connectivity == 0) {
            // random rook move
            if (randval < 0.25)      p.x(p.x() + 1);
            else if (randval < 0.5)  p.x(p.x() - 1);
            else if (randval < 0.75) p.y(p.y() + 1);
            else                     p.y(p.y() - 1);
          }
          else if (connectivity == 1) {
            // random bishop move
            if (randval < 0.25)      {p.x(p.x() + 1); p.y(p.y() + 1);}
            else if (randval < 0.5)  {p.x(p.x() + 1); p.y(p.y() - 1);}
            else if (randval < 0.75) {p.x(p.x() - 1); p.y(p.y() + 1);}
            else                     {p.x(p.x() - 1); p.y(p.y() - 1);}
          }
          else {
            // random king move
            if (randval < 0.125)      {p.y(p.y() - 1); p.x(p.x() - 1);}
            else if (randval < 0.25)  {p.y(p.y() - 1);}
            else if (randval < 0.375) {p.y(p.y() - 1); p.x(p.x() + 1);}
            else if (randval < 0.5)   {p.x(p.x() + 1);}
            else if (randval < 0.625) {p.x(p.x() + 1); p.y(p.y() + 1);}
            else if (randval < 0.75)  {p.y(p.y() + 1);}
            else if (randval < 0.875) {p.x(p.x() - 1); p.y(p.y() + 1);}
            else                      {p.x(p.x() - 1);}
          }
          speckles->set(p,blackval);
        }
      }
    }
  }

  // do a morphological closing
  if (k>1) {
    typename view_type::vec_iterator q;
    // build structuring element
    data_type* se_data = new data_type(Dim(k,k), Point(0,0));
    view_type* se = new view_type(*se_data);
    for (q=se->vec_begin(); q!=se->vec_end(); q++)
      *q = blackval;
    view_type* dilated = dilate_with_structure(*speckles, *se, Point(k/2,k/2));
    view_type* closed = erode_with_structure(*dilated, *se, Point(k/2,k/2));
    delete dilated->data(); delete dilated;
    delete speckles->data(); delete speckles;
    delete se_data; delete se;
    speckles = closed;
  }

  // subtract speckles from input images image
  // the full image is written to "speckles", 
  // the staffonly image to "specklesnostaff"
  data_type* specklesnostaff_data = new data_type(src.size(), src.origin());
  view_type* specklesnostaff = new view_type(*specklesnostaff_data);
  for (y=0; y <= maxy; y++) {
    for (x=0; x <= maxx; x++) {
      Point p(x,y);
      if (is_black(speckles->get(p))) {
        speckles->set(p,whiteval);
        specklesnostaff->set(p,whiteval);
      } else {
        speckles->set(p,src.get(p));
        specklesnostaff->set(p,staffonly.get(p));
      }
    }
  }

  // build list of the two return images
  ImageList* retval = new ImageList();
  retval->push_back(speckles);
  retval->push_back(specklesnostaff);

  return retval;
}

#endif
