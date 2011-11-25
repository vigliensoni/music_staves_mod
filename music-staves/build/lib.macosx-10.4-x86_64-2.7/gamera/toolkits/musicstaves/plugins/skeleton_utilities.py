# -*- mode: python; indent-tabs-mode: nil; tab-width: 4; py-indent-offset: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2005 Christoph Dalitz
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

from gamera.plugin import *
# to avoid name conflicts:
#from gamera.args import Point as PointArg
#from gamera.core import Point
import _skeleton_utilities

class SkeletonSegment:
    """Represents a skeleton segment as a list of adjacent Points.

Properties (except for points, all are computed in the constructor):

  *points*
    list of adjacent skeleton points of type ``Point``
  *branching_points*
    list of branching points adjacent to the segment
  *offset_x*, *offset_y*, *nrows*, *ncols*
    like in connected components
  *orientation_angle*
    angle of the least square fitted line through the segment or ``None``
    when the segment contains less than two points.
  *straightness*
    deviation (variance, ie. sum of square deviations) of the segment
    from the least square fitted line through the. ``None``
    when the segment contains less than two points.
"""
    def __init__(self, points, branching_points=[]):
        """Creates a skeleton segment and computes its properties
from a list of points. Signature:

  ``__init__(self, points, branching_points=[])``

To do the same on the C++ side, use the function

  ``PyObject* create_skeleton_segment(PointVector* points, PointVector* branchpoints)``
"""
        if points:
            s = _skeleton_utilities.create_skeleton_segment(points, branching_points)
            self.points = s.points
            self.branching_points = s.branching_points
            self.orientation_angle = s.orientation_angle
            self.offset_x = s.offset_x; self.offset_y = s.offset_y
            self.nrows = s.nrows; self.ncols = s.ncols
            self.straightness = s.straightness
        else:
            self.points = []
            self.branching_points = []
            self.orientation_angle = None
            self.offset_x = 0; self.offset_y = 0
            self.nrows = 0; self.ncols = 0
            self.straightness = None

