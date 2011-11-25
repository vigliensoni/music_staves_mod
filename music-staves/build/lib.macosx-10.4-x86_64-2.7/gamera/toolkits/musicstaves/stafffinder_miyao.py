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

from sets import Set
from math import atan, atan2, tan, pi
from sys import stdout, exit
from time import time

from gamera.gui import has_gui
from gamera import toolkit
from gamera.args import *
from gamera.args import Point as PointArg # is also defined in gamera.core
from gamera.core import *
from stafffinder import StaffFinder as _virtual_StaffFinder
from stafffinder import StafflineAverage, StafflineSkeleton, StafflinePolygon
from equivalence_grouper import EquivalenceGrouper
#from gamera.toolkits.musicstaves.plugins import *

##############################################################################
#
#
class StaffFinder_miyao(_virtual_StaffFinder):
    """This staff finding algorithm is a based on a paper by H. Miyao
(see below).

The method as described by Miyao:

1) Find stave candidate points on equally spaced vertical scan lines
2) Use Dynamic Programming Matching to link stave candidate points
3) Link points vertically and identify stave groups as connected components
4) Adjust the edge point positions

Deviations from Miyao's method by Dalitz:

- The mismatch penalty in the Dynamic Programming Matching is set to
  three instead of two
- The number of lines per staff system is automatically guessed as
  the most frequent vertical link runlength
- Points on the same vertical scanline with the same staff/line label are
  melted into a single point

Limitations:

- Cannot deal with a varying number of lines per staff as in mixed
  music and tablature prints

References:

- Hidetoshi Miyao: *Stave Extraction for Printed Music Scores*. in
  H. Yin et al. (Eds.): IDEAL 2002, LNCS 2412, pp. 562-568, Springer (2002)

- Hidetoshi Miyao, Masayuki Okamoto: *Stave Extraction for Printed Music
  Scores Using DP Matching*. Journal of Advanced Computational Intelligence
  and Intelligent Informatics, Vol. 8 No. 2 (2004)

- Gina M. Cannarozzi: `String Alignment using Dynamic Programming`__

.. __: http://cbrg.inf.ethz.ch/bio-recipes/DynProgBasic/code.html

Note that the first two references are almost identical, with the second
somewhat more detailed.

:Author: Christoph Dalitz (after an algorithm by H. Miyao)
"""
    def __init__(self, img, staffline_height=0, staffspace_height=0):
        _virtual_StaffFinder.__init__(self, img,\
                staffline_height, staffspace_height)
        self.find_staves_args = Args([Int("num_lines", default=0),
                                      Int("scanlines", default=20),
                                      Float("blackness", range=(0,1), default=0.8),
                                      Int("tolerance", default=-1)
                                      ])


    ######################################################################
    # returns index in list with maximum value
    #
    def __max_ind(self, seq):
        maxval = seq[0]
        maxind = 0
        for i,s in enumerate(seq):
            if s > maxval:
                maxval = s
                maxind = i
        return maxind

    ######################################################################
    # computes Miyao's minimum distance matrix
    #  Input:
    #    cp1/2 - two candidate point lists as returned by get_candidate_points
    #  Output:
    #    the distance matrix g
    #
    def __compute_matrix(self, cp1, cp2, blackness, prev_angle):

        class matrix_point:
            def __init__(self):
                self.value = -1
                self.distance = -1  # distance to g[i-1,j-1]

        col1 = cp1.x; col2 = cp2.x
        I = len(cp1.anchorlist)+1; J = len(cp2.anchorlist)+1

        # initalize matrix
        g = [[matrix_point() for j in range(J)] for i in range(I)]
        g[0][0].value = 0
        for i in range(I-1):
            g[i+1][0].value = i+1
        for j in range(J-1):
            g[0][j+1].value = j+1

        # fill matrix successively
        for i in range(1,I):
            for j in range(1,J):
                try:
                    distance = self.image.miyao_distance(col1, cp1.anchorlist[i-1].y,\
                                               col2, cp2.anchorlist[j-1].y,\
                                               blackness=blackness,\
                                               pr_angle=prev_angle)
                except:
                    print "Error with anchorpoints", col1, cp1.anchorlist[i-1].y, col2, cp2.anchorlist[j-1].y
                    raise RuntimeError, "Error"
                g[i][j].value = min([ g[i-1][j-1].value + distance,\
                                     g[i-1][j].value + 1, g[i][j-1].value + 1 ])
                g[i][j].distance = distance
        return g



    ######################################################################
    # find_staves
    #
    # Christoph Dalitz
    #
    def find_staves(self, num_lines=0, scanlines=20, blackness=0.8, tolerance=-1, debug=0):
        """Finds staff lines from a music/tablature image with Miyao's
method.

Signature:

  ``find_staves(num_lines=0, scanlines=20, blackness=0.8, tolerance=-1, debug=0)``

with

  *num_lines*:
    Number of lines within one staff. When zero, the number is automatically
    detected.

  *scan_lines*:
    Number of vertical scan lines for extracting candidate points.

  *blackness*:
    Required blackness on the connection line between two candidate points
    in order to consider them matching.

  *tolerance*:
    How much the vertical black runlength of a candidate point may deviate
    from staffline_height. When negative, this value is set to
    *max([2, staffline_height / 4])*.

  *debug*:
    When > 0, information about each step is printed to stdout (flushed
    immediately). When > 1, runtime information is printed. When > 2, the
    information about some steps is more detailed. When > 3,
    a number of images is written into the current working directory;
    the image names have the prefix *miyaodebug*.
"""

        # some functions for debugging
        def logmsg(msg):
            stdout.write(msg)
            stdout.flush()
        def getcplistimage(cplist):
            size = max([self.staffline_height*2, self.staffspace_height/3])
            rgb = self.image.to_rgb()
            col1 = None
            for scanline in cplist:
                col2 = scanline.x
                for a in scanline.anchorlist:
                    if col1 != None:
                        for l in a.left:
                            rgb.draw_line((col1, l.y), (col2, a.y), RGBPixel(0,255,0))
                    rgb.draw_marker((col2, a.y), size, 1, RGBPixel(255,0,0))
                    if a.top:
                        rgb.draw_line((col2, a.y), (col2, a.top.y), RGBPixel(0,0,255))
                col1 = col2
            return rgb
        def drawcplistimage(filename, cplist):
            rgb = getcplistimage(cplist)
            logmsg("writing " + filename + "\n")
            rgb.save_PNG(filename)
        def drawcplistlabels(filename, cplist):
            size = max([self.staffline_height*2, self.staffspace_height/3])
            rgb = self.image.to_rgb()
            for line in cplist:
                for ap in line.anchorlist:
                    rgb.draw_marker((line.x, ap.y), size, 3, \
                                    RGBPixel((111*ap.staff) % 256,\
                                             (31*(ap.line+1)) % 256,\
                                             (201*(ap.staff+1)) % 256))
            logmsg("writing " + filename + "\n")
            rgb.save_PNG(filename)
            return rgb

            


        # for profiling
        runtimes = []

        #
        # Step 1: Extraction of Candidate Points
        #----------------------------------------------------------------
        if tolerance < 0:
            tolerance = max([2, self.staffline_height / 4])
        if debug:
            logmsg("find candidate points (scanline_count=%d, tolerance=%d)...\n" \
                  % (scanlines, tolerance))
        t = time()
        cp = self.image.miyao_candidate_points(scanline_count=scanlines,\
                                             tolerance=tolerance)
        runtimes.append(["candidate points:", time() - t])

        # build up anchor points data structure
        cplist = [ vertical_anchorline(c[0], c[1:]) for c in cp if len(c) > 1 ]
        for c in cp:
            print c
        #
        # Step 2: Connection with DP Matching
        #----------------------------------------------------------------

        if debug:
            logmsg("connect with DP matching...\n")
        t = time()

        # previous average link angle (pi means unknown)
        prev_angle = pi

        for n in range(len(cplist)-1):
            cp1 = cplist[n]; cp2 = cplist[n+1]
            g = self.__compute_matrix(cp1, cp2, blackness, prev_angle)
            I = len(cp1.anchorlist)+1; J = len(cp2.anchorlist)+1
            sumangle = 0
            nangle = 0

            # backtrace matches
            col1 = cp1.x; col2 = cp2.x
            i = I - 1; j = J - 1
            while (i>0) and (j>0):
                if g[i][j].value - g[i][j].distance == g[i-1][j-1].value:
                    # link found
                    left = cp1.anchorlist[i-1]
                    right = cp2.anchorlist[j-1]
                    left.right.append(right); right.left.append(left)
                    sumangle += atan2(float(right.y - left.y), float(col2-col1))
                    nangle += 1
                    i = i - 1
                    j = j - 1
                elif g[i][j].value - 1 == g[i-1][j].value:
                    i = i-1
                elif g[i][j].value - 1 == g[i][j-1].value:
                    j = j-1
                else:
                    raise RuntimeError, "Error in backtracing distance matrix"

            # store angle info
            if nangle > 0:
                prev_angle = sumangle / nangle
            cp1.avgangle = prev_angle
            cp2.avgangle = prev_angle

        runtimes.append(["DP matching:", time() - t])
        if debug > 3:
            drawcplistimage("miyaodebug1.png", cplist)

        #
        # interpolate missing points
        #

        interpolation_tolerance = int((self.staffline_height + \
                                       self.staffspace_height)*0.15)
        if interpolation_tolerance < 2:
            interpolation_tolerance = 2
        if debug:
            logmsg("interpolate missing points (tolerance=%d)...\n" %\
                   interpolation_tolerance)
        t = time()

        # from left to right
        for n in range(len(cplist)-1):
            cp1 = cplist[n]; cp2 = cplist[n+1]
            col1 = cp1.x; col2 = cp2.x
            dy = int(tan(cp1.avgangle) * (col2-col1))
            for a in cp1.anchorlist:
                if not a.right:
                    if (a.y + dy < self.image.nrows) and \
                       (0 == self.image.miyao_distance(col1, a.y, col2, a.y + dy, \
                                                       blackness=blackness, \
                                                       pr_angle=cp1.avgangle)):
                        right = anchor_point(a.y + dy)
                        right.interpolated = True
                        right.left.append(a)
                        right = cp2.insert(right, tolerance=interpolation_tolerance)
                        if right:
                            a.right.append(right)

        # from right to left
        n = len(cplist)
        while n > 1:
            n -= 1
            cp1 = cplist[n-1]; cp2 = cplist[n]
            col1 = cp1.x; col2 = cp2.x
            dy = int(tan(cp2.avgangle) * (col2-col1))
            for a in cp2.anchorlist:
                if not a.left:
                    if (a.y - dy >= 0) and \
                       (0 == self.image.miyao_distance(col1, a.y - dy, col2, a.y, \
                                                       blackness=blackness, \
                                                       pr_angle=cp2.avgangle)):
                        left = anchor_point(a.y - dy)
                        left.interpolated = True
                        left.right.append(a)
                        left = cp1.insert(left, tolerance=interpolation_tolerance)
                        if left:
                            a.left.append(left)

        # remove all points without links
        for i, line in enumerate(cplist):
            remove = []
            anchlast = anchor_point()
            for i, anchor in enumerate(line.anchorlist):
                if not anchor.left and not anchor.right:
                    remove.append(i)
                    anchlast = anchor
            n = 0
            for i in remove:
                del line.anchorlist[i-n]
                n += 1
            lastline = line

        runtimes.append(["Interpolation:", time() - t])
        if debug > 3:
            drawcplistimage("miyaodebug2.png", cplist)

        #
        # Step 3: Composition of Stave Groups
        #----------------------------------------------------------------

        # added by CD:
        # autoguess numlines as maximum from histogram of consecutive link numbers
        if num_lines == 0:
            dist = self.staffline_height + self.staffspace_height
            eps = dist * 0.15
            vlinknum = {}
            for line in cplist:
                consecutive = 0
                for i in range(len(line.anchorlist)-1):
                    if abs(line.anchorlist[i+1].y - line.anchorlist[i].y - dist) <= eps:
                        consecutive += 1
                    else:
                        if consecutive > 0:
                            if consecutive in vlinknum:
                                vlinknum[consecutive] += 1
                            else:
                                vlinknum[consecutive] = 1
                            consecutive = 0
            maxval = 0; maxind = 0
            for num in vlinknum.keys():
                if vlinknum[num] > maxval:
                    maxval = vlinknum[num]; maxind = num
            num_lines = maxind + 1
            print "Number of lines per staff system:", num_lines

        # add vertikal links based upon staffspace_height
        for line in cplist:
            dist = self.staffline_height + self.staffspace_height
            eps = dist * 0.15
            consecutive = 0
            for i in range(len(line.anchorlist)-1):
                if abs(line.anchorlist[i+1].y - line.anchorlist[i].y - dist) <= eps:
                    consecutive += 1
                else:
                    consecutive = 0
                if consecutive >= num_lines-1:
                    for j in range(i - consecutive + 1, i + 1):
                        if not (line.anchorlist[j].bottom):
                            line.anchorlist[j].bottom = line.anchorlist[j+1]
                            line.anchorlist[j+1].top = line.anchorlist[j]

        if debug > 3:
            drawcplistimage("miyaodebug3.png", cplist)

        #
        # find staff systems via cc analysis
        #

        if debug:
            logmsg("label staff systems and lines...\n")
        t = time()

        # first labeling round
        stafflabel = -1
        equivstaffs = EquivalenceGrouper()
        for line in cplist:
           for ap in line.anchorlist:
              # set staff label
              if not ap.top and not ap.left:
                 stafflabel += 1
                 ap.staff = stafflabel
              elif ap.top and not ap.left:
                 ap.staff = ap.top.staff
              elif not ap.top and ap.left:
                 ap.staff = ap.left[0].staff
                 for left in ap.left[1:]:
                    # mark for equivalence
                    if left.staff != ap.staff:
                       equivstaffs.join(ap.staff, left.staff)
              else:
                 ap.staff = ap.top.staff
                 for left in ap.left:
                    # mark for equivalence
                    if ap.top.staff != left.staff:
                       equivstaffs.join(ap.top.staff, left.staff)

        # build map of labels which need a change and unify labels
        newlabel = {}
        for s in equivstaffs.__iter__():
           stafflabel = min(s)
           for lbl in s:
              if lbl != stafflabel:
                 newlabel[lbl] = stafflabel
        if len(newlabel) > 0:
           for line in cplist:
              for ap in line.anchorlist:
                 if ap.staff in newlabel:
                    ap.staff = newlabel[ap.staff]

        # traverse CCs for line labeling
        for line in cplist:
           for ap in line.anchorlist:
              if not ap.line: # new CC found
                 # do a breadth first search for labeling all nodes
                 ap.line = 0
                 cp = ap
                 neighbors = []
                 for left in cp.left:
                    if left.line == None:
                       left.line = cp.line; neighbors.append(left)
                 for right in cp.right:
                    if right.line == None:
                       right.line = cp.line; neighbors.append(right)
                 if cp.top and cp.top.line == None:
                    cp.top.line = cp.line-1; neighbors.append(cp.top)
                 if cp.bottom and cp.bottom.line == None:
                    cp.bottom.line = cp.line+1; neighbors.append(cp.bottom)
                 while len(neighbors) > 0:
                    nneighbors = []
                    for cp in neighbors:
                       for left in cp.left:
                          if left.line == None:
                             left.line = cp.line; neighbors.append(left)
                       for right in cp.right:
                          if right.line == None:
                             right.line = cp.line; neighbors.append(right)
                       if cp.top and cp.top.line == None:
                          cp.top.line = cp.line-1; nneighbors.append(cp.top)
                       if cp.bottom and cp.bottom.line == None:
                          cp.bottom.line = cp.line+1; nneighbors.append(cp.bottom)
                    neighbors = nneighbors         

        if debug > 3:
            rgb = drawcplistlabels("miyaodebug4.png", cplist)

        # remove points on one scanline with same staff/line label
        # Warning:
        #    this will introduce dead links
        #    we don't care about this as we do not need the actual
        #    link values any further
        if debug:
            logmsg("remove some points and lines...\n")
        for line in cplist:
           lastap = anchor_point()
           samelinelabel = EquivalenceGrouper()
           for ap in line.anchorlist:
              if lastap.staff == ap.staff and lastap.line == ap.line:
                 samelinelabel.join(ap, lastap)
              lastap = ap
           for same in samelinelabel.__iter__():
              validap = same[0]
              for ap in same[1:]:
                 #if validap.interpolated:
                 #   if ap.interpolated:
                 #      validap.y = (validap.y + ap.y) / 2
                 #   else:
                 #      validap.y = ap.y
                 #      validap.interpolated = False
                 validap.y = (validap.y + ap.y) / 2
                 line.anchorlist.remove(ap)

        # determine staff/line labels that need to be removed
        # herefore count the number of links for each staffline
        numlinks = {} # map: staff -> line -> linknum
        for line in cplist:
           for ap in line.anchorlist:
              if ap.staff not in numlinks:
                 numlinks[ap.staff] = {ap.line : 0}
              elif ap.line not in numlinks[ap.staff]:
                 numlinks[ap.staff][ap.line] = 0
              if ap.right:
                 numlinks[ap.staff][ap.line] += 1
        #print "all Lines:", numlinks

        # remove lines which are too many within a staff group
        staffstoremove = []
        toomanylines = []
        for s in numlinks.keys():
           if len(numlinks[s]) < num_lines:
              staffstoremove.append(s)
           elif len(numlinks[s]) > num_lines:
              toomanylines.append(s)
        #print "Too few lines:", staffstoremove
        #print "Too many lines:", toomanylines

        linestoremove = [] # list of [staff,line] pairs
        for s in toomanylines:
           linelinks = numlinks[s]
           while len(linelinks) > num_lines:
              lines = linelinks.keys()
              top = min(lines); bot = max(lines)
              if linelinks[top] > linelinks[bot]:
                 linestoremove.append([s,bot])
                 del linelinks[bot]
              else:
                 linestoremove.append([s,top])
                 del linelinks[top]
        #print "Lines to remove:", linestoremove

        # remove all points on staffs/lines to remove
        for line in cplist:
           pointstoremove = []
           for ap in line.anchorlist:
              if ap.staff in staffstoremove or [ap.staff, ap.line] in linestoremove:
                 pointstoremove.append(ap)
           for ap in pointstoremove:
              if debug > 3:
                  rgb.draw_marker((line.x, ap.y), 10, 1, RGBPixel(255,0,0))
              line.anchorlist.remove(ap)

        runtimes.append(["Labeling and Removal:", time() - t])

        if debug > 3:
            rgb.save_PNG("miyaodebug4a.png")
            drawcplistlabels("miyaodebug5.png", cplist)

        #
        # Step 4: Edge Adjustment
        #----------------------------------------------------------------

        if debug:
            logmsg("egde adjustment...\n")
        t = time()

        # build up allpoints-map: staff -> line -> pointlist
        class LinePoints:
           def __init__(self, x, y):
              self.points = [[x,y]]
              self.leftangle = 0; self.rightangle = 0
        allpoints = {}
        for line in cplist:
           for ap in line.anchorlist:
              if ap.staff not in allpoints:
                 allpoints[ap.staff] = {ap.line : LinePoints(line.x, ap.y)}
                 # must be most left point in line
                 allpoints[ap.staff][ap.line].leftangle = line.avgangle
              elif ap.line not in allpoints[ap.staff]:
                 allpoints[ap.staff][ap.line] = LinePoints(line.x, ap.y)
                 # must be most left point in line
                 allpoints[ap.staff][ap.line].leftangle = line.avgangle
              else:
                 allpoints[ap.staff][ap.line].points.append([line.x, ap.y])
                 # might be most right point in line
                 allpoints[ap.staff][ap.line].rightangle = line.avgangle

        # adjust left edge
        for staff in allpoints.values():

           if debug > 2:
               logmsg("adjust staff with y between ")
               logmsg("%d and %d (%d lines)\n" % \
                      (staff[min(staff.keys())].points[0][1],\
                       staff[max(staff.keys())].points[0][1],\
                       len(staff.keys())))
        
           # extrapolate edge points
           for lp in staff.values():
              leftpoint = lp.points[0]
              [x,y] = self.image.miyao_find_edge(leftpoint[0], leftpoint[1],\
                                            lp.leftangle, True)
              if x < leftpoint[0]:
                 lp.points.insert(0,[x,y])
              else:
                 lp.points[0] = [x,y]
              rightpoint = lp.points[-1]
              [x,y] = self.image.miyao_find_edge(rightpoint[0], rightpoint[1],\
                                            lp.rightangle, False)
              if x > rightpoint[0]:
                 lp.points.append([x,y])
              else:
                 lp.points[-1] = [x,y]

           # compute average staff angle
           staffangle = 0.0
           nlinks = 0
           for lp in staff.values():
              for i in range(1,len(lp.points)):
                 p1 = lp.points[i-1]; p2 = lp.points[i]
                 if p2[0] != p1[0]:
                    staffangle += float(p2[1] - p1[1]) / (p2[0] - p1[0])
                 else:
                    logmsg("Oops, two points with same y on one staff line:"+\
                          ("[%d,%d] and [%d,%d]\n" % (p1[0],p1[1],p2[0],p2[1])))
                 nlinks += 1
           tanstaffangle = staffangle / nlinks
           staffangle = atan(tanstaffangle)

           # align edge points on a line
           tolerance = 5 * pi / 180
           for edgepoints in [ [ lp.points[0] for lp in staff.values() ],
                               [ lp.points[-1] for lp in staff.values() ] ]:
              # find reference ("standard") edge point
              alignfits = [0] * len(edgepoints)
              for i in range(len(edgepoints)):
                 for j in range(i+1,len(edgepoints)):
                    if abs( atan2(abs(float(edgepoints[i][0]-edgepoints[j][0])), \
                                abs(edgepoints[i][1]-edgepoints[j][1])) \
                            - staffangle) < tolerance:
                       alignfits[i] += 1
                       alignfits[j] += 1
              standard_edge_point_index = self.__max_ind(alignfits)
              standard_edge_point = edgepoints[standard_edge_point_index]
              # align all edge points on a line
              factor = 1 / (1 + tanstaffangle*tanstaffangle)
              for p in edgepoints:
                 if p != standard_edge_point:
                    y = (tanstaffangle * (standard_edge_point[0] - p[0] + \
                                          standard_edge_point[1] * tanstaffangle) \
                                          + p[1]) * factor
                    x = (standard_edge_point[1] - y) * tanstaffangle + \
                        standard_edge_point[0]
                    p[0] = int(x); p[1] = int(y)

           # remove point left (right) from left (right) edgepoint
           for lp in staff.values():
               while 1 < len(lp.points) and lp.points[0][0] >= lp.points[1][0]:
                   del lp.points[1]
               while 1 < len(lp.points) and lp.points[-1][0] <= lp.points[-2][0]:
                   del lp.points[-2]
               
        runtimes.append(["Edge adjustment:", time() - t])

        #
        # Step 5: Copy over result into StafflinePolygon structures
        #----------------------------------------------------------------

        if debug:
            logmsg("copy to StafflinePolygon structures...\n")
        t = time()

        # sort lines from top to bottom
        def sortlines(line1, line2):
           if line1[0] == line2[0]:
              # same staff number => compare line number
              return line1[1] - line2[1]
           else:
              # different staves => compare (x,y)-position of left
              # edgepoint in each staff
              staff1 = allpoints[line1[0]]
              staff2 = allpoints[line2[0]]
              leftedge1 = staff1[min(staff1.keys())].points[0]
              leftedge2 = staff2[min(staff2.keys())].points[0]
              rc = leftedge1[1] - leftedge2[1]
              if rc == 0:
                  rc = leftedge1[0] - leftedge2[0]
              return rc
              #return line1[2].points[0][1] - line2[2].points[0][1]

        alllines = [ [staff, line[0], line[1]] for staff in allpoints.keys() \
                     for line in allpoints[staff].items()]
        alllines.sort(sortlines)

        # build up data structure
        self.linelist = []
        staff = []
        laststaff = None
        for line in alllines:
            if laststaff != None and laststaff != line[0]:
                self.linelist.append(staff)
                staff = []
            pg = StafflinePolygon()
            pg.vertices = [ Point(p[0],p[1]) for p in line[2].points ]
            staff.append(pg)
            laststaff = line[0]
        # do not forget last staff
        self.linelist.append(staff)

        runtimes.append(["Copy to linelist:", time() - t])

        if debug > 1:
            total = 0.0
            for rt in runtimes:
                print ("%-21s" % rt[0]), rt[1]
                total += rt[1]
            print "%-21s------------------" % " "
            print ("%-21s" % " "), total

