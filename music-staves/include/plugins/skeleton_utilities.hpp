/*
 * Copyright (C) 2005 Christoph Dalitz
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

#ifndef SKELETON_UTILITIES_CD_2005
#define SKELETON_UTILITIES_CD_2005

using namespace std;
using namespace Gamera;

#include <math.h>
#include <vector>
#include <algorithm>
#include <map>
#include <stdio.h>

#include <gamera.hpp>
#include <plugins/segmentation.hpp>
#include <plugins/draw.hpp>
#include <plugins/structural.hpp>

typedef std::vector<FloatPoint> FloatPointVector;

// p is an array with 6 elements
#define CALC_XT(p, t) (((p)[0]*(t)+(p)[2])*(t)+(p)[4])
#define CALC_YT(p, t) (((p)[1]*(t)+(p)[3])*(t)+(p)[5])

#define N_POINTS_DEFAULT 5

static void __check_parabola(const Point& p, const FloatVector& p_v,
    const double t, coord_t* dx, coord_t* dy);
void __linear(const FloatPointVector& points, FloatVector* params);

/*****************************************************************************
 * nconnectivity
 *
 * returns the connectivity number of a skeleton point
 * WARNING: does not check whether r+-1 and c+-1 are outside of the image
 *
 * chris, 2005-07-13
 ****************************************************************************/
template<class T>
inline int nconnectivity(T& image, int c, int r)
{
  int n;
  bool lastblack;
  bool neighbors[8] = { is_black(image.get(Point(c - 1, r - 1))),
                        is_black(image.get(Point(c, r - 1))),
                        is_black(image.get(Point(c + 1, r - 1))),
                        is_black(image.get(Point(c + 1, r))),
                        is_black(image.get(Point(c + 1, r + 1))),
                        is_black(image.get(Point(c, r + 1))),
                        is_black(image.get(Point(c - 1, r + 1))),
                        is_black(image.get(Point(c - 1, r))) };

  // note that two neigboring black pixels must be ignored
  // because in that case one of these is the critical point
  n = 0; 
  lastblack = neighbors[7];
  for (int i=0; i<8; i++) {
    if (neighbors[i] && !lastblack)
      n++;
    lastblack = neighbors[i];
  }

  // special case squares of four black points:
  // set northwest and southeast point as "critical"
  if ((neighbors[7] && neighbors[0] && neighbors[1]) ||
      (neighbors[3] && neighbors[4] && neighbors[5]))
    n+=2;

  // special case squares of four black points:
  // set northeast and southwest point as "critical"
  if ((neighbors[7] && neighbors[6] && neighbors[5]) ||
      (neighbors[1] && neighbors[2] && neighbors[3]))
    n+=2;

  return n;
}

/*****************************************************************************
 * get_neighbors
 *
 * Fills a given PointVector with all neighbor points of a point that
 * have a pixel value == 1.
 * In case of adjacent neighborpoints, only the 4-connected points are
 * considered.
 *
 * Returns the number of neighbors (usually 1, but can be 0 for
 * endpoints and >1 for branching points)
 *
 * chris, 2005-08-09
 ****************************************************************************/
template<class T>
inline int get_neighbors(T& image, int r, int c, PointVector* neighbors)
{
  int n;
  int x[8] = { c-1, c-1, c-1, c, c+1, c+1, c+1, c };
  int y[8] = { r+1, r, r-1, r-1, r-1, r, r+1, r+1 };
  int cmax = image.ncols() - 1;
  int rmax = image.nrows() - 1;
  bool nextdoorniks[8] = { 
    (r >= rmax || c <= 0) ? false :    (1 == image.get(Point(c - 1, r + 1))),
    (c <= 0) ? false :                 (1 == image.get(Point(c - 1, r))),
    (r <= 0 || c <= 0) ? false :       (1 == image.get(Point(c - 1, r - 1))),
    (r <= 0) ? false :                 (1 == image.get(Point(c, r - 1))),
    (r <= 0 || c >= cmax) ? false :    (1 == image.get(Point(c + 1, r - 1))),
    (c >= cmax) ? false :              (1 == image.get(Point(c + 1, r))),
    (r >= rmax || c >= cmax) ? false : (1 == image.get(Point(c + 1, r + 1))),
    (r >= rmax) ? false :              (1 == image.get(Point(c, r + 1)))
  };

  neighbors->clear();

  // in case of adjacent neighbors only the 4-connected
  // neighbors are counted, so that skeletons can be followed around corners
  n = 0;
  for (int i=0; i<8; i++) {
    if (nextdoorniks[i]) {
      // pick 4-connected always and 8-connected only
      // when no adjacent 4-connected neighbor exists
      // (note the workaround for the modulo bug in C, which yields
      // incorrect values for negative operands)
      if ((i%2) || (!nextdoorniks[(i+7)%8] && !nextdoorniks[(i+1)%8])) {
        neighbors->push_back(Point(x[i],y[i]));
        n++;
      }
    }
  }

  return n;
}

/*****************************************************************************
 * get_neighbors_with5
 *
 * Like get_neighbors, but pixel values may be 1 or 5.
 *
 * Returns the number of neighbors (usually 1, but can be 0 for
 * endpoints and >1 for branching points)
 *
 * chris, 2006-02-03
 ****************************************************************************/
template<class T>
inline int get_neighbors_with5(T& image, int r, int c, PointVector* neighbors)
{
  int n;
  int x[8] = { c-1, c-1, c-1, c, c+1, c+1, c+1, c };
  int y[8] = { r+1, r, r-1, r-1, r-1, r, r+1, r+1 };
  int cmax = image.ncols() - 1;
  int rmax = image.nrows() - 1;
  bool nextdoorniks[8] = { 
    (r >= rmax || c <= 0) ? false:    ((1 == image.get(Point(c - 1, r + 1))) ||
                                       (5 == image.get(Point(c - 1, r + 1)))),
    (c <= 0) ? false:                 ((1 == image.get(Point(c - 1, r))) ||
                                       (5 == image.get(Point(c - 1, r)))),
    (r <= 0 || c <= 0) ? false:       ((1 == image.get(Point(c - 1, r - 1))) ||
                                       (5 == image.get(Point(c - 1, r - 1)))),
    (r <= 0) ? false:                 ((1 == image.get(Point(c, r - 1))) ||
                                       (5 == image.get(Point(c, r - 1)))),
    (r <= 0 || c >= cmax) ? false:    ((1 == image.get(Point(c + 1, r - 1))) ||
                                       (5 == image.get(Point(c + 1, r - 1)))),
    (c >= cmax) ? false:              ((1 == image.get(Point(c + 1, r))) ||
                                       (5 == image.get(Point(c + 1, r)))),
    (r >= rmax || c >= cmax) ? false: ((1 == image.get(Point(c + 1, r + 1))) ||
                                       (5 == image.get(Point(c + 1, r + 1)))),
    (r >= rmax) ? false:              ((1 == image.get(Point(c, r + 1))) ||
                                       (5 == image.get(Point(c, r + 1))))
  };

  neighbors->clear();

  // in case of adjacent neighbors only the 4-connected
  // neighbors are counted, so that skeletons can be followed around corners
  n = 0;
  for (int i=0; i<8; i++) {
    if (nextdoorniks[i]) {
      // pick 4-connected always and 8-connected only
      // when no adjacent 4-connected neighbor exists
      // (note the workaround for the modulo bug in C, which yields
      // incorrect values for negative operands)
      if ((i%2) || (!nextdoorniks[(i+7)%8] && !nextdoorniks[(i+1)%8])) {
        neighbors->push_back(Point(x[i],y[i]));
        n++;
      }
    }
  }

  return n;
}

