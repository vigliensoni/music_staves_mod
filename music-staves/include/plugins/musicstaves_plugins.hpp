/*
 * Copyright (C) 2004 Christoph Dalitz
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


#ifndef cd15102004_musicstaves_plugins
#define cd15102004_musicstaves_plugins

#include <vector>
#include <algorithm>
#include <deque>
#include <gamera.hpp>
#include <plugins/image_utilities.hpp>
#include <image_types.hpp>

using namespace std;

namespace Gamera {

  template<class T>
  void fill_horizontal_line_gaps(T &image, size_t width, size_t blackness, bool fill_average=false) {
    float percentage = blackness / 100.0;
    // data types for pixel value calculations
    typedef typename T::value_type value_type;
    typedef typename NumericTraits<value_type>::Promote sum_type;
    typedef typename NumericTraits<value_type>::RealPromote avg_type;
    value_type leftval;
    sum_type windowsum;
    avg_type average;
    // experiments have shown that for row/col iteration
    // iterators are faster than indices
    typedef typename T::row_iterator IteratorRow;
    IteratorRow r;
    size_t n;

    if (image.ncols() <= width)
      return;

    // get values for black and white
    value_type blackval = black(image);
    value_type whiteval = white(image);
    bool black_greater_white = (blackval > whiteval);
    if (black_greater_white)
      percentage = percentage * ((float)blackval - (float)whiteval);
    else
      percentage = (1.0 - percentage) * ((float)whiteval - (float)blackval);

    // vector for original image values (as we overwrite the image)
    deque<value_type> imageval(0);

    // scan image with window
    for (r=image.row_begin(); r != image.row_end(); ++r) {
      typename IteratorRow::iterator c1, c2;
      windowsum = 0;
      imageval.clear();
      for (c2 = r.begin(), n=0; n < width-1; ++c2, ++n) {
        imageval.push_front(*c2);
        windowsum += imageval.front();
      }
      c1 = r.begin();
      for (n=0; n < width/2; ++n)
        ++c1;
      leftval = whiteval;
      for (; c2 != r.end(); ++c1, ++c2) {
        imageval.push_front(*c2);
        windowsum = windowsum + imageval.front() - leftval;
        leftval = imageval.back();
        imageval.pop_back();
        average = windowsum/(float)width;
        if (black_greater_white) {  // ONEBIT
          if (average >= percentage)
            *c1 = blackval;
        } else {  // GREYSCALE
          if ((average <= percentage) && (average < *c1))
            if (fill_average)
              *c1 = NumericTraits<value_type>::fromRealPromote(average);
            else
              *c1 = blackval;
        }
      }
    }
  }

  template<class T>
  void fill_vertical_line_gaps(T &image, size_t height, size_t blackness, bool fill_average=false) {
    float percentage = blackness / 100.0;
    typedef typename T::value_type value_type;
    typedef typename NumericTraits<value_type>::Promote sum_type;
    typedef typename NumericTraits<value_type>::RealPromote avg_type;
    value_type topval;
    sum_type windowsum;
    avg_type average;
    // experiments have shown that for col/row iteration
    // index access is faster
    size_t r, c;
    

    if (image.nrows() <= height)
      return;

    // get values for black and white
    value_type blackval = black(image);
    value_type whiteval = white(image);
    bool black_greater_white = (blackval > whiteval);
    if (black_greater_white)
      percentage = percentage * ((float)blackval - (float)whiteval);
    else
      percentage = (1.0 - percentage) * ((float)whiteval - (float)blackval);

    // vector for original image values (as we overwrite the image)
    deque<value_type> imageval(0);

    // scan image with window
    for (c=0; c < image.ncols(); ++c) {
      windowsum = 0;
      imageval.clear();
      for (r = 0; r < height-1; ++r) {
        imageval.push_front((int)is_black(image.get(Point(c, r))));
        windowsum += imageval.front();
      }
      topval = whiteval;
      for (r = height-1; r < image.nrows(); ++r) {
        imageval.push_front(image.get(Point(c, r)));
        windowsum = windowsum + imageval.front() - topval;
        topval = imageval.back();
        imageval.pop_back();
        average = windowsum/(float)height;
        if (black_greater_white) {  // ONEBIT
          if (average >= percentage)
            image.set(Point(c, r - height / 2), blackval);
        } else {  // GREYSCALE
          if ((average <= percentage) && (average < image.get(Point(c, r - height / 2))))
            if (fill_average)
              image.set(Point(c, r - height / 2),
                        NumericTraits<value_type>::fromRealPromote(average));
            else
              image.set(Point(c, r - height / 2), blackval);
        }
      }
    }
  }


  /***************************************************************************
   * match_staff_template
   *
   * Extracts all points from an image that match a staff template.
   *
   * parameters: 
   *   width = width of the horizontal lines in the template
   *   line_distance = vertical distance between template lines
   *   nlines = number of template lines
   *   blackness = threshold for deciding whter a point matches
   *               (must be surpassed in *all* lines of the template)
   *
   * return value:
   *   a onebit image containing the matching staff points
   *
   * 2006-04-10 chris
   ***************************************************************************/
  template<class T>
  OneBitImageView* match_staff_template(T& image, int width, int line_distance,
                                        size_t nlines, size_t blackness)
  {
    OneBitImageData* dest = new OneBitImageData(image.size(),
                                                Point(image.offset_x(),
                                                      image.offset_y()));
    OneBitImageView* dest_view = new OneBitImageView(*dest);

    if ((width < 1) || ((size_t)width > image.ncols()))
      return dest_view;

    float percentage = blackness / 100.0;
    // data types for pixel value calculations
    typedef typename T::value_type value_type;
    typedef typename NumericTraits<value_type>::Promote sum_type;
    vector<sum_type> windowsum(nlines);
    size_t n,x,maxy,y0,y1,dy,ncols,nrows;
    int leftx,midx;
    IntVector y(nlines);
    bool all_above_threshold;

    ncols = image.ncols(); nrows = image.nrows();

    // get values for black and white
    value_type blackval = black(image);

    // scan image with template
    maxy = nrows - (nlines -1 ) * (line_distance + 1) - 1;
    dy = line_distance + 1;
    for (y0=0; y0 < maxy; ++y0) {
      y1 = y0;
      for (n=0; n<nlines; ++n) {
        windowsum[n] = 0;
        y[n] = y1; y1 += dy;
      }
      for (x=0, leftx=-width, midx=-(width/2); x<ncols; ++x, ++leftx, ++midx) {
        all_above_threshold = true;
        for (n=0; n<nlines; ++n) {
          windowsum[n] += image.get(Point(x,y[n]));
          if (leftx>=0) windowsum[n] -= image.get(Point(leftx,y[n]));
          if (windowsum[n]/(float)width < percentage) 
            all_above_threshold = false;
        }
        if (midx >= 0 && all_above_threshold) {
          for (n=0; n<nlines; ++n) dest_view->set(Point(midx,y[n]), blackval);
        }
      }
    }

    // extrapolate staff edges
    for (y0 = 0; y0 < nrows; ++y0) {
      // left edge
      for (x = 0; x < ncols; ++x) {
        if (is_black(dest_view->get(Point(x,y0)))) {
          for (; x>=0 && is_black(image.get(Point(x,y0))); --x)
            dest_view->set(Point(x,y0),blackval);
          break;
        }
      }
      // right edge
      for (x = ncols - 1; x >= 0; --x) {
        if (is_black(dest_view->get(Point(x,y0)))) {
          for (; x < ncols && is_black(image.get(Point(x,y0))); ++x)
            dest_view->set(Point(x,y0),blackval);
          break;
        }
      }
    }

    return dest_view;
  }
}
#endif


