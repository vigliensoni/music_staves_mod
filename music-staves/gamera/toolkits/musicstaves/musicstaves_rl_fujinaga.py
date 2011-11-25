#
#
# Copyright (C) 2005 Michael Droettboom
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

# There's not much interesting in this file.  It is really a wrapper around
# C++ code in order to support the MusicStaves API.
# See include/plugins/staff_removal_fujinaga.hpp for the real action.

from musicstaves import MusicStaves as _virtual_MusicStaves
from plugins import staff_removal_fujinaga

class MusicStaves_rl_fujinaga(_virtual_MusicStaves):
   """This staff removal algorithm is based directly on Ichiro
Fujinaga's staff removal code, though through the filter of two major
rewrites, the first by Karl MacMillan and the second by Michael
Droettboom.

This code is known to be functionally equivalent to Fujinaga's
original code with the following exceptions:

- the case where systems stop in the middle of the page (as can be
  found in some chorale collections, for example) are not handled.

- a few things that were obviously bugs were fixed, though you can
  expect the output to be slightly different.

A notable feature of this algorithm is that it attempts to straighten
staves through deskewing, by shearing thin vertical strips up and down
by cross-correlating them with their neighbors.  This deskewing first
happens globally on the page and then on each staff individually.
This helps to rectify images where the staves

  a) are not straight, as in the spine of a large book, or other
  anomalies in the printing

  b) are not parallel with one another, as is often the case in older
  hand-printed music.

A consequence of this is that staffline detection is not easily
decoupled from deskewing.  The deskewing can not finish without
knowing where the stafflines are, and the stafflines can only be found
as the iterative deskewing takes place, (since stafflines are modelled
as perfectly straight and horizontal line segments).

In the last rewrite of the code, staff removal was decoupled from staff finding
and deskewing.  So there are three separate functions available so that the
removal step could be interchanged with something else:

   find_and_remove_staves_fujinaga
   find_and_deskew_staves_fujinaga
   remove_staves_fujinaga

TODO: There's a lot more to explain here.
  
References:

- Ichiro Fujinaga: *Adaptive Optical Music Recognition*. Dissertation,
  McGill University Montreal, Canada (1996)

- Ichiro Fujinaga: *Staff Detection and Removal*. in S. George (editor):
  \"Visual Perception of Music Notation\". Idea Group (2004)

:Author: Michael Droettboom, Karl MacMillan and Ichiro Fujinaga
"""
   def __init__(self, img, staffline_height=0, staffspace_height=0):
      _virtual_MusicStaves.__init__(self, img, staffline_height, staffspace_height)
      self.page = None

   def remove_staves(self, crossing_symbols='all', num_lines=5,
                     skew_strip_width=0, max_skew=5.0, undo_deskew=False):
      """Detects and removes staff lines from a music/tablature image.

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=5, skew_strip_width=0, max_skew=5.0, undo_deskew=False)``

with

  *crossing_symbols*:
    Determines which symbols crossing staff lines should be kept intact.
    Supported values are 'all', 'bars' or 'none'.
    **Currently ignored**

  *num_lines*:
    The number of stafflines in each staff.  (Autodetection of number of stafflines not yet
    implemented).

  *skew_strip_width* = 0
    The width (in pixels) of vertical strips used to deskew the image.  Smaller values will
    deskew the image with higher resolution, but this may make the deskewing overly sensitive.
    Larger values may miss fine detail.  If *skew_strip_width* <= 0, the width will be
    autodetermined to be the global *staffspace_h* * 2.

  *max_skew* = 5.0
    The maximum amount of skew that will be detected within each vertical strip.  Expressed in
    degrees.  This value should be fairly small, because deskewing only approximates rotation
    at very small degrees.

  *undo_deskew* = False
    since the fujinaga performs deskewing on the input image, it might be necessary
    to undo it, for evaluational reasons.
"""
      crossing_symbols = ['all', 'bars', 'none'].index(crossing_symbols)
      deskewed, self.image, self.page = staff_removal_fujinaga.find_and_remove_staves_fujinaga(
         self.image, crossing_symbols, num_lines, self.staffline_height, self.staffspace_height,
         skew_strip_width, max_skew, undo_deskew = undo_deskew)
      return self.image

   def get_staffpos(self, x=0):
      """Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

Signature:

  ``get_staffpos(x=0)``

with

  *x*:
    This parameter is currently ignored, but we do know where the left and right
    sides of the staff are, so we could do something with that.

The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj
"""
      if self.page is None:
         raise RuntimeError("You must call remove_staves before get_staffpos")
      return self.page
