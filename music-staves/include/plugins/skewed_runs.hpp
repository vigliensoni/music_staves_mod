/*
 * Copyright (C) 2004 Thomas Karsten, Christoph Dalitz
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

/*
 * skewed_runs.hpp
 *
 * plugin for MusicStaves
 *
 * toom, 2004-12-28
 */
#ifndef SKEWED_RUNS_TOOM_2004
#define SKEWED_RUNS_TOOM_2004

#include <iostream>
#include <vector>
#include <math.h>

#include "gamera.hpp"
#include "plugins/image_utilities.hpp"

//#define CDEBUG(x) do {cerr x; fflush(stderr);} while (0) //cerr x;
#define CDEBUG(x) do; while (0) //cerr x;

using namespace Gamera;
using namespace std;

/*
 * contains values *relative* to a reference pixel
 * (i.e. both values can be negative)
 */
struct pixel_t {
  int x;
  int y;
};

struct path_t {
  bool mark;
  int len_up, len_down; // length in up/down direction

  unsigned marked_pixels; // number of pixels to remove
  double degree; // for testing

  vector<struct pixel_t> pixel_list;

  struct pixel_t out_first; // first pixel *before* the path
  struct pixel_t out_last;  // first pixel *behind* the path
};

struct path_window_t {
  vector<struct path_t> path_list;

  // borders of the paths
  int min_x, max_x, min_y, max_y;
};

// directions for height measurement
enum {DIRECTION_UP, DIRECTION_DOWN, DIRECTION_BOTH};

/*
 * prototype declarations
 */
static struct path_window_t make_paths(int, double, double);

// function to test if a path matches the criterion
template<class T>
static int kt_mark_paths(vector<struct path_t>&, T&, size_t, size_t, int, int);
template<class T>
static int kt_check_paths(vector<struct path_t>&, T&, size_t, size_t, int);

// copy the marked paths
template<class T>
static void kt_cp_paths(vector<struct path_t>&, T&,
                        typename ImageFactory<T>::view_type&, size_t, size_t, int);

// calling functions (for convenience)
// implement the different behaviours of the plugin
template<class T>
static void kt_handle_points(T&, typename ImageFactory<T>::view_type&,
                             vector<struct path_t>&, PointVector&, int, int);
template<class T>
static void kt_handle_all(T&, typename ImageFactory<T>::view_type&,
                          vector<struct path_t>&, int);

// small inline functions for testing pixels
template<class T>
inline static bool __is_inside(T&, size_t, size_t);
template<class T>
inline static bool __is_valid_black(T&, size_t, size_t);


/*****************************************************************************
 * keep_tall_skewed_runs
 *
 * see the documentation of the plugin for how to use this function.
 *
 * toom, 2005-01-04
 ****************************************************************************/
template<class T> 
typename ImageFactory<T>::view_type* 
keep_tall_skewed_runs(T& image,
                      double l_angle, double r_angle, int height,
                      PointVector* points, char* directionstr)
{
  typedef typename ImageFactory<T>::data_type data_t;
  typedef typename ImageFactory<T>::view_type view_t;
  int direction;

  vector<struct path_t> paths;
  struct path_window_t window;

  // image objects
  data_t* dest=new data_t(image.size(), Point(image.offset_x(),
                                              image.offset_y()));
  view_t* dest_view=new view_t(*dest);

  // return the original image in case of wrong specified
  // parameters
  if (((size_t)height > dest_view->nrows()/2) || (height < 1) ||
      (l_angle < -90.) || (l_angle > 90.) ||
      (r_angle < -90.) || (r_angle > 90.) ||
      (l_angle > r_angle))
    return dest_view;

  // set direction
  if (0==strcmp(directionstr,"up"))
    direction = DIRECTION_UP;
  else if (0==strcmp(directionstr,"down"))
    direction = DIRECTION_DOWN;
  else
    direction = DIRECTION_BOTH;

  // compute given angles to radian
  l_angle=(90-l_angle)*M_PI/180;
  r_angle=(90-r_angle)*M_PI/180;

  CDEBUG(<< "before make_paths\n");
  // height+1 because of "<" instead of "<=" tests in make_paths
  window=make_paths(2*(height+1), l_angle, r_angle);
  CDEBUG(<< "after make_paths\n");
  paths=window.path_list;

  // scan the columns of the rows specified in the vector 'y_list'.
  // if 'y_list' is empty, *all* rows will be scanned instead.
  if (points->size())
    kt_handle_points(image, *dest_view, paths, *points, height, direction);
  else
    kt_handle_all(image, *dest_view, paths, height);

  return dest_view;
}

