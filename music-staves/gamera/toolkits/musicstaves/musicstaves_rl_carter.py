#
#
# Copyright (C) 2005 Michael Droettboom
#
# This code is a clean-room reimplmentation of the staff removal algorithm
# described in:
#
#    Carter, N.  Automatic recognition of printed music in
#    the context of electronic publishing.  Ph.D. Thesis.  Depts. of
#    Physics and Music.  University of Surrey.
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

from gamera.args import *
from gamera.core import Rect, Point, RGBPixel, Image
from musicstaves import MusicStaves as _virtual_MusicStaves
from musicstaves import StaffObj
from gamera.plugins import structural, listutilities
from gamera import util

from math import atan, pi
from sys import maxint
import sys
import time
try:
    set
except NameError:
    from sets import Set as set

output_count = 0

################################################################################
# UTILITIES

if hasattr(listutilities, "all_subsets"):
   all_subsets = listutilities.all_subsets
else:
   def all_subsets(sequence, n):
      """Returns all subsets of length n from the sequence."""
      if not sequence or n == 0:
         return []
      if n == 1:
         return [[x] for x in sequence]
      case1 = [[sequence[0]] + s for s in subsets(sequence[1:], n-1)]
      case2 = subsets(sequence[1:], n)
      return case1 + case2

class Grouper(object):
   """Manages the grouping together of disjoint sets.

Objects can be joined using .join() and then the disjoint sets
can be obtained using .get()
"""
   def __init__(self, init=[]):
      self._mapping = {}
      for a in init:
         self._mapping[a] = [a]

   def join(self, a, *args):
      """Join all objects given as arguments into the same set."""
      mapping = self._mapping
      set_a = mapping.setdefault(a, [a])

      for arg in args:
         set_b = mapping.get(arg, None)
         if not set_b is None:
            if not set_b is set_a:
               set_a.extend(set_b)
               for elem in set_b:
                  mapping[elem] = set_a
         else:
            set_a.append(arg)
            mapping[arg] = set_a

   def joined(self, a, b):
      """Returns True if a and b are members of the same set."""
      mapping = self._mapping
      return (mapping.get(b, -1) is mapping.get(a, -2))

   def get(self):
      """Returns all of the disjoint sets as a nested list."""
      keys = self._mapping.copy()
      result = []
      while len(keys):
         key, val = keys.popitem()
         result.append(val)
         for i in val:
            try:
               del keys[i]
            except KeyError:
               pass
      return result

################################################################################
# SECTIONS

class Section(Rect):
   """This class manages "sections" as defined by Carter.

Each section is a chunk of the image as determined by the line adjacency
graph algorithm -- somewhat analogous to a connected component, except these
are not guaranteed to be surrounded by whitespace.

Runs can be added after creation to the section using add_run().

After the section is complete, calculate_features() should be called to
generate features of the section.
"""
   def __init__(self, run):
      Rect.__init__(self, run)
      # The number of black pixels in the section
      self.area = 0
      # The average vertical thickness of all the runs in the section
      self.average_thickness = 0.0
      # The runs that make up the section.  Each run is a Rect object as
      # returned by the iterate_*_*_runs plugin method.
      self.runs = []
      # The center points of all the runs.  The points together make
      # up a line down the center of the section.
      self.center_points = []
      # The number of connected sections to the right
      self.right_connections = []
      # The number of connected sections to the left
      self.left_connections = []
      # Marked True by find_potential_stafflines if the section appears to be a thin, straight, horizontal line
      self.is_filament = False
      # Marked True by find_stafflines if the section is part of an evenly spaced set of sections
      self.is_staffline_marker = False
      # Marked True by remove_stafflines if the section is aligned with a recognized staffline
      self.is_staffline = False

      # Add the first run
      self.add_run(run)

   def add_run(self, run):
      self.union(run)
      nruns = len(self.runs)
      self.average_thickness = ((self.average_thickness * nruns + run.nrows) /
                                (float(nruns) + 1))
      self.runs.append(run)
      self.area += run.nrows

   def calculate_features(self):
      # This is a funny definition of aspect ratio...
      self.aspect_ratio = float(self.ncols) / self.average_thickness
      # Line fit to center points: y = mx + b (q is the gamma fit confidence)
      center_points = [run.center for run in self.runs]
      if len(center_points) > 2:
         center_points = center_points[1:-1]
      self.m, self.b, self.q = structural.least_squares_fit(center_points)
      # Angle within a single quadrant
      self.angle = (abs(atan(self.m)) / (pi / 2)) * 90.0
      start_y=[run.ul_y for run in self.runs]
      start_y.sort()
      self.ul_y = start_y[len(start_y)/2]
      end_y=[run.lr_y for run in self.runs]
      end_y.sort()
      self.lr_y = end_y[len(end_y)/2]

   def avg_distance_from_line(self, m, b):
      distance = 0.0
      for point in self.runs[0].center, self.runs[-1].center:
         distance += point.y - (point.x * m + b)
      distance /= 2
      return abs(distance)

   def highlight(self, image, pixel):
      for run in self.runs:
         image.subimage(run).fill(pixel)