// /*****************************************************************************
//  * get_neighbors_prefer5
//  *
//  * like get_neighbors, but when a neighbor has pixel value 5, this
//  * neighbor is preferred over its adjacent points. Thus you can ensure
//  * that branches always end at branching points, provided branching
//  * points are marked with pixel value 5 (used in split_skeletoen).
//  *
//  * chris, 2005-08-19
//  ****************************************************************************/
// template<class T>
// inline int get_neighbors_prefer5(T& image, int r, int c, PointVector* neighbors)
// {
//   int n;
//   int x[8] = { c-1, c-1, c-1, c, c+1, c+1, c+1, c };
//   int y[8] = { r+1, r, r-1, r-1, r-1, r, r+1, r+1 };
//   int cmax = image.ncols() - 1;
//   int rmax = image.nrows() - 1;
//   bool nextdoorniks[8] = { 
//     (r >= rmax || c <= 0) ? false :    (1 == image.get(Point(c - 1, r + 1))),
//     (c <= 0) ? false :                 (1 == image.get(Point(c - 1, r))),
//     (r <= 0 || c <= 0) ? false :       (1 == image.get(Point(c - 1, r - 1))),
//     (r <= 0) ? false :                 (1 == image.get(Point(c, r - 1))),
//     (r <= 0 || c >= cmax) ? false :    (1 == image.get(Point(c + 1, r - 1))),
//     (c >= cmax) ? false :              (1 == image.get(Point(c + 1, r))),
//     (r >= rmax || c >= cmax) ? false : (1 == image.get(Point(c + 1, r + 1))),
//     (r >= rmax) ? false :              (1 == image.get(Point(c, r + 1)))
//   };
//   bool branchpoints[8] = { 
//     (r >= rmax || c <= 0) ? false :    (5 == image.get(Point(c - 1, r + 1))),
//     (c <= 0) ? false :                 (5 == image.get(Point(c - 1, r))),
//     (r <= 0 || c <= 0) ? false :       (5 == image.get(Point(c - 1, r - 1))),
//     (r <= 0) ? false :                 (5 == image.get(Point(c, r - 1))),
//     (r <= 0 || c >= cmax) ? false :    (5 == image.get(Point(c + 1, r - 1))),
//     (c >= cmax) ? false :              (5 == image.get(Point(c + 1, r))),
//     (r >= rmax || c >= cmax) ? false : (5 == image.get(Point(c + 1, r + 1))),
//     (r >= rmax) ? false :              (5 == image.get(Point(c, r + 1)))
//   };

//   neighbors->clear();

//   // in case of branching points, include them into nextdoorniks,
//   // but ignore their neighborhood
//   for (int i=0; i<8; i++) {
//     if (branchpoints[i]) {
//       nextdoorniks[i] = true;
//       nextdoorniks[(i+7)%8] = nextdoorniks[(i+1)%8] = false;
//     }
//   }

//   // in case of adjacent neighbors only the 4-connected
//   // neighbors are counted, so that skeletons can be followed around corners
//   n = 0;
//   for (int i=0; i<8; i++) {
//     if (nextdoorniks[i]) {
//       // pick 4-connected always and 8-connected only
//       // when no adjacent 4-connected neighbor exists
//       // (note the workaround for the modulo bug in C, which yields
//       // incorrect values for negative operands)
//       if ((i%2) || (!nextdoorniks[(i+7)%8] && !nextdoorniks[(i+1)%8])) {
//         neighbors->push_back(Point(x[i],y[i]));
//         n++;
//       }
//     }
//   }

//   return n;
// }

/*****************************************************************************
 * remove_spurs_from_skeleton
 *
 * removes branches shorter than length from a skeleton image
 *
 * chris, 2005-08-16
 ****************************************************************************/
template<class T>
typename ImageFactory<T>::view_type* remove_spurs_from_skeleton(T& image, int length, int endtreatment)
{
  typename ImageFactory<T>::view_type* newimage;
  typename T::value_type white_value = white(image);
  typename T::value_type black_value = black(image);
  vector<PointVector*> spurs, nospurs;
  PointVector startpoints, neighbors;
  PointVector *branch;
  PointVector::iterator p,q;
  Point pp;
  int n, nn;
  newimage = simple_image_copy(image);

  // check for branching points
  for (size_t r=1; r<image.nrows()-1; r++) {
    for (size_t c=1; c<image.ncols()-1; c++) {
      if (is_black(image.get(Point(c, r))) && (get_neighbors(image,r,c,&startpoints) > 2)) {
        //printf("branching point at (%d,%d) with %d branches\n",c,r,startpoints.size());
        image.set(Point(c, r), 2);
        for (p=startpoints.begin(); p!=startpoints.end(); p++)
          image.set(Point(p->x(), p->y()), 2);
        for (p=startpoints.begin(); p!=startpoints.end(); p++) {
          // follow branch (note that visited points must be set to two)
          branch = new PointVector(0); n = 0;
          neighbors.clear(); neighbors.push_back(*p);
          do {
            pp = neighbors.front();
            branch->push_back(pp);
            image.set(Point(pp.x(), pp.y()), 2);
            n++;
            nn = get_neighbors(image,pp.y(),pp.x(),&neighbors);
          } while ((n<=length) && (nn==1));
          if ((n<=length) && (nn==0)) {
            // mark as spur
            spurs.push_back(branch);
          } else {
            // mark as no spur
            nospurs.push_back(branch);
          }
        }
        // do we have an endpoint?
        // criterion: two equally long spurs + one non spur
        bool isendpoint = false;
        if ((nospurs.size()==1) && (spurs.size()==2) &&
            (abs((int)(spurs[0]->size() - spurs[1]->size())) < 2))
          isendpoint = true;
        // remove spurs in new image ...
        if (endtreatment == 0 || !isendpoint) {
          for (size_t i=0; i<spurs.size(); i++) {
            for (p=spurs[i]->begin(); p!=spurs[i]->end(); p++) {
              newimage->set(Point(p->x(), p->y()), white_value);
            }
          }
        }
        // ... or interpolate endpoints
        else if (endtreatment == 2) {
          coord_t x = (spurs[0]->back().x() + spurs[1]->back().x()) / 2;
          coord_t y = (spurs[0]->back().y() + spurs[1]->back().y()) / 2;
          for (size_t i=0; i<spurs.size(); i++) {
            for (p=spurs[i]->begin(); p!=spurs[i]->end(); p++) {
              newimage->set(Point(p->x(), p->y()), white_value);
            }
          }
          // plausi check: is the new endpoint beyond the old?
          // i.e. is the scalar product <b-a,c-b> > 0?
          if (0 < (x-c)*(c-nospurs[0]->front().x()) +
              (y-r)*(r-nospurs[0]->front().y())) {
            draw_line(*newimage, FloatPoint(c, r), FloatPoint(x, y), black_value);
          }
        }
        // reset all changed pixels back to one in old image
        // and clean up dynamically allocated vectors
        image.set(Point(c, r), 1);
        for (size_t i=0; i<spurs.size(); i++) {
          for (p=spurs[i]->begin(); p!=spurs[i]->end(); p++) {
            image.set(Point(p->x(), p->y()), black_value);
          }
          delete spurs[i];
        }
        spurs.clear();
        for (size_t i=0; i<nospurs.size(); i++) {
          for (p=nospurs[i]->begin(); p!=nospurs[i]->end(); p++)
            image.set(Point(p->x(), p->y()), black_value);
          delete nospurs[i];
        }
        nospurs.clear();
      }
    }
  }
  return newimage;
}


/*****************************************************************************
 * get_corner_points
 *
 * fills the given IntVector with the indices of all corner points in segment
 * the number of found corner points is returned
 *
 * chris, 2005-08-18
 ****************************************************************************/
