/*
 * Copyright (C) 2005 Christoph Dalitz, Florian Pose
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

#ifndef STAFF_FINDING_MIYAO_2005
#define STAFF_FINDING_MIYAO_2005

#include "gamera.hpp"

using namespace Gamera;

template<class T>
PyObject *miyao_candidate_points(const T &m,
                               unsigned int scan_line_count,
                               unsigned int tolerance,
                               unsigned int staff_line_height)
{
  size_t scan_line, row, column, black_count, cp_row;
  double scan_pos_factor;
  PyObject* pyobj; // helper variable

  PyObject *return_list = PyList_New(0);

  scan_pos_factor = (double) m.ncols() / (scan_line_count + 1);

  for (scan_line = 1; scan_line <= scan_line_count; scan_line++)
  {
    column = (size_t) (scan_line * scan_pos_factor);
    black_count = 0;

    PyObject *scan_line_list = PyList_New(0);

    // mark column in list
    pyobj = PyLong_FromUnsignedLong(column);
    PyList_Append(scan_line_list, pyobj);
    Py_DECREF(pyobj);

    for (row = 0; row < m.nrows(); row++)
    {
      if (is_black(m.get(Point(column, row))))
      {
        black_count++;
      }
      else // pixel not black => evaluate counter
      {
        
        // black runlength within tolerance?
        if (black_count > 0 &&
            black_count >= staff_line_height - tolerance &&
            black_count <= staff_line_height + tolerance)
          {
            // yes: determine coordinates and mark in list
            cp_row = row - black_count / 2 - 1;
            pyobj = PyLong_FromUnsignedLong(cp_row);
            PyList_Append(scan_line_list, pyobj);
            Py_DECREF(pyobj);
          }

        black_count = 0;
      }
    }

    // add candidate points to list
    PyList_Append(return_list, scan_line_list);
    Py_DECREF(scan_line_list);
  }

  return return_list;
}


template<class T>
int miyao_distance(T &img, int xa, int ya, int xb, int yb,
                    float blackness, float pr_angle, int penalty)
{
  // some constants
  float onedegree = 0.017453;  // one degree in radian
  float twentydegree = 0.34907; // twenty degree in radian
  float pihalf = 1.5708; // pi / 2
  float angle;
  double dy, mm;
  int y;
  size_t blacksum = 0;

  // make sure a is left from b
  if (xa > xb) {
    int help;
    help=xa; xa=xb; xb=help;
    help=ya; ya=yb; yb=help;
  }

  // first criterium: check angle
  mm = double(yb-ya) / double(xb-xa);
  angle = atan(mm);
  if (abs(pr_angle) >= pihalf) {
    if (abs(angle) > twentydegree)
      return penalty;
  } else {
    if (abs(pr_angle - angle) > onedegree)
      return penalty;
  }

  // second criterium: blackness along line
  dy = 0.0;
  for (int x=xa; x < xb; x++) {
    y = ya + int(dy+0.5);
    if (is_black(img.get(Point(x, y))) || is_black(img.get(Point(x, y-1))) ||
        is_black(img.get(Point(x, y+1)))) {
      blacksum += 1;
    }
    dy += mm;
  }
  if (blacksum * 1.0 / (xb-xa) < blackness)
    return penalty;

  // both conditions are met
  return 0;

}

template<class T>
IntVector* miyao_find_edge(T &img, int x, int y, float angle,
                           bool leftdirection)
{
  int xx, yy, nrows, ncols;
  int direction;
  double dy, mm;
  IntVector* edgepoint = new IntVector(2, 0);

  mm = tan(angle);
  if (leftdirection)
    direction = -1;
  else
    direction = +1;
  xx = x;
  yy = y;
  dy = 0.0;
  nrows = img.nrows();
  ncols = img.ncols();

  // when pixel black, scan until white pixel is found
  if (is_black(img.get(Point(xx, yy))) ||
      ((yy+1 < nrows) && is_black(img.get(Point(xx, yy+1)))) ||
      ((yy > 0) && is_black(img.get(Point(xx, yy-1))))) {
    do {
      xx += direction;
      dy += direction * mm;
      yy = y + int(dy + 0.5);
    } while ((xx > 0) && (xx+1 < ncols) && (yy > 0) && (yy+1 < nrows) &&
             (is_black(img.get(Point(xx, yy))) ||
              is_black(img.get(Point(xx, yy+1))) ||
              is_black(img.get(Point(xx, yy-1)))));
    // return paenultimate point
    (*edgepoint)[0] = xx - direction;
    (*edgepoint)[1] = yy + int(dy - (direction * mm) + 0.5);
  }

  // otherwise scan in opposite direction until black pixel is found
  else {
    do {
      xx -= direction;
      dy -= direction * mm;
      yy = y + int(dy + 0.5);
    } while ((xx > 0) && (xx+1 < ncols) && (yy > 0) && (yy+1 < nrows) &&
             (is_white(img.get(Point(xx, yy))) ||
              is_white(img.get(Point(xx, yy + 1))) ||
              is_white(img.get(Point(xx, yy - 1)))));
    // return paenultimate point
    (*edgepoint)[0] = xx + direction;
    (*edgepoint)[1] = yy + int(dy + (direction * mm) + 0.5);
  }

  return edgepoint;
}

#endif
