/*
 *
 * Copyright (C) 2005 Michael Droettboom

 * This code is a clean-room reimplmentation of the algorithm described in
 *   
 *    Roach, J. W., and J. E. Tatem. 1988. Using domain knowledge in
 *    low-level visual processing to interpret handwritten music: an
 *    experiment. Pattern Recognition 21 (1): 33-44.
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

#ifndef mgd01042005_roachtatem
#define mgd01042005_roachtatem

#include <math.h>
#include "plugins/segmentation.hpp"
#include "plugins/image_utilities.hpp"

struct WindowPoint {
  WindowPoint(unsigned char r_, unsigned char c_, double distance_, double angle_) :
    r(r_), c(c_), distance(distance_), angle(angle_)
  {
    adjacent[0] = NULL;
    adjacent[1] = NULL;
    adjacent[2] = NULL;
    adjacent[3] = NULL;
    adjacent[4] = NULL;
  };
  int r, c;
  bool value;
  double distance;
  double angle;
  WindowPoint* adjacent[5];
};
typedef std::vector<WindowPoint> WindowPointVector;
typedef std::vector<WindowPoint*> WindowPointPtrVector;

bool window_point_sorter(const WindowPoint& a, const WindowPoint& b) {
  return a.distance > b.distance;
}

template<class T>
void compute_vector_field(const T& image, FloatImageView& theta_image, const int window_radius) {
  if (window_radius < 1)
    throw std::runtime_error("Window radius must be >= 1");

  int window_size = window_radius * 2 + 1;

  WindowPointVector points;
  // Overestimate the size of the vector
  points.reserve(size_t(M_PI * window_radius * (window_radius + 2)));
  for (size_t r = 0; r < size_t(window_size); ++r) {
    double r_distance = double(r) - double(window_radius);
    for (size_t c = 0; c < size_t(window_size); ++c) {
      double c_distance = double(c) - double(window_radius);
      double distance = sqrt(r_distance*r_distance + c_distance*c_distance);
      // Scaled up slightly in the x direction to bias toward horizontal lines
      if (size_t(distance + 0.5) <= (size_t)window_radius) {
        double angle = atan2(r_distance, c_distance);
        double distance = sqrt(r_distance*r_distance + (c_distance+2)*(c_distance+2));
        points.push_back(WindowPoint((int)r, (int)c, distance, angle));
      }
    }
  }
  std::stable_sort(points.begin(), points.end(), &window_point_sorter);

  for (WindowPointVector::iterator wp = points.begin(); wp != points.end(); ++wp) {
    size_t adjacent_count = 0;
    for (WindowPointVector::iterator wp0 = wp + 1; wp0 != points.end(); ++wp0) {
      if ((*wp0).r >= (*wp).r - 1 && (*wp0).r <= (*wp).r + 1 &&
	  (*wp0).c >= (*wp).c - 1 && (*wp0).c <= (*wp).c + 1) {
	if (((*wp0).distance) < ((*wp).distance)) {
	  double angular_distance = abs((*wp0).angle - (*wp).angle);
	  if (std::min(angular_distance, 2 * M_PI - angular_distance) < (M_PI / (180.0 / 8.0))) {
	    (*wp).adjacent[adjacent_count++] = &(*wp0);
	  }
	}
      }
    }
  }

  typename ImageFactory<T>::view_type* padded_image = pad_image_default
    (image, window_radius, window_radius, window_radius, window_radius);

  try {
    for (size_t r = 0; r != image.nrows(); ++r) {
      std::cerr << (double)r / (double)image.nrows() << "\r";
      for (size_t c = 0; c != image.ncols(); ++c) {
	if (is_black(image.get(Point(c, r)))) {
	  WindowPointVector::iterator first_black = points.end();
	  WindowPointVector::iterator wp = points.begin();

	  // Copy up to the first black value
	  for (; wp != points.end(); ++wp) {
	    int r0 = (int)r + (*wp).r;
	    int c0 = (int)c + (*wp).c;
	    bool value = is_black(padded_image->get(Point(c0, r0)));
	    (*wp).value = value;
	    if (value) {
	      first_black = wp;
	      break;
	    }
	  }

	  // Copy the rest
	  for (; wp != points.end(); ++wp) {
	    int r0 = (int)r + (*wp).r;
	    int c0 = (int)c + (*wp).c;
	    (*wp).value = is_black(padded_image->get(Point(c0, r0)));
	  }

	  // Filter the window
	  bool filtered = false;
	  do {
	    filtered = false;
	    for (wp = first_black; wp != points.end(); ++wp) {
	      if ((*wp).value) {
		if (first_black == points.end())
		  first_black = wp;
		for (WindowPoint** adj = (*wp).adjacent; *adj != NULL; ++adj) {
		  if (!(*adj)->value) {
		    (*wp).value = false;
		    filtered = true;
		    if (wp == first_black) {
		      first_black = points.end();
		    }
		  }
		}
	      }
	    }
	  } while (filtered);

	  // r_image.set(r, c, (*wp).distance);
	  theta_image.set(Point(c, r), (*first_black).angle);
	}
      }
    }
  } catch (std::exception e) {
    delete padded_image->data(); delete padded_image;
  }
  delete padded_image->data(); delete padded_image;
}

template<class T>
FloatImageView* compute_vector_field(const T& image, const int window_size) {
  FloatImageData* theta_image_data = new FloatImageData(Dim(image.ncols(), image.nrows()),
                                                        Point(image.offset_x(), image.offset_y()));
  FloatImageView* theta_image = new FloatImageView(*theta_image_data);

  try {
    compute_vector_field(image, *theta_image, window_size);
  } catch (std::exception e) {
    delete theta_image_data; delete theta_image;
    throw;
  }

  return theta_image;
}


template<class T, class U, class V>
void mark_horizontal_lines_rt(T& image, U& theta_image, V& lines,
    V& question, double angle_threshold, FloatImageView* thickness_image,
    size_t staffline_height)
{
  angle_threshold = (angle_threshold / 180.0) * M_PI;

  // The following iterations correspond to the horizontal line rules in section 3.4
  // It doesn't make sense to apply them in the order in the paper, however.

  // Line rule #2: A black pixel is a horizontal line pixel if it is
  // horizontal and 8-adjacent to a line pixel (The 8-adjacent part is
  // taken care of by the cc_analysis step below
  // ADDED FEATURE: A black pixel is a horizontal line pixel if its thickness
  //                is not bigger than staffline_height (toom, Dalitz)
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r)))) {
        double theta = abs(theta_image.get(Point(c, r)));

        if (thickness_image == (FloatImageView*)NULL) {
          if (theta < angle_threshold)
            lines.set(Point(c, r), 1);
        } else {
          double thickness=thickness_image->get(Point(c, r));
          if (theta < angle_threshold && thickness <= staffline_height)
            lines.set(Point(c, r), 1);
        }
      }
    }
  }

  // Line rule #1: a black pixel is a horizontal line pixel if it is
  // to the left or right of a line pixel
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 1; c != image.ncols() - 1; ++c) {
      if (is_black(image.get(Point(c, r))))
	if (lines.get(Point(c - 1, r)) == 1 || lines.get(Point(c + 1, r)) == 1)
	  lines.set(Point(c, r), 2);
    }
  }

  // Line rule #3: a white pixel is a horizontal line pixel if it has
  // a marked pixel to its left and its right
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 1; c != image.ncols() - 1; ++c) {
      if (is_white(image.get(Point(c, r))))
 	if (lines.get(Point(c - 1, r)) != 0 && lines.get(Point(c + 1, r)) != 0)
 	  lines.set(Point(c, r), 3);
    }
  }

  // Set all black values to 1
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (lines.get(Point(c, r)) != 0)
	lines.set(Point(c, r), 1);
    }
  }

  // The following iterations correspond to the questionable pixel rules in section 3.4
  // It doesn't make sense to apply them in the order in the paper, however.

  // Questionable rule #1: if it is not horizontal and is above or below a line pixel
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r))))
	if (is_white(lines.get(Point(c, r))) && (is_black(lines.get(Point(c, r + 1)))
                                             || is_black(lines.get(Point(c, r - 1)))))
	  question.set(Point(c, r), black(question));
    }
  }

  // Questionable rule #2: if it is above or below another questionable pixel
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r)))) {
	if (is_black(question.get(Point(c, r - 1))) && is_black(question.get(Point(c, r + 1))))
	  question.set(Point(c, r), black(question));
      }
    }
  }

  // Questionable rule #3: a questionable pixel above a line pixel is
  // relabeled as part of that line if not of the 8-connected pixels
  // above it are black
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(question.get(Point(c, r))) && is_black(lines.get(Point(c, r + 1)))) {
	if (is_white(image.get(Point(c - 1, r - 1))) &&
	    is_white(image.get(Point(c, r - 1))) &&
	    is_white(image.get(Point(c + 1, r - 1))))
	  lines.set(Point(c, r), black(question));
      }
    }
  }

  // Questionable rule #4: a questionable pixel below a line pixel is
  // relabeled as part of that line if not of the 8-connected pixels
  // below it are black
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(question.get(Point(c, r))) && is_black(lines.get(Point(c, r - 1)))) {
	if (is_white(image.get(Point(c - 1, r + 1))) &&
	    is_white(image.get(Point(c, r + 1))) &&
	    is_white(image.get(Point(c + 1, r + 1))))
	  lines.set(Point(c, r), black(question));
      }
    }
  }
}

template<class T, class U>
ImageList* mark_horizontal_lines_rt(T& image, U& theta_image,
    double angle_threshold, PyObject* __thickness, size_t staffline_height)
{
  FloatImageView* thickness;

  /*
   * Check if '__thickness' is an image or of type None. In case it is None,
   * convert it to NULL in order to ignore the thickness in the following
   * computations.
   */
  if (!PyObject_TypeCheck(__thickness, Py_None->ob_type)) {
 
    /*
     * '__thickness' is not None, so finally check the type and convert it
     */
    if (is_ImageObject(__thickness) &&
        (get_image_combination(__thickness) == FLOATIMAGEVIEW)) {
      thickness=((FloatImageView*)((RectObject*)__thickness)->m_x);
      image_get_fv(__thickness, &thickness->features,
          &thickness->features_len);

      // check image dimensions
      if (image.nrows() != thickness->nrows() ||
          image.ncols() != thickness->ncols())
        throw std::runtime_error("self and thickness must be the same size.");
    } else
      throw std::runtime_error("'thickness' must be a FloatImage.");

  // '__thickness' is initialised with type None from Python
  } else
    thickness=(FloatImageView*)NULL;

  if (image.nrows() != theta_image.nrows() || image.ncols() != theta_image.ncols())
    throw std::runtime_error("self and theta_image must be the same size.");

  OneBitImageData* lines_data = new OneBitImageData(Dim(image.ncols(), image.nrows()),
                                                    Point(image.offset_x(), image.offset_y()));
  OneBitImageView* lines = new OneBitImageView(*lines_data);
  OneBitImageData* question_data = new OneBitImageData(Dim(image.ncols(), image.nrows()),
                                                       Point(image.offset_x(), image.offset_y()));
  OneBitImageView* question = new OneBitImageView(*question_data);

  try {
    mark_horizontal_lines_rt(image, theta_image, *lines, *question,
        angle_threshold , thickness, staffline_height);
  } catch (std::exception e) {
    delete lines_data; delete lines;
    delete question_data; delete question;
    throw;
  }

  ImageList* result_list = new ImageList();
  result_list->push_back(lines);
  result_list->push_back(question);

  return result_list;
}

