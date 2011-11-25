# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
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

from sys import stdout
from array import array

from gamera.gui import has_gui
from gamera import toolkit
from gamera.args import *
from gamera.args import Point as PointArg # is also defined in gamera.core
from gamera.core import *
#from stafffinder import StaffFinder as _virtual_StaffFinder
#from stafffinder import StafflineAverage, StafflineSkeleton, StafflinePolygon
from gamera.toolkits.musicstaves.stafffinder import StaffFinder as _virtual_StaffFinder
from gamera.toolkits.musicstaves.stafffinder import StafflineAverage, StafflineSkeleton, StafflinePolygon
#from gamera.toolkits.musicstaves.plugins import *

##############################################################################
#
#
class StaffFinder_projections(_virtual_StaffFinder):
    """Finds staff lines as maxima in the y projection profile.

The process is as follows:

1) Optionally, the image is preprocessed in order to remove
   stuff \"irrelevant\" for staff line finding (eg. short black runs)
    
2) The horizontal projection profile is computed and lowpass filtered
   with an averaging kernel of width *staffspace_height*. In this filtered
   profile, staff breaks are seeked as minima below a certain
   (hard coded) threshold.

   Per estimated staff system, a local threshold for accepting maxima
   in step 3) is set as a (choosable) fraction of the maximum projection
   value per estimated staff system. Apart from setting the local
   threshold values, the estimated staff breaks have no further effect
   on the algorithm.

3) The original projection profile is lowpass filtered with an
   averaging kernel of width *staffline_height*. All maxima above the
   threshold from step 2) are computed. These peaks are grouped into
   staff systems based upon their distance.

4) In each staff system, peaks lying on a grid with distance about
   *staffspace_height* are detected. As reference peak the highest
   peak in the staff system is chosen.

5) Step 4) typically results in more or less than *num_lines* peaks.
   Staff systems with less peaks are removed, staff systems with
   more peaks are reduced to *num_lines* peaks by removing the lowest
   edge peaks.

   If *num_lines* is not given, it is computed as the maximum in
   the lines-per-staff histogram.

The horizontal edge points are found by scanning the vertical projection
profile from the left resp. right until a projetion value larger than
*staffline_height* is found. Note that this fails when the image contains
black borders from the scanning process; in that case use the preprocessing
plugin `remove_border` from the gamera core.

:Author: Christoph Dalitz
"""
    def __init__(self, img, staffline_height=0, staffspace_height=0):
        _virtual_StaffFinder.__init__(self, img,\
                staffline_height, staffspace_height)
        self.find_staves_args = Args([Int("num_lines", default=0),
                                      Float("peakthreshold", range=(0,1), default=0.0),
                                      Choice("preprocessing", ["None", "Extract dark runs", "Filter short blcak runs"], default=0),
                                      Check("postcorrect", check_box="Enforce num_lines", default=True),
                                      Check("follow_wobble", check_box="Postprocess with follow_staffwobble", default=False)
                                      ])
        self.debug = 0


    ######################################################################
    # find_staves
    #
    # Christoph Dalitz
    #
    def find_staves(self, num_lines=0, peakthreshold=0.0, preprocessing=0, postcorrect=True, follow_wobble=False, debug=0):
        """Finds staff lines from a music/tablature image with the aid
of y projections.
Signature:

  ``find_staves(num_lines=0, peakthreshold=0.0, preprocessing=1, postcorrect=True, debug=0)``

with

  *num_lines*:
    Number of lines within one staff. When zero, the number is automatically
    detected.

  *peakthreshold*:
    Threshold factor (relative to maximum projection value within a stave
    group) for accepting peaks. When zero, it is set automatically,
    depending on the chosen preprocessing (see next parameter).

  *preprocessing*:
    Whether the image should be prepared in such a way that \"obvious\"
    non staff segments should be removed. Possible values:
    0: no preprocessing
    1: extract filled horizontal runs
    2: filter short horizontal black runs

  *postcorrect*:
    When `True`, the number of lines per staff is automatically adjusted
    to the *num_lines*.

  *follow_wobble*:
    When `True`, each staffline is postprocessed with `follow_staffwobble`__
    for a (hopefully) more accurate representation.

.. __: musicstaves.html#follow-staffwobble

  *debug*:
    When > 0, information about each step is printed to stdout (flushed
    immediately). When > 1, a number of images and datafiles are written
    into the current working directory; file names have the prefix *projdebug*.
    For the datafiles a *gnuplot* command file *projdebug.plot* is written.
"""

        # clear potential result from previous calls to this functions
        self.linelist = []
        self.staffdistance = self.staffspace_height + self.staffline_height

        self.debug = debug
        if debug > 0:
            self.logmsg("Write debug information into projdebug1.*\n" +\
                        "Call 'gnuplot projdebug.plot' to display graphs\n" +\
                        "Staffline distance: %d\n" % self.staffdistance)

        # workaround for the lack of 'None'-support in GUI argument dialog
        if peakthreshold == 0.0:
            peakthreshold = None

        #
        # Step 1)
        # -------
        #
        # Preprocess image and take projection profile
        #---------------------------------------------------------------

        if preprocessing == 1:
            img = self.image.extract_filled_horizontal_black_runs(\
            windowwidth = 7*self.staffspace_height, blackness = 75)
            if peakthreshold == None:
                peakthreshold = 0.2
        elif preprocessing == 2:
            img = self.image.image_copy()
            img.filter_narrow_runs(self.staffspace_height, 'black')
            if peakthreshold == None:
                peakthreshold = 0.3
        else:
            img = self.image
            if peakthreshold == None:
                peakthreshold = 0.4
        yproj = img.projection_rows()
        maxproj = max(yproj)
        if debug > 1:
            img.save_PNG("projdebug1.png")
            self.save_projection(yproj,"projdebug1.dat")

        #
        # Step 2)
        # -------
        #
        # Compute local peakheight threshold:
        #
        # we will need a threshold for accepting a peak height or not
        # this could be based on maxproj, but a global threshold does
        # not take differently wide staff systems into account
        # Thus we make this local to a staff system which is determined
        # from a lowpass filtered profile with a wide averaging kernel
        #---------------------------------------------------------------

        # filter with wide averaging kernel
        avgproj = self.__lowpass(yproj, int(self.staffspace_height * 1.5))
        if debug > 1:
            self.save_projection(avgproj,"projdebug2.dat")

        # split at possible staff system breaks and compute local maxima
        staffbreaks = []
        threshold = 0.05 * maxproj
        above = False
        lastbelow = 0
        lastabove = None
        # set minimum height for staff incl. adjacent interstaff space
        if num_lines > 0:
            minstaffheight = num_lines * self.staffspace_height
        else:
            minstaffheight = 4 * self.staffspace_height
        for i,p in enumerate(avgproj):
            if p >= threshold and not above:
                above = True
                if lastabove != None and i - lastabove > minstaffheight:
                    staffbreaks.append((i+lastbelow)/2)
                    lastabove = i
                elif lastabove == None:
                    lastabove = i
            elif p < threshold and above:
                above = False
                lastbelow = i

        # do the actual staff line finding on a profile
        # filtered with a narrow lowpass kernel
        if self.staffline_height % 2 == 0:
            width = self.staffline_height + 1
        else:
            width = self.staffline_height + 1
        avgproj = self.__lowpass(yproj, width)

        # compute local maxima for each staff system candidate
        minpeakheight = []
        staffbegin = 0
        for staffend in staffbreaks:
            #minpeakheight += [threshold * max(avgproj[staffbegin:staffend])] \
            minpeakheight += [peakthreshold * max(yproj[staffbegin:staffend])] \
                             * (staffend - staffbegin)
            staffbegin = staffend
        staffend = self.image.nrows
        #minpeakheight += [threshold * max(avgproj[staffbegin:staffend])] \
        minpeakheight += [peakthreshold * max(yproj[staffbegin:staffend])] \
                         * (staffend - staffbegin)

        if debug > 1:
            const = max(avgproj) * 1.1
            f = open("projdebug3.dat","w")
            if len(staffbreaks) > 0:
                for i in staffbreaks:
                    f.write("%d %d\n" % (i, const))
            else:
                f.write("0 0\n")
            f.close()
            self.save_projection(minpeakheight,"projdebug4b.dat")

        #
        # Step 3)
        # -------
        #
        # Compute peak candidates
        #---------------------------------------------------------------

        # find all local maxima in lowpass filtered profile
        if debug > 1:
            self.save_projection(avgproj,"projdebug4a.dat")
		peaks = []
        lastdx = 0
        lasti = 0
        i = 0
        while i < len(avgproj) - 1:
            i += 1
            dx = avgproj[i] - avgproj[i-1]
            while dx == 0 and i < len(avgproj) - 1:
                i += 1
                dx = avgproj[i] - avgproj[i-1]
            peaki = (lasti + i) / 2
            if lastdx > 0 and dx < 0 and \
                   avgproj[peaki] > minpeakheight[i]:
                peaks.append([peaki,avgproj[peaki]])
            elif dx > 0:
                lasti = i - 1
            lastdx = dx

        # pick peaks larger than local thresholds
        highpeaks = [ p for p in peaks if p[1] > minpeakheight[p[0]] ]

        if debug > 1:
            const = max(avgproj) * 1.1
            f = open("projdebug4c.dat","w")
            for p in highpeaks:
                #f.write("%d %d\n" % (p[0], avgproj[i]))
                f.write("%d %d\n" % (p[0], const))
            f.close()

        if len(highpeaks) == 0:
            self.logmsg("No Stafflines found\n")
            return

        #
        # Step 4)
        # -------
        #
        # Group peaks into staff groups based upon their distance
        # and find peaks on a grid within each group
        #---------------------------------------------------------------

        # group lines together based upon their distance
        # relative to staffspace_height
        staffs = []
        lines = []
        lastp = None
        for p in highpeaks:
            if lastp == None or p[0] - lastp[0] < \
                   (self.staffspace_height*1.5 + self.staffline_height):
                lines.append(p)
            elif lastp != None:
                staffs.append(lines)
                lines = [p]
            lastp = p
        if len(lines) > 0:
            staffs.append(lines)

        if self.debug > 0:
            self.logmsg(("num_lines unknown: %d staff systems detected\n" %\
                         len(staffs)) + "Select peaks on grid around " +\
                        "heighest peak (Refpeak) per staff system\n")

        # align peaks per staff on grid
        # Note: peakheight information is not returned
        tolerance = 0.15 * self.staffspace_height + self.staffline_height
        for staffno, staff in enumerate(staffs):
            staffs[staffno] = self.__get_gridpeaks(staff, tolerance)

        if debug > 1:
            f = open("projdebug4d.dat","w")
            for staff in staffs:
                for line in staff:
                    f.write("%d %d\n" % (line, avgproj[line]))
            f.close()

        #
        # Step 5)
        # -------
        #
        # Postcorrection for staves with more/less than num_lines lines
        #---------------------------------------------------------------

        # for unknown num_lines we must guess it first
        # as the most frequent line number per system
        if num_lines == 0 and postcorrect:
            nums = [len(s) for s in staffs]
            freq = [0] * (max(nums) + 1)
            for n in nums:
                freq[n] += 1
            maxf = 0
            for n, f in enumerate(freq):
                if maxf < f:
                    num_lines = n
                    maxf = f
            if debug > 0:
                print "Line number frequencies:", freq[1:]
            self.logmsg("Lines per system autodetected as %d\n" \
                        % num_lines)

        if num_lines > 0 and postcorrect:
            # rebuild peakheight information using raw projection profile
            staffpeaks = []
            for s in staffs:
                staffpeaks.append([ [p, self.__get_peakheight(p,yproj)] for p in s ])
            staffs = self.__fit_to_num_lines(staffpeaks, peaks, num_lines)

        if debug > 1:
            f = open("projdebug4e.dat","w")
            for staff in staffs:
                shift = max(avgproj) / 50 + 1
                for line in staff:
                    f.write("%d %d\n" % (line, avgproj[line] + shift))
            f.close()

        #
        # Copy over result
        #---------------------------------------------------------------

        # store result
        for staff in staffs:
            [leftx, rightx] = self.__get_staffedges(self.image, staff[0], staff[-1])
            newstaff = []
            for line in staff:
                avg = StafflineAverage()
                avg.average_y = line
                avg.variance_y = 0
                avg.left_x = leftx
                avg.right_x = rightx
                newstaff.append(avg)
            self.linelist.append(newstaff)

        # optionally postprocess with follow_staffwobble
        if (follow_wobble):
            if debug > 0:
                logmsg("perform follow_staffwobble\n")
            newlinelist = []
            for staff in self.linelist:
                newstaff = [self.image.follow_staffwobble(line,self.staffline_height) \
                            for line in staff]
                newlinelist.append(newstaff)
            self.linelist = newlinelist


        if debug > 1:
            # write gnuplot command file
            threshold = 0.05
            f = open("projdebug.plot", "w")
            f.write("# plot commands for plotting profile with gnuplot\n\n")
            f.write("# projection maximum based threshold\n" + \
                    ("f(x) = %d * 0.05\n\n" % maxproj))
            f.write("plot 'projdebug1.dat' with impulses title \"raw projection profile\", f(x) with lines title \"threshold %4.2f\"\npause -1 \"press ENTER to continue\"\n\n" % threshold)
            f.write("plot 'projdebug2.dat' with impulses title \"wide lowpass filtered profile\", f(x) with lines title \"threshold %4.2f\", 'projdebug3.dat' with impulses title \"staff break candidates\"\npause -1 \"press ENTER to continue\"\n\n" % threshold)
            if num_lines > 0:
                f.write("plot 'projdebug4c.dat' with impulses title \"detected peaks\", 'projdebug4b.dat' with lines title \"local threshold\", 'projdebug4a.dat' with impulses title \"narrow lowpass filtered profile\", 'projdebug4d.dat' with points title \"stafflines on grid\", 'projdebug4e.dat' with points title \"selected stafflines\"\npause -1 \"press ENTER to continue\"\n\n")
            else:
                f.write("plot 'projdebug4c.dat' with impulses title \"detected peaks\", 'projdebug4b.dat' with lines title \"local threshold\", 'projdebug4a.dat' with impulses title \"narrow lowpass filtered profile\", 'projdebug4d.dat' with points title \"stafflines on grid\"\npause -1 \"press ENTER to continue\"\n\n")
            f.close()

    ######################################################################
    # __lowpass
    #
    # lowpass filters vector vec with averaging kernel of width windowwidth
    #
    def __lowpass(self, vec, windowwidth):
        avg = [0] * len(vec)
        window = [0] * windowwidth
        avgi = - windowwidth / 2
        zeros = array(vec.typecode, [0] * (windowwidth/2))
        for i,p in enumerate(vec + zeros):
            del window[0]
            window.append(p)
            if avgi > 0:
                avg[avgi] = sum(window) * 1.0 / windowwidth
            avgi += 1
        return avg

    ######################################################################
    # __selectpeak
    #
    # returns the largest peak in peaklist within i plusminus tolerance
    #
    def __selectpeak(self, peaklist, i, tolerance, interpolate=True):
        maxpeak = [0, 0]
        for p in peaklist:
            if abs(p[0] - i) < tolerance and p[1] > maxpeak[1]:
                maxpeak = p
        if maxpeak[1] == 0:
            # nothing found: interpolate line
            if interpolate:
                return i
            else:
                return None
        else:
            return maxpeak[0]

    ######################################################################
    # __get_gridpeaks
    #
    # pick peaks that fall on a grid with staffdistance plusminus tolrance
    #
    def __get_gridpeaks(self, peaks, tolerance):

        if self.debug > 2:
            print "Staff:", peaks
        # pick largest peak as reference position
        refpeak = [0, 0.0]
        min_i = self.image.nrows
        max_i = 0
        for line in peaks:
            if line[1] > refpeak[1]:
                refpeak = line
            if line[0] < min_i:
                min_i = line[0]
            if line[0] > max_i:
                max_i = line[0]
        if self.debug > 2:
            print "Refpeak:", refpeak
        lines = [refpeak[0]]
        # move upwards from reference peak along grid
        i = refpeak[0] - self.staffdistance
        while i >= min_i - tolerance:
            i = self.__selectpeak(peaks, i, tolerance)
            lines.append(i)
            i = i - self.staffdistance
        # move downwards from reference peak along grid
        i = refpeak[0] + self.staffdistance
        while i <= max_i + tolerance:
            i = self.__selectpeak(peaks, i, tolerance)
            lines.append(i)
            i = i + self.staffdistance
        lines.sort()
        return lines


    ######################################################################
    # __get_peakheight
    #
    # returns the maximum height around ypos plusminus staffline_height/2
    # This is necessary for two reasons:
    #  a) __selectpeak could have interpolated the peak value
    #  b) the peak could have been detected in a filtered profile
    #     while the peakheight is to be taken from the raw profile
    #
    def __get_peakheight(self, ypos, yproj):
        tolerance = self.staffline_height / 2
        ymin = max([0, ypos - tolerance/2])
        ymax = min([len(yproj) - 1, ypos + tolerance/2])
        return max(yproj[ymin:ymax+1])

    ######################################################################
    # __fit_to_n_peaks
    #
    # try to adjust each staff system to num_lines lines
    # arguments:
    #   staffs: nested list of staffline peaks
    #   peaks:  all peaks found in the original image
    #   num_lines: lines per system
    #
    def __fit_to_num_lines(self, staffs, peaks, num_lines):

        # TODO: split inadvertently joined systems
        #       (i.e. systems larger than 2*num_lines*staffdistance)

        # check for staves with more than num_lines lines
        newstaffs1 = []
        for staff in staffs:
            n = len(staff)
            while n > num_lines:
                # remove the edge with lower peak height
                if staff[0][1] < staff[n-1][1]:
                    del staff[0]
                else:
                    del staff[n-1]
                n -= 1
            newstaffs1.append(staff)

        # remove staves with less than num_lines lines
        todelete = []
        for i, staff in enumerate(newstaffs1):
            if len(staff) < num_lines:
                todelete.append(i)
        offset = 0
        for i in todelete:
            del newstaffs1[i-offset]
            offset += 1

        # TODO: alternative idea: try to etrapolate/join staves
        # with only one less line or which are rather close

        return [ [line[0] for line in staff] for staff in newstaffs1]


    ######################################################################
    # __get_staffedges
    #
    # return left and right edge of staff between top and bot
    #
    def __get_staffedges(self, image, top, bot):
        ntop = top - self.staffspace_height
        if ntop < 0:
            ntop = top
        nbot = bot + self.staffspace_height
        if nbot >= image.nrows:
            nbot = bot
        staff = SubImage(image, Point(0,ntop),Point(image.ncols-1,nbot))
        proj = staff.projection_cols()
        leftx = 0
        while leftx < image.ncols and proj[leftx] < self.staffline_height:
            leftx += 1
        if leftx == image.ncols:
            leftx = 0
        rightx = image.ncols - 1
        while rightx > 0 and proj[rightx] < self.staffline_height:
            rightx -= 1
        if rightx == 0:
            rightx = image.ncols - 1
        return [leftx, rightx]


    ######################################################################
    # some functions for creating debugging information
    def logmsg(self, msg):
        stdout.write(msg)
        stdout.flush()
    def save_projection(self, proj, filename):
        f = open(filename, "w")
        for p in proj:
            f.write("%d\n" % p)
        f.close()