class create_skeleton_segment(PluginFunction):
    """Constructor for a SkeletonSegment__ from a list of points.

.. __: gamera.toolkits.musicstaves.plugins.skeleton_utilities.SkeletonSegment.html
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([PointVector('points'), PointVector('branching_points')])
    return_type = Class('skeleton_segment', SkeletonSegment)
    author = "Christoph Dalitz"


class remove_spurs_from_skeleton(PluginFunction):
    """Removes short branches (\"spurs\") from a skeleton image.

Parameters:

  *length*:
    only end branches not longer than length are removed
  *endtreatment*:
    determines how end points (branching points with two spurs and one
    branch longer than *length*) are treated: 0 = spurs are removed,
    1 = spurs are kept, 2 = end point is interpolated between the two
    end branches.

.. note:: Setting *endtreatment* = 0 will result in shortened
          skeletons that no longer extend to the corners. The same
          may happen for *endtreatment* = 2, though to a lesser extent.
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = ImageType([ONEBIT])
    args = Args([Int('length'),\
                 Choice('endtreatment', ['remove','keep','extrapolate'])])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"


# class extend_skeleton_horizontally(PluginFunction):
#     """Extends a skeleton image at end points horizontally until a white
# pixel in the provided unskeletonized image is found. For safety reasons
# no more pixels are added than the distance transform value at the skeleton
# end point.

# This is typically useful only for thinned out skeletons, because these do
# not extend to the edges. For medial axis transform skeletons use
# ``remove_spurs_from_skeleton`` instead.

# The argument must be a distance transform of the original unskeletonized
# onebit image.
# """
#     category = "MusicStaves/Skeleton_utilities"
#     self_type = ImageType([ONEBIT])
#     args = Args([ImageType([FLOAT], 'distancetransform')])
#     return_type = ImageType([ONEBIT])
#     author = "Christoph Dalitz"


class split_skeleton(PluginFunction):
    """Splits a skeleton image at branching and corner points and returns
a list of `SkeletonSegments`__.

.. __: gamera.toolkits.musicstaves.plugins.skeleton_utilities.SkeletonSegment.html

The input image must be a skeleton, the `FloatImage` must be a distance
transform obtained from the original image. The splitting procedure
consists of two steps:

1) Split at \"branching\" (more than two connected) points.

2) In the remaining branches, corners are detected with get_corner_points_rj,
   which requires a parameter *cornerwidth*. A reasonable value for
   *cornerwidth* could be *max(staffspace_height/2,5)*; setting *cornerwidth*
   to zero will skip the corner detection.

In both steps, a number of pixels is removed around each splitting point.
This number is taken from the *distance_transform* image. If you
do not want this, simply pass as *distance_transform* an image with all
pixel values equal to zero.
The parameter *norm* should determine the removal shape (diamond for
chessboard, square for manhatten and circle for euclidean distance), but
is currently ignored.
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([FLOAT], 'distance_transform'),
                 Int('cornerwidth'),
                 Choice('norm', ['chessboard','manhattan','euclidean'], default=0)])
    return_type = Class('skeleton_segments', SkeletonSegment, list_of=True)
    author = "Christoph Dalitz"

    # wrapper for passing default arguments
    def __call__(self, distance_transform, cornerwidth, norm=0):
        return _skeleton_utilities.split_skeleton(\
                self, distance_transform, cornerwidth, norm)
    __call__ = staticmethod(__call__)


class get_corner_points(PluginFunction):
    """Returns the corner points of the skeleton segment consisting of the
provided points by looking for angles below a certain threshold.

Corners are detected by measuring the angle at each point plusminus
*cornerwidth* neighbor points (see the figure below). The maximum angle is
currently hard coded to 135 degrees. Apart from an angle threshold we
also check whether the branches are approximately straight.

.. image:: images/split_skeleton.png

For an overview of more sophisticated corner detection algorithms, see
C.H. Teh and R.T. Chin:
*On the Detection of Dominant points on Digital Curves.* IEEE Transactions
on Pattern Analysis and Machine Intelligence, vol. 11, no. 8, pp. 859-872
(1989).
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([PointVector('points'),Int('cornerwidth', default=1)])
    return_type = PointVector('result')
    author = "Christoph Dalitz"

    def __call__(points,cornerwidth):
        res = _skeleton_utilities.get_corner_points(points,cornerwidth)
        return res
    __call__ = staticmethod(__call__)
    
class get_corner_points_rj(PluginFunction):
    """Returns the corner points (maximum curvature points) of the skeleton
segment consisting of the provided points according to an algorithm by
Rosenfeld and Johnston.

See A. Rosenfeld and E. Johnston: *Angle detection on digital curves.* IEEE
Trans. Comput., vol. C-22, pp. 875-878, Sept. 1973
Compared to the algorithm in the paper the following modifications have been
made:

 - While Rosenfeld and Johnston's algorithm only works on closed curves,
   this function operates on an open curve. The first checked point is the
   one with the index *conerwidth* + 1 (*cornerwidth* corresponds to the
   parameter *m* in the original paper).

 - This function does not allow adjacent dominant points, with a (chessboard)
   distance less than *cornerwidth*. When this happens, the algorithm decides
   based upon the angle found (it takes the sharper one).

 - Dominant points are considered corner points only if the angle of
   the branches (see figure below) is less than *maxangle* (measured
   in degrees; currently hard coded to 140 degrees).

.. image:: images/split_skeleton.png
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([PointVector('points'),Int('cornerwidth', default=1)])
    return_type = PointVector('result')
    author = "Bastian Czerwinski"

    def __call__(points,cornerwidth):
        res = _skeleton_utilities.get_corner_points_rj(points,cornerwidth)
        return res
    __call__ = staticmethod(__call__)

class distance_precentage_among_points(PluginFunction):
    """Returns the fraction of the given points that have pixel value
between *mindistance* and *maxdistance* in the image *distance_transform*.
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = ImageType([FLOAT], 'distance_transform')
    args = Args([PointVector('points'),Int('mindistance'),Int('maxdistance')])
    return_type = Float('fraction')
    author = "Christoph Dalitz"

class remove_vruns_around_points(PluginFunction):
    """Removes all vertical runs to which any of the given points belongs.
When the parameter *threshold* is greater than zero, only vertical runlengths
shorter than *threshold* are removed.
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = ImageType([ONEBIT])
    args = Args([PointVector('points'),Int('threshold',default=0)])
    return_type = None
    author = "Christoph Dalitz"
    # wrapper for passing default arguments
    def __call__(self, points, threshold=0):
        return _skeleton_utilities.remove_vruns_around_points(\
                self, points, threshold)
    __call__ = staticmethod(__call__)

class extend_skeleton(PluginFunction):
    """Extends a skeleton image at end points until a white
pixel in the provided unskeletonized image is found.

This is typically useful only for thinned out skeletons, because these do
not extend to the edges. For medial axis transform skeletons use
``remove_spurs_from_skeleton`` instead.

Each skeleton segment in *self* will be extended, until the skeleton segment
is as long as the segment in *orig_image*. In case that the skeleton segment
consists of 3 or more points, an estimating parabola will be calculated using
the function parabola_. In case of a skeleton segment consisting of 2 points,
the segment will be extended using the fitting line.

Arguments:

  *self*
    The skeleton image to be extended

  *distance_transform*
    Distance transform of the original unskeletonized image. Determines
    how far the skeleton is extrapolated at each end point.

  *extrapolation_scheme*, *n_points*
    Determines how new points are calculated. For 'linear' or 'parabolic'
    extrapolation a line or parabola is fitted through *n_points* end
    points of the skeleton branch. For 'horizontal' extrapolation the
    skeleton branch is not investigated.

    As horizontal extrapolation can lead to errenous results,
    no more pixels are added in horizontal extrapolation than the distance
    transform value at the skeleton end point.
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([FLOAT], 'distance_transform'),
                 ChoiceString('extrapolation_scheme', ['horizontal','linear','parabolic'], default='linear'),
                 Int('n_points', default=3)])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz, Thomas Karsten"

    def __call__(self, distance_transform, extrapolation_scheme='linear', n_points=5):
        if extrapolation_scheme not in ['horizontal','linear','parabolic']:
            raise ValueError("Unknown extrapolation scheme: '%s'" %\
                             extrapolation_scheme)

        return _skeleton_utilities.extend_skeleton(self,\
                                                   distance_transform,\
                                                   extrapolation_scheme,\
                                                   n_points)
    __call__ = staticmethod(__call__)