/*****************************************************************************
 * scan the image at the rows given in 'y_list' for valid paths
 *
 * toom, 2005-01-15
 ****************************************************************************/
template<class T>
void kt_handle_points(T& image, typename ImageFactory<T>::view_type& view,
                      vector<struct path_t>& paths, PointVector& points, 
                      int height, int direction)
{
  PointVector::iterator p;

  for (p=points.begin(); p != points.end(); p++) {

    if ((p->x() >= image.ncols()) || (p->y() >= image.nrows()))
      continue;

    // path is not complete without the current pixel
    if (is_white(image.get(Point(p->x(), p->y()))))
      continue;

    if (kt_mark_paths(paths, image, p->x(), p->y(), height, direction))
      kt_cp_paths(paths, image, view, p->x(), p->y(), height);
  }
}

/*****************************************************************************
 * go through all rows of the image to look for valid paths.
 *
 * toom, 2005-01-15
 ****************************************************************************/
template<class T>
void kt_handle_all(T& image, typename ImageFactory<T>::view_type& view,
                   vector<struct path_t>& paths, int height)
{
  size_t row;
  size_t col;
  typename T::value_type black_color=black(view);

  for (row=0; row < image.nrows(); row++)

    for (col=0; col < image.ncols(); col++) {

      // path is not complete without the current pixel
      if (is_white(image.get(Point(col, row))))
        continue;

      // if the pixel is part of a path, then copy it
      if (kt_check_paths(paths, image, col, row, 2*height))
        view.set(Point(col, row), black_color);
    }
}

/*****************************************************************************
 * keep_tall_skewed_runs: kt_mark_paths
 *
 * mark the paths than can be kept (copied) when a pointlist was given by the
 * user. a path is marked as valid when it consists of black pixels and
 * (x_pos,y_pos) is in the center of the path.
 *
 * return value: 0 no valid path found
 *               1 a valid path exists and was marked
 *
 * toom, 2005-01-14
 ****************************************************************************/
template<class T>
int kt_mark_paths(vector<struct path_t>& paths, T& image, size_t x_pos,
                  size_t y_pos, int height, int direction)
{
  // iterators
  vector<struct path_t>::iterator path_iter;
  vector<struct pixel_t>::iterator pix_iter;

  int test_x, test_y;
  int path_found;
  int cnt,length,height2;

  height2=2*height;
  path_found=0;
  path_iter=paths.begin();

  CDEBUG(<< "testing (" << x_pos << "," << y_pos << ")\n");
  for (; path_iter != paths.end(); path_iter++) {

    CDEBUG(<< "\tpath: " << path_iter->degree << "\n");

    // suppose this path cannot be copied
    path_iter->mark=false;
    path_iter->len_up = path_iter->len_down = 0;

    /*
     * test the first part (upper half) of the path
     */
    if (direction != DIRECTION_DOWN) {
      CDEBUG(<< "\t    testing upper half...\n");
      cnt=length=0;
      for (pix_iter=path_iter->pixel_list.begin();
           cnt < height2 && pix_iter != path_iter->pixel_list.end();
           pix_iter++, cnt=pix_iter->y) {

        test_x=x_pos+pix_iter->x;
        test_y=y_pos-pix_iter->y;

        CDEBUG(<< "\t\t(" << test_x << "," << test_y << ")");
        if (!__is_valid_black(image, test_x, test_y)) {
          CDEBUG(<< "---> white\n");
          break;
        }
        CDEBUG(<< "---> black\n");
        length++;
      }

      // the first half of the path does not contain black
      // pixels only, so test the next path
      if (cnt < height)
        continue;

      path_iter->len_up = length;
    }

    /*
     * test the second part (lower half) of the path
     */
    if (direction != DIRECTION_UP) {
      cnt=length=0;
      CDEBUG(<< "\t    testing lower half...\n");
      for (pix_iter=path_iter->pixel_list.begin();
           cnt < height2 && pix_iter != path_iter->pixel_list.end();
           pix_iter++, cnt=pix_iter->y) {

        test_x=x_pos-pix_iter->x;
        test_y=y_pos+pix_iter->y;

        CDEBUG(<< "\t\t(" << test_x << "," << test_y << ")");
        if (!__is_valid_black(image, test_x, test_y)) {
          CDEBUG(<< "---> white\n");
          break;
        }
        CDEBUG(<< "---> black\n");
        length++;
      }

      // the second half of the path does not contain black
      // pixels only, so test the next path
      if (cnt < height)
        continue;

      path_iter->len_down = length;
    }

    // since all pixels in each half are black, mark the path
    CDEBUG(<< "\tpath will be marked\n");
    path_iter->mark=true;
    path_found=1;
  }

  return path_found;
}

