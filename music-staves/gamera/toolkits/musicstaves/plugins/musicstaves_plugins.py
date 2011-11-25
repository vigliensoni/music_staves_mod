# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
#
# Copyright (C) 2004 Christoph Dalitz, Thomas Karsten, Florian Pose
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

#----------------------------------------------------------------

from gamera.plugin import *
import _musicstaves_plugins

#----------------------------------------------------------------

class MusicStaves_rl_simple(PluginFunction):
    """Creates a MusicStaves_rl_simple__ object.

.. __: gamera.toolkits.musicstaves.musicstaves_rl_simple.MusicStaves_rl_simple.html

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
        from gamera.toolkits.musicstaves import musicstaves_rl_simple
        return musicstaves_rl_simple.MusicStaves_rl_simple(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Thomas Karsten"

#----------------------------------------------------------------

class MusicStaves_rl_carter(PluginFunction):
    """Creates a MusicStaves_rl_carter__ object.

.. __: gamera.toolkits.musicstaves.musicstaves_rl_carter.MusicStaves_rl_carter.html

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
        from gamera.toolkits.musicstaves import musicstaves_rl_carter
        return musicstaves_rl_carter.MusicStaves_rl_carter(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Michael Droettboom (after an algorithm by N. P. Carter)"

#----------------------------------------------------------------

class MusicStaves_skeleton(PluginFunction):
    """Creates a MusicStaves_skeleton__ object.

.. __: gamera.toolkits.musicstaves.musicstaves_skeleton.MusicStaves_skeleton.html

Leave the parameters *staffspace_height* and *staffline_height* zero
unless you have computed them somehow beforehand. When left zero,
they are computed automatically. As skeletonization can be rather time
consuming, you can optionally provide a precomputed skeleton in
*skeleton_image*.
"""
    pure_python = 1
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])
    return_type = Class("musicstaves")

    def __call__(image, staffline_height=0, staffspace_height=0):
        from gamera.toolkits.musicstaves import musicstaves_skeleton
        return musicstaves_skeleton.MusicStaves_skeleton(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Christoph Dalitz"

#----------------------------------------------------------------

class StaffFinder_projections(PluginFunction):
    """Creates a StaffFinder_projections__ object.

.. __: gamera.toolkits.musicstaves.stafffinder_projections.StaffFinder_projections.html

Leave the parameters *staffspace_height* and *staffline_height* zero
unless you have computed them somehow beforehand. When left zero,
they are computed automatically.
"""
    pure_python = 1
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])
    return_type = Class("stafffinder")

    def __call__(image, staffline_height=0, staffspace_height=0):
        from gamera.toolkits.musicstaves import stafffinder_projections
        return stafffinder_projections.StaffFinder_projections(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Christoph Dalitz"

#----------------------------------------------------------------

class StaffFinder_dalitz(PluginFunction):
    """Creates a StaffFinder_dalitz__ object.

.. __: gamera.toolkits.musicstaves.stafffinder_dalitz.StaffFinder_dalitz.html

Leave the parameters *staffspace_height* and *staffline_height* zero
unless you have computed them somehow beforehand. When left zero,
they are computed automatically.
"""

    pure_python = 1
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default = 0), \
                 Int('staffspace_height', default = 0)])
    return_type = Class("stafffinder")

    def __call__(image, staffline_height = 0, staffspace_height = 0):
        from gamera.toolkits.musicstaves import stafffinder_dalitz
        return stafffinder_dalitz.StaffFinder_dalitz(\
                image, staffline_height, staffspace_height)

    __call__ = staticmethod(__call__)
    author = "Florian Pose"

#----------------------------------------------------------------