class parabola(PluginFunction):
    """Calculate an estimating parabola for the given points. The first given
point (*points[0]*) is considered to be within the segment, whereas the last
given point (*points[-1]*) is considered to be the end point of the segment.

Since the segment (consisting of the given points) does not necessarily have to be a function, the values of *x* and *y* are described using a parameter *t*
with 2 functions:

    x = x(t)

    y = y(t)

where

    x(t) = ax*t^2 + bx*t + cx

    y(t) = ay*t^2 + by*t + cy

The distance *t* is the sum of the distances between all points and is
estimated with the eclidean distance:

    t = sum(sqrt(dx^2 + dy^2))

Thus, the distance for *points[0]* is 0 and the distance for *points[-1]* is
*t*.

The return value is the FloatVector [ax, ay, bx, by, cx, cy]. The values can
be used to calculate further points using the function estimate_next_point_.

.. note:: Using the calculated parameters it is always possible to
          re-calculate the last given point (+-1). If the parabola of all
          given points does not match this criteria, the number of used points
          will be reduced and the parabola will be calculated again (see the
          image below).

.. image:: images/parabola.png
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([PointVector('points')])
    return_type = FloatVector('parameters', length=2)
    author = "Thomas Karsten"

    def __call__(points):
        fv = _skeleton_utilities.parabola(points)
        return [fv[0:-1], fv[-1]]
    __call__ = staticmethod(__call__)

class lin_parabola(PluginFunction):
    """Calculate an estimating parabola for the given points. The first given
