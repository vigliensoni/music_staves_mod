# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2005 Thomas Karsten
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
import _vector_field

class compute_longest_chord_vectors(PluginFunction):
    """Computes the vector field from a onebit image.

This function determines about the longest chord where each black pixel is
located at. A chord is a straight line, consisting of black pixels only. The
return value consists of three FloatImages.

*1st image*:
    Contains the angle (in radians) of each pixel
    the line running through the corresponding pixel in the given image. Each
    pixel value will be in the range (-pi, pi].

*2nd image*:
    length of the longest chord at each pixel position

*3rd image*:
    Contains the thickness of the longest chord at each pixel position.
    Thereby the thickness is the length of the chord perpendicular to the
    detected longest chord (depending on *resolution* more or less
    perpendicular).


This function is similar to compute_vector_field_, but is not that
computationally intensive (depending on *resolution*).

Arguments:
    
*path_length*:
    maximum length that will be tested for each chord

*resolution*:
   Angle that is used to scan the image (in degree). When the resolution is
   set to 0, then the default value will be used instead.
"""
    self_type = ImageType([ONEBIT])
    args = Args([Int('path_length', default=0),\
		    Float('resolution', default=3.0, range=(0.0, 90.0))])
    return_type = ImageList("vectors")
    category = "MusicStaves/vector_field"
    author = "Thomas Karsten"

    def __call__(self, path_length=0, resolution=3.0):
        if path_length == 0:
            path_length = self.most_frequent_run('white',\
                    'vertical') * 5
        if not resolution > 0.0:
            resolution=3.0
        return _vector_field.compute_longest_chord_vectors(self,\
                path_length, resolution)

    __call__ = staticmethod(__call__)

class angles_to_RGB(PluginFunction):
    """Converts angle information for each pixel to RGB values.

Each angle is displayed in a specific color, e.g. 0 degrees is visualised in 
cyan (RGB (0, 255, 255)), and an angle of 90 degrees is shown in red
(RGB (255, 0, 0)). The HSI color space is mapped to 180 degrees. Thus, angles
of opposed chords will result in same colors, which is shown in the color
circle below.

Example:

.. image:: images/angles_to_RGB.png

Argument:

*v_images*:
  List of 1 to 3 FloatImages, typically returned by
  compute_longest_chord_vectors_. Their meaning is as follows:

  *1st image*:
    Affects the hue of the RGB image. The pixel values must be in the range
    [-pi, pi] and are mapped to [0, 360] degrees.

  *2nd image*:
    Affects the saturation of the RGB image. The higher the pixel value the
    higher the saturation. If not given, the saturation defaults to 1.0 (100
    percent).

  *3rd image*:
    Affects the intensity of the RGB image. The smaller the pixel value the
    higher the intensity. If not given, the intensity defaults to 1.0 (100
    percent).

*path_length*:
  Maximum pixel value of the 2nd and the 3rd image. This value is used to
  normalise the pixel information and to keep both the saturation and the
  intensity in a range from [0, 1.0].
"""

    self_type = ImageType([ONEBIT])
    args = Args([ImageList('v_images'), Int('path_length')])
    return_type = ImageType([RGB], "rgb_image")
    category = "MusicStaves/vector_field"

    def __call__(self, v_images, path_length=0):
        if path_length == 0:
            path_length = self.most_frequent_run('white',\
                    'vertical') * 5

        return _vector_field.angles_to_RGB(self, v_images, path_length)

    __call__ = staticmethod(__call__)


class keep_vectorfield_runs(PluginFunction):
    """The returned image only contains those black pixels that belong to black
runs crossing the given points at an angle equal the vector field value at
these points. All runs are only extracted in the given *direction* and up
to the given *height*.

Arguments:

*height*:
  Black runs are only kept up to *height* in the given *direction* (see below).

*points*:
  Only tall runs crossing these points are kept.

*direction*:
  In which direction (``up`` or ``down``) ``height`` takes effect.

*maxlength*, *numangles*:
  Technical details for the computation of the vector field values
  which are computed by picking the angle of the longest chord through
  the point. *numangles* is the number of sample chords tested; chords
  longer than *maxlength* are truncated (for performance reasons).
"""
    self_type = ImageType([ONEBIT])
    args = Args([PointVector('points'), Int('height'), \
                 ChoiceString('direction',['up','down']), \
                 Int('maxlength'), Int('numangles')])
    return_type = ImageType([ONEBIT])
    category = "MusicStaves/vector_field"
    author = "Christoph Dalitz"


class vector_field_module(PluginModule):
    category = None
    cpp_headers = ["vector_field.hpp"]
    functions = [compute_longest_chord_vectors, angles_to_RGB,
                 keep_vectorfield_runs]
    author = "Thomas Karsten"

module = vector_field_module()