int get_corner_points_cpp(const PointVector &segment, int cornerwidth, IntVector* cornerindices) {
  int n = 0;
  int maxi;
  Point p1,p2,p3;  // anchor point
  double v1x, v1y; // vector p2 -> p1
  double v2x, v2y; // vector p2 -> p3
  IntVector candindices(0);  // corner point candidate indices
  FloatVector candangles(0); // corner point candidate angles
  double angle, var1, var2, delta, x, y, diff;
  const double maxangle = (180.0 - 45.0) * M_PI / 180.0;

  cornerindices->clear();
  if ((cornerwidth < 2) || ((int)segment.size() < 2*cornerwidth+1)) {
    return 0;
  }

  // 1) collect corner point candidates
  maxi = segment.size() - cornerwidth;
  for (int i=cornerwidth; i<maxi; i++) {
    // measure angle
    p1 = segment[i-cornerwidth]; p2 = segment[i]; p3 = segment[i+cornerwidth];
    v1x = (double)p1.x() - p2.x(); v1y = (double)p1.y() - p2.y();
    v2x = (double)p3.x() - p2.x(); v2y = (double)p3.y() - p2.y();
    angle = acos( (v1x*v2x + v1y*v2y) / 
                  sqrt((v1x*v1x+v1y*v1y) * (v2x*v2x+v2y*v2y)) );
    if (angle < maxangle) {
      // measure deviation (variance) from straight line
      var1 = 0.0;
      if (fabs(v1x) > fabs(v1y)) {
        if (fabs(v1x) < 0.01) continue;
        delta = v1y/fabs(v1x); y = p2.y();
        for (int j=1; j<cornerwidth; j++) {
          y += delta;
          diff = y - segment[i-j].y(); var1 += diff*diff;
        }
      } else {
        if (fabs(v1y) < 0.01) continue;
        delta = v1x/fabs(v1y); x = p2.x();
        for (int j=1; j<cornerwidth; j++) {
          x += delta;
          diff = x - segment[i-j].x(); var1 += diff*diff;
        }
      }
      var2 = 0.0;
      if (fabs(v2x) > fabs(v2y)) {
        if (fabs(v2x) < 0.01) continue;
        delta = v2y/fabs(v2x); y = p2.y();
        for (int j=1; j<cornerwidth; j++) {
          y += delta;
          diff = y - segment[i+j].y(); var1 += diff*diff;
        }
      } else {
        if (fabs(v2y) < 0.01) continue;
        delta = v2x/fabs(v2y); x = p2.x();
        for (int j=1; j<cornerwidth; j++) {
          x += delta;
          diff = x - segment[i+j].x(); var1 += diff*diff;
        }
      }
      // when close enough to straight line: mark as candidate point
      if ((var1 <= cornerwidth*0.5) && (var2 <= cornerwidth*0.5)) {
        candindices.push_back(i); candangles.push_back(angle);
      }
    }
  }

  // 2) pick corner points from candidates as minimum angle
  //    in contiguous candidate region
  if (candindices.empty()) return 0;
  double minangle = M_PI; int minindex = 0;
  int lastindex = candindices.front() - 1;
  for (size_t i=0; i<candindices.size(); i++) {
    if ((lastindex+1 != candindices[i]) && (candindices[i]-minindex > cornerwidth)) {
      cornerindices->push_back(minindex); n++;
      minangle = candangles[i]; minindex = candindices[i];
    }
    if (candangles[i] < minangle) {
      minangle = candangles[i]; minindex = candindices[i];
    }
    lastindex = candindices[i];
  }
  cornerindices->push_back(minindex); n++;

  return n;
}

/* interface to python */
PointVector *get_corner_points(PointVector *pv, int m)
{
  IntVector iv;
  get_corner_points_cpp(*pv,m,&iv);
  PointVector *result=new PointVector();
  for(int i=0;i<(int)iv.size();++i)
    result->push_back((*pv)[iv[i]]);
  return result;
}

/*****************************************************************************
 * get_corner_points_rj
 *
 * fills the given IntVector with the indices of all corner points (dominant
 * points) in segment. The number of found corner points is returned.
 *
 * Based on an Algorithm described in:
 * A. Rosenfeld and E. Johnston, "Angle detection on digital curves," IEEE
 * Trans. Comput., vol. C-22, pp. 875-878, Sept. 1973
 *
 * The Paper describes finding dominant points on a closed curve. This function
 * in contrast will operate on an open curve. The first checked point is the
 * one with the index conerwidth+1 (cornerwidth corresponds to the m
 * parameter of the original paper)
 *
 * The original algorithm might find adjacent dominant points, this one does
 * not allow dominant points, whose distance (by index) is lower than
 * cornerwidth. When this happens, the algorithm decides based upon the angle
 * found (it takes the sharper one)
 *
 * bcz, 2006-04-06
 ****************************************************************************/
int get_corner_points_rj_cpp(const PointVector &segment, int cornerwidth, IntVector* cornerindices)
{
  cornerindices->clear();

  // Set cik_last_point to a high value, so is_rising will remain true
  // during the first iteration of the outer for loop
  double cik_last_point=10.0, first_cik_last_point=-2.0;
  bool is_rising=false;
  int last_cpi=-1;
  double last_cp_cik=-2;
  double minimal_cik=cos((180-40)*M_PI/180);

  if((int)segment.size()<cornerwidth) return 0;

  for(int i=cornerwidth;i<(int)segment.size()-cornerwidth;++i)
  {
    Point pi=segment[i];

    // Calculate cik=cos(alpha) of the point pi
    double cik_old=-2.0,cik=-2.0,first_cik=-2.0;
    for(int k=cornerwidth;k>=3;--k)
    {
      cik_old=cik;
      Point pipk=segment[i+k];
      Point pimk=segment[i-k];
      int ax=pi.x()-pipk.x();
      int ay=pi.y()-pipk.y();
      int bx=pi.x()-pimk.x();
      int by=pi.y()-pimk.y();
      cik=(ax*bx+ay*by)/(sqrt((double)(ax*ax+ay*ay))*sqrt((double)(bx*bx+by*by)));
      if(k==cornerwidth)
        first_cik=cik;
      if(cik<cik_old)
        break;
    }
    cik=cik_old;

    // was the curve getting sharper before and now getting less sharp?
    if(is_rising&&cik_last_point>cik&&first_cik_last_point>=minimal_cik) {
      // was the last cornerpoint more than cornerwidth away?
      if(i-last_cpi>cornerwidth)
      {
        cornerindices->push_back(i-1);
        last_cpi=i-1;
        last_cp_cik=cik_last_point;
      }
      else
        // if not, is the new cik higher than the old one?
        if(cik_last_point>last_cp_cik)
        {
          cornerindices->pop_back();
          cornerindices->push_back(i-1);
          last_cpi=i-1;
          last_cp_cik=cik_last_point;
        }
    }

    // keep is_rising unchanged, if cik==cik_last_point
    if(cik>cik_last_point)
      is_rising=true;
    else if(cik<cik_last_point)
      is_rising=false;

    cik_last_point=cik;
    first_cik_last_point=first_cik;
  }

  return cornerindices->size();
}

/* interface to python */
PointVector *get_corner_points_rj(PointVector *pv, int m)
{
  IntVector iv;
  get_corner_points_rj_cpp(*pv,m,&iv);
  PointVector *result=new PointVector();
  for(int i=0;i<(int)iv.size();++i)
    result->push_back((*pv)[iv[i]]);
  return result;
}

/*****************************************************************************
 * create_skeleton_segment
 *
 * Creates a Python object "skeleton_segment"
 *
 * chris, 2005-08-09
 ****************************************************************************/
