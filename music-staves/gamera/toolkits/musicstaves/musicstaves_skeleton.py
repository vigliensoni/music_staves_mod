# -*- mode: python; indent-tabs-mode: nil; tab-width: 4; py-indent-offset: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2005,2006 Christoph Dalitz, Bastian Czerwinski
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

from sets import Set
from sys import stdout
from math import cos, sin, pi, tan, sqrt, atan, acos
import time

from gamera.gui import has_gui
## from wxPython.wx import *
from gamera import toolkit
from gamera.args import *
from gamera.core import *
from gamera.plugins.structural import least_squares_fit, least_squares_fit_xy
from gamera.toolkits.musicstaves.musicstaves import MusicStaves as _virtual_MusicStaves
from gamera.toolkits.musicstaves.musicstaves import StaffObj
from gamera.toolkits.musicstaves.stafffinder import StaffFinder, StafflineSkeleton, StafflineAverage
from gamera.toolkits.musicstaves.stafffinder_projections import StaffFinder_projections
from gamera.toolkits.musicstaves.plugins.skeleton_utilities import lin_parabola,parabola,estimate_next_point

##############################################################################
#
#
class MusicStaves_skeleton(_virtual_MusicStaves):
    """Implements a skeleton based staff line removal.

The algorithm consists of the following steps:

1) Depending on the sekeleton type (medial axis or thinned), the skeleton
   needs some \"improvement\". Medial axis skeletons contain a lot of short
   spurs; these are removed when they are shorter than *staffline_height* / 2.
   Thinned out skeletons do not extend to the edges; these are extended
   by extrapolating the skeleton ends.

2) The \"improved\" skeleton is split at branching
   and corner points; around each splitting point a number of pixels (taken
   from the distance tranform at the splitting point) is removed. The latter
   avoids that staff line skeleton segments extent into crossing objects.

3) Staff line segment candidates are picked as skeleton segments with
   the following properties:

   - the orientation angle (least square fitted line) is below
     25 degrees
   - the segment is wider than high
   - the \"straightness\" (mean square deviation from least square fitted
     line) is below *staffline_height* \*\*2 / 2

4) The overall skew angle is determined as the median of the orientation
   angle probability distribution. In this distribution, each segment
   is weighted according to its width.

5) The staff segment candidates are then horizontally and vertically linked. 
   A vertical link is added between segments with a horizontal overlap
   and a vertical distance around staffline_height + staffspace_height.
   A horizontal link is added when the extrapolated end points are
   close enough (below staffline_height); for the extrapolation the
   previously calculated skew angle or the segment orientation is used.
    
6) Staff systems and lines are then detected as connected components
   in the segmant graph as in ``StaffFinder_miyao``. Connected components
   that are too small are removed. When *num_lines*
   is not given, it is guessed as in ``StaffFinder_miyao`` and lines
   are removed from overfull staff systems.

7) A staff now typically still contains some \"false positives\" (non
   staff segments which are detected as staff segments). When staff segments
   assigned to the same line overlap, this is a clear indication of a false
   positive. Thus from each overlapping staff segment group on the same line
   the one that is closest to its least-square-fitted neighborhood
   is picked and the other are discarded.

8) To check for further \"false positives\", non staff segments that have
   the same branching point as a staff segment are extrapolated by a
   parametric parabola. If this parabola is approximately tangential
   to the staff segment, the latter is considered a false positive.

9) In order to remove staff lines, all vertical black runs around the
   detected staff systems are removed. As skeleton branches occasionally
   extend into solid regions, vertical runs are only removed when they
   are not longer than *2 \* staffline_height*.

References:

  Skeletonization is a commonly used technique in optical music recognition,
  though it is usually applied *after* staff line removal. As a proof of
  concept, D. Nehab detected staff lines in a vectorized skeleton, see
  Diego Nehab: *Staff Line Detection by Skewed Projection.*
  `http://www.cs.princeton.edu/~diego/academic/phd/`__ (2003)

.. __: http://www.cs.princeton.edu/~diego/academic/phd/496/writeup.pdf

:Author: Christoph Dalitz
"""

    def __init__(self, img, staffline_height=0, staffspace_height=0, skelimage=None, medialaxis=False):
        """In addition to the arguments of the constructor of the MusicStaves
base class, the constructor optionally allows to pass a precomputed skeleton
image. This can be useful for testing purposes, because skeleton computation
can be time consuming.

Signature:

  ``__init__(image, staffline_height=0, staffspace_height=0, skelimage=None, medialaxis=False)``

with

  *sekelimage*:
    a onebit image of the skeleton. When the skeleton is a medial axis
    transform, set the following parameter to ``True``.

  *medialaxis*:
    Depending on the skeleton generating algorithm, the skeleton can
    be a medial axis transform or a thinned out image.

    .. image:: images/medialaxis.png

As each skeleton type requires a different preprocessing in
*remove_staves*, you must provide the type as a parameter. When you do
not provide a precomputed skeleton and leave the skeleton computation
to *remove_staves*, set ``medialaxis=False``, because a thinned out
skeleton is significantly faster to compute than a medial axis transform.
"""
        _virtual_MusicStaves.__init__(self, img,\
                staffline_height, staffspace_height)
        self.skelimage = skelimage
        self.medialaxis = medialaxis
        self.debug = 0

        # GUI stuff
        crossing_symbols_list = ['all', 'bars', 'none']
        self.remove_staves_args = Args([ChoiceString("crossing_symbols", \
                                                     crossing_symbols_list), \
                                        Int("num_lines", default = 0), \
                                        Int("debug", default = 1)])
        # for caching staff position information
        self.__staffobjlist = None
        self.linelist = None # structure like in StaffFinder


    ######################################################################
    # remove_staves
    #
    # Christoph Dalitz
    #
    def remove_staves(self, crossing_symbols='all', num_lines=0, debug=0, false_positive_criterion='quad_angle', horizontal_extrapolation='segment_angle'):
        """Detects and removes staff lines from a music/tablature image.
A skeleton of the found staff lines is stored as a nested list of
``StafflineSkeleton`` in the property ``linelist``, where each sublist
represents a staff system (as in the ``StaffFinder`` base class).

Signature:

  ``remove_staves(crossing_symbols='all', num_lines=0, debug=0,\
                  false_positive_criterion='quad_angle',\
                  horizontal_extrapolation='segment_angle')``

with

  *crossing_symbols*:
    This algorithm only supports the value 'all'.

  *num_lines*:
    Number of lines within one staff. A value of zero for automatic
    detection is supported, but if you give the actual number, staff line
    detection will improve.

  *debug*:
    Set the debug level verbosity. For *debug* >= 1, tracing information
    is printed to stdout, for *debug* >= 2 images with prefix
    ``skeldebug`` are written into the current working directory.

  *false_positive_criterion*
    Criterion to be used to identify false positives. Supports the values
    'none', 'lin_meets', 'lin_angle' and 'quad_angle'.

  *horizontal_extrapolation*
    In which direction the extrapolation for horizontal linking of segments
    shall be done. Possible values are 'segment_angle' (angle of least square
    line through the segment) or 'skew_angle' (global document skew angle).
"""

        self.debug = debug
        self.false_positive_criterion = false_positive_criterion
        timestat = []

        # compute skeleton data
        if debug > 0:
            self.logmsg("staffline_height: %d, staffspace_height: %d\n" % \
                        (self.staffline_height, self.staffspace_height))
            self.logmsg("compute distance transform\n")
        t = time.time()
        distance_transform = self.image.distance_transform(2)
        timestat.append(["distance transform", time.time()-t])
        if (self.skelimage == None):
            if debug > 0:
                self.logmsg("compute skeleton\n")
            t = time.time()
            if self.medialaxis:
                self.skelimage = self.image.thin_hs()
            else:
                self.skelimage = self.image.thin_zs()
            timestat.append(["skletonization", time.time()-t])

        #
        # 1) compute all branches of purged skeleton
        #
        if debug > 0:
            self.logmsg("compute skeleton branches\n")
        t = time.time()
        if self.medialaxis:
            nospurs = self.skelimage.remove_spurs_from_skeleton((self.staffline_height+1)/2,2)
        else:
            nospurs = self.skelimage.extend_skeleton(distance_transform, extrapolation_scheme='linear', n_points=5)
        allsegs = nospurs.split_skeleton(distance_transform, max([(self.staffspace_height+1)/2, 5]))
        timestat.append(["branch computation", time.time()-t])

        #
        # 2) filter staff segment candidates
        #
        if debug > 0:
            self.logmsg("filter staff segment candidates\n")
        t = time.time()
        candidates = []
        nocandidates = []
        maxstraightness = max([0.125 * self.staffline_height**2, 3])
        distmax = self.staffline_height
        distmin = max([self.staffline_height/4,1])
        for s in allsegs:
            if s.nrows < s.ncols and s.orientation_angle != None and \
               abs(s.orientation_angle) < 25 and \
               s.straightness < maxstraightness and \
               distance_transform.distance_precentage_among_points(s.points,distmin,distmax) > 0.7:
                candidates.append(s)
            else:
                nocandidates.append(s)
        timestat.append(["prefilter staffsegs", time.time()-t])
        if debug > 1:
            rgb = self.image.to_rgb()
            rgb.highlight(self.skelimage,RGBPixel(120,120,120))
            for s in candidates:
                for p in s.points:
                    rgb.set((p.x, p.y), RGBPixel(255, 0, 255))
            for s in nocandidates:
                for p in s.points:
                    rgb.set((p.x, p.y), RGBPixel(0, 0, 255))
            rgb.save_PNG("skeldebug-allsegs.png")

        #
        # alternative methods for filtering staffline segments
        #
        t = time.time()
        #[staffsegs, nostaffsegs] = \
        #            self.__get_staffsegs_projections(candidates, num_lines)
        [staffsegs, nostaffsegs] = \
                    self.__get_staffsegs_graphccs(candidates, num_lines, horizontal_extrapolation)
        nocandidates += nostaffsegs
        timestat.append(["finding staff lines", time.time()-t])

        # remove points with identical x-value from staffsegs
        for s in staffsegs:
            toremove = []
            if not s.points: continue
            lastx = s.points[0].x - 1
            for i,p in enumerate(s.points):
                if p.x == lastx: toremove.append(i)
                lastx = p.x
            nremoved = 0
            for i in toremove:
                del s.points[i - nremoved]
                nremoved += 1

        if self.debug > 1:
            for s in staffsegs:
                for p in s.points:
                    rgb.set((p.x, p.y), RGBPixel(255, (150 * s.line) % 256, 0))
            rgb.save_PNG("skeldebug-staffsegs.png")

        #
        # remove overlapping segments (among overlapping segments
        # all but one must be false positives)
        # This also sorts segments by staff, line and offset_x
        #
        if debug > 0: self.logmsg("remove overlaps\n")
        t = time.time()
        [staffsegs, removed] = self.__remove_overlaps(staffsegs)
        nocandidates += removed
        timestat.append(["overlap removal",time.time()-t])

        if self.debug > 1:
            for s in removed:
                for p in s.points:
                    rgb.set(p, RGBPixel(0, 200, 0))
            rgb.save_PNG("skeldebug-overlaps.png")

        #
        # 6) remove false positives, i.e. alleged staff line
        #    segments which are possibly symbol parts
        #
        if self.false_positive_criterion!="none":
            if debug > 0: self.logmsg("remove false positives\n")
            t = time.time()
            if debug > 1:
                rgb2 = rgb.image_copy()
                [staffsegs, tested] = self.__remove_false_positives(staffsegs, nocandidates,rgb,rgb2)
            else:
                [staffsegs, tested] = self.__remove_false_positives(staffsegs, nocandidates,None,None)

            for seg in tested:
                if seg.nostaff and seg in staffsegs:
                    staffsegs.remove(seg)
                    nostaffsegs.append(seg)

            #nocandidates += removed
            timestat.append(["false positives removal",time.time()-t])
            if self.debug > 1:
                # begin DEBUG
                for s in nocandidates:
                    color = RGBPixel(0,0,255)
                    for p in s.points:
                        rgb.set((p.x, p.y), color)
                        rgb2.set((p.x, p.y), color)
                # end DEBUG
                rgb.save_PNG("skeldebug-extrapolation1.png")
                rgb2.save_PNG("skeldebug-extrapolation2.png")

        # just in case no staff system has been found
        if len(staffsegs) == 0:
            if self.debug:
                print "Warning: no staff system found => nothing removed"
            return

        # store staff positions in self.linelist as in StaffFinder:
        # sublists of StafflineSkeleton, each sublist representing
        # a staff system
        self.linelist = []
        staff = []; line = StafflineSkeleton()
        staffno = staffsegs[0].staff; lineno = staffsegs[0].line
        lastx = None; lasty = None
        for s in staffsegs:
            if not s.points: continue
            if s.staff != staffno or s.line != lineno:
                staff.append(line); lineno = s.line
                line = StafflineSkeleton(); lastx = None; lasty = None
            if s.staff != staffno:
                self.linelist.append(staff); staff = []; staffno = s.staff
            if lastx == None:
                line.left_x = s.points[0].x
            elif lastx + 1 < s.points[0].x:
                # interpolate
                dy = (s.points[0].y - lasty) * 1.0 / (s.points[0].x - lastx)
                y = lasty
                for i in range(s.points[0].x - lastx - 1):
                    y += dy; line.y_list.append(int(y))
            line.y_list += [p.y for p in s.points]
            lastx = s.points[-1].x; lasty = s.points[-1].y
            if self.debug > 1:
                if line.left_x + len(line.y_list) > self.image.ncols:
                    print "hoppla! leftx:", line.left_x, "y_list:", line.y_list[0]
                    x = line.left_x
                    for y in line.y_list:
                        rgb.set(Point(x,y),RGBPixel(0,200,0))
                        x += 1
                        if x >= self.image.ncols: break
                    rgb.save_PNG("skeldebug-errorline.png")
        # last line and staff
        staff.append(line); self.linelist.append(staff)
        if self.debug > 1:
            sf = StaffFinder(self.image, self.staffline_height, self.staffspace_height)
            sf.linelist = self.linelist
            rgb = sf.show_result()
            rgb.save_PNG("skeldebug-foundstaves.png")
        
        # remove vertical runs around staff segments
        for s in staffsegs:
            self.image.remove_vruns_around_points(s.points,2*self.staffline_height)
        # print final runtime statistics
        if self.debug > 0:
            total = 0.0
            for t in timestat:
                print ("%-18s %.4f sec" % (t[0], t[1]))
                total += t[1]
            print "--------------------------------------------------"
            print ("%-18s %.4f sec" % ("total", total))

    
    ######################################################################
    # (angle) __compute_skew_angle(SkeletonSegemnt_list)
    #
    # Computes the median skew angle. The orientation angle probability
    # density is built from the staff line segment candidates, such that
    # each segment obtains a weight proportional to its width.
    # The median of this distribution is assumed to be the document skew angle.
    #
    def __compute_skew_angle(self,segments):
        angle_pdf = {}
        for s in segments:
            if s.ncols < self.staffspace_height:
                continue
            angle = round(s.orientation_angle,4)
            if angle not in angle_pdf:
                angle_pdf[angle] = s.ncols
            else:
                angle_pdf[angle] += s.ncols
        keys = angle_pdf.keys()
        keys.sort()
        angle_cdf = []
        angle_sum = 0
        for k in keys:
            angle_sum += angle_pdf[k]
            angle_cdf.append([k, angle_sum])
        if len(angle_cdf) == 0:
            return 0
        angle_sum_2 = angle_sum * 0.5
        minangle = angle_cdf[0]
        maxangle = angle_cdf[-1]
        for a in angle_cdf:
            if a[1] <= angle_sum_2:
                minangle = a
            if a[1] >= angle_sum_2:
                maxangle = a
                angle = a[0]
                break
        if abs(minangle[1] - maxangle[1]) < 0.0001:
            angle = minangle[0]
        else:
            angle = minangle[0] + (maxangle[0]-minangle[0]) * \
                    (angle_sum_2 - minangle[1]) / (maxangle[1] - minangle[1])
        if self.debug > 1:
            print "skew angle detected as", angle, "degree"
        return angle

    

    ######################################################################
    # (staffsegs, nostaffsegs) __get_staffsegs_graphccs
    #
    # Returns all actual staff segments from a list of staff segments
    # candidates. For each returned segment the properties "staff" and
    # "line" are set. The algorithm works as follows:
    #
    # 1) A segment graph is built by linking segments horizontally and
    #    vertically. A vertical link is added between segments with a
    #    horizontal overlap and a vertical
    #    distance around staffline_height + staffspace_height. A horizontal
    #    link is added when the extrapolated (via a least square fitted line
    #    or the global skew angle) segments match.
    #
    # 2) Staff systems and lines are then detected as connected components
    #    in the segmant graph as in StaffFinder_miyao.
    #
    def __get_staffsegs_graphccs(self, candidates, num_lines, horizontal_extrapolation):
             
        #
        # 1) build graph of segments (unfortunately O(n^2))
        #
        if self.debug > 0:
            self.logmsg("link %d segments... " % len(candidates))
        self.skew_angle = self.__compute_skew_angle(candidates)
        for s in candidates:
            s.right = []; s.left = []; s.top = []; s.bot = []
            s.right_x = s.offset_x + s.ncols - 1
        # sort for better performance
        candidates.sort(lambda s1,s2: s1.offset_y - s2.offset_y)
        linedist = self.staffline_height + self.staffspace_height
        tolvlink = linedist * 0.15 # tolerance vertical link
        if tolvlink < 1.5:
            tolvlink = 1.5
        tolhlink = 0.5*self.staffline_height # tolerance horizontal link
        tana = tan(self.skew_angle * pi / 180)
        maxj = len(candidates) - 1
        for i,s1 in enumerate(candidates):
            miny = s1.offset_y - 2*linedist
            maxy = s1.offset_y + 2*linedist
            # walk upwards until miny
            j = i
            while j > 0:
                j -= 1; s2 = candidates[j]
                # avoid unneccessary computations
                if s2.offset_y < miny: break
                self.__link_segments(s1,s2,linedist,tolvlink,tolhlink,tana,horizontal_extrapolation)
            j = i
            # walk downwards until maxy
            while j < maxj:
                j += 1; s2 = candidates[j]
                # avoid unneccessary computations
                if s2.offset_y > maxy: break
                self.__link_segments(s1,s2,linedist,tolvlink,tolhlink,tana,horizontal_extrapolation)
                            
        if self.debug > 1:
            rgb = self.image.to_rgb()
            for c in candidates:
                for p in c.points:
                    rgb.set((p.x, p.y), RGBPixel(255, 0, 0))
                for cr in c.right:
                    rgb.draw_line(c.points[-1],cr.points[0],RGBPixel(0,255,0))
                for ct in c.top:
                    width = min([ct.right_x,c.right_x]) - \
                            max([ct.offset_x,c.offset_x])
                    if ct.offset_x < c.offset_x:
                        p1 = c.points[width/2]
                        p2 = ct.points[(width/2)+c.offset_x-ct.offset_x]
                    else:
                        p1 = ct.points[width/2]
                        p2 = c.points[(width/2)+ct.offset_x-c.offset_x]
                    rgb.draw_line(p1,p2,RGBPixel(0,255,0))
            rgb.save_PNG("skeldebug-links.png")

        #
        # 5) do a cc analysis and assign staff and line numbers
        #
        if self.debug > 0:
            self.logmsg("assign staff and line numbers... ")

        # traverse CCs for line/staff labeling and store CCs in structure
        segmentccs = []; addedsegs = 0; traversed = 0
        maxwidth = 0; stafflabel = -1
        for s in candidates:
            if not hasattr(s,"line"): # new CC found
                # do a breadth first search for labeling all nodes
                stafflabel += 1
                s.staff = stafflabel
                s.line = 0
                newcc = SegmentCC()
                neighbors = [s]
                while len(neighbors) > 0:
                    nneighbors = []
                    for cs in neighbors:
                        traversed += 1
                        newcc.append_seg(cs)
                        for left in cs.left:
                            if not hasattr(left,"line"):
                                left.staff = stafflabel
                                left.line = cs.line; nneighbors.append(left)
                        for right in cs.right:
                            if not hasattr(right,"line"):
                                right.staff = stafflabel
                                right.line = cs.line; nneighbors.append(right)
                        for top in cs.top:
                            if not hasattr(top,"line"):
                                top.staff = stafflabel
                                top.line = cs.line-1; nneighbors.append(top)
                        for bot in cs.bot:
                            if not hasattr(bot,"line"):
                                bot.staff = stafflabel
                                bot.line = cs.line+1; nneighbors.append(bot)
                    neighbors = nneighbors
                segmentccs.append(newcc)
                addedsegs += len(newcc.segments)
                if maxwidth < newcc.botright.x - newcc.topleft.x:
                    maxwidth = newcc.botright.x - newcc.topleft.x

        #
        # 5) remove CCs that are too small or overlap with other wider CCs
        #
        if self.debug > 0:
            self.logmsg("filter %d segment CCs\n" % len(segmentccs))
        # prefilter too small segments
        staffccs = []
        nostaffsegs = []
        if num_lines == 0:
            minheight = 1.5*linedist
        else:
            minheight = (num_lines-1.3)*linedist
        for cc in segmentccs:
            #print "topleft:", cc.topleft, "botright:", cc.botright
            ncols = cc.botright.x - cc.topleft.x
            nrows = cc.botright.y - cc.topleft.y
            if ncols > maxwidth / 3 and nrows > minheight:
                staffccs.append(cc)
            else:
                nostaffsegs += cc.segments
        # TODO: check for serious overlap (not necessary for test images)

        #
        # 6) remove staff lines based upon num_lines
        #
        # build up list of points per staff/line
        stafflinepoints = {}
        for cc in staffccs:
            linepoints = {}
            for s in cc.segments:
                if linepoints.has_key(s.line):
                    linepoints[s.line] += s.ncols
                else:
                    linepoints[s.line] = s.ncols
            stafflinepoints[cc.segments[0].staff] = linepoints
        # autoguess num_lines from histogram of lines per staff; for
        # building histogram only keep lines above a certain number of points
        if num_lines == 0:
            linenumhistogram = {} # lines_per_staff => how_often
            for staffno, linepoints in stafflinepoints.iteritems():
                threshold = max(linepoints.values()) / 2
                n = len([p for p in linepoints.values() if p > threshold])
                if linenumhistogram.has_key(n):
                    linenumhistogram[n] += 1
                else:
                    linenumhistogram[n] = 1
            maxoften = 0
            for lines, howoften in linenumhistogram.iteritems():
                if howoften > maxoften:
                    maxoften = howoften; num_lines = lines
            self.logmsg("Lines per system (num_lines) guessed as %d\n" % num_lines)
        # pick num_lines lines around line with most points
        linestokeep = {} # assume not more than 100 staffs per page
                         # and store staff*100+line
        for staffno, linepoints in stafflinepoints.iteritems():
            maxpoints = 0; reflineno = 0
            minlineno = min(linepoints.keys())
            maxlineno = max(linepoints.keys())
            if maxlineno - minlineno + 1 < num_lines:
                continue # skip staffs with too few lines
            for lineno, points in linepoints.iteritems():
                if points > maxpoints:
                    maxpoints = points; reflineno = lineno
            # start at maximum and move in both directions,
            # always picking the larger value
            linestokeep[staffno*100+reflineno] = 1
            n = 1; top = reflineno - 1; bot = reflineno + 1
            while n < num_lines:
                n += 1
                if linepoints.has_key(top):
                    toppoints = linepoints[top]
                else:
                    toppoints = 0
                if linepoints.has_key(bot):
                    botpoints = linepoints[bot]
                else:
                    botpoints = 0
                if toppoints > botpoints:
                    linestokeep[staffno*100+top] = 1; top -= 1
                else:
                    linestokeep[staffno*100+bot] = 1; bot += 1
        # remove all marked lines
        for cc in staffccs:
            staffno = cc.segments[0].staff
            newsegments = []
            for s in cc.segments:
                if linestokeep.has_key(100*s.staff+s.line):
                    newsegments.append(s)
                else:
                    nostaffsegs.append(s)
            cc.segments = newsegments

        # recount staff and line numbers
        staffccs.sort(lambda s1,s2: s1.topleft.x - s2.topleft.x)
        for i,cc in enumerate(staffccs):
            cc.staff = i
            linediff = abs(cc.minline)
            if linediff > 0:
                for s in cc.segments:
                    s.line += linediff
        
        if self.debug > 0:
            self.logmsg("%d staff systems found\n" % len(staffccs))

        staffsegs = []
        for cc in staffccs: staffsegs += cc.segments
        return [staffsegs, nostaffsegs]


    ######################################################################
    # __link_segments(seg1,seg2,linedist,tolvlink,tolhlink,tana)
    #
    # adds a vertical or horizontal link between the two given segments
    # if certain conditions are matched
    #
    def __link_segments(self,s1,s2,linedist,tolvlink,tolhlink,tana,horizontal_extrapolation):
        # vertical links
        left = max([s1.offset_x,s2.offset_x])
        right = min([s1.right_x,s2.right_x])
        hoverlap = right - left
        if hoverlap > 0:
            p1 = s1.points[left - s1.offset_x + hoverlap / 2]
            p2 = s2.points[left - s2.offset_x + hoverlap / 2]
            if (p1.y > p2.y) and \
               (abs(p1.y - p2.y - linedist) < tolvlink):
                s1.top.append(s2)
                s2.bot.append(s1)
        # horizontal links
        elif hoverlap > - 1.5*linedist:
            if s1.offset_x < s2.offset_x:
                p1 = s1.points[-1]; p2 = s2.points[0]
                if horizontal_extrapolation == "skew_angle":
                    if abs(p1.y-p2.y+tana*(p2.x-p1.x)) < tolhlink:
                        s1.right.append(s2)
                        s2.left.append(s1)
                else:
                    if abs(p1.y-p2.y+tan(s1.orientation_angle*0.017453)*(p2.x-p1.x)) < tolhlink \
                       or abs(p1.y-p2.y+tan(s2.orientation_angle*0.017453)*(p2.x-p1.x)) < tolhlink:
                        s1.right.append(s2)
                        s2.left.append(s1)

    ######################################################################
    # (kept, removed) __remove_overlaps(staffsegs)
    #
    # removes horizontally overlapping staff segments and keeps only those
    # which are closest to the least square fitted line.
    # Each segment must have set the staff, line and right_x property!
    #
    # The returned segments are sorted by staff, line and offset_x
    #
    def __remove_overlaps(self,staffsegs):
        if len(staffsegs) == 0:
            return [staffsegs,[]]
        removed = []
        kept = []
        # sort by staff, line and xpos
        def cmpsegs(s1,s2):
            diff = s1.staff - s2.staff
            if diff != 0: return diff
            diff = s1.line - s2.line
            if diff != 0: return diff
            return s1.offset_x - s2.offset_x
        staffsegs.sort(cmpsegs)
        # set property right_x if not yet set
        if not hasattr(staffsegs[0],"right_x"):
            for s in staffsegs:
                s.right_x = s.offset_x + s.ncols - 1
        # build sublists per line
        staffno = staffsegs[0].staff; lineno = staffsegs[0].line
        linelist = []; linesegs = []
        for s in staffsegs:
            if staffno != s.staff or lineno != s.line:
                linelist.append(linesegs)
                linesegs = [s]
                staffno = s.staff; lineno = s.line
            else:
                linesegs.append(s)
        linelist.append(linesegs)
        # check overlap
        for line in linelist:
            linelen = len(line)
            i = 0; j = 0
            while (i<linelen):
                # is next segment overlapping?
                s = line[i]
                if j <= i:
                    j = i+1
                else:
                    j = j+1
                if not (j < linelen and \
                        min([s.right_x,line[j].right_x]) -\
                        max([s.offset_x,line[j].offset_x]) >= 0):
                    # no overlap found
                    kept.append(s)
                    i = max(i+1, j)
                else:
                    # overlap detected
                    # compute least square fit
                    leftx = s.offset_x
                    minx = leftx - self.staffspace_height * 2
                    rightx = max([s.right_x, line[j].right_x])
                    maxx = rightx + self.staffspace_height * 2
                    points = []
                    k = i - 1
                    while k >= 0:
                        if line[k].right_x > minx:
                            points += line[k].points
                        else:
                            break
                        k -= 1
                    k = j + 1
                    while k < linelen:
                        if line[k].offset_x < maxx:
                            points += line[k].points
                        else:
                            break
                        k += 1
                    [m,b,q] = least_squares_fit(points)
                    distances = []
                    for k in [i,j]:
                        # simplification (for performance reasons):
                        # only check midpoint of segment
                        p = line[k].points[line[k].ncols/2]
                        distances.append(abs(p.y-(m*p.x+b)))
                    if distances[0] < distances[1]:
                        removed.append(line[j])
                        pass # keep i segment
                    else:
                        removed.append(s)
                        i = j
        #for s in kept:
        #    print "line:", s.line, "points:", [(p.x,p.y) for p in s.points]
        return [kept, removed]

    ######################################################################
    # (kept, removed) __remove_false_positives(staffsegs,nostaffsegs)
    #
    # removes short staffsegments which are tangential to non staffsegments
    #
    def __remove_false_positives(self,staffsegs,nostaffsegs,debugimage,debugimage2):
        #removed = []; kept = []
        # build up connectivity information
        branchingpoints = {} # point -> nostaffsegsegments
        for s in nostaffsegs:
            #print "nostaff Segment:", s.points[0], s.points[-1]
            # begin DEBUG
            #if (s.points[0].x == 76) and (s.points[0].y == 62):
            #    print "nostaff branchingpoints:",
            #    for b in s.branching_points: print b,
            #    print ""
            # end DEBUG
            if s.orientation_angle == None: continue
            for p in s.branching_points:
                if branchingpoints.has_key(p):
                    branchingpoints[p].append(s)
                else:
                    branchingpoints[p] = [s]
        # find links
        tested = [] # for debugging: list of tested segmants
        maxwidth = 1.5*self.staffspace_height
        for s in staffsegs:
            s.nostaff = False
            if s.ncols > maxwidth: continue
            # begin DEBUG
            #if (s.points[-1].x == 223) and (s.points[-1].y == 185):
            #    print "staff branchingpoints:",
            #    for b in s.branching_points: print b,
            #    print ""
            #    self.criticalsegment = True
            #else:
            #    self.criticalsegment = False
            # end DEBUG
            segmentwastested = False
            for p in s.branching_points:
                if branchingpoints.has_key(p):
                    segs4test = self.__remove_segs_on_staffline(s,branchingpoints[p],p)
                    if len(segs4test):
                        segmentwastested = True
                        if self.__is_collinear_parabola(s,segs4test,p,debugimage):
                            s.nostaff = True; break
            if segmentwastested:
                tested.append(s)
                if self.debug > 1:
                    # mark segments checked during first extrapolation
                    if s.nostaff: color = RGBPixel(0,130,0)
                    else: color = RGBPixel(130,255,255)
                    for p in s.points:
                        debugimage.set((p.x, p.y), color)

            if hasattr(s,"nostaff") and not s.nostaff:
                segmentwastested = False
                if len(s.branching_points)>1:
                    br1=[]
                    br2=[]
                    for br in s.branching_points:
                        if branchingpoints.has_key(br):
                            if abs(br.x-s.branching_points[0].x)<s.ncols:
                                br1.extend(branchingpoints[br])
                            else:
                                br2.extend(branchingpoints[br])
                    for s1 in br1:
                        for s2 in br2:
                            segmentwastested = True
                            if self.__nostaffsegs_collinear(s1,s2,s,debugimage2):
                                s.nostaff = True
                                break
                        if s.nostaff:
                            break
                if self.debug > 1 and segmentwastested:
                    # mark segments checked during second extrapolation
                    if s.nostaff: color = RGBPixel(0,130,0)
                    else: color = RGBPixel(130,255,255)
                    for p in s.points:
                        debugimage2.set((p.x, p.y), color)

        return [staffsegs, tested]

    ######################################################################
    # bool __nostaffsegs_collinear(segment1, segment2, staffseg)
    #
    # tests whether segment1 and segment2 possibly belong to the same line.
    # criteria: a) their extrapolated ends are roughly collinear
    #           b) the extrapolations come close enough in the middle
    #
    def __nostaffsegs_collinear(self,s1,s2,s,debugimage2):
        npts=5
        #allowedangle=0.174533              # 10*pi/180=0.174533
        #allowedangle=0.35              # 20*pi/180=0.174533
        allowedangle=0.5                # 28.65 degrees
        #allowedmiddist=self.staffspace_height/2
        allowedmiddist=self.staffline_height*2

        if s1==s2:
            return False

        def quaddist(p1,p2):
            dx=p1.x-p2.x
            dy=p1.y-p2.y
            return dx*dx+dy*dy

        # enough points for extrapolation?
        if len(s1.points)<npts or len(s2.points)<npts:
            return False

        # find closest edge points
        p11=s1.points[0]
        p12=s1.points[-1]
        p21=s2.points[0]
        p22=s2.points[-1]
        disttab=[quaddist(p11,p21),
                 quaddist(p11,p22),
                 quaddist(p12,p21),
                 quaddist(p12,p22)]
        mindist=min(disttab)
        mindistidx=disttab.index(mindist)

        if mindistidx==0 or mindistidx==1:
            p1=p11
            ms1=s1.points[0:npts]
        else:
            p1=p12
            ms1=s1.points[-npts:]

        if mindistidx==0 or mindistidx==2:
            p2=p21
            ms2=s2.points[0:npts]
        else:
            p2=p22
            ms2=s2.points[-npts:]

        # least square extrapolation
        s1mbx = least_squares_fit_xy(ms1)
        s2mbx = least_squares_fit_xy(ms2)

        # first condition: the extrapolated lines come close
        #                   enough in the middle
        midy = (p1.y + p2.y) / 2
        # we need to distinguish y = mx + b versus x = my + b
        if s1mbx[3]:
            s1x = s1mbx[0] * midy + s1mbx[1]
        else:
            if abs(s1mbx[0]) < 0.0001:
                return False
            s1x = (midy - s1mbx[1]) / s1mbx[0]
        if s2mbx[3]:
            s2x = s2mbx[0] * midy + s2mbx[1]
        else:
            if abs(s2mbx[0]) < 0.0001:
                return False
            s2x = (midy - s2mbx[1]) / s2mbx[0]

        # draw extrapolation if desired
        if self.debug > 1:
            debugimage2.draw_line(p1,Point(int(s1x),midy),RGBPixel(255,128,128))
            debugimage2.draw_line(p2,Point(int(s2x),midy),RGBPixel(255,128,128))
            #print "extrapolated from", p1, p2

        middist = abs(s1x - s2x)
        if middist > allowedmiddist or middist > s.ncols:
            return False

        # second condition: lest square fit extrapolated lines are
        #                  nearly parallel
        if s1mbx[3]:
            x1 = FloatPoint(s1mbx[0],1)
        else:
            x1 = FloatPoint(1,s1mbx[0])
        if s2mbx[3]:
            x2 = FloatPoint(s2mbx[0],1)
        else:
            x2 = FloatPoint(1,s2mbx[0])
        v = abs(x1.x*x2.x + x1.y*x2.y) / \
            (sqrt(x1.x**2+x1.y**2) * sqrt(x2.x**2+x2.y**2))
        if v >  1.0: v =  1.0
        if v < -1.0: v = -1.0
        alpha = acos( v )
        if abs(alpha)>allowedangle:
            return False

        return True

    
    ######################################################################
    # segs4test = __remove_segs_on_staffline(staffseg, nostaffsegs, branchpoint)
    #
    # returns only those segments from nostaffsegs which are probably
    # not on the same staff line as staffseg
    #
    def __remove_segs_on_staffline(self, staffseg, nostaffsegs, branchpoint):
        # find closest point to branchpoint
        pa = staffseg.points[0]; pe = staffseg.points[-1]
        if abs(pa.x - branchpoint.x) < abs(pe.x - branchpoint.x):
            y_staffseg = pa.y
        else:
            y_staffseg = pe.y
        #y_staffseg = staffseg.points[len(staffseg.points)/2].y
        segs4test = []
        for s in nostaffsegs:
            if abs(s.offset_y - y_staffseg) > self.staffline_height+1 or \
               abs(s.offset_y + s.nrows - y_staffseg) > self.staffline_height+1:
                segs4test.append(s)
        return segs4test

    ######################################################################
    # bool __is_collinear_parabola(segment1, segmentlist, branchpoint, img)
    #
    # tests whether segment1 is collinear with any of the segments
    # in segmentlist; the segments are joind through the branchpoint
    #
    # this is done by forming x(t) and y(t) (parabola) functions from
    # the points of each segment in segmentlist and of segment1 and then
    # testing the angle of the tangents
    #
    def __is_collinear_parabola(self, s1, slist, branchpoint,im):

        # returns the *cnt* points of the end point closer to p
        def closest_end_points(seg,p,cnt):
            p0 = seg.points[0]; p1 = seg.points[-1]
            if (p0.x-p.x)**2 + (p0.y-p.y)**2 < (p1.x-p.x)**2 + (p1.y-p.y)**2:
                retp=seg.points[:cnt]
                retp.reverse()
                return retp
            else:
                return seg.points[-cnt:]

        # returns true, if the left endpoint of seg is closer to p
        # than the right endpoint
        def is_left_end(seg,p):
            p0 = seg.points[0]; p1 = seg.points[-1]
            if (p0.x-p.x)**2 + (p0.y-p.y)**2 < (p1.x-p.x)**2 + (p1.y-p.y)**2:
                return p0.x<p1.x
            else:
                return p1.x<p0.x

        # returns a sorted list of t values, which correspond to the
        # given x value (v). call with is_y=True, to use the y value
        def t_from_v(par,v,is_y=False):
            pofs=0
            if is_y: pofs=1
            if par[0+pofs]==0:
                if par[2+pofs]==0:
                    return []
                return [(v-par[4+pofs])/par[2+pofs]]
            fr=par[2+pofs]/(2*par[0+pofs])
            r=(fr)**2-(par[4+pofs]-v)/par[0+pofs]
            if r<0:
                return []
            else:
                if r>0:
                    rt=sqrt(r)
                    return [-fr-rt,-fr+rt]
                else:
                    return [-fr]

        def x_from_t(par,t):
            return par[0]*t*t+par[2]*t+par[4]

        def y_from_t(par,t):
            return par[1]*t*t+par[3]*t+par[5]

        if len(s1.points)<3: return False

        b = branchpoint
        n = self.staffspace_height
        p = closest_end_points(s1,branchpoint,1)[0]

        # are we looking on the left end of the staffline?
        left=is_left_end(s1,b)

        for s2 in slist:
            if len(s2.points)<3:
                continue

            if abs(s2.orientation_angle)>80 and abs(s2.orientation_angle)<100 and s2.straightness<0.1*self.staffline_height**2:
                continue

            # calculate x(t) and y(t) coefficients
            s2endpts = closest_end_points(s2,branchpoint,n)
            if self.false_positive_criterion=="quad_angle":
                [s2endpar,s2tlast] = parabola(s2endpts)
            else:
                [s2endpar,s2tlast] = lin_parabola(s2endpts)

            dx=branchpoint.x-s2endpts[-1].x
            dy=branchpoint.y-s2endpts[-1].y
            if sqrt(dx*dx+dy*dy)>2*self.staffline_height:
                continue

            if self.debug > 1:
                pp=s2endpts[-1]
                tt=s2tlast
                for i in range(20):
                    [pp,tt,delta]=estimate_next_point(pp,s2endpar,tt)
                    if pp.x<0 or pp.y<0 or pp.x>=im.width or pp.y>=im.height:
                        continue
                    im.set(pp,RGBPixel(255,128,128))

            # get the t values for the points to watch at
            for s2t in t_from_v(s2endpar,p.x):

                # point must be beyond the last point
                if s2t<s2tlast:
                    continue

                # calculate tangent
                s2tandx=2*s2endpar[0]*s2t+s2endpar[2]
                s2tandy=2*s2endpar[1]*s2t+s2endpar[3]

                # calculate angle
                quot=s2tandx/sqrt(s2tandx**2+s2tandy**2)
                angle=acos(quot)*180/pi
                # we want to have the full 360 degrees to enable us to subtract the skew_angle
                if s2tandy<0: angle=360-angle
                angle=(angle-self.skew_angle)%360
                if left:
                    # an angle near 0 or 360 is good:
                    dangle=180-abs(angle-180)
                else:
                    # an angle near 180 is good, so we use:
                    dangle=abs(angle-180)

                if not (self.false_positive_criterion=="quad_angle" and dangle>25):
                    # point must be closer than the last point
                    if abs(p.y-y_from_t(s2endpar,s2t))>=abs(p.y-s2endpts[-1].y):
                        continue

                # point must be closer than 2*staffline_height
                if abs(p.y-y_from_t(s2endpar,s2t))>=2*self.staffline_height:
                    continue

                if self.false_positive_criterion=="lin_meets":
                    for s2t2 in t_from_v(s2endpar,p.y,True):

                        # point must be beyond the last point
                        if s2t<s2tlast:
                            continue

                        # point must be on the correct side
                        if left:
                            if(x_from_t(s2endpar,s2t2)>p.x):
                                return True
                        else:
                            if(x_from_t(s2endpar,s2t2)<p.x):
                                return True


                else:
                    if dangle<25:
                        return True

        return False

    ######################################################################
    # (staffsegs, nostaffsegs) __get_staffsegs_projections
    #
    #  Returns all actual staff segments from a list of staff segment
    #  candidates. For each returned segment the properties "staff" and
    # "line" are set. The algorithm works as follows:
    #
    # 1) From the image containing only the staff line skeleton segment candidates
    #    staff lines are detected with `StaffFinder_projections`__; the previously
    #    computed skew angle is hereby taken into account.
  
    # .. __: gamera.toolkits.musicstaves.stafffinder_projections.StaffFinder_projections.html
    #
    # 2) Staff segment candidates close enough to any of the detected staff lines
    #    are assumed to belong to this staff line. This typically results in
    #    some inadvertently detected \"staff line segments\" which actually
    #    belong to symbols (\"false positives\").
    #
    # 3) TODO: cleanup among false positives.
    #
    def __get_staffsegs_projections(self, candidates, num_lines):

        #
        # 1) find stafflines via projections
        #
        if self.debug > 0:
            self.logmsg("find stafflines with projections\n")

        class StaffLine:
            def __init__(self, y, staffnr, linenr):
                self.y=y; self.staffnr=staffnr; self.linenr=linenr

        angle = self.__compute_skew_angle(candidates)
        cosa = cos(- angle * pi / 180); sina = sin(- angle * pi / 180)
        nrows = self.skelimage.nrows; ncols = self.skelimage.ncols
        candidate_image = Image((0, 0), Dim(ncols, nrows), ONEBIT)
        for c in candidates:
            for p in c.points:
                x = int(p.x*cosa - p.y*sina + 0.5)
                y = int(p.x*sina + p.y*cosa + 0.5)
                if (x >= 0 and x < ncols and y >= 0 and y < nrows):
                    candidate_image.set((x, y), 1)

        sf = StaffFinder_projections(candidate_image, staffline_height=self.staffline_height, staffspace_height=self.staffspace_height)
        sf.find_staves(num_lines=num_lines,preprocessing=0,peakthreshold=0.1)
        linelist = []
        for staffnr,staff in enumerate(sf.get_average()):
            for linenr,line in enumerate(staff):
                linelist.append(StaffLine(line.average_y,staffnr,linenr))

        if self.debug > 1:
            rgb = candidate_image.to_rgb()
            for staff in sf.get_average():
               for line in staff:
                   lx = line.left_x; rx = line.right_x; y = line.average_y
                   rgb.draw_line(Point(lx,y), Point(rx,y), RGBPixel(0,255,0))
            rgb.save_PNG("skeldebug-rotstaffs.png")

        #
        # 2) assign staff segment candidates to stafflines
        #
        t = time.time()
        tolerance = max([self.staffline_height * 2, self.staffspace_height/3])
        #tolerance = max([self.staffline_height, 3])
        staffsegs = []
        nostaffsegs = []
        for c in candidates:
            foundline = None
            n = 0
            for p in c.points:
                py = int(p.x*sina + p.y*cosa + 0.5)
                if not foundline:
                    for line in linelist:
                        if abs(py - line.y) <= tolerance:
                            foundline = line
                            n += 1
                            break
                else:
                    if abs(py - foundline.y) <= tolerance:
                        n += 1
            # a minimum pixel percentage must lie around the staffline
            if (n * 1.0 / len(c.points) >= 0.66):
                c.staff = foundline.staffnr
                c.line = foundline.linenr
                staffsegs.append(c)
            else:
                nostaffsegs.append(c)

        #
        # 3) TODO: only keep best fitting segments
        #

        return [staffsegs, nostaffsegs]


    ######################################################################
    # get_staffpos
    #
    # Christoph Dalitz
    #
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


    ######################################################################
    # some functions for creating debugging information
    def logmsg(self, msg):
        stdout.write(msg)
        stdout.flush()

######################################################################
# Class for storing a connceted component of SkeletonSegments
#
class SegmentCC:
    def __init__(self):
        self.topleft = None
        self.botright = Point(0,0)
        self.segments = []
        self.minline = None; self.maxline = None
    def append_seg(self, seg):
        self.segments.append(seg)
        if self.topleft:
            self.topleft = Point(min([self.topleft.x, seg.offset_x]),\
                                 min([self.topleft.y, seg.offset_y]))
        else:
            self.topleft = Point(seg.offset_x,seg.offset_y)
        self.botright = Point(max([self.botright.x, seg.offset_x+seg.ncols]),\
                         max([self.botright.y, seg.offset_y+seg.nrows]))
        if self.minline == None or self.minline > seg.line:
            self.minline = seg.line
        if self.maxline == None or self.maxline < seg.line:
            self.maxline = seg.line


