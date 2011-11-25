#
#
# Copyright (C) 2005 Michael Droettboom, Karl MacMillan, Ichiro Fujinaga
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
import _staff_removal_fujinaga

crossing_symbols_choices = ['all', 'bar', 'none']

class find_and_remove_staves_fujinaga(PluginFunction):
    """Locates staves while deskewing the image, and then removes the
staves.  Returns a tuple of the form (*deskewed_image*,
*removed_image*, *staves*) where *deskewed_image* is the original
image deskewed, *removed_image* is the deskewed with the stafflines
removed, and *staves* is a list of ``StaffObj`` instances defining
where the stafflines are.

*crossing_symbols*
   Not yet implemented -- just a placeholder for now.

*n_stafflines* = 5
   The number of stafflines in each staff.  (Autodetection of number
   of stafflines not yet implemented).

*staffline_height* = 0
   The height (in pixels) of the stafflines. If *staffline_height* <=
   0, the staffline height is autodetected both globally and for each
   staff.

*staffspace_height* = 0
   The height (in pixels) of the spaces between the stafflines.  If
   *staffspace_height* <= 0, the staffspace height is autodetected
   both globally and for each staff.

*skew_strip_width* = 0
   The width (in pixels) of vertical strips used to deskew the image.
   Smaller values will deskew the image with higher resolution, but
   this may make the deskewing overly sensitive.  Larger values may
   miss fine detail.  If *skew_strip_width* <= 0, the width will be
   autodetermined to be the global *staffspace_h* * 2.

*max_skew* = 8.0
   The maximum amount of skew that will be detected within each
   vertical strip.  Expressed in degrees.  This value should be fairly
   small, because deskewing only approximates rotation at very small
   degrees.

*deskew_only* = ``False``
   When ``True``, only perform staffline detection and deskewing.  Do
   not remove the stafflines.

*find_only* = ``False``
   When ``True``, only perform staffline detection.  Do not deskew the
   image or remove the stafflines.
"""
    category = "MusicStaves/Fujinaga"
    self_type = ImageType([ONEBIT])
    args = Args([
       Choice('crossing_symbols', crossing_symbols_choices),
       Int('n_stafflines', (3, 12), default=5),
       Float('staffline_height', (0.0, 512.0), default=0.0),
       Float('staffspace_height', (0.0, 512.0), default=0.0),
       Int('skew_strip_width', (0, 512), default=0),
       Float('max_skew', (1, 20), default=8.0),
       Check('deskew_only', default=False),
       Check('find_only', default=False),
       Check('undo_deskew', default=False)
       ])
    return_type = Class("result")
    def __call__(self, crossing_symbols=0, n_stafflines=5, 
                 staffline_height=0.0, staffspace_height=0.0, skew_strip_width=0,
                 max_skew=8.0, deskew_only=False, find_only=False,
                 undo_deskew=False):
       if type(crossing_symbols) == str:
           if crossing_symbols in crossing_symbols_choices:
               crossing_symbols = crossing_symbols_choices.index(crossing_symbols)
           else:
               raise ValueError("crossing symbols must be one of %s" % repr(crossing_symbols))
       return _staff_removal_fujinaga.find_and_remove_staves_fujinaga(
          self, crossing_symbols, n_stafflines, staffline_height, staffspace_height,
          skew_strip_width, max_skew, deskew_only, find_only, undo_deskew)
    __call__ = staticmethod(__call__)

class find_and_deskew_staves_fujinaga(PluginFunction):
    """Locates staves while deskewing the image.  Returns a tuple of
the form (*image*, *staves*) where *image* is the image deskewed, and
*staves* is a list of ``StaffObj``.

*n_stafflines* = 5
   The number of stafflines in each staff.  (Autodetection of number
   of stafflines not yet implemented).

*staffline_height* = 0
   The height (in pixels) of the stafflines. If *staffline_height* <=
   0, the staffline height is autodetected both globally and for each
   staff.

*staffspace_height* = 0
   The height (in pixels) of the spaces between the stafflines.  If
   *staffspace_height* <= 0, the staffspace height is autodetected
   both globally and for each staff.

*skew_strip_width* = 0
   The width (in pixels) of vertical strips used to deskew the image.
   Smaller values will deskew the image with higher resolution, but
   this may make the deskewing overly sensitive.  Larger values may
   miss fine detail.  If *skew_strip_width* <= 0, the width will be
   autodetermined to be the global *staffspace_h* * 2.

*max_skew* = 8.0
   The maximum amount of skew that will be detected within each
   vertical strip.  Expressed in degrees.  This value should be fairly
   small, because deskewing only approximates rotation at very small
   degrees.
"""
    category = "MusicStaves/Fujinaga"
    self_type = ImageType([ONEBIT])
    args = Args([
       Int('n_stafflines', (3, 12), default=5),
       Float('staffline_height', default=0.0),
       Float('staffspace_height', default=0.0),
       Int('skew_strip_width', (1, 64), default=0),
       Float('max_skew', (1, 20), default=8.0)
       ])
    return_type = Class("result")
    pure_python = True
    def __call__(self, n_stafflines=5, staffline_height=0.0, staffspace_height=0.0,
                 skew_strip_width=0, max_skew=8.0):
        return _staff_removal_fujinaga.find_and_remove_staves_fujinaga(
            self, 0, n_stafflines, staffline_height, staffspace_height,
            skew_strip_width, max_skew, True, False)
    __call__ = staticmethod(__call__)