######################################################################
# data structures for anchor points
#
# one anchor point
class anchor_point:
   def __init__(self,y=-1):
      self.y = y
      # link points
      # (left/right could be multiple, top/bottom only single)
      self.left = []; self.right = []
      self.top = None; self.bottom = None
      # labels
      self.staff = None; self.line = None
      self.interpolated = False
#
# one vertical line of anchor points
class vertical_anchorline:
   def __init__(self, x=-1, ylist=[]):
      self.x = x
      self.anchorlist = [anchor_point(y) for y in ylist]
      self.avgangle = pi # average angle of right hand links
   def insert(self, anchorpoint, tolerance):
      # find insert location
      i = 0
      while i < len(self.anchorlist):
         if self.anchorlist[i].y > anchorpoint.y:
            break
         i += 1
      # we do not insert when y-position within tolerance
      replace = -1
      if (i < len(self.anchorlist)) and \
         (abs(self.anchorlist[i].y - anchorpoint.y) < tolerance):
         replace = i
      elif (i > 0) and \
           (abs(self.anchorlist[i-1].y - anchorpoint.y) < tolerance):
         replace = i-1
      if replace < 0:
         # no point nearby: insert new point
         self.anchorlist.insert(i, anchorpoint)
         ap = anchorpoint
      else:
         ap = self.anchorlist[replace]
         if not ap.left and not ap.right:
             # nearby point has no links => replace it with new point
             self.anchorlist[replace] = anchorpoint
             ap = anchorpoint
         elif (ap.left and anchorpoint.left) or \
                  (ap.right and anchorpoint.right):
             # nearby point has link from same direction
             # => do not introduce close parallel links
             ap = None
         else:
             # nearby point has link in different direction
             # => add link to old nearby point
             for left in anchorpoint.left:
                 if left not in ap.left: ap.left.append(left)
             for right in anchorpoint.right:
                 if right not in ap.right: ap.right.append(right)
             # currently not needed in algorithm:
             #if not ap.top: ap.top = anchorpoint.top
             #if not ap.bottom: ap.bottom = anchorpoint.bottom
      # for feedback on y-value
      return ap