/*****************************************************************************
 * keep_tall_skewed_runs: check the paths
 *
 * check each path of 'paths' if it contains black pixels only. since
 * (x_pos,y_pos) should only be a part of a path it is necessary to check both
 * the actual path and its mirrored one (other direction). if only 1 valid
 * path was found, (x_pos,y_pos) can be copied.
 *
 * return value: 0 (x_pos,y_pos) is not part of a path
 *               1 a path was found for (x_pos,y_pos)
 *
 * chris, 2005-11-04
 ****************************************************************************/
template<class T>
int kt_check_paths(vector<struct path_t>& paths, T& image, size_t x_pos,
                   size_t y_pos, int height)
{
  // iterators
  vector<struct path_t>::iterator path_iter;
  vector<struct pixel_t>::iterator pix_iter;

  int test_x, test_y;
  int path_found;
  int heightup, heightdown, heighttotal;

  path_found=0;
  path_iter=paths.begin();

  CDEBUG(<< "testing (" << x_pos << "," << y_pos << ")\n");
  for (; path_iter != paths.end(); path_iter++) {

    CDEBUG(<< "\tpath: " << path_iter->degree << "\n");

    // suppose this path cannot be copied
    //path_iter->mark=false;

    /*
     * test the first part (upper half) of the path
     */
    CDEBUG(<< "\t    testing upper half...\n");
    heightup=0;
    for (pix_iter=path_iter->pixel_list.begin();
         heightup <= height;
         pix_iter++) {

      test_x=x_pos+pix_iter->x;
      test_y=y_pos-pix_iter->y;

      CDEBUG(<< "\t\t(" << test_x << "," << test_y << ")");
      if (!__is_valid_black(image, test_x, test_y)) {
        CDEBUG(<< "---> white\n");
        break;
      }
      CDEBUG(<< "---> black\n");
      heightup=pix_iter->y;
    }

    // the first half of the path is already high enough
    if (heightup >= height)
      return 1;

    /*
     * test the second part (lower half) of the path
     */
    heightdown=0;
    heighttotal=heightup;
    CDEBUG(<< "\t    testing lower half...\n");
    for (pix_iter=path_iter->pixel_list.begin();
         (heighttotal < height) && (heightdown <= height);
         pix_iter++) {

      test_x=x_pos-pix_iter->x;
      test_y=y_pos+pix_iter->y;

      CDEBUG(<< "\t\t(" << test_x << "," << test_y << ")");
      if (!__is_valid_black(image, test_x, test_y)) {
        CDEBUG(<< "---> white\n");
        break;
      }
      CDEBUG(<< "---> black\n");
      heightdown=pix_iter->y;
      heighttotal=heightup+heightdown;
    }
    if (heightdown >= height)
      return 1;

    // path longer 2*height found
    if (heighttotal >= height)
      return 1;
  }

  // nothing found
  return 0;

}


/*****************************************************************************
 * keep_tall_skewed_runs: kt_cp_paths
 *
 * copy the paths that have been marked (i.e. they match the user's
 * conditions) and (x_pos,y_pos) is a member of.
 *
 * toom, 2005-01-14
 ****************************************************************************/
template<class T>
void kt_cp_paths(vector<struct path_t>& paths, T& image,
                 typename ImageFactory<T>::view_type& view, size_t x_pos,
                 size_t y_pos, int height)
{
  // iterators
  vector<struct path_t>::iterator path;
  vector<struct pixel_t>::iterator pixel;

  // pixels to remove
  size_t cp_x, cp_y;

  int length;

  // get the value of black
  typename T::value_type b=black(view);

  /*
   * go through all paths and find the ones that can be copied
   */
  path=paths.begin();

  for (; path != paths.end(); path++) {
    if (path->mark) {
      // copy the current pixel
      // (beginning of a path)
      view.set(Point(x_pos, y_pos), b);

      /*
       * copy the upper part of the path
       */
      length=0;
      for (pixel=path->pixel_list.begin();
           length < path->len_up;
           pixel++, length++) {

        cp_x=x_pos+pixel->x;
        cp_y=y_pos-pixel->y;

        view.set(Point(cp_x, cp_y), b);
      }

      /*
       * copy the lower part of the path
       */
      length=0;
      for (pixel=path->pixel_list.begin();
           length < path->len_down;
           pixel++, length++) {

        cp_x=x_pos-pixel->x;
        cp_y=y_pos+pixel->y;

        view.set(Point(cp_x, cp_y), b);
      }

      path->mark=false;
      path->len_up=path->len_down=0;
    }
  }
}