################################################################################
# FILAMENT STRINGS
class FilamentString(Rect):
   """A "Filament String" is a set of vertically aligned horizontal line sections that are
close together.  This concept is introduced to reduce the number of lines that need to
be tested to find staves.

Filaments can be considered \"potential stafflines\".
"""
   def __init__(self, filament):
      Rect.__init__(self, filament)

      self.is_staffline_marker = False

   def set_filaments(self, filaments):
      self.filaments = filaments
      for filament in filaments:
         self.union(filament)

   def set_as_staffline(self):
      self.is_staffline_marker = True
      for filament in self.filaments:
         filament.is_staffline_marker = True

################################################################################
# STAFF CHUNKS
class StaffChunk(Rect):
   """A StaffChunk is a set of filaments that are horizontally aligned and are
evenly spaced in what appears to be a staff.  Staff chunks that are vertically aligned
are later formed into entire staves/"""
   def __init__(self):
      Rect.__init__(self)

   def set_stafflines(self, stafflines):
      new_rect = self.union_rects(stafflines)
      self.rect_set(Point(new_rect.ul_x, new_rect.ul_y), Dim(new_rect.ncols, new_rect.nrows))
      self.stafflines = stafflines
      for staffline in stafflines:
         staffline.set_as_staffline()

################################################################################
# STAFFLINES
class Stafflines:
   """Manages a set of stafflines.  The lines themselves are modeled as a set
of points connected by line segments.  The staffline y-position for a given x
can then be retreived using linear interpolation along these points."""
   def __init__(self):
      self.stafflines = []
      self.staves = {}

   def add_staffline(self, staff_no, points, ncols):
      """Add a staffline to the collection of stafflines.

*points* is a list of points describing the line.  They will be
sorted in the x-direction.

*ncols* is the number of columns in the image."""
      if len(points) < 2:
         raise ValueError("There must be at least two points to define a line")

      points.sort(lambda a, b: cmp(a.x, b.x))

      # Filter the points -- we often get duplicates
      filtered_points = points[:1]
      min_y = max_y = points[0].y
      last_point = points[0]
      for point in points[1:]:
         if point.x != last_point.x and point.y != last_point.y:
            filtered_points.append(point)
            min_y = min(min_y, point.y)
            max_y = max(max_y, point.y)
            last_point = point
      if points[-1] != filtered_points[-1]:
         filtered_points.append(points[-1])
      points = filtered_points
      if len(points) < 2:
         raise ValueError("There must be at least two points to define a line")

      y = (max_y + min_y) / 2.0
      self.stafflines.append((y, points))
      self.stafflines.sort()

      if not self.staves.has_key(staff_no):
         self.staves[staff_no] = []
      self.staves[staff_no].append(points)

      return points

   def lookup_stafflines(self, y):
      """Given a *y* position, returns up to two of the closest stafflines."""
      # Binary search
      stafflines = self.stafflines
      lo = 0
      hi = len(stafflines)
      while lo < hi:
         mid = (lo + hi)//2
         if y < stafflines[mid][0]:
            hi = mid
         else:
            lo = mid + 1
      if lo == 0:
         return [stafflines[lo][1]]
      elif lo == len(stafflines):
         return [stafflines[lo-1][1]]
      else:
         return [stafflines[lo-1][1], stafflines[lo][1]]

   def linear_interpolate(self, x, line):
      # Binary search
      lo = 0
      hi = len(line)
      while lo < hi:
         mid = (lo + hi) // 2
         if x < line[mid].x:
            hi = mid
         else:
            lo = mid + 1
      if lo == 0:
         return None
      elif lo == len(line):
         return None
      after = line[lo]
      before = line[lo-1]

      # Linear interpolation
      y = before.y + float((after.y - before.y) * (x - before.x)) / (after.x - before.x)
      return int(y)

   def on_staffline(self, section, threshold):
      lines = self.lookup_stafflines(section.center_y)
      for line in lines:
         y = self.linear_interpolate(section.center_x, line)
         if (abs(y - section.center_y) < threshold):
            return True
      return False