point (*points[0]*) is considered to be within the segment, whereas the last
given point (*points[-1]*) is considered to be the end point of the segment.

Since the segment (consisting of the given points) does not necessarily have to be a function, the values of *x* and *y* are described using a parameter *t*
with 2 functions:

    x = x(t)

    y = y(t)

where

    x(t) = ax*t^2 + bx*t + cx

    y(t) = by*t + cy

The distance *t* is the sum of the distances between all points and is
estimated with the eclidean distance:

    t = sum(sqrt(dx^2 + dy^2))

Thus, the distance for *points[0]* is 0 and the distance for *points[-1]* is
*t*.

The return value is the FloatVector [ax, ay, bx, by, cx, cy] of which ay is
always 0. The values can be used to calculate further points using the
function estimate_next_point_.

.. note:: Using the calculated parameters it is always possible to
          re-calculate the last given point (+-1). If the parabola of all
          given points does not match this criteria, the number of used points
          will be reduced and the parabola will be calculated again (see the
          image below).

.. image:: images/parabola.png
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([PointVector('points')])
    return_type = FloatVector('parameters', length=2)
    author = "Bastian Czerwinski"

    def __call__(points):
        fv = _skeleton_utilities.lin_parabola(points)
        return [fv[0:-1], fv[-1]]
    __call__ = staticmethod(__call__)


class estimate_next_point(PluginFunction):
    """Estimates the next point on a given parabola.

Arguments:
  *current*
    The current point, which has to lie on the parabola.

  *parameters*
    The parameters [ax, ay, bx, by, cx, cy] that describe a parametric
    parabola (see parabola_).

  *t_end*
    The distance of the current point to the first point (see parabola_ for
    a possible computation).

  *dt_end*
    The distance of the current point to the previous one. When set to
    the default value, it will be estimated automatically.

  *direction*
    The direction where to go. This option is not supported yet.

The return value is the vector [p, t_end, dt_end] where

  *p* is the estimated point,

  *t_end* is the distance of the estimated point to the first point,

  *dt_end* is the distance of the estimated point to the current point.

.. code:: Python

    # calculate the parameters of the parabola and estimate the following point
    parameters, t_end = parabola(points)
    next_point, t_new, dt_new = estimate_next_point(points[-1], parameters, t_end)
"""
    category = "MusicStaves/Skeleton_utilities"
    self_type = None
    args = Args([Point('current'), FloatVector('parameters'), Float('t_end'),\
            Float('dt_end', default=0.0), Int('direction', default=1)])
    return_type = FloatVector('ret_value')
    author = "Thomas Karsten"

    def __call__(current, parameters, t_end, dt_end=0.0, direction=1):
        from gamera.core import Point
        fv = _skeleton_utilities.estimate_next_point(current, parameters,\
            t_end, dt_end, direction)
        p = Point(int(fv[0]), int(fv[1]))

        return [p, fv[2], fv[3]]
    __call__ = staticmethod(__call__)

class skeleton_utilities_module(PluginModule):
    category = None
    cpp_headers = ["skeleton_utilities.hpp"]
    functions = [split_skeleton,
                 get_corner_points,
                 get_corner_points_rj,
                 remove_spurs_from_skeleton,
                 distance_precentage_among_points,
                 create_skeleton_segment,
                 remove_vruns_around_points,
                 extend_skeleton,
                 parabola,
                 lin_parabola,
                 estimate_next_point]
    author = "Christoph Dalitz"

module = skeleton_utilities_module()
parabola = parabola()
lin_parabola = lin_parabola()
get_corner_points = get_corner_points()
get_corner_points_rj = get_corner_points_rj()
estimate_next_point = estimate_next_point()