/*****************************************************************************
 * create the paths relative to the current pixel
 *
 * the returned list starts with the leftmost path and ends with the
 * rightmost path.
 * as all paths start in (0,0) the pixel values in the paths are relative
 * to (0,0) although the point (0,0) itself is *not* included.
 * to test if a path starts in (0,0), ('out_first.x','out_first.y') will be
 * computed. to look behind a path (to check if it reaches its end),
 * ('out_last.x','out_last.y') are computed as well.
 *
 * toom, 2004-12-27
 ****************************************************************************/
static struct path_window_t make_paths(int height, double l_angle,
                                       double r_angle)
{
  // coordinates to where a path is going to (from (0,0))
  int ref_x, ref_y;

  int factor;     // indicates the quadrant the pixel is in
  double alpha;
  double tmp_alpha;
  double tan_alpha;
  struct pixel_t pixel;
  struct path_t path;
  vector<struct pixel_t>::iterator pix_iter;
  struct path_window_t window;

  if ((r_angle > l_angle) || (l_angle-r_angle > 2*M_PI) ||
      (l_angle < -2*M_PI) || (l_angle > 2*M_PI) ||
      (r_angle < -2*M_PI) || (r_angle > 2*M_PI))
    return window;

  alpha=r_angle;
  if (alpha < 0)
    alpha+=2*M_PI;

  /*
   * compute start values
   */
  if ((alpha <= 3*M_PI_4) && (alpha >= M_PI_4)) {
    ref_y=height;
    if (alpha != M_PI_2)
      ref_x=(int)::round(ref_y/tan(alpha));
    else
      ref_x=0;
  } else if ((alpha >= 5*M_PI_4) && (alpha <= 7*M_PI_4)) {
    ref_y=-height;
    if (alpha != 3*M_PI_2)
      ref_x=(int)::round(ref_y/tan(alpha));
    else
      ref_x=0;
  } else if ((alpha > 3*M_PI_4) && (alpha < 5*M_PI_4)) {
    ref_x=-height;
    ref_y=(int)::round(ref_x*tan(alpha));
  } else {
    ref_x=height;
    ref_y=(int)::round(ref_x*tan(alpha));
  }

  // get the starting angle as specified
  alpha=r_angle;

  // set the window
  window.min_x=0;
  window.max_x=0;
  window.min_y=0;
  window.max_y=0;

  /*
   * create the paths
   */
  while (alpha <= l_angle) {
    // get the pixel that is *behind* the path (seen from (0,0))
    path.out_last.x=ref_x;
    path.out_last.y=ref_y;

    /*
     * regions where     PI/4 < alpha < 3*PI/4 or
     *                 5*PI/4 < alpha < 7*PI/4
     */
    if ((ref_x > -height) && (ref_x < height)) {
      if (ref_x != 0) {
        tan_alpha=tan(alpha);

        // for debugging output
        path.degree=atan(tan_alpha)*180/M_PI;

        /*
         * take care of the different directions
         * of each path
         */
        if (ref_y < 0) {
          // get the pixel that is *before* the
          // path (seen from (0,0))
          path.out_first.y=1;
          path.out_first.x=(int)::round(1/tan_alpha);

          for (pixel.y=-1; pixel.y > -height; pixel.y--) {
            pixel.x=(int)::round(pixel.y/tan_alpha);
            path.pixel_list.push_back(pixel);
          }
        } else {
          // get the pixel that is *before* the
          // path (seen from (0,0))
          path.out_first.y=-1;
          path.out_first.x=(int)::round(-1/tan_alpha);

          for (pixel.y=1; pixel.y < height; pixel.y++) {
            pixel.x=(int)::round(pixel.y/tan_alpha);
            path.pixel_list.push_back(pixel);
          }
        }
        /*
         * paths along the x-axis
         */
      } else {
        // for debugging output
        path.degree=90.0;

        pixel.x=0;

        /*
         * take care of the different directions
         * of each path
         */
        if (ref_y < 0) {
          path.out_first.y=1;
          path.out_first.x=0;

          for (pixel.y=-1; pixel.y > -height; pixel.y--) {
            path.pixel_list.push_back(pixel);
          }
        } else {
          path.out_first.y=-1;
          path.out_first.x=0;

          for (pixel.y=1; pixel.y < height; pixel.y++) {
            path.pixel_list.push_back(pixel);
          }
        }
      }
      /*
       * regions where    3*PI/4 < alpha < 5*PI/4   or
       *                      0 <= alpha < PI/4     or
       *                  7*PI/4 < alpha <= 2*PI
       */
    } else {
      tan_alpha=tan(alpha);

      // for debugging output
      path.degree=atan(tan_alpha)*180/M_PI;

      /*
       * take care of the different directions of each path
       */
      if (ref_x > 0) {
        path.out_first.x=-1;
        path.out_first.y=(int)::round(-tan_alpha);
        CDEBUG(<< "path.out_first.x=" << path.out_first.x << "\n");
        CDEBUG(<< "path.out_first.y=" << path.out_first.y << "\n");

        pixel.x = 1;
        pixel.y=(int)::round(pixel.x*tan_alpha);
        do {
        //for (pixel.x=1; abs(pixel.y) < height; pixel.x++) {
          path.pixel_list.push_back(pixel);
          pixel.x++;
          pixel.y=(int)::round(pixel.x*tan_alpha);
        } while (abs(pixel.y) < height);
      } else {
        path.out_first.x=1;
        path.out_first.y=(int)::round(tan_alpha);
        CDEBUG(<< "path.out_first.x=" << path.out_first.x << "\n");
        CDEBUG(<< "path.out_first.y=" << path.out_first.y << "\n");

        pixel.x = -1;
        pixel.y=(int)::round(pixel.x*tan_alpha);
        do {
        //for (pixel.x=-1; abs(pixel.y) < height; pixel.x--) {
          path.pixel_list.push_back(pixel);
          pixel.y=(int)::round(pixel.x*tan_alpha);
          pixel.x--;
        } while (abs(pixel.y) < height);
      }
    }

    /*
     * finally append the path to the list
     */
    path.mark=false;
    path.marked_pixels=0;
    window.path_list.push_back(path);

    /*
     * get the maximum values of the window,
     * described by all path
     */
    // access the last pixel of the current path
    pix_iter=path.pixel_list.end();
    CDEBUG(<< "--pixiter... (length " << path.pixel_list.size() << ")" );
    pixel=*(--pix_iter);
    CDEBUG(<< "done\n");

    if (pixel.x < window.min_x)
      window.min_x=pixel.x;
    else if (pixel.x > window.max_x)
      window.max_x=pixel.x;

    if (pixel.y < window.min_y)
      window.min_y=pixel.y;
    else if (pixel.y > window.max_y)
      window.max_y=pixel.y;

    path.pixel_list.clear();

    /*
     * compute the next values for 'ref_x' and 'ref_y'
     */
    // 1st and 4th quadrant
    if (ref_x > 0) {
      if ((ref_x < height) && (ref_y == -height)) {
        ref_x++;
        factor=2;
      } else if ((ref_x == height) && (ref_y < height)) {
        ref_y++;
        if (ref_y < 0)
          factor=2;
        else
          factor=0;
      } else {
        ref_x--;
        factor=0;
      }
      // 2nd and 3rd quadrant
    } else {
      factor=1;
      if ((ref_x > -height) && (ref_y == height))
        ref_x--;
      else if ((ref_x == -height) && (ref_y > -height))
        ref_y--;
      else {
        ref_x++;
        if (ref_x > 0)
          factor=2;
      }
    }

    /*
     * compute the correct angle 'alpha'
     */
    if (ref_x != 0) {
      tmp_alpha=atan((double)ref_y/ref_x);
      tmp_alpha+=factor*M_PI;
    } else {
      if (ref_y < 0)
        tmp_alpha=3*M_PI_2;
      else
        tmp_alpha=M_PI_2;
    }

    // handle overflows
    if (tmp_alpha < alpha)
      tmp_alpha+=2*M_PI;
    else if (tmp_alpha-2*M_PI > alpha)
      tmp_alpha-=2*M_PI;

    alpha=tmp_alpha;
  }

  return window;
}

/*****************************************************************************
 * test if (x,y) is inside of the image.
 *
 * toom, 2005-01-11
 ****************************************************************************/
template<class T>
bool __is_inside(T& image, size_t x, size_t y)
{
  if (x < 0 || x >= image.ncols() || y < 0 || y >= image.nrows())
    return false;
  else
    return true;
}

/*****************************************************************************
 * test if (x,y) is a valid black pixel.
 *
 * toom, 2005-01-11
 ****************************************************************************/
template<class T>
bool __is_valid_black(T& image, size_t x, size_t y)
{
  if (!__is_inside(image, x, y) || is_white(image.get(Point(x, y))))
    return false;
  else
    return true;
}

#endif