################################################################################
# THE REMOVAL CLASS

class MusicStaves_rl_carter(_virtual_MusicStaves):
   """This code is a clean room re-implementation of the staff removal algorithm
described in Carter's PhD thesis:

  Carter, N.  Automatic recognition of printed music in
  the context of electronic publishing.  Ph.D. Thesis.  Depts. of
  Physics and Music.  University of Surrey.

Carter's approach segments the image using a concept known as the
line-adjacency graph.  Each segment resulting from this analysis is
either part of a staffline or not, so once the sections have been
found, no further analysis at the pixel level is necessary.
Stafflines are found by finding obvious straight and horizontal
candidates for staffline sections (called filaments), and then finding
filaments that are evenly spaced from one another.  These filaments
are used to build models of the stafflines (modeled in this
implementation as a set of connected line segments) and then thin
sections that fall along these modeled lines are removed.

Noteably, Carter's algorithm does not perform any deskewing or
rotation to straighten the stafflines, and the line model allows the
stafflines to be slightly curved and rotated below a certain (unknown)
threshold.  The stafflines are not even required to be parallel within
a staff.  Any systems using the results of this algorithm will have
to take the non-straight and non-horizontal line models into account
when determining the pitches of noteheads etc.

The LAG is different from the common-case graph as used in the Gamera
graph library in that each node must have two distinct sets of
undirected edges (leftward and rightward).

There is (at least) one hole in Carter's description where an
assumption was made.

  "Once a stave had been found, each staveline was tracked across the
  page in both directions and all sections which fell within the
  projected path of the staveline and were below the threshold for
  permitted filament thickness were flagged as staveline sections."

How the "projected path" is calculated is not clear, so I have
implemented something that seems to work and at least doesn't
contradict Carter's thesis.  Each staffline filament is used as a
starting point and the graph is traced in both directions looking for
additional line segments that are part of the staff.  If a likely
staffline candidate is not found directly through the graph, a second
more expensive search is performed that allows for gaps in the staff
lines.  The projected path of the line is computed by using a least
squares fit on the last *n* points in the opposite direction of tracing,
where *n* is a relatively large number, in this case staffspace_height
* 8.0.  This keeps the projected path relatively free of errors in the
angle, and allows for a moderate amount of local curvature.  (Using the
projected path of each line segment individually caused too many
over-skewed projected angles, particularly on short, noisy segments.
In some cases, these projected lines would cross into adjacent
stafflines and all kinds of terrible things would happen.)

Additionally, Carter does not specify a threshold as to when a
set of filaments are evenly spaced enough to be considered a
set of stafflines.  From experimentation I arrived at

  if (\|dist - (staffspace_height + staffline_height)\| <= 3) then is staff

but that may fail in some untested cases.

.. note: Staff lines with no crossing symbols are not identified as stafflines
         by Carter's algorithm, because they only consist of one segment
         lacking the property of being horizontally linked with other staff
         segment candidates. Consequently the *entire staff system* is not
         detected. In most scores this is rare because bar lines touch all
         staff lines, but it can cause problems in barless music.

:Author: Michael Droettboom after an algorithm by Nicholas Carter.
"""
   def __init__(self, img, staffline_height=0, staffspace_height=0):
      _virtual_MusicStaves.__init__(self, img, staffline_height, staffspace_height)
      self.stafflines = None
      self.remove_staves_args = Args([ChoiceString("crossing_symbols",\
                                                   choices=['all','bars','none'],\
                                                   strict=False),\
                                      Int("num_lines", default=5)])


   def _make_line_adjacency_graph(self, image, adj_ratio=2.5):
      """From the given image, returns a list of line adjacency graph sections.

*adj_ratio* is the maximum ratio of the heights of two adjacent runs in order to
be considered part of the same section.
"""
      inv_adj_ratio = 1.0 / adj_ratio
      sections = []
      last_column = []
      runs_to_sections = {}
      for column in image.iterate_runs('black', 'vertical'):
         column = list(column)
         possible_connections = {}
         for run in column:
            possible_connections[run] = []
         for run in last_column:
            possible_connections[run] = []

         i = j = 0
         while i < len(last_column) and j < len(column):
            run = column[j]
            last_run = last_column[i]
            if run.intersects_y(last_run):
               possible_connections[run].append(last_run)
               possible_connections[last_run].append(run)
            if last_run.lr_y < run.lr_y:
               i += 1
            else:
               j += 1

         new_runs_to_sections = {}
         for run in column:
            joined = False
            run_connections = possible_connections[run]
            if len(run_connections) == 1:
               last_run = run_connections[0]
               last_run_connections = possible_connections[last_run]
               if len(last_run_connections) == 1:
                  section = runs_to_sections[last_run]
                  ratio = section.average_thickness / run.nrows
                  if ratio < adj_ratio and ratio > inv_adj_ratio:
                     section.add_run(run)
                     new_runs_to_sections[run] = section
                     joined = True
            if not joined:
               section = Section(run)
               sections.append(section)
               new_runs_to_sections[run] = section
               for other_run in possible_connections[run]:
                  left_section = runs_to_sections[other_run]
                  left_section.right_connections.append(section)
                  section.left_connections.append(left_section)
         runs_to_sections = new_runs_to_sections
         last_column = column
      return sections

   def _remove_noise(self, sections, noise_size=5):
      """Returns a list of sections with the noise sections removed.
Sections with an area smaller than *noise_size* which are not connected
to other sections are removed."""
      result = []
      for section in sections:
         if (section.area < noise_size and
             len(section.left_connections) + len(section.right_connections) <= 1):
            for other_section in section.left_connections:
               other_section.right_connections.remove(section)
            for other_section in section.right_connections:
               other_section.left_connections.remove(section)
         else:
            result.append(section)
      return result

   def _calculate_features(self, sections):
      """Calculates features for all of the given sections."""
      for section in sections:
         section.calculate_features()

   def _find_potential_stafflines(self, sections, alpha=10.0, beta=0.1, angle_threshold=3):
      """Finds potential staffline sections.

These are sections that:

  1. Are thinner than staffline_height * 2.0

  2. Have an aspect ratio > *alpha*

  3. Have at least one connection to the left or right.

  4. Are straighter than *beta*.

Lastly, sections above are further filtered to remove sections whose
angle is not near the mean angle.
"""
      histogram = [0] * 91
      for section in sections:
         if (section.average_thickness < self.staffline_height * 2.0 and
             section.aspect_ratio > alpha and
             len(section.left_connections) + len(section.right_connections) >= 1 and
             abs(section.q) > beta):
            section.is_filament = True
            histogram[int(section.angle)] += 1
      
      angle_mode = histogram.index(max(histogram))
      filaments = []
      for section in sections:
         if section.is_filament:
            if abs(section.angle - angle_mode) < angle_threshold:
               filaments.append(section)
            else:
               section.is_filament = False

      return filaments

   def _create_filament_strings(self, filaments, string_join_factor=0.25):
      """Joins filaments into strings.  The purpose of this is mainly to reduce the
number of filaments that need to be tested for membership in a staff.

*string_join_factor* controls how close two filaments need to be in order to
be joined into a string
"""
      filament_groups = Grouper()
      for i, sec0 in enumerate(filaments):
         filament_groups.join(sec0)
         ext_length = sec0.ncols * string_join_factor
         for j in range(i + 1, len(filaments)):
            sec1 = filaments[j]
            if (abs(sec1.center_y - sec0.center_y) < self.staffline_height * 2.0):
               if ((sec1.ul_x <= sec0.lr_x + ext_length and
                    sec1.ul_x >= sec0.ul_x) or
                   (sec1.lr_x >= sec0.ul_x - ext_length and
                    sec1.lr_x <= sec0.lr_x)):
                  filament_groups.join(sec0, sec1)

      strings = []
      for string in filament_groups.get():
         filament_string = FilamentString(string[0])
         filament_string.set_filaments(string)
         strings.append(filament_string)

      return strings

   def _find_stafflines(self, strings, num_lines=5, staffdist_threshold=3):
      """Given a list of filament strings finds sets that are horizontally
overlapping and evenly spaced to form staves.

The result is a Staffline object that models all stafflines on the whole page
as a set of connected line segments.
"""
      # The maximum distance between two filament strings that will be
      # considered as part of the same staff
      threshold = (self.staffspace_height + self.staffline_height) * num_lines * 1.1
      staffdist = self.staffspace_height + self.staffline_height

      # Sort so the staves are found in top-to-bottom order.
      strings.sort(lambda x, y: cmp(x.center_y, y.center_y))

      # Find evenly spaced sets of *num_lines* filament strings
      staffline_strings = set()
      for string0 in strings:
         overlapping = []
         for string1 in strings:
            if string0 is not string1:
               if (string0.intersects_x(string1) and
                   abs(string0.center_y - string1.center_y) < threshold):
                  overlapping.append(string1)
         if len(overlapping) < num_lines - 1:
            continue
         for subset in all_subsets(overlapping, num_lines - 1):
            spacings = [string0.center_y] + [x.center_y for x in subset]
            spacings.sort()
            ok=True
            for i in range(1,len(spacings)):
               if abs(spacings[i]-spacings[i-1]-staffdist)>staffdist_threshold:
                  ok=False
                  break
            if ok:
               staffline_strings.add(string0)
               #staffline_strings.union_update(set(subset))
               staffline_strings.update(set(subset))

      return list(staffline_strings)

   def _trace_stafflines(self, sections, staffline_strings, angle_threshold=3, horizontal_gaps=None):
      """Given a list of sections and staffline markers, removes the
sections that are thin and fall on the locally projected stafflines."""
      line_match_threshold = self.staffline_height * 2.0
      if horizontal_gaps is None:
         horizontal_gaps = self.staffspace_height * 3.0
      # num_points is used to determine how far back the interpolation
      # of projection line direction should go
      num_points = int(self.staffspace_height * 8.0)

      # line_groups will be used to group the line segments together
      # into stafflines as they are traced across the page
      line_groups = Grouper()
      for string in staffline_strings:
         line_groups.join(*(string.filaments))
         for filament in string.filaments:
            filament.is_staffline = True


      def recurse(direction, staffline, last_points):
         # The "projected line direction" is a combination of the current
         # line segment and the "history" of line segments we've seen
         # up until now.  This prevents little line segments with very
         # off-skew angles from overly affecting the tracing process
         if direction: # Right
            stack = staffline.right_connections[:]
            if len(staffline.runs) > num_points:
               points = [x.center for x in staffline.runs[-num_points:]]
            else:
               points = last_points[-(num_points - len(staffline.runs)):] + [x.center for x in staffline.runs]
         else:
            stack = staffline.left_connections[:]
            if len(staffline.runs) > num_points:
               points = [x.center for x in staffline.runs[:num_points]]
            else:
               points = [x.center for x in staffline.runs] + last_points[:(num_points - len(staffline.runs))]
         # This is the projected line
         m, b, q = structural.least_squares_fit(points)

         # Search A:  Try things that are directly connected in the LAG
         #            first.
         # Perform a breadth-first search in the current direction
         visited = {}
         while len(stack):
            # Sort the potential line segments by vertical distance
            # from the current line segment so the best match will be
            # found first
            stack.sort(lambda x, y: cmp(
               abs(staffline.center_y - x.center_y),
               abs(staffline.center_y - y.center_y)))

            section = stack.pop(0)
            if not visited.has_key(section):
               visited[section] = None
               if (section.average_thickness <= line_match_threshold and
                   abs(section.angle - staffline.angle) <= angle_threshold):
                  if section.avg_distance_from_line(m, b) < line_match_threshold:
                     line_groups.join(staffline, section)
                     if not section.is_staffline:
                        section.is_staffline = True
                        recurse(direction, section, points)
                     return
               if not section.is_staffline:
                  if direction:
                     stack.extend(section.right_connections)
                  else:
                     stack.extend(section.left_connections)

         # Search B: If Search A fails, search for line segments across small
         #           gaps (less than horizontal_gaps wide)
         for section in sections:
            if not section.is_staffline:
               if section.average_thickness <= line_match_threshold:
                  if ((direction and
                       section.ul_x > staffline.lr_x and
                       section.ul_x < staffline.lr_x + horizontal_gaps) or
                      (not direction and
                       section.lr_x < staffline.ul_x and
                       section.lr_x > staffline.ul_x - horizontal_gaps)):
                     if section.avg_distance_from_line(m, b) < line_match_threshold:
                        line_groups.join(staffline, section)
                        if not section.is_staffline:
                           section.is_staffline = True
                           recurse(direction, section, points)
                        return

         # We didn't find anything, but that's really okay...

      # Do the easy ones that we can walk to through the graph
      stafflines = [x for x in sections if x.is_staffline]
      stafflines.sort(lambda x, y: cmp(x.center_y, y.center_y))
      for staffline in stafflines:
         recurse(0, staffline, [])
         recurse(1, staffline, [])

      return line_groups

   def _remove_stafflines(self, sections):
      """Remove the sections marked as is_staffline from the image."""
      for section in sections:
         if section.is_staffline:
            for run in section.runs:
               self.image.subimage(run).fill(0)

   def _make_line_models(self, line_groupings):
      """Generate the staffline models."""
      stafflines = Stafflines()
      for i, staffline in enumerate(line_groupings.get()):
         line_points = []
         for line in staffline:
            line_points.extend([x.center for x in line.runs])
         stafflines.add_staffline(i, line_points, self.image.ncols)
      return stafflines

   def remove_staves(self, crossing_symbols='all', num_lines=5, adj_ratio=2.5, noise_size=5, alpha=10.0, beta=0.1, angle_threshold=3, string_join_factor=0.25, debug=False):
      """Detects and removes staff lines from a music/tablature image.

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=5, adj_ratio=2.5, noise_size=5, alpha=10.0, beta=0.1, angle_threshold=3.0, string_join_factor=0.25)``

with

  *crossing_symbols*:
    Ignored.

  *num_lines*:
    The number of stafflines in each staff.  (Autodetection of number of stafflines not yet
    implemented).

It is unlikely one would need to provide the arguments below:

  *adj_ratio*
    The maximum ratio between adjacent vertical runs in order to be considered part of the
    same section.  Higher values of this number will result in fewer, larger sections.

  *noise_size*
    The maximum size of a section that will be considered noise.

  *alpha*
    The minimum aspect ratio of a potential staffline section.

  *beta*
    The minimum \"straightness\" of a potential staffline section (as given by the gamma fittness
    function of a least-squares fit line).

  *angle_threshold*
    The maximum distance from the mean angle a section may be to be considered as a potential
    staffline (in degrees).

  *string_join_factor*
    The threshold for joining filaments together into filament strings.

  *debug*
    When True, returns a tuple of images to help debug each stage of the algorithm.
    Times for each stages are also displayed.  The tuples elements are:

    *result*
       The normal result, with stafflines removed.

    *sections*
       Shows the segmentation by LAG.  Use display_ccs to show the segments differently
       coloured.

    *potential_stafflines*
       Shows the sections that are potentially part of stafflines

    *filament_strings*
       Shows how the vertically aligned potential stafflines are grouped into
       filament_strings

    *staffline_chunks*
       The filament strings believed to be part of stafflines

    *stafflines*
       Draws the modelled lines on top of the image.
"""
      if num_lines <= 0:
         num_lines = 5

      t = time.clock()
      sections = self._make_line_adjacency_graph(self.image, adj_ratio)
      if debug:
         print ("Line adjacency graph time: %.04f, num sections: %d" %
                (time.clock() - t, len(sections)))
         sections_image = Image(self.image)
         for i, section in enumerate(sections):
            section.i = i
            for run in section.runs:
               sections_image.subimage(run).fill(i+1)

      t = time.clock()
      sections = self._remove_noise(sections, noise_size)
      if debug:
         print ("Remove noise time: %.04f, num sections: %d" %
                (time.clock() - t, len(sections)))

      t = time.clock()
      self._calculate_features(sections)
      if debug:
         print "Calculate features time: %.04f" % (time.clock() - t)

      t = time.clock()
      filaments = self._find_potential_stafflines(sections, alpha, beta, angle_threshold)
      if debug:
         print ("Find potential stafflines time: %.04f, num filaments: %d" %
                (time.clock() - t, len(filaments)))
         filaments_image = Image(self.image)
         for i, section in enumerate(filaments):
            if section.is_filament:
               for run in section.runs:
                  filaments_image.subimage(run).fill(i+1)

      t = time.clock()
      strings = self._create_filament_strings(filaments, string_join_factor)
      if debug:
         print ("Create filament strings time: %.04f, num strings: %d" %
                (time.clock() - t, len(strings)))
         strings_image = Image(self.image)
         for i, string in enumerate(strings):
            for filament in string.filaments:
               for run in filament.runs:
                  strings_image.subimage(run).fill(i+1)

      del filaments

      t = time.clock()
      staffline_strings = self._find_stafflines(strings, num_lines)
      if debug:
         print ("Find stafflines time: %.04f num staffline fragments: %d" %
                (time.clock() - t, len(staffline_strings)))
         staffline_image = Image(self.image)
         staffline_model_image = self.image.to_rgb()
         for i, staffline in enumerate(staffline_strings):
            for section in staffline.filaments:
               for run in section.runs:
                  staffline_image.subimage(run).fill(i+1)

      if len(staffline_strings) == 0:
         return self.image

      t = time.clock()
      line_groupings = self._trace_stafflines(sections, staffline_strings, angle_threshold)
      if debug:
         print ("Trace stafflines time: %.04f" % (time.clock() - t))

      t = time.clock()
      self._remove_stafflines(sections)
      if debug:
         print ("Remove stafflines time: %.04f" % (time.clock() - t))

      t = time.clock()
      self.stafflines = self._make_line_models(line_groupings)
      if debug:
         print ("Make line models time: %.04f" % (time.clock() - t))
         for y, staffline in self.stafflines.stafflines:
            last_point = staffline[0]
            for point in staffline[1:]:
               staffline_model_image.draw_line(last_point, point, RGBPixel(255, 0, 0))
               last_point = point
         return (self.image, sections_image, filaments_image, strings_image,
                 staffline_image, staffline_model_image)
      else:
         return self.image

   def get_staffpos(self, x=-1):
      """Returns the y-positions of all staff lines at a given x-position.
Can only be called *after* ``remove_staves``.

Signature:

  ``get_staffpos(x=0)``

Since the lines are modelled as connected sets of non-horizontal line segments,
*x* is relevant to the result.

The return value is a list of `StaffObj`__.

.. __: gamera.toolkits.musicstaves.musicstaves.StaffObj.html#staffobj
"""
      if self.stafflines is None:
         raise RuntimeError("You must call remove_staves before get_staffpos")

      result = []
      if x == -1:
         x = self.image.ncols / 2
      for i, staff in enumerate(self.stafflines.staves.values()):
         staff_result = StaffObj()
         staff_result.staffno = i
         for line in staff:
            staff_result.yposlist.append(self.stafflines.linear_interpolation(x, line))
         result.append(staff_result)
      return result