PyObject* create_skeleton_segment(PointVector* pvec, PointVector* bpvec)
{
  PointVector::const_iterator p;
  unsigned int top, bot, left, right;
  if (pvec->size()) {
    top = pvec->front().y(); bot = top;
    left = pvec->front().x(); right = left;
  } else {
    top = bot = left = right = 0;
  }

  // helper object for creating class instances
  // declared static so this is initialized only once
  static PyObject* skeleton_segment_class = NULL;
  if (skeleton_segment_class == NULL) {
    PyObject* class_dict = PyDict_New();
    PyObject* class_name = PyString_FromString("SkeletonSegment");
    skeleton_segment_class = PyClass_New(NULL, class_dict, class_name);
  }

  // set points and collect some properties
  PyObject* segment_dict = PyDict_New();
  PyObject* points;
  points = PyList_New(0);
  for (p = pvec->begin(); p != pvec->end(); ++p) {
    PyObject* po = create_PointObject(*p);
    PyList_Append(points, po);
    Py_DECREF(po);
    if (top > p->y()) top = p->y();
    if (bot < p->y()) bot = p->y();
    if (left > p->x()) left = p->x();
    if (right < p->x()) right = p->x();
  }
  PyDict_SetItemString(segment_dict, "points", points);
  Py_DECREF(points);

  // add the branching points to the object
  points=PyList_New(0);
  for (p=bpvec->begin(); p != bpvec->end(); p++) {
    PyObject* po = create_PointObject(*p);
    PyList_Append(points, po);
    Py_DECREF(po);
  }
  PyDict_SetItemString(segment_dict, "branching_points", points);
  Py_DECREF(points);

  // set dimension properties
  PyObject* prop;
  prop = PyInt_FromLong(left);
  PyDict_SetItemString(segment_dict, "offset_x", prop);
  Py_DECREF(prop);
  prop = PyInt_FromLong(top);
  PyDict_SetItemString(segment_dict, "offset_y", prop);
  Py_DECREF(prop);
  if (pvec->size()) {
    prop = PyInt_FromLong(right - left + 1);
  } else {
    prop = PyInt_FromLong(0);
  }
  PyDict_SetItemString(segment_dict, "ncols", prop);
  Py_DECREF(prop);
  if (pvec->size()) {
    prop = PyInt_FromLong(bot - top + 1);
  } else {
    prop = PyInt_FromLong(0);
  }
  PyDict_SetItemString(segment_dict, "nrows", prop);
  Py_DECREF(prop);

  // fit straight line
  double m,b,confidence;
  int x_of_y;
  if (pvec->size() > 1) {
    PyObject* po = least_squares_fit_xy(pvec);
    PyArg_ParseTuple(po, "dddi", &m, &b, &confidence, &x_of_y);
    Py_DECREF(po);

    if(x_of_y)
      prop=PyFloat_FromDouble(atan2(1,m)*57.296);   // radian->degree
    else
      prop=PyFloat_FromDouble(atan2(m,1)*57.296);   // radian->degree

  } else {
    prop = Py_BuildValue("");
  }
  PyDict_SetItemString(segment_dict, "orientation_angle", prop);
  Py_DECREF(prop);

  // deviation (variance) from fitted line
  if (pvec->size() > 2) {
    double sum = 0.0, v;
    if( x_of_y )
      for( p = pvec->begin(); p < pvec->end(); ++p ){
        v = m * p->y() + b - p->x();
        sum += v * v;
      }
    else
      for( p = pvec->begin(); p < pvec->end(); ++p ){
        v = m * p->x() + b - p->y();
        sum += v * v;
      }
    prop = PyFloat_FromDouble( sum / ( pvec->size() * ( m * m + 1 ) ) );

  } else {
    prop = PyFloat_FromDouble(0.0);
  }
  PyDict_SetItemString(segment_dict, "straightness", prop);
  Py_DECREF(prop);

  // create the actual new object
  PyObject *ret = PyInstance_NewRaw(skeleton_segment_class, segment_dict);
  Py_DECREF(segment_dict);
  return ret;
}

/*****************************************************************************
 * get all adjacent branching points of the branching point (x,y), note that
 * this function does not clear the PointVector, but only pushes the new
 * elements
 *
 * this function considers that branching points are marked with 5
 *
 * toom, 2006-01-18
 ****************************************************************************/
template<class T>
inline int __get_adjacent_branching_points(T& flags, coord_t x, coord_t y,
    PointVector* adjacent_bp)
{
  size_t xmax=flags.ncols();
  size_t ymax=flags.nrows();

  // is this point a valid branching point?
  if (!(x < xmax) || !(y < ymax) || flags.get(Point(x, y)) != 5)
    return adjacent_bp->size();
  
  size_t neighbor_x[8]={x-1, x, x+1, x+1, x+1, x, x-1, x-1};
  size_t neighbor_y[8]={y-1, y-1, y-1, y, y+1, y+1, y+1, y};
  PointVector bp;
  PointVector tmp_bp;
  PointVector::iterator p;

  bp.push_back(Point(x, y));

  /*
   * check all detected branching points for adjacent branching points,
   * the already processed branching points will be added to 'adjacent_bp'
   */
  while (!bp.empty()) {
    for (p=bp.begin(); p != bp.end(); p++) {
      adjacent_bp->push_back(*p);

      tmp_bp.clear();

      for (int i=0; i < 8; i++) {
        if (neighbor_x[i] < xmax && neighbor_y[i] < ymax &&
            flags.get(Point(neighbor_x[i], neighbor_y[i])) == 5) {
          tmp_bp.push_back(Point(neighbor_x[i], neighbor_y[i]));
          flags.set(Point(neighbor_x[i], neighbor_y[i]), 6);
        }
      }
    }
    bp=tmp_bp;
  }

  // finally reset the values of the detected branching points to 5
  for (p=adjacent_bp->begin(); p != adjacent_bp->end(); p++)
    flags.set(*p, 5);

  return adjacent_bp->size();
}

/*****************************************************************************
 * split_skeleton
 *
 * splits a given skeleton image at branching and corner points
 *
 * chris, 2005-08-18
 ****************************************************************************/