template<class T, class U, class V>
void remove_stafflines_rt(T& image, U& lines, V& result) {
  // The following iterations correspond to the horizontal line deletion rules
  // in Section 3.5

  // Line deletion rule #1: it is not a line pixel
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r))))
	if (is_white(lines.get(Point(c, r))))
	  result.set(Point(c, r), black(result));
    }
  }

  // Line deletion rule #2: it is not horizontal, but the pixel above
  // or below it is horizontal
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r))))
	if (is_white(lines.get(Point(c, r))) &&
	    (is_black(lines.get(Point(c, r - 1))) || is_black(lines.get(Point(c, r + 1)))))
	  result.set(Point(c, r), black(result));
    }
  }

  // Line deletion rule #3: it is a line pixel and is above or below a
  // marked pixel
  for (size_t r = 1; r != image.nrows() - 1; ++r) {
    for (size_t c = 0; c != image.ncols(); ++c) {
      if (is_black(image.get(Point(c, r))))
	if (is_black(lines.get(Point(c, r))) &&
	    (is_black(result.get(Point(c, r - 1))) || is_black(result.get(Point(c, r + 1)))))
	  result.set(Point(c, r), black(result));
    }
  }

  // Line deletion rule #4: it has marked pixels to its left and to
  // its right
  for (size_t r = 0; r != image.nrows(); ++r) {
    for (size_t c = 1; c != image.ncols() - 1; ++c) {
      if (is_black(image.get(Point(c, r))))
        if (is_white(result.get(Point(c, r))) &&
            is_black(result.get(Point(c - 1, r))) &&
            is_black(result.get(Point(c + 1, r))))
          result.set(Point(c, r), black(result));
    }
  }

  return;
}

template<class T, class U>
OneBitImageView* remove_stafflines_rt(T& image, U& lines) {
  if (image.nrows() != lines.nrows() || image.ncols() != lines.ncols())
    throw std::runtime_error("self and theta_image must be the same size.");

  OneBitImageData* result_data = new OneBitImageData(Dim(image.ncols(), image.nrows()),
                                                     Point(image.offset_x(), image.offset_y()));
  OneBitImageView* result = new OneBitImageView(*result_data);

  try {
    remove_stafflines_rt(image, lines, *result);
  } catch (std::exception e) {
    delete result_data; delete result;
    throw;
  }

  return result;
}

#endif
