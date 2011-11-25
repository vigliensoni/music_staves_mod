#
#
# Copyright (C) 2005 Michael Droettboom, Thomas Karsten, Christoph Dalitz
#
# This code is a clean-room reimplementation of the algorithm described in
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

# There's not much interesting in this file.  It is really a wrapper around
# C++ code in order to support the MusicStaves API.
# See include/plugins/roach_tatem_plugins.hpp for the real action.

from gamera.core import *

from musicstaves import MusicStaves as _virtual_MusicStaves
from stafffinder_dalitz import StaffFinder_dalitz
from plugins import roach_tatem_plugins
from plugins import vector_field
import time

class MusicStaves_rl_roach_tatem(_virtual_MusicStaves):
   """This code is a clean-room reimplementation of the algorithm
described in
   
    Roach, J. W., and J. E. Tatem. 1988. Using domain knowledge in
    low-level visual processing to interpret handwritten music: an
    experiment. Pattern Recognition 21 (1): 33-44.

For a general overview of the algorithm, readers are referred to the
paper. In addition to the algorithm described in this paper, we have
added two improvements described below.

Implementation notes:

  When computing the vector field, only the angle is stored for each
  pixel.  The distance and thickness values are used by higher-level
  parts of their system and are not used for raw staff removal, so to
  save run- and programming-time, this was not implemented.

  Roach and Tatem suggest a radius for the vector field window of \"at
  least half the width of one staves in the image.\"  I assume the
  authors meant \"width in the vertical direction\" as the horizontal
  direction would be so large as to take hours to compute a vector
  field.

  The original algorithm given by Roach and Tatem for computing the vector
  field was in our experiments computationally too expensive for practical
  use. Hence we have used a brute force approach instead, which picks the
  longest chord for a predefined range of angles (see
  `compute_longest_chord_vectors`__).

.. __: musicstaves.html#compute-longest-chord-vectors

  The rules described with bullet points in Sections 3.4 and 3.5 are
  quite clear, but exactly how those rules are applied is not entirely
  clear to me.  For instance, are the rules applied in parallel or in
  sequence?  Should the side effect of one rule be immediately
  applied?  Adding to the confusion, it appears the first rule can not
  be applied in sequence before the second.  Through experimentation,
  I decided to apply each rule to the entire image, one at a time.
  The side effects are applied all at once before applying the next
  rule.

  The algorithm has a concept of \"questionable pixels\", which are
  pixels that likely belong to both a staffline and a musical figure.
  Nothing is made of this information at present.  

  Roach and Tatem claim it is a simple matter to find stafflines because
  they are long horizontal connected components.  In our experiments,
  however, the stafflines were so broken up that we were unable to find
  a threshold to separate them from other noise using horizontal width
  alone.

As the application of Roach and Tatem was handwritten music, their
method simply removes all perfectly straight horizontal
shapes. As this yields very poor results in printed music (it also
removes beams and parts of lyrics) we have added two improvements:

  - Vertical black runlengths longer than 2*staffline_height
    are not removed. This should leave most beams intact.

  - On the horizontal lines detected by Roach and Tatem's algorithm
    we apply the staff finding algorithm `StaffFinder_dalitz`__ and
    only remove staff segments that cross the detected staff lines.

.. __: gamera.toolkits.musicstaves.stafffinder_dalitz.StaffFinder_dalitz.html

:Author: Michael Droettboom, Thomas Karsten and Christoph Dalitz, based on Roach, J. W. and J. E. Tatem
"""
   def __init__(self, img, staffline_height=0, staffspace_height=0):
      _virtual_MusicStaves.__init__(self, img, staffline_height, staffspace_height)
      # for caching staff position information
      self.__staffobjlist = None
      self.linelist = None # structure like in StaffFinder


   def remove_staves(self, crossing_symbols='all', num_lines=0, resolution=3.0, debug=0):
      """Detects and removes staff lines from a music/tablature image.

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=0, resolution=3.0, debug=0)``

with

  *crossing_symbols*:
    This algorithm only supports the value 'all'.

  *num_lines*:
    Number of lines within one staff. A value of zero for automatic
    detection is supported, but if you give the actual number, staff line
    detection will improve.

  *resolution*:
    the resolution parameter passed to `compute_longest_chord_vectors`__

.. __: musicstaves.html#compute-longest-chord-vectors

  *debug*:
    Set the debug level verbosity. For *debug* >= 1, tracing information
    is printed to stdout, for *debug* >= 2 images with prefix
    ``debug`` are written into the current working directory.

.. note:: The property *image* is the result of our \"improved\" algorithm.
          For comparison, the result of Roach and Tatem's original method
          is stored in the property *image_original*.
"""
      t = time.time()
      theta_image, chordlength, chordthickness = self.image.compute_longest_chord_vectors(\
		      path_length = self.staffspace_height * 3,
                      resolution = resolution)
      if debug > 0:
         print "computation vectorfield:", time.time() - t, "sec"
      t = time.time()
      lines, question = self.image.mark_horizontal_lines_rt(theta_image, 8)
      if debug > 0:
         print "mark lines:", time.time() - t, "sec"
      if debug > 1:
         rgb = self.image.to_rgb()
         rgb.highlight(lines,RGBPixel(255,0,0))
         rgb.save_PNG("debug-alllines.png")
      # safe Roach/Tatems original result for reference
      self.image_original = self.image.remove_stafflines_rt(lines)

      # this has removed too much, so we must improve the result
      # by investigating the removed segments and keeping some of then
      toremove = self.image.subtract_images(self.image_original)
      removedorig = None
      if debug > 1:
         removedorig = toremove.image_copy()

      # 1) do not remove long vertical runs
      threshold = 2*self.staffline_height + 1
      longverticalruns = self.image.image_copy()
      longverticalruns.filter_short_runs(threshold, "black")
      toremove.subtract_images(longverticalruns,in_place=True)
      del longverticalruns

      # 2) find actual stafflines among detected lines and only remove
      #    segments crossing the stafflines
      t = time.time()
      sf = StaffFinder_dalitz(lines,self.staffline_height,self.staffspace_height)
      sf.find_staves(num_lines=num_lines)
      self.linelist = sf.linelist
      staffpoints = []
      staffs = sf.get_skeleton()
      for s in staffs:
         for l in s:
            x = l.left_x
            for y in l.y_list:
               staffpoints.append(Point(x,y))
               x += 1
      if debug > 0:
         print "staff line finding:", time.time() - t, "sec"
      if debug > 1:
         for p in staffpoints:
            rgb.set(p,RGBPixel(0,255,0))
         rgb.save_PNG("debug-foundlines.png")
      t = time.time()
      toremove = self.__keep_only_intersection(toremove,staffpoints)
      if debug > 0:
         print "removing false positives:", time.time() - t, "sec"
      if debug > 1:
         rgb = self.image.to_rgb()
         rgb.highlight(removedorig,RGBPixel(255,0,0))
         rgb.highlight(toremove,RGBPixel(0,255,0))
         rgb.save_PNG("debug-falsepositives.png")
      self.image = self.image.subtract_images(toremove)
         
   def __keep_only_intersection(self,image,points):
      """Removes all CCs from *image* that do not contain any of *points*."""
      ccs = image.cc_analysis()
      # collect cc labels of all points in "points"
      cclabels = {}
      for p in points:
         cclabels[image.get(p)] = 1
      for cc in ccs:
         if not cclabels.has_key(cc.label):
            cc.fill_white()
      return image

   def get_staffpos(self, x=0):
      """Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

Signature:

  ``get_staffpos(x=0)``

with

  *x*:
    This parameter has no effect, because the staffline is assumed to
    be horizontal.

The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj

Note that a more accurate way to obtain the staff lines for this class
is by accessing ``self.linelist``, because this does not just yield the average
y-position, but the full staff line skeletons. Example:

.. code:: Python

   ms = MusicStaves_skeleton(image)
   ms.remove_staves()
   sf = StaffFinder(image, ms.staffline_height, ms.staffspace_height)
   sf.linelist = ms.linelist
   rgb = sf.show_result()
   rgb.save_PNG(\"foundstaves.png\")
"""
      if not self.__staffobjlist and self.linelist != None:
         self.__staffobjlist = []
         for i,staff in enumerate(self.linelist):
            if len(staff) > 0:
               so = StaffObj()
               so.staffno = i+1
               so.yposlist=[avg.average_y for avg in \
                            [line.to_average() for line in staff]]
               so.staffrect = Rect(Point(so.yposlist[0],staff[0].left_x),\
                                   Point(so.yposlist[-1],staff[-1].y_list[-1]))
               self.__staffobjlist.append(so)

      return self.__staffobjlist