class find_staves_fujinaga(PluginFunction):
    """Locates staves, without performing any unnecessary deskewing or
removal of stafflines.  This function is useful only to determine
whether an image has stafflines or not.  Returns a list of
``StaffObj``.

*n_stafflines* = 5
   The number of stafflines in each staff.  (Autodetection of number
   of stafflines not yet implemented).

*staffline_height* = 0
   The height (in pixels) of the stafflines. If *staffline_height* <=
   0, the staffline height is autodetected both globally and for each
   staff.

*staffspace_height* = 0
   The height (in pixels) of the spaces between the stafflines.  If
   *staffspace_height* <= 0, the staffspace height is autodetected
   both globally and for each staff.

*skew_strip_width* = 0
   The width (in pixels) of vertical strips used to deskew the image.
   Smaller values will deskew the image with higher resolution, but
   this may make the deskewing overly sensitive.  Larger values may
   miss fine detail.  If *skew_strip_width* <= 0, the width will be
   autodetermined to be the global *staffspace_h* * 2.

*max_skew* = 8.0
   The maximum amount of skew that will be detected within each
   vertical strip.  Expressed in degrees.  This value should be fairly
   small, because deskewing only approximates rotation at very small
   degrees.
"""
    category = "MusicStaves/Fujinaga"
    self_type = ImageType([ONEBIT])
    args = Args([
       Int('n_stafflines', (3, 12), default=5),
       Float('staffline_height', default=0.0),
       Float('staffspace_height', default=0.0),
       Int('skew_strip_width', (1, 64), default=0),
       Float('max_skew', (1, 20), default=8.0)
       ])
    return_type = Class("result")
    pure_python = True
    def __call__(self, n_stafflines=5, staffline_height=0.0, staffspace_height=0.0,
                 skew_strip_width=0, max_skew=8.0):
        return _staff_removal_fujinaga.find_and_remove_staves_fujinaga(
            self, 0, n_stafflines, staffline_height, staffspace_height,
            skew_strip_width, max_skew, True, True)
    __call__ = staticmethod(__call__)

class global_staffline_deskew(PluginFunction):
    """Performs a global deskewing on the entire image by
cross-correlating thin vertical strips of the image.  Since this
function is global and does not deskew on staff at a time, the results
are not great, and in particular long slurs can introduce errors.
This same deskewing also happens early on in the
find_and_remove_staves_fujinaga process.

Returns deskewed copy of the image.

It is available here for primarily for experimentation rather than as
anything ultimately useful.

*staffline_height* = 0
   The height (in pixels) of the stafflines. If *staffline_height* <=
   0, the staffline height is autodetected both globally and for each
   staff.

*staffspace_height* = 0
   The height (in pixels) of the spaces between the stafflines.  If
   *staffspace_height* <= 0, the staffspace height is autodetected
   both globally and for each staff.

*skew_strip_width* = 0
   The width (in pixels) of vertical strips used to deskew the image.
   Smaller values will deskew the image with higher resolution, but
   this may make the deskewing overly sensitive.  Larger values may
   miss fine detail.  If *skew_strip_width* <= 0, the width will be
   autodetermined to be the global *staffspace_h* * 2.

*max_skew* = 8.0
   The maximum amount of skew that will be detected within each
   vertical strip.  Expressed in degrees.  This value should be fairly
   small, because deskewing only approximates rotation at very small
   degrees.
"""
    category = "MusicStaves/Fujinaga"
    self_type = ImageType([ONEBIT])
    args = Args([
        Float('staffline_height', default=0.0),
        Float('staffspace_height', default=0.0),
        Int('skew_strip_width', default=0),
        Float('max_skew', default=5.0)])
    return_type = ImageType([ONEBIT])
    def __call__(self, staffline_height=0.0, staffspace_height=0.0,
                 skew_strip_width=0, max_skew=8.0):
        return _staff_removal_fujinaga.global_staffline_deskewing(
            self, staffline_height, staffspace_height,
            skew_strip_width, max_skew)
    __call__ = staticmethod(__call__)