template<class T>
PyObject* split_skeleton(const T& image, const FloatImageView& distance,
    int cornerwidth, int norm)
{
  if (image.nrows() != distance.nrows() || image.ncols() != distance.ncols())
    throw std::runtime_error(
        "The sizes of image and distance do not match.");

  typename ImageFactory<T>::view_type* newimage;
  Point second;
  PointVector neighbors;
  PointVector segment;
  PointVector branching_points_first;
  PointVector branching_points_second;
  PointVector::iterator p;
  IntVector cornerindices;
  IntVector::iterator corner;
  int n, nn, x, y;
  PyObject* pyobj; // helper variable

  PyObject* retlist = PyList_New(0); // return value
  newimage = simple_image_copy(image);

  /*
   * 1)
   * 
   * find all branching points and mark them with 5
   */
  for (coord_t r=1; r<image.nrows()-1; r++)
    for (coord_t c=1; c<image.ncols()-1; c++)
      if (is_black(image.get(Point(c, r))))
        // branching points
        if (nconnectivity(image, c, r) > 2) {
          newimage->set(Point(c, r), 5);
        }

  /*
   * 2)
   * 
   * split the skeleton into segments (a segment starts (and ends) either
   * with a starting pixel (ending pixel) or with a branching point)
   *
   * processed pixels are marked by setting their value to 2
   */
  for (coord_t r=0; r<newimage->nrows(); r++) {
    for (coord_t c=0; c<newimage->ncols(); c++) {
      if (1 == newimage->get(Point(c, r))) {

        // new segment detected
        segment.clear();
        branching_points_first.clear();
        branching_points_second.clear();
        segment.push_back(Point(c, r));
        newimage->set(Point(c, r), 2);

        n = get_neighbors_with5(*newimage, r, c, &neighbors);
        for (p=neighbors.begin(); p!=neighbors.end(); p++)
          if (5 != newimage->get(*p))
            newimage->set(Point(p->x(), p->y()), 2);

        if (n>2) { // branching point
          continue;
        } else if (n) {
          bool found_bp=false;

          // save the second branch for later use
          second = neighbors.back();

          // follow first branch until end point or branching point is found
          nn = 1;
          while (nn==1) {
            x = neighbors.front().x(); y = neighbors.front().y();
            if (5 == newimage->get(Point(x, y))) {
              found_bp=true;
              break;
            }
            segment.insert(segment.begin(),neighbors.front());
            newimage->set(Point(x, y), 2);
            nn = get_neighbors_with5(*newimage, y, x, &neighbors);
          }

          if (found_bp) {
            // store all branching points around this point (8-connected)
            __get_adjacent_branching_points(*newimage, x, y,
                &branching_points_first);
            found_bp=false;
          }

          // follow second branch (if it exists)
          if (n==2) {
            neighbors.clear(); neighbors.push_back(second);
            nn = 1;
            while (nn==1) {
              x = neighbors.front().x(); y = neighbors.front().y();
              if (5 == newimage->get(Point(x, y))) {
                found_bp=true;
                break;
              }
              segment.push_back(neighbors.front());
              newimage->set(Point(x, y), 2);
              nn = get_neighbors_with5(*newimage, y, x, &neighbors);
            }

            if (found_bp) {
              // store all branching points around this point (8-connected)
              __get_adjacent_branching_points(*newimage, x, y,
                  &branching_points_second);
            }
          }
        }

        /*
         * remove the pixels within the distance of the branching points
         */
        PointVector::iterator q;
        bool keep = false;
        bool oldkeep = false;
        PointVector::iterator begin_keep = segment.begin(),
                              end_keep = segment.begin();
        coord_t l, t; // left and top
        int dist; // distance of the branching point

        // go through all pixels and check whether they are within the
        // distance of a branching point, if so remove them
        for (p=segment.begin(); p != segment.end(); p++) {
          keep=true;

          // check the branching points at the beginning of this segment
          for (q=branching_points_first.begin();
              q != branching_points_first.end(); q++) {
            dist=(int)distance.get(Point(*q));
            if (dist == 0) dist=1;

            l=q->x() < (size_t)dist ? 0 : q->x()-(size_t)dist;
            t=q->y() < (size_t)dist ? 0 : q->y()-(size_t)dist;

            // do not keep the point if it is within the distance of
            // a branching point
            if (l < p->x() && p->x() < q->x()+(size_t)dist &&
                t < p->y() && p->y() < q->y()+(size_t)dist) {
              keep=false;
              break;
            }
          }

          if (keep) {
            // the pixel has not been marked for deletion so far:
            // check the branching points at the end of this segment
            for (q=branching_points_second.begin();
                q != branching_points_second.end(); q++) {
              dist=(int)distance.get(Point(*q));
              if (dist == 0) dist=1;

              l=q->x() < (size_t)dist ? 0 : q->x()-(size_t)dist;
              t=q->y() < (size_t)dist ? 0 : q->y()-(size_t)dist;

              // do not keep the point if it is within the distance of
              // a branching point
              if (l < p->x() && p->x() < q->x()+(size_t)dist &&
                  t < p->y() && p->y() < q->y()+(size_t)dist) {
                keep=false;
                break;
              }
            }
          }

          if(keep && !oldkeep && begin_keep == segment.begin())
            begin_keep = p;

          if(!keep && oldkeep)
            end_keep = p;

          oldkeep = keep;
        }

        if(keep)
          end_keep = p;

        if( end_keep == segment.begin() )
          continue;

        segment = PointVector(begin_keep, end_keep);

        // no point is left, so continue
        if (segment.empty())
          continue;

        /*
         * 3)
         *
         * both segment and its branching points (in ..._first and ..._second)
         * are now collected and the pixel within the distance of the
         * branching points have been erased
         * 
         * now split the segment at corner points: the corner points will
         * become branching points and the pixels within the distance of each
         * corner point will be removed as well
         */
        if ((cornerwidth>0) && 
            get_corner_points_rj_cpp(segment, cornerwidth, &cornerindices)) {
          int ibegin = 0;
          int imax, dist;

          for (corner=cornerindices.begin(); corner!=cornerindices.end();
              corner++) {
            dist = (int)distance.get(segment[*corner]);
            if (dist == 0) dist=1;
            imax = *corner-dist;

            if (imax >= ibegin) {
              PointVector p(0);
              PointVector bp(0);

              /*
               * check if the first part (until the first detected corner) of
               * this segment is next to a branching point, if so, link this
               * branching point to this segment and leave the others in
               * 'branching_points'
               */
              if (ibegin == 0)
                bp=branching_points_first;

              for (int i=ibegin; i <= imax; i++)
                p.push_back(segment[i]);

              // treat the corner points (both the current and the last one)
              // as branching points after splitting
              if (corner != cornerindices.begin())
                bp.push_back(segment[*(corner-1)]);
              bp.push_back(segment[*corner]);

              // copy over new fragment
              if (p.front().x() > p.back().x()) reverse(p.begin(),p.end());
              pyobj = create_skeleton_segment(&p, &bp);
              PyList_Append(retlist, pyobj);
              Py_DECREF(pyobj);
            }
            ibegin = *corner + dist;
          }

          // do not forget last fragment
          if (ibegin < (int)segment.size()) {
            PointVector p(0);
            PointVector bp(0);

            imax=(int)segment.size();

            bp=branching_points_second;

            // treat the corner point as a branching point
            if (corner != cornerindices.begin())
              bp.insert(bp.begin(), segment[*(corner-1)]);

            if (ibegin < imax) {
              for (int i=ibegin; i < imax; i++) {
                p.push_back(segment[i]);
              }

              // copy over new fragment
              if (p.front().x() > p.back().x()) reverse(p.begin(),p.end());
              pyobj = create_skeleton_segment(&p, &bp);
              PyList_Append(retlist, pyobj);
              Py_DECREF(pyobj);
            }
          }
        } else {
          // copy over entire segment
          for (PointVector::iterator i=branching_points_second.begin();
              i != branching_points_second.end(); i++)
            branching_points_first.push_back(*i);

          if (segment.front().x() > segment.back().x())
            reverse(segment.begin(),segment.end());

          pyobj = create_skeleton_segment(&segment,&branching_points_first);
          PyList_Append(retlist, pyobj);
          Py_DECREF(pyobj);
        }
      }
    }
  }

  delete newimage->data();
  delete newimage;
  return retlist;
}


/*****************************************************************************
 * distance_precentage_among_points
 *
 * returns the fraction of points with a border distance value of distance
 * plusminus tolerance
 *
 * chris, 2005-08-19
 ****************************************************************************/
template<class T>
bool distance_precentage_among_points(T& image, PointVector* points, int mindistance, int maxdistance)
{
  size_t nthin, ntotal;
  float dist;
  PointVector::const_iterator p;

  if (points->empty()) return 0.0;

  nthin = ntotal = 0;
  for (p=points->begin(); p!=points->end(); p++) {
    ntotal++;
    dist = image.get(Point(p->x(),p->y()));
    if ((dist <= maxdistance) && (dist >= mindistance))
      nthin++;
  }

  return ((double)nthin) / ntotal;
}

/*****************************************************************************
 * remove_vruns_around_points
 *
 * removes all vertical runs to which any of the given points belongs
 *
 * chris, 2005-08-26
 ****************************************************************************/
template<class T>
void remove_vruns_around_points(T& image, PointVector* points, int threshold)
{
  typename T::value_type white_value = white(image);
  PointVector::iterator p;
  int runlength;
  coord_t x,y;

  for (p=points->begin(); p!=points->end(); p++) {
    x = p->x();
    // check total vertical runlength
    if (threshold > 0) {
      runlength = 0;
      for (y=p->y(); is_black(image.get(Point(x, y))); y++) runlength++;
      for (y=p->y()-1; is_black(image.get(Point(x, y))); y--) runlength++;
      if (runlength > threshold) continue;
    }
    // remove vertical strip
    for (y=p->y(); is_black(image.get(Point(x, y))); y++)
      image.set(Point(x, y), white_value);
    for (y=p->y()-1; is_black(image.get(Point(x, y))); y--)
      image.set(Point(x, y), white_value);
  }
}

/*****************************************************************************
 * toom, 2006-01-27
 ****************************************************************************/
