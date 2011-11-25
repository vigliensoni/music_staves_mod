#
#
# Copyright (C) 2005 Michael Droettboom
#
# This code is a clean-room reimplmentation of the algorithm described in
#   
#    Roach, J. W., and J. E. Tatem. 1988. Using domain knowledge in
#    low-level visual processing to interpret handwritten music: an
#    experiment. Pattern Recognition 21 (1): 33-44.
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
import _roach_tatem_plugins

from math import tan

class compute_vector_field(PluginFunction):
    """Computes the vector field from a onebit image.

Part of the Roach and Tatem staff removal algorithm and implements
Section 3.2 of the paper.

Since the distance and thickness are not used by the staff removal
part of Roach and Tatem's algorithm, they are not stored.

*radius*
   The radius of the circular window used to compute the vector
   field.

The return value is a FloatImage, where each pixel is the angle (in
radians) of the line running through the corresponding pixel in the
given image.  Each pixel value will be in the range (-pi, pi].

This function is very computationally intensive.
"""
    self_type = ImageType([ONEBIT])
    args = Args([Int('window_radius', range=(1,1000), default=0)])
    return_type = ImageType([FLOAT], "theta")
    def __call__(self, window_size=0):
       if window_size == 0:
          window_size = self.most_frequent_run('white', 'vertical') * 3
       return _roach_tatem_plugins.compute_vector_field(self, window_size)
    __call__ = staticmethod(__call__)

class draw_vector_field(PluginFunction):
    """Draws a vector field as returned by compute_vector_field_ using
lines at the appropriate angles.

*cell_size*
   The size of each cell representing a single pixel in the original
   image.

The resulting images can become quite large.  Make sure there is
adequate memory, or run this function on a small subset of the image.
"""
    pure_python = True
    self_type = ImageType([FLOAT])
    args = Args([Int('cell_size', range=(5, 25), default=10)])
    return_type = ImageType([ONEBIT])

    def __call__(self, size=10):
        from gamera.core import Image, GREYSCALE, Dim
        result = Image((0, 0), Dim(self.ncols * size, self.nrows * size), GREYSCALE)
        half_size = size / 2
   
        for r in xrange(self.nrows):
            for c in xrange(self.ncols):
                direction = self.get((c, r))
                if self.get((c, r)) != 0:
                    center_r = r * size + (size / 2)
                    size_c = c * size
                    subimage = result.subimage(Point(r*size, c*size), Dim(size, size))
                    dir = half_size * tan(direction)
                    subimage.draw_line((size_c, center_r - dir), (size_c + size, center_r + dir), 128)
        return result
    __call__ = staticmethod(__call__)

class mark_horizontal_lines_rt(PluginFunction):
    """Given the original image and a vector field, returns a tuple of two
images: the first containing pixels believed to be part of stafflines,
and the second of "questionable pixels", meaning pixels that could be
either part of stafflines or musical figures.

When only the original image and a vector field are given, this function is
part of the Roach and Tatem staff removal algorithm, and implements Section
3.4 of their paper.

When the additional FloatImage *thickness* and *staffline_height* are given,
the implementation will not mark pixels as part of horizontal lines, which
thickness is greater than *staffline_height*. When *staffline_height* is set
to 0 it will default to the size of the most frequent black vertical run
(considered to be the average staffline height).
"""
    self_type = ImageType([ONEBIT])
    args = Args([
       ImageType([FLOAT], 'vector_image'), Float('angle_threshold'),
       Class('thickness_image'), Int('staffline_height', default=0)])
    return_type = ImageList("lines_questionable")

    def __call__(self, vector_image, angle_threshold, thickness_image=None,\
            staffline_height=0):

        if thickness_image is not None and staffline_height == 0:
            staffline_height=self.most_frequent_run('black', 'vertical')

        return _roach_tatem_plugins.mark_horizontal_lines_rt(self,\
                vector_image, angle_threshold, thickness_image,\
                staffline_height)

    __call__ = staticmethod(__call__)

class remove_stafflines_rt(PluginFunction):
    """Given the original image and an image of probably stafflines,
removes the stafflines and returns the result.

Part of the Roach and Tatem staff removal algorithm, and implements
Section 3.5 of their paper.
"""
    self_type = ImageType([ONEBIT])
    args = Args([
       ImageType([ONEBIT], 'lines')])
    return_type = ImageType([ONEBIT], "result")

class MusicStaves_rl_roach_tatem(PluginFunction):
    """Creates a MusicStaves_rl_roach_tatem__ object.

.. __: gamera.toolkits.musicstaves.musicstaves_rl_roach_tatem.MusicStaves_rl_roach_tatem.html

Leave the parameters *staffspace_height* and *staffline_height* zero
unless you have computed them somehow beforehand. When left zero,
they are computed automatically.
"""
    pure_python = 1
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])
    return_type = Class("musicstaves")

    def __call__(image, staffline_height=0, staffspace_height=0):
        from gamera.toolkits.musicstaves import musicstaves_rl_roach_tatem
        return musicstaves_rl_roach_tatem.MusicStaves_rl_roach_tatem(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Michael Droettboom, after an algorithm by Roach & Tatem"

class StaffRemovalRoachTatem(PluginModule):
    cpp_headers = ["roach_tatem_plugins.hpp"]
    category = "MusicStaves/RoachTatem"
    functions = [compute_vector_field, mark_horizontal_lines_rt,
                 remove_stafflines_rt, draw_vector_field,
                 MusicStaves_rl_roach_tatem]
    author = "Michael Droettboom after an algorithm by J. W. Roach and J. E. Tatem"

module = StaffRemovalRoachTatem()