class remove_staves_fujinaga(PluginFunction):
    """Removes the staves from an image that has already been deskewed
and had its staves detected by find_and_deskew_staves_fujinaga_.
Returns the image with the staves removed.

*staves*
   A list of ``StaffObj`` instances as returned from
   find_and_deskew_staves_fujinaga.  Other lists of StaffObj's may be
   used, but if they do not have a ``staffrect`` member variable, each
   staff will be treated as if it extends across the entire page from
   left to right, and the margins of the image will not be treated
   differently.

*crossing_symbols* = 0
   Not yet implemented -- just a placeholder for now.

*staffline_height* = 0
   The height (in pixels) of the stafflines. If *staffline_height* <=
   0, the staffline height is autodetected both globally and for each
   staff.

*staffspace_height* = 0
   The height (in pixels) of the spaces between the stafflines.  If
   *staffspace_height* <= 0, the staffspace height is autodetected
   both globally and for each staff.

Note that the following are essentially (*) equivalent, (though the
first is slightly faster):

.. code:: Python

   # All in one call
   deskewed, removed, staves = source.find_and_remove_staves_fujinaga(...)

   # Two separate calls, to try different staff removal methods
   deskewed, staves = source.find_and_deskew_staves_fujinaga(...)
   removed = deskewed.remove_staves_fujinaga(staves, ...)

(*) There are some slight differences, but none that should affect the
result.
"""
    category = "MusicStaves/Fujinaga"
    self_type = ImageType([ONEBIT])
    args = Args(
        [Class('staves'),
         Choice('crossing_symbols', ['all', 'bars', 'none']),
         Float('staffline_height', default=0.0),
         Float('staffspace_height', default=0.0)])
    return_type = ImageType([ONEBIT])
    def __call__(self, staves, crossing_symbols=0, staffline_height=0.0, staffspace_height=0.0):
        if type(crossing_symbols) == str:
            if crossing_symbols in crossing_symbols_choices:
                crossing_symbols = crossing_symbols_choices.index(crossing_symbols)
            else:
                raise ValueError("crossing symbols must be one of %s" % repr(crossing_symbols))
        return _staff_removal_fujinaga.remove_staves_fujinaga(
            self, staves, crossing_symbols, staffline_height, staffspace_height)
    __call__ = staticmethod(__call__)

class MusicStaves_rl_fujinaga(PluginFunction):
    """Returns a new MusicStaves_fl_fujinaga__ object from the image.

.. __: gamera.toolkits.musicstaves.musicstaves_rl_fujinaga.MusicStaves_rl_fujinaga.html
"""
    pure_python = True
    category = "MusicStaves/Classes"
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0),\
            Int('staffspace_height', default=0)])
    return_type = Class("musicstaves")

    def __call__(image, staffline_height=0, staffspace_height=0):
        from gamera.toolkits.musicstaves import musicstaves_rl_fujinaga
        return musicstaves_rl_fujinaga.MusicStaves_rl_fujinaga(\
                image, staffline_height, staffspace_height)
    __call__ = staticmethod(__call__)
       
class StaffRemovalFujinaga(PluginModule):
    cpp_headers = ["staff_removal_fujinaga.hpp", "staff_removal_fujinaga_python.hpp"]
    category = "MusicStaves"
    # extra_libraries = ['tiff']
    functions = [find_and_remove_staves_fujinaga, global_staffline_deskew,
                 find_and_deskew_staves_fujinaga, find_staves_fujinaga,
                 remove_staves_fujinaga, MusicStaves_rl_fujinaga]
                 # remove_staves_fujinaga2]
    author = "Michael Droettboom, Karl MacMillan and Ichiro Fujinaga"

module = StaffRemovalFujinaga()

find_and_remove_staves_fujinaga = find_and_remove_staves_fujinaga()
global_staffline_deskew = global_staffline_deskew()
find_staves_fujinaga = find_staves_fujinaga()
find_and_deskew_staves_fujinaga = find_and_deskew_staves_fujinaga()
remove_staves_fujinaga = remove_staves_fujinaga()