void __calc_sums(const FloatPointVector& points, double* x4, double* x3,
    double* x2, double* x, double* x2y, double* xy, double* y)
{
  FloatPointVector::const_iterator i;
  register double tmp;

  *x4=*x3=*x2=*x=*x2y=*xy=*y=0.;

  for (i=points.begin(); i != points.end(); i++) {
    *x+=i->x();
    *xy+=i->x()*i->y();
    tmp=i->x()*i->x();
    *x2+=tmp;
    *x2y+=tmp*i->y();
    tmp*=i->x();
    *x3+=tmp;
    tmp*=i->x();
    *x4+=tmp;
    *y+=i->y();
  }
}

/*****************************************************************************
 * returns the determinant of e
 *
 * WARNING: e is supposed to be a 3x3 matrix. No verification is done.
 *
 * toom, 2006-01-27
 ****************************************************************************/
double __det(double** e)
{
  double h;
  double n;

  h=e[0][0]*e[1][1]*e[2][2]+e[0][1]*e[1][2]*e[2][0]+e[0][2]*e[1][0]*e[2][1];
  n=e[2][0]*e[1][1]*e[0][2]+e[2][1]*e[1][2]*e[0][0]+e[2][2]*e[1][0]*e[0][1];

  return h-n;
}

/*****************************************************************************
 * Ax=c
 *
 * toom, 2006-01-27
 ****************************************************************************/
void __parabola(const FloatPointVector& points, FloatVector* params)
{
  double sum_x4, sum_x3, sum_x2, sum_x, sum_x2y, sum_xy, sum_y;
  double** A, **D1, **D2, **D3;
  double det_A;

  if( points.size() < 3 ) {
    __linear( points, params );
    return;
  }

  __calc_sums(points, &sum_x4, &sum_x3, &sum_x2, &sum_x, &sum_x2y, &sum_xy,
      &sum_y);

  // vector c
  double c[]={sum_x2y, sum_xy, sum_y};

  /*
   * create the matrix A and the matrices D1, D2 and D3
   * for computing the help determinants 
   */
  A=new double*[3];
  D1=new double*[3];
  D2=new double*[3];
  D3=new double*[3];
  for (int i=0; i < 3; i++) {
    A[i]=new double[3];
    D1[i]=new double[3];
    D2[i]=new double[3];
    D3[i]=new double[3];
    if (i == 0) {
      A[0][0]=sum_x4; A[0][1]=sum_x3; A[0][2]=sum_x2;
      D1[0][0]=c[0]; D1[0][1]=sum_x3; D1[0][2]=sum_x2;
      D2[0][0]=sum_x4; D2[0][1]=c[0]; D2[0][2]=sum_x2;
      D3[0][0]=sum_x4; D3[0][1]=sum_x3; D3[0][2]=c[0];
    } else if (i == 1) {
      A[1][0]=sum_x3; A[1][1]=sum_x2; A[1][2]=sum_x;
      D1[1][0]=c[1]; D1[1][1]=sum_x2; D1[1][2]=sum_x;
      D2[1][0]=sum_x3; D2[1][1]=c[1]; D2[1][2]=sum_x;
      D3[1][0]=sum_x3; D3[1][1]=sum_x2; D3[1][2]=c[1];
    } else {
      A[2][0]=sum_x2; A[2][1]=sum_x; A[2][2]=points.size();
      D1[2][0]=c[2]; D1[2][1]=sum_x; D1[2][2]=points.size();
      D2[2][0]=sum_x2; D2[2][1]=c[2]; D2[2][2]=points.size();
      D3[2][0]=sum_x2; D3[2][1]=sum_x; D3[2][2]=c[2];
    }
  }

  det_A=__det(A);
  if (fabs(det_A) < 1.e-10)
    throw std::runtime_error("det_A == 0");

  params->push_back(__det(D1)/det_A);
  params->push_back(__det(D2)/det_A);
  params->push_back(__det(D3)/det_A);

  /*
   * finally delete the matrices
   */
  for (int i=0; i < 3; i++) {
    delete[] A[i];
    delete[] D1[i];
    delete[] D2[i];
    delete[] D3[i];
  }
  delete[] A;
  delete[] D1;
  delete[] D2;
  delete[] D3;
}

/*****************************************************************************
 * Y=d*X²+e*X+f
 *
 * always sets d to zero, so the resulting function is linear
 *
 * bcz, 2006-03-23
 ****************************************************************************/
void __linear(const FloatPointVector& points, FloatVector* params)
{
  double sum_x2 = 0.0, sum_x = 0.0, sum_xy = 0.0, sum_y = 0.0;
  double n = points.size();
  double d, e, f;

  FloatPointVector::const_iterator i;

  for (i=points.begin(); i != points.end(); i++) {
    sum_x  += i->x();
    sum_xy += i->x() * i->y();
    sum_x2 += i->x() * i->x();
    sum_y  += i->y();
  }

  if (n < 2)
    throw std::runtime_error("points.count() < 2");

  d = 0;
  e = ( n * sum_xy - sum_x * sum_y ) / ( n * sum_x2 - sum_x * sum_x );
  f = ( sum_y - e * sum_x ) / n;

  params->push_back(d);
  params->push_back(e);
  params->push_back(f);
}

/*****************************************************************************
 * Compute the parabola and its parameters. The points given in the
 * PointVector are used to create an estimating parabola. The distance of the
 * last point to the first point is stored in t2.
 *
 * toom, 2006-01-27
 ****************************************************************************/
FloatVector parabola_cxx(const PointVector& points, double* t2, bool lineary)
{
  double dx, dy, dt, t;
  coord_t tmp0, tmp1;
  FloatPointVector xt_points, yt_points;
  FloatVector x_params, y_params, xy_params;

  // create parameters
  xt_points.push_back(FloatPoint(0, points[0].x()));
  yt_points.push_back(FloatPoint(0, points[0].y()));

  for (size_t i=1; i < points.size(); i++) {
    tmp0=points[i-1].x();
    tmp1=points[i].x();
    dx=tmp1 > tmp0 ? tmp1-tmp0 : tmp0-tmp1;
    tmp0=points[i-1].y();
    tmp1=points[i].y();
    dy=tmp1 > tmp0 ? tmp1-tmp0 : tmp0-tmp1;
    dt=sqrt(dx*dx+dy*dy);

    t=xt_points[i-1].x()+dt;

    xt_points.push_back(FloatPoint(t, (double)points[i].x()));
    yt_points.push_back(FloatPoint(t, (double)points[i].y()));
  }

  // store the distance between the end point of the segment
  // and its last detected neighbor
  *t2=xt_points.back().x();

  __parabola(xt_points, &x_params);
  if(lineary)
    __linear(yt_points, &y_params);
  else
    __parabola(yt_points, &y_params);

  for (int i=0; i < 3; i++) {
    xy_params.push_back(x_params[i]);
    xy_params.push_back(y_params[i]);
  }

  return xy_params;
}

/*****************************************************************************
 * wrapper function to call 'parabola_cxx' from the python side.
 *
 * toom, 2006-02-05
 ****************************************************************************/
FloatVector* parabola(const PointVector* points)
{
  double t2;
  FloatVector* p=new FloatVector;

  (*p)=parabola_cxx(*points, &t2, false);
  p->push_back(t2);

  return p;
}

/*****************************************************************************
 * wrapper function to call 'parabola_cxx' from the python side.
 *
 * bcz, 2006-03-23
 ****************************************************************************/
FloatVector* lin_parabola(const PointVector* points)
{
  double t2;
  FloatVector* p=new FloatVector;

  (*p)=parabola_cxx(*points, &t2, true);
  p->push_back(t2);

  return p;
}

/*****************************************************************************
 * Given the parameters of the parabola, the current point, the distance of
 * the current point to the first point and the distance of the current point
 * to the previous one, the next point is estimated.
 *
 * Note: The direction flag is currently not used.
 *
 * toom, 2006-01-26
 ****************************************************************************/