class fill_horizontal_line_gaps(PluginFunction):
    """Fills white gaps in horizontal lines with black.

The image is scanned with a line of *windowwidth* pixels.
When the average blackness within this window is at least *blackness*
percent, the middle pixel is turned black.

The parameter *fill_average* only applies to greyscale images: when *True*,
the middle pixel is not set to black but to the window average,
if this average value is darker than the current pixel value.
Note that this is different from a convolution because pixels
are never turned lighter.
    """
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('windowwidth'),\
                 Int('blackness', range=(1, 100)),\
                 Check('fill_average', default=False)])
    author = "Christoph Dalitz"

    # necessary because this is the only way to specify default arguments
    def __call__(self, windowwidth, blackness, fill_average=False):
        return _musicstaves_plugins.fill_horizontal_line_gaps(\
                  self, windowwidth, blackness, fill_average)
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class fill_vertical_line_gaps(PluginFunction):
    """Fills white gaps in vertical lines with black.

The image is scanned with a vertical line of *windowwidth* pixels.
When the average blackness within this window is at least *blackness*
percent, the middle pixel is turned black.

The parameter *fill_average* only applies to greyscale images: when *True*,
the middle pixel is not set to black but to the window average,
if this average value is darker than the current pixel value.
Note that this is different from a convolution because pixels
are never turned lighter.
    """
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('windowheight'),\
                 Int('blackness', range=(1, 100)),\
                 Check('fill_average', default=False)])
    author = "Christoph Dalitz"

    # necessary because this is the only way to specify default arguments
    def __call__(self, windowheight, blackness, fill_average=False):
        return _musicstaves_plugins.fill_vertical_line_gaps(\
                  self, windowheight, blackness, fill_average)
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class match_staff_template(PluginFunction):
    """Extracts all points from an image that match a staff template.

The template consists of *nlines* horizontal lines of width *width* with
a vertical distance *line_distance*. The midpoint of each line is extracted
if each line contains more than *blackness* percent black pixels. In order
to avoid cutting off the staff lines at the beginning and end of each staff,
the lines are extrapolated at the ends until no further black point is found.

Reasonable parameter values are *3 * staffspace_height / 2* for *width* and
*staffspace_height + staffline_height* for *line_distance*.
"""
    category = "MusicStaves"
    self_type = ImageType([ONEBIT])
    args = Args([Int('width'), Int('line_distance'),\
                 Int('nlines', default=2),\
                 Int('blackness', range=(1, 100), default=70)])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

    def __call__(self,  width, line_distance, nlines=2, blackness=70):
        return _musicstaves_plugins.match_staff_template(\
                self, width, line_distance, nlines, blackness)
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class correct_rotation(PluginFunction):
    """Corrects a possible rotation angle with the aid of skewed projections.

When the image is large, it is temporarily scaled down for performance
reasons. The parameter *staffline_height* determines how much the image
is scaled down: when it is larger than 3, the image is scaled down so that
the resulting staffline height is 2. Thus if you want to suppress the
downscaling, set *staffline_height* to one.

When *staffline_height* is given as zero, it is computed automatically
as most frequent black vertical run.
    """
    pure_python = 1
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0)])
    return_type = ImageType([ONEBIT, GREYSCALE], "nonrot")
    author = "Christoph Dalitz"

    def __call__(self,slh=0):
        if self.data.pixel_type != ONEBIT:
            bwtmp = self.to_onebit()
            rotatebordervalue = 255
        else:
            bwtmp = self
            rotatebordervalue = 0
        # rescale large images for performance reasons
        if slh == 0:
            slh = bwtmp.most_frequent_run('black', 'vertical')
        if (slh > 3):
            print "scale image down by factor %4.2f" % (2.0/slh)
            bwtmp = bwtmp.scale(2.0/slh, 0)
        # find and remove rotation
        skew = bwtmp.rotation_angle_projections(-2,2)
        if (abs(skew[0]) > abs(skew[1])):
            print ("rot %0.4f degrees found ..." % skew[0]),
            retval = self.rotate(skew[0], rotatebordervalue)
            print "and corrected"
        else:
            print "no rotation found"
            retval = self.image_copy()
        return retval

    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class MusicStavesModule(PluginModule):
    cpp_headers = ["musicstaves_plugins.hpp"]
    category = "MusicStaves"
    functions = [MusicStaves_rl_simple,\
                 MusicStaves_rl_carter,\
                 MusicStaves_skeleton,\
                 StaffFinder_projections,\
                 StaffFinder_dalitz,\
                 fill_horizontal_line_gaps,\
                 fill_vertical_line_gaps,\
                 match_staff_template,\
                 correct_rotation]
    author = "Christoph Dalitz, Thomas Karsten, Florian Pose"

module = MusicStavesModule()

#----------------------------------------------------------------