Point estimate_next_point_cxx(const Point& point,
    const FloatVector& parameters, double* t_end, double* dt_end,
    int direction)
{
  if (parameters.size() < 6)
    throw std::runtime_error("At least 6 parameters are needed.");
  
  double t, dt;
  double x, y;
  size_t dx, dy;
  Point p;
  bool ok;

  if (*dt_end == 0.0)
    dt=1.0;
  else
    dt=*dt_end;

  ok=false;
  while (!ok) {
    t=*t_end+dt;

    // compute the (possible) adjacent point
    x=CALC_XT(parameters, t);
    x+=0.5;
    if (x < 0.0 && point.x() == 0)
      // the new x is outside the image, so return
      return Point((coord_t)-1, (coord_t)-1);

    dx=(coord_t)x < point.x() ? point.x()-(coord_t)x : (coord_t)x-point.x();

    // test if the new x is adjacent to the current point.x
    if (dx > 1)
      dt/=2;
    else {
      y=CALC_YT(parameters, t);
      y+=0.5;
      if (y < 0.0 && point.y() == 0)
        // the new y is outside the image, so return
        return Point((coord_t)-1, (coord_t)-1);

      dy=(coord_t)y < point.y() ? point.y()-(coord_t)y : (coord_t)y-point.y();

      // test if the new y is adjacent to the current point.y
      if (dy > 1)
        dt/=2;
      else if (dx == 0 && dy == 0) {
        // the calculated point is the current one, so increment the
        // parameter t
        *t_end+=dt;
        dt=1.0;
      } else {
        ok=true;
        p.x((coord_t)x);
        p.y((coord_t)y);
      }
    }
  }

  *dt_end=dt;   // return the distance from the new to the current point
  *t_end=t;     // return the distance from the new to the first point

  return p;
}

/*****************************************************************************
 * wrapper function to call 'estimate_next_point_cxx' from the python side.
 *
 * toom, 2006-02-07
 ****************************************************************************/
FloatVector* estimate_next_point(const Point& point,
    const FloatVector* parameters, double t_end, double dt_end,
    int direction)
{
  FloatVector* ret_value=new FloatVector;
  Point p = point;

  coord_t dx, dy;

  __check_parabola(p, *parameters, t_end, &dx, &dy);
  if (dx > 1 || dy > 1)
  {
    p.x( (coord_t)CALC_XT(*parameters,t_end) );
    p.y( (coord_t)CALC_YT(*parameters,t_end) );
  }
  //  throw std::runtime_error("The given point does not match the parabola"
  //        " at the given distance.");

  p=estimate_next_point_cxx(p, *parameters, &t_end, &dt_end, direction);

  ret_value->push_back((double)p.x());
  ret_value->push_back((double)p.y());
  ret_value->push_back(t_end);
  ret_value->push_back(dt_end);

  return ret_value;
}

/*****************************************************************************
 * Using the distance t and the parameters in p_v, the function calculates 
 * the (real) point on the parabola. The return value (dx dy) is the distance 
 * between the calculated point and the given point p
 *
 * toom, 2006-01-31
 ****************************************************************************/
inline void __check_parabola(const Point& p, const FloatVector& p_v,
    const double t, coord_t* dx, coord_t* dy)
{
  double x, y;

  x=CALC_XT(p_v, t)+0.5;
  y=CALC_YT(p_v, t)+0.5;

  *dx=(coord_t)x < p.x() ? p.x()-(coord_t)x : (coord_t)x-p.x();
  *dy=(coord_t)y < p.y() ? p.y()-(coord_t)y : (coord_t)y-p.y();
}

/*****************************************************************************
 * toom, 2006-02-03
 ****************************************************************************/
template<class T>
inline int nconnectivity_safe(const T& image, coord_t c, coord_t r)
{
  int n;

  const size_t ncols=image.ncols();
  const size_t nrows=image.nrows();

  n=0;

  for (int i=-1; i < 2; i++)
    if (c+i < ncols)
      for (int j=-1; j < 2; j++)
        if (r+j < nrows)
          if (is_black(image.get(Point(c+i, r+j))))
            n++;

  // return the number of detected black points,
  // but _without_ Point(c, r)
  return --n;
}

/*****************************************************************************
 * extend_skeleton_horizontal
 *
 * extends skeleton end points horizontally
 *
 * chris, 2006-04-03
 ****************************************************************************/
template<class T, class U>
typename ImageFactory<T>::view_type* extend_skeleton_horizontal(T& image, U& distancetransform)
{
  typename ImageFactory<T>::view_type* newimage;
  typename T::value_type black_value = black(image);
  PointVector neighbors;
  PointVector::iterator p,q;
  Point lastp;
  int xx, n, npoints, maxpoints;
  newimage = simple_image_copy(image);

  for (coord_t y=0; y<image.nrows()-1; y++)
    for (coord_t x=1; x<image.ncols()-1; x++) {
      if (is_black(image.get(Point(x, y))) && 
          (nconnectivity(image, x, y) == 1)) {
        // endpoint found: determine extrapolation direction
        int direction = 0;
        n = get_neighbors(image, y, x, &neighbors);
        lastp = neighbors.front(); // can only be one neighbor
        while (lastp.x() == x) {
          n = get_neighbors(image, lastp.y(), lastp.x(), &neighbors);
          if (n != 2) break;
          if (neighbors.front() == lastp) lastp = neighbors.back();
          else lastp = neighbors.front();
        }
        if (x > lastp.x()) direction = +1;
        else if (x < lastp.x()) direction = -1;
        // extrapolate into direction
        npoints = 0;
        maxpoints = (int)distancetransform.get(Point(x,y));
        if (direction > 0) {
          xx = x+1;
          while ((npoints < maxpoints) &&
                 (xx < (int)image.ncols()) && 
                 (distancetransform.get(Point(xx,y)) > 0.4)) {
            newimage->set(Point(xx,y), black_value);
            xx++; npoints++;
          }
        } else if (direction < 0) {
          xx = x-1;
          while ((npoints < maxpoints) &&
                 (xx > -1) && 
                 (distancetransform.get(Point(xx,y)) > 0.4)) {
            newimage->set(Point(xx,y), black_value);
            xx--; npoints++;
          }
        }
      }
    }

  return newimage;
}

/*****************************************************************************
 * extend_skeleton_linear
 *
 * extends skeleton end points linearly
 * the extrapolation angle is found by a least squares fit
 *
 * chris, 2006-04-03
 ****************************************************************************/
template<class T, class U>
typename ImageFactory<T>::view_type* extend_skeleton_linear(T& image, U& distancetransform, size_t n_points)
{
  typename ImageFactory<T>::view_type* newimage;
  typename T::value_type black_value = black(image);
  PointVector neighbors, skelbranch;
  PointVector::iterator p,q;
  Point lastp;
  int n, x_of_y, intxx, intyy;
  double xx, yy, m, b, confidence, dx, dy;
  newimage = simple_image_copy(image);

  for (coord_t y=1; y<image.nrows()-1; y++)
    for (coord_t x=1; x<image.ncols()-1; x++) {
      if (is_black(image.get(Point(x, y))) && 
          (nconnectivity(image, x, y) == 1)) {
        // endpoint found: collect up to n_points points from skeleton branch
        n = 0;
        lastp = Point(x,y);
        skelbranch.clear();
        skelbranch.push_back(lastp);
        image.set(lastp,2);
        while (n < (int)n_points &&
               1 == get_neighbors(image, lastp.y(), lastp.x(), &neighbors)) {
          lastp = neighbors.front(); // can only be one neighbor
          skelbranch.push_back(lastp);
          image.set(lastp,2);
          n++;
        }
        // reset pixel values for later processing
        for (p = skelbranch.begin(); p != skelbranch.end(); p++)
          image.set(*p,1);

        // compute line through skeleton branch
        if (skelbranch.size() < 2)
          continue;
        PyObject* po = least_squares_fit_xy(&skelbranch);
        PyArg_ParseTuple(po, "dddi", &m, &b, &confidence, &x_of_y);
        Py_DECREF(po);

        // compute extrapolation direction
        p = skelbranch.begin(); q = p + 1;
        if (!x_of_y) {
          if (p->x() < q->x()) dx = -1.0; else dx = 1.0;
          dy = dx * m;
        } else {
          if (p->y() < q->y()) dy = -1.0; else dy = 1.0;
          dx = dy * m;
        }

        // extrapolate into direction
        //int npoints = 0;
        //int maxpoints = (int)distancetransform.get(Point(x,y));
        xx = x + dx; intxx = (int)xx; 
        yy = y + dy; intyy = (int)yy; 
        while (//npoints < maxpoints &&
               intxx > -1 && intxx < (int)image.ncols() && 
               intyy > -1 && intyy < (int)image.nrows() && 
               distancetransform.get(Point(intxx,intyy)) > 0.4) {
          newimage->set(Point(intxx,intyy), black_value);
          xx += dx; intxx = (int)xx;
          yy += dy; intyy = (int)yy;
          //npoints++;
        }
      }
    }

  return newimage;
}

/*****************************************************************************
 * Each skeleton segment will be extended, until the skeleton segment is as
 * long as the original segment. In case that the skeleton segment consists of
 * 3 or more points, an estimating parabola will be calculated. This parabola
 * is used to estimate the extension points. In case of a skeleton segment
 * consisting of 2 points, the segment will be extended using the fitting
 * line.
 *
 * Since the skeleton segments do not necessarily have to be functions, the
 * values of x and y are described by two functions:
 *
 *     x = x(t)
 *     y = y(t)
 *
 * where
 *
 *     x(t) = ax*t^2 + bx*t + cx
 *     y(t) = ay*t^2 + by*t + cy
 *
 * The distance t is the sum of the distances between all points and is
 * estimated with the euclidian distance:
 *
 *     t = sum(sqrt(dx^2 + dy^2))
 *
 * image: original image
 * skeleton: skeleton of 'image'
 * n_pixels: number of pixels to use for the interpolation (starting with the
 *           end point of a skeleton)
 *
 * toom, 2006-01-26
 ****************************************************************************/
template<class T, class U>
typename ImageFactory<T>::view_type* extend_skeleton_parabolic(const T& skeleton,
    const U& distance, size_t n_points)
{

  typename ImageFactory<T>::view_type* ext_skeleton;
  PointVector points;     // relevant points for calculating the parabola
  FloatVector parameters; // [ax, ay, bx, by, cx, cy]
  double t_end=0.0;       // distance of the end point to the first point
  coord_t c, r, tmp_c, tmp_r;
  bool no_point_left;

  ext_skeleton = simple_image_copy(skeleton);

  for (coord_t row=0; row < skeleton.nrows(); row++) {
    for (coord_t col=0; col < skeleton.ncols(); col++) {
      if (skeleton.get(Point(col, row)) != 1 ||
            nconnectivity_safe(skeleton, col, row) != 1)
        // no end point
        continue;

      points.clear();
      points.push_back(Point(col, row));
      ext_skeleton->set(Point(col, row), 2);

      c=col;
      r=row;
      
      /*
       * 1)
       *
       * find the neighbors and add them to the PointVector:
       *
       * IMPORTANT:
       *
       * the end point of the segment is at the _end_ of the vector, so
       * the vector starts within the segment and ends up at the end point
       */
      if (n_points > 1) {
        do {
          no_point_left=true;

          // start with the upper left neighbor
          c--;
          r--;

          for (int i=0; i < 9; i++) {
            tmp_c=c+i%3;
            if (tmp_c < ext_skeleton->ncols()) {
              tmp_r=r+i/3;
              if (tmp_r < ext_skeleton->nrows()) {
                // the extended skeleton already contains new added points,
                // so test the skeleton as well (already scanned points in
                // the extended skeleton are marked with 2)
                if (1 == skeleton.get(Point(tmp_c, tmp_r)) &&
                    1 == ext_skeleton->get(Point(tmp_c, tmp_r))) {
                  c=tmp_c;
                  r=tmp_r;

                  points.insert(points.begin(), Point(c, r));
                  ext_skeleton->set(Point(c, r), 2);
                  no_point_left=false;
                  break;
                }
              }
            }
          }
        } while (points.size() < n_points && !no_point_left &&
            nconnectivity_safe(skeleton, c, r) == 2);
      }

      // reset the detected points to 1
      for (PointVector::iterator q=points.begin(); q != points.end(); q++)
        ext_skeleton->set(*q, 1);

      /*
       * 2)
       *
       * calculate the estimating parabola that fits all given points
       *
       * As a boundary condition of this parabola, it _must_ be possible to
       * calculate the last point (end point of the skeleton segment) using
       * this parabola.
       * In case the parabola cannot reach the end point, the point vector
       * is reduced by setting the numbers of points to the default value,
       * in order to make the parabola align to the end point (necessary for
       * further estimation).
       */
      if (points.size() > 2) {
        coord_t dx, dy;

        do {
          parameters=parabola_cxx(points, &t_end, false);
          __check_parabola(points.back(), parameters, t_end, &dx, &dy);

          if (dx > 1 || dy > 1) {
            // the end point could not be reached, so the parabola is not
            // exact enough.
            // --> reset the number of points to the default value
            int diff=points.size()-N_POINTS_DEFAULT;

            if (diff < 3)
              points.erase(points.begin());
            else
              for (int i=0; i < diff; i++)
                points.erase(points.begin());
          }
        } while ((dx > 1 || dy > 1) && points.size() > 2);
      }

      /*
       * 3)
       *
       * The parabola fits the given points. Now estimate further points.
       */
      if (points.size() > 2) {
        FloatVector::iterator i;
        Point p;
        bool black_in_orig;
        double dt_end=0.0;
        
        // get the starting point
        p=points.back();

        // estimate all the other points
        black_in_orig=true;
        do {
          p=estimate_next_point_cxx(p, parameters, &t_end, &dt_end, 1);

          if (p.x() < distance.ncols() && p.y() < distance.nrows() &&
              distance.get(p) > 0.4 && !is_black(skeleton.get(p)))
            ext_skeleton->set(p, 1);
          else
            black_in_orig=false;
        } while (black_in_orig);
      } else if (points.size() > 1) {
        // estimate further points by a line.
        int dx, dy;
        Point p;

        dx=points.front().x()-points.back().x();
        dy=points.front().y()-points.back().y();

        p=points.back();
        p.x(p.x()+dx);
        p.y(p.y()-dy);

        while (p.x() < distance.ncols() && p.y() < distance.nrows() &&
            distance.get(p) > 0.4 && !is_black(skeleton.get(p))) {
          ext_skeleton->set(p, 1);
          p.x(p.x()+dx);
          p.y(p.y()-dy);
        }
      }
    }
  }

  return ext_skeleton;
}

/*****************************************************************************
 * wrapper function that calls adequate extrapolation routine
 *
 * chris, 2006-04-03
 ****************************************************************************/
template<class T, class U>
typename ImageFactory<T>::view_type* extend_skeleton(T& skeleton,
    const U& distance, char* extrapolation_scheme, size_t n_points)
{
  if (distance.nrows() != skeleton.nrows() || 
      distance.ncols() != skeleton.ncols())
    throw std::runtime_error(
        "Dimensions of skeleton and distance transform do not match.");

  if (0 == strcmp(extrapolation_scheme, "parabolic") && n_points > 2) {
    return extend_skeleton_parabolic(skeleton, distance, n_points);
  }
  else if (0 == strcmp(extrapolation_scheme, "linear") && n_points > 1) {
    return extend_skeleton_linear(skeleton, distance, n_points);
  }
  else {
    return extend_skeleton_horizontal(skeleton, distance);
  }
}

#endif
