# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2006-2010 Christoph Dalitz, Bastian Czerwinski
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
from gamera.args import *
from math import *
from random import normalvariate,random,seed

import _staffdeformation


#----------------------------------------------------------------

class degrade_kanungo_parallel(PluginFunction):
    """Degrades an image and its corresponding staff only image in parallel
due to a scheme proposed by Kanungo et alii (see the reference below). This is
supposed to emulate image defects introduced through printing and scanning.

The argument *im_staffonly* must be an image containing only the staff
segments (without symbols) of the *self* image.
The degradation scheme depends on six parameters *(eta,a0,a,b0,b,k)* with
the following meaning:

  - each foreground pixel (black) is flipped with probability
    `a0*exp(-a*d^2) + eta`, where d is the distance to the closest
    background pixel

  - each background pixel (white) is flipped with probability
    `b0*exp(-b*d^2) + eta`, where d is the distance to the closest
    foreground pixel

  - eventuall a morphological closing operation is performed with a disk
    of diameter *k*. If you want to skip this step set *k=0*; in that
    case you should do your own smoothing afterwards.

The random generator is initialized with *random_seed* for allowing
reproducable results.

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

References:

  T. Kanungo, R.M. Haralick, H.S. Baird, et al.:
  *A statistical, nonparametric methodology for document
  degradation model validation.*
  IEEE Transactions on Pattern Analysis and Machine Intelligence
  22 (11), 2000, pp. 1209-1223

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('eta', range=(0.0,1.0)),
                 Float('a0', range=(0.0,1.0)),
                 Float('a'),
                 Float('b0', range=(0.0,1.0)),
                 Float('b'),
                 Int('k', default=2),
                 Int('random_seed', default=0)])
    return_type = Class('images_and_skel')
    author = "Christoph Dalitz"

    def __call__(self, im_staffonly, eta, a0, a, b0, b, k=2, random_seed=0):
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        # the C++ plugin only returns a single image
        # random_seed should guarantee compatible results
        # additionally we subtract staffless image from nostaffdef
        # so that not too many symbol pixels are redefined as staff pixels
        staffdef = _staffdeformation.degrade_kanungo_single_image(self, eta, a0, a, b0, b, k, random_seed)
        nostaffdef = _staffdeformation.degrade_kanungo_single_image(im_staffonly, eta, a0, a, b0, b, k, random_seed)
        im_nostaff = self.subtract_images(im_staffonly)
        nostaffdef = nostaffdef.subtract_images(im_nostaff)
        # create staffline skeletons
        [first_x, last_x, stafflines, thickness] =find_stafflines_int(im_staffonly)
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)
        return [staffdef, nostaffdef, staffline_skel]

    __call__ = staticmethod(__call__)

class degrade_kanungo_single_image(PluginFunction):
    """C++ part of degrade_kanungo for deforming a single image.
"""
    category = None
    self_type = ImageType([ONEBIT])
    args = Args([Float('eta', range=(0.0,1.0)),
                 Float('a0', range=(0.0,1.0)),
                 Float('a'),
                 Float('b0', range=(0.0,1.0)),
                 Float('b'),
                 Int('k', default=2),
                 Int('random_seed', default=0)])
    return_type = ImageType([ONEBIT], 'deformed_image')
    author = "Christoph Dalitz"

#----------------------------------------------------------------

class white_speckles_parallel(PluginFunction):
    """Adds white speckles to a music image and in parllel to its
corresponding staff only image. This is supposed to emulate image
defects introduced through printing, scanning and thresholding.

The degradation scheme depends on three parameters *(p,n,k)* with
the following meaning:

 - Each black pixel in the input image is taken with probability *p*
   as a starting point for a random walk of length *n*.
   Consequently *p* can be interpreted as the speckle frequency and *n*
   as a measure for the speckle size.

 - An image containing the random walk is smoothed by a closing operation
   with a rectangle of size *k*.

 - Eventually the image with the random walks is subtracted from both the
   original image and the provided staffless image, which results in white
   speckles at the random walk positions

Input arguments:

 *im_staffonly*:
   image containing only the staff segments (without symbols) of the
   *self* image

 *p, n, k*:
   speckle frequency, random walk length and closing disc size

 *connectivity*:
   effects the connectivity of adjacent random walk points as shown
   in the following figure (in case of bishop moves the final closing
   operation whitens the neighbouring 4-connected pixels too):

   .. image:: images/randomwalk_connectivity.png

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('p', range=(0.0,1.0)),
                 Int('n'),
                 Int('k', default=2),
                 Choice('connectivity', ['rook','bishop','king'], default=2),
                 Int('random_seed', default=0)])
    return_type = Class('images_and_skel')
    author = "Christoph Dalitz"

    def __call__(self, im_staffonly, p, n, k=2, connectivity=2, random_seed=0):
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        # the C++ plugin only returns the images
        [staffdef, nostaffdef] = _staffdeformation.white_speckles_parallel_noskel(self, im_staffonly, p, n, k, connectivity, random_seed)
        # create staffline skeletons
        [first_x, last_x, stafflines, thickness] =find_stafflines_int(im_staffonly)
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)
        return [staffdef, nostaffdef, staffline_skel]

    __call__ = staticmethod(__call__)


class white_speckles_parallel_noskel(PluginFunction):
    """C++ part of add_white_speckles.
"""
    category = None
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('p', range=(0.0,1.0)),
                 Int('n'),
                 Int('k', default=2),
                 Choice('connectivity', ['rook','bishop','king'], default=2),
                 Int('random_seed', default=0)])
    return_type = ImageList('deformed_images')
    author = "Christoph Dalitz"


#----------------------------------------------------------------
# helper function for the deformations

def find_stafflines_int(im_staffonly):
    """finds the y-postitions, from-to-x-values and the thickness
of stafflines
"""
    # note: stafflines appear in the picture as rectangles. we describe
    #       them with left and right x-coordinates, y-center and thickness.
    # the vertical projection contains zeroes, where no line is
    proj=im_staffonly.projection_rows()
    stafflines=[]
    lastline=False
    line_thickness=0
    for line in range(len(proj)):
        if proj[line]>0:       # at this y-pos is a staffline
            if not lastline:   # is it the starting y-value of a rectangle?
                stafflines.append(line)
                lastline=True
        else:                  # at this y-pos is no staffline
            if lastline:       # is it the last y-value of a rectangle
                line_thickness=line-stafflines[-1]
                # calculate the center y position
                stafflines[-1]=(stafflines[-1]+line)/2
                lastline=False

    # search the left x-pos of the first staffline
    first_x=0
    while im_staffonly.get((first_x,stafflines[0]))==0:
        first_x=first_x+1
    
    # search the right x-pos of the first staffline
    last_x=im_staffonly.width-1
    while im_staffonly.get((last_x,stafflines[0]))==0:
        last_x=last_x-1

    return [first_x,last_x,stafflines,line_thickness]

class binomial:
    """Helper class to create random values over a binomial distribution.
Uses pythons internal random-generator"""
    
    def __init__(self,n,p):
        self.distr=bnp(n,p)

    def get_k(self,r):
        for i in range(len(self.distr)):
            r=r-self.distr[i]
            if r<0:
                k=i
                break
        else:
            k=len(self.distr)-1
        return k

    def random(self):
        return self.get_k(random())

# binmonial distribution
def bnp(n,p):
    # nCombinations (n over k)
    def comb(n,k):
        if k<1:
            return 1
        else:
            return (n*1.0/k)*comb(n-1,k-1)
    rpn=p**n
    return [comb(n,k)*rpn for k in range(n+1)]

class states_mh:
    """Helper class for staffline_thickness_variation and
staffline_y_variation

Implements a Metropolis/Hastings state machine. The stationary
probabilties for the states are bnp distributed (saved in self.p).
*n* is the count of the states. *stay* is a lower bound for the
probability to stay in any state.

next() returns a new state everytime it is called.
"""
    def __init__(self, n, stay):
        # we start in the middle state
        self.i=(n-1)/2
        # for n states we take the probabilities for
        # 0 out of n-1
        # 1 out of n-1
        # ...
        # n-1 out of n-1
        # p being 0.5
        self.p=bnp(n-1,0.5)
        self.n=n

        # create a zero filled nxn-matrix
        self.q_mat=[n*[0] for i in range(n)]

        for i in range(n):
            # on the main diagonal we have the *stay* probability
            self.q_mat[i][i]=stay
            # the rest is equally distributed on the direct neighbours
            if i==0:
                self.q_mat[1][0]=1-stay
            else:
                if i==n-1:
                    self.q_mat[i-1][i]=1-stay
                else:
                    self.q_mat[i-1][i]=(1-stay)/2
                    self.q_mat[i+1][i]=(1-stay)/2

    def next(self):
        z=random()
        
        # set a default, so python will not complain about a possibly unassigned variable
        j=self.i
        for k in range(max((self.i-1,0)),self.n):
            z=z-self.q_mat[k][self.i]
            if z<0:
                j=k
                break

        # this is the connection to the bnp distribution (see reference)
        if self.i!=j:
            if random()<((self.p[j]     *self.q_mat[j][self.i])
                        /(self.p[self.i]*self.q_mat[self.i][j])):
                self.i=j

        return self.i


#----------------------------------------------------------------

class typeset_emulation(PluginFunction):
    """Emulates typeset music by interrupting staff lines between symbols
and shifting the resulting vertical slices randomly up or down.

Staff lines and systems are detected in *im_staffonly* by horizontal
projections. Splitting points for the interruptions (gaps) of each
staff system are detected by vertical projections in *im_staffonly*.

Input arguments:

 *im_staffonly*:
   image containing only the staff segments (without symbols) of the
   *self* image

 *n_gap*, *p_gap*:
   n and p values for gap widths (binomial distribution)

 *n_shift*:
   n value for y-shift (binomial distribution; p always 0.5)

 *random_seed*:
   Initializer for random generator allowing reproducable results.

 *add_noshift*:
   Add images to the output only containing the breaks, and no shifts

Returns a tuple *[def_full, def_staffonly, list_staffline]* or (if
*add_noshift* is set to True) *[def_full, def_staffonly, list_staffline,
ns_full, ns_staffonly]* with the following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

 *ns_full*
   deformed version of the *self* image, only breaks, no shifts

 *ns_staffonly*
   deformed version of the *im_staffonly*, only breaks, no shifts

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Int('n_gap'),
                 Float('p_gap'),
                 Int('n_shift'),
                 Int('random_seed', default=0),
                 Check('add_noshift', default=False)])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self,im_staffonly, n_gap, p_gap, n_shift, random_seed=0, add_noshift=False ):
        from gamera.core import RGBPixel,Point,FloatPoint,Rect
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        seed(random_seed)
        bnp_gap=binomial(n_gap,p_gap)
        bnp_shift=binomial(n_shift,0.5)
        im_full=self.image_copy()
        im_staffless=im_full.xor_image(im_staffonly)
        im_staffonly=im_staffonly.image_copy()
        il_shifts=[im_staffless,im_staffonly,im_full]
        il_breaks=il_shifts
        if add_noshift:
            ns_full=im_full.image_copy()
            ns_staffonly=im_staffonly.image_copy()
            il_breaks=[im_staffless,im_staffonly,im_full,ns_full,ns_staffonly]
        
        #find stafflines
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)

        #find breaks (where are the symbols?)
        stafflinedist=stafflines[1]-stafflines[0]
        staffline_skel=[]
        for sl in range(len(stafflines)):
            if sl==0 or stafflines[sl]-stafflines[sl-1]>3*stafflinedist:
                #first staffline of a system
                first_line=stafflines[sl]
                lines_per_system=0

            lines_per_system=lines_per_system+1

            if sl==len(stafflines)-1 or stafflines[sl+1]-stafflines[sl]>3*stafflinedist:
                #last staffline of a system
                last_line=stafflines[sl]
                # a hoizontal projection of the symbols helps us to find the x positions of the symbols.
                syst=im_staffless.subimage(Point(0,max((0,first_line-3*stafflinedist))),Point(im_staffless.width,min((last_line+3*stafflinedist,im_full.lr_y-1))))
                symbolcols=syst.projection_cols()

                # collect the breaks, i.e. the mid of the white spaces between two symbols
                breaks=[]
                whiterun=False
                for col in range(len(symbolcols)):
                    if (not whiterun) and symbolcols[col]==0:
                        whiterun=True
                        runbegin=col
                    else:
                        if whiterun and symbolcols[col]>0:
                            whiterun=False
                            br=[(runbegin+col)/2,col-runbegin]
                            if br[0]>=first_x:
                                breaks.append(br)

                # replace the first break (before the first symbol) with a break at the beginning of the stafflines
                breaks[0]=[first_x,1]
                # and append a break after the last symbol
                if whiterun:
                    breaks.append([(last_x+runbegin)/2,last_x-runbegin])
                else:
                    breaks.append([last_x,1])

                # draw white lines at the breaks
                new_breaks=[]
                for br in breaks:
                    w=bnp_gap.random()
                    # draw line if it fits only
                    if w<br[1]:
                        for im in il_breaks:
                            im.draw_line(FloatPoint(br[0],first_line-stafflinedist),
                                         FloatPoint(br[0],last_line+stafflinedist),
                                         RGBPixel(0,0,0),
                                         w)
                        new_breaks.append(br)
                breaks=new_breaks
                skeleton=[]

                # do the vertical shift by making a temporary copy (orig_type), white them out (draw_filled_rect) and "or" them again in with a new y-position
                for t in range(len(breaks)-1):
                    vertical_shift=bnp_shift.random()-n_shift/2
                    typerect=Rect(Point(breaks[t][0],max((first_line-3*stafflinedist,0))),
                                  Point(breaks[t+1][0],min((last_line+3*stafflinedist,im_full.lr_y))))
                    moveto=Rect(Point(typerect.ul_x,
                                      typerect.ul_y+vertical_shift),
                                typerect.size)
                    if moveto.ul_y<0:
                        typerect.ul_y=typerect.ul_y-moveto.ul_y
                        moveto.ul_y=0
                    if moveto.lr_y>im_full.lr_y:
                        typerect.lr_y=typerect.lr_y-(moveto.lr_y-im_full.lr_y)
                        moveto.lr_y=im_full.lr_y

                    for im in il_shifts:
                        orig_type=im.subimage(typerect)
                        orig_type=orig_type.image_copy()
                        im.draw_filled_rect(typerect,RGBPixel(0,0,0))
                        im.subimage(moveto).or_image(orig_type,True)

                    # collect data for later construction of the skeletons
                    for line in range(lines_per_system):
                        if len(skeleton)<=line:
                            skeleton.append([])
                        st_line=stafflines[sl-lines_per_system+1+line]
                        y=st_line+vertical_shift
                        skeleton[line].append(Point(typerect.ul_x,y))
                        skeleton[line].append(Point(typerect.ur_x,y))

                # now construct the skeletons
                for sk in skeleton:
                    x=sk[0].x
                    y=sk[0].y
                    left_x=x
                    y_list=[]
                    i=1
                    while i<len(sk):
                        y_list.append(y)
                        if x>=sk[i].x:
                            y=sk[i].y
                            i=i+1
                        x=x+1
                    o=StafflineSkeleton()
                    o.left_x=left_x
                    o.y_list=y_list
                    staffline_skel.append(o)

        if add_noshift:
            return [im_full,im_staffonly,staffline_skel,ns_full,ns_staffonly]
        else:
            return [im_full,im_staffonly,staffline_skel]

    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class rotation(PluginFunction):
    """Rotates a music image and computes the position of the stafflines
after the rotation.

The stafflines are detected in *im_staffonly* with horizontal projections.
Calculating the skeleton positions after rotation takes into account
that the Gamera `rotate` plugin not only does a rotation, but also
pads and translates the image.

Input arguments:

 *im_staffonly*:
   image containing only the staff segments (without symbols) of the
   *self* image

 *angle*
   rotation angle (counter clockwise)

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('angle')])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self, im_staffonly, angle):
        from gamera.core import RGBPixel
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        im_full=self
        im_staffless=im_full.xor_image(im_staffonly)
        # let someone else do the work for us
        # we use -angle because rotate() uses the wrong direction
        rot_staffonly=im_staffonly.rotate(-angle,RGBPixel(0,0,0))
        rot_full=im_full.rotate(-angle,RGBPixel(0,0,0))

        def apply_transformation(mat,pt):
            return [pt[0]*mat[0]+pt[1]*mat[1]+mat[2],pt[0]*mat[3]+pt[1]*mat[4]+mat[5]]

        # find offset by which the image is moved while rotating it
        m=[ cos(angle*pi/180),sin(angle*pi/180),0,
           -sin(angle*pi/180),cos(angle*pi/180),0]
        pts_orig=[[0,0],
                  [im_full.width-1,0],
                  [0,im_full.height-1],
                  [im_full.width-1,im_full.height-1]]
        pts=[apply_transformation(m,p) for p in pts_orig]
        m[2]=-min([p[0] for p in pts])
        m[5]=-min([p[1] for p in pts])

        # find the stafflines in the original image
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)

        # rotate/move them and wrap them inside a list of StafflineSkeleton objects
        staffline_skel=[]
        for y in stafflines:
            startpt=apply_transformation(m,[first_x,y])
            endpt=apply_transformation(m,[last_x,y])
            # swap point, if the rotation was so hard, that what was left is now right (ie. angle>90 or <-90)
            if startpt[0]>endpt[0]:
                tempswap=startpt
                startpt=endpt
                endpt=tempswap
            o=StafflineSkeleton()
            o.left_x=startpt[0]
            o.y_list=[startpt[1]]
            ty=startpt[1]
            dy=(endpt[1]-startpt[1])/(endpt[0]-startpt[0])
            for n in range(int(round(endpt[0]-startpt[0]))):
                ty=ty+dy
                o.y_list.append(ty)
            staffline_skel.append(o)

        return [rot_full,rot_staffonly,staffline_skel]
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class curvature(PluginFunction):
    """Applies Gamera's *wave* plugin and computes the the position of the
stafflines after the deformation.

Input arguments:

 *im_staffonly*:
   image containing only the staff segments (without symbols) of the
   *self* image

 *ampx*:
   amplitude of the sine wave divided by the width of the staff system

 *period*:
   is the count of the half periods of to the sine wave over the
   whole staff width

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('ampx'),
                 Float('period', default=1.0)])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self, im_staffonly, ampx, period=1):
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        from gamera.plugins.deformation import wave
        im_full=self
        [first_x, last_x, stafflines, thickness] =find_stafflines_int(im_staffonly)

        periodwidth=int((last_x-first_x)*2/period)
        amp=int(ampx*periodwidth)

        # the wave-plugin does the main work
        wave_staffonly=im_staffonly.wave(amp,periodwidth,0,0,first_x)
        wave_full=im_full.wave(amp,periodwidth,0,0,first_x)

        # now we put together the new staffline skeletons
        staffline_skel=[]
        for i in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            skel.y_list=[]
            staffline_skel.append(skel)

        xfactor=2*pi/periodwidth
        yfactor=-amp*0.5
        yoffset=amp*0.5
        for x in range(last_x-first_x+1):
            y_shift=int(yoffset+yfactor*sin(x*xfactor))
            # y_shift applies to all stafflines at the same x
            for i in range(len(stafflines)):
                staffline_skel[i].y_list.append(stafflines[i]+y_shift)

        return [wave_full,wave_staffonly,staffline_skel]
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class staffline_thickness_ratio(PluginFunction):
    """Changes the ratio `staffline_height:staffspace_height` to
the given *ratio*.

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT], 'im_staffonly'), Float('ratio')])
    return_type= Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self,im_staffonly, ratio):
        from gamera.core import Image,RGBPixel,Point
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        im_full=self
        im_staffless=im_full.xor_image(im_staffonly)
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)

        # ratio=staffline_height/staffspace_height
        thickness=(stafflines[1]-stafflines[0])/(1/ratio+1)
        im_newlines=Image(im_full.ul,im_full.size)

        # we just draw the lines ourselves
        for y in stafflines:
            im_newlines.draw_line(Point(first_x,y),Point(last_x,y),RGBPixel(255,255,255),thickness)

        # new full: lines OR symbols
        def_full=im_staffless.or_image(im_newlines)
        # new staffonly: lines AND NOT symbols
        def_staffonly=im_staffless.image_copy()
        def_staffonly.invert()
        def_staffonly.and_image(im_newlines,True)
        # staffless image doesn't change
        def_staffless=im_staffless.image_copy()

        # construct skeletons
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)

        return [def_full,def_staffonly,staffline_skel]

    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class staffline_thickness_variation(PluginFunction):
    """Applies variable random thickness to the stafflines using a
Metropolis-Hastings Markov chain simulation.

The new staff lines are centered at the y-position of the former
stafflines. For every *x* value a new thickness is estamined. It
is then drawn from `y-(thickness-1)/2` to `y+(thickness-1)/2`. In
cases where this produces decimal places (for even values of
thickness), an additional shift is applied, with is reestimated
for every change of the thickness to an even value. It is randomly
generated: in half of the cases 1, or otherwise 0.

The probability to stay in the same state i.e. to stay at the same
thickness is >= *c*. Thus *c* can be considered as an inertia factor
allowing for smooth transitions. The stationary distribution of the
Markov chain is set to a binomial distribution with *p = 0.5* and
*n = max - min*.

A *random_seed* can be given for reproducable results.

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

References:
    W.K. Hastings: *Monte Carlo sampling methods using Markov
    chains and their applications*, Biometrika, 57:97-109 (1970)

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Int('min'),
                 Int('max'),
                 Float('c',default=0.5),
                 Int('random_seed', default=0)])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self,im_staffonly,min,max,c=0.5,random_seed=0):
        from gamera.core import Image,RGBPixel
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        seed(random_seed)
        im_full=self
        im_staffless=im_full.xor_image(im_staffonly)
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)
        def_staffonly=Image(im_full.ul,im_full.size)
        # state machine for the thickness
        states=states_mh(max-min+1,c)

        # draw the deformed stafflines in a blank image
        last_thickness=0
        even_shift=0
        for l in range(len(stafflines)):
            y=stafflines[l]
            for x in range(first_x,last_x+1):
                thickness=states.next()+min
                y_st=y-thickness/2
                if thickness%2==0:
                    if thickness!=last_thickness:
                        if random()<0.5:
                            even_shift=1
                        else:
                            even_shift=0
                    y_st=y_st+even_shift
                if thickness>0:
                    def_staffonly.draw_line((x,y_st),(x,y_st+thickness-1),RGBPixel(255,255,255))
                last_thickness=thickness

        # stafflines = stafflines AND NOT symbols
        im_staffless_invert=im_staffless.image_copy()
        im_staffless_invert.invert()
        def_staffonly.and_image(im_staffless_invert,True)
        # full = symbols OR stafflines
        def_full=im_staffless.image_copy()
        def_full.or_image(def_staffonly,True)

        # construct skeletons (in fact they didn't change)
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)

        return [def_full,def_staffonly,staffline_skel]
    
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class staffline_y_variation(PluginFunction):
    """Applies variable random y-value to the stafflines using a
Metropolis-Hastings Markov chain simulation.

The probability to stay in the same state i.e. to stay at the
same y-value is >= *c*. Thus *c* can be considered as an inertia factor
allowing for smooth transitions. The stationary distribution of the
Markov chain is set to a binomial distribution with *p = 0.5* and
*n = maxdiff*.

*maxdiff* is the maximum variation (i.e. *max-min+1*) of the new
y value of the line.

A *random_seed* can be given for reproducable results.

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

References:
    W.K. Hastings: *Monte Carlo sampling methods using Markov
    chains and their applications*, Biometrika, 57:97-109 (1970)

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Int('maxdiff'),
                 Float('c',default=0.5),
                 Int('random_seed', default=0)])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self,im_staffonly,maxdiff,c=0.5,random_seed=0):
        from gamera.core import Image,RGBPixel
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        seed(random_seed)
        im_full=self
        im_staffless=im_full.xor_image(im_staffonly)
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)
        def_staffonly=Image(im_full.ul,im_full.size)
        states=states_mh(2*maxdiff+1,c)

        m=(thickness-1)/2
        if (thickness and 1)==1:
            p=m
        else:
            p=m+1
        
        staffline_skel=[]
        for l in range(len(stafflines)):
            y=stafflines[l]-maxdiff
            skel=StafflineSkeleton()
            skel.left_x=first_x
            skel.y_list=(last_x-first_x+1)*[y]
            for x in range(first_x,last_x+1):
                y_l=y+states.next()
                skel.y_list[x-first_x]=y_l
                def_staffonly.draw_line((x,y_l-m),(x,y_l+p),RGBPixel(255,255,255))
            staffline_skel.append(skel)

        im_staffless_invert=im_staffless.image_copy()
        im_staffless_invert.invert()
        def_staffonly.and_image(im_staffless_invert,True)
        def_staffless=im_staffless.image_copy()
        def_full=im_staffless.image_copy()
        def_full.or_image(def_staffonly,True)

        return [def_full,def_staffonly,staffline_skel]
    
    __call__ = staticmethod(__call__)

#----------------------------------------------------------------

class staffline_interruptions(PluginFunction):
    """Applies random interruptions (with random size) to the stafflines.

Paramaters:
 *alpha*
   The probability for each pixel to be the center of an interruption

 *n*, *p*
   The n and p parameters for the binomial distribution deciding the size
   of the interruption

A *random_seed* can be given for reproducable results.

Returns a tuple *[def_full, def_staffonly, list_staffline]* with the
following components:

 *def_full*
   deformed version of the *self* image
   
 *def_staffonly*
   deformed version of *im_staffonly*

 *list_staffline*
   a list of StafflineSkeleton__ objects describing the staffline
   positions after the deformation

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.

"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'im_staffonly'),
                 Float('alpha'),
                 Int('n'),
                 Float('p', default=0.5),
                 Int('random_seed', default=0)])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self,im_staffonly,alpha,n,p=0.5,random_seed=0):
        from gamera.core import RGBPixel,FloatPoint
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        seed(random_seed)
        im_full=self
        im_staffless=im_full.xor_image(im_staffonly)
        def_staffonly=im_staffonly.image_copy()
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(im_staffonly)
        bnp=binomial(n,p)
        # walk along all pixels of all stafflines
        for y in stafflines:
            for x in range(first_x, last_x):
                # now do the russian roulette
                if random()<alpha:
                    w=bnp.random()
                    if w>0:
                        def_staffonly.draw_filled_rect(FloatPoint(x-w/2,y-thickness),FloatPoint(x+w/2+w%2-1,y+thickness),RGBPixel(0,0,0))
        def_full=im_staffless.or_image(def_staffonly)

        # construct skeletons (in fact they didn't change)
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)

        return [def_full,def_staffonly,staffline_skel]



#----------------------------------------------------------------

class find_stafflines(PluginFunction):
    """Returns a list of StafflineSkeleton__ objects describing the
stafflines in *self* image, which has contain only the stafflines of
a computer-generated image

.. __: gamera.toolkits.musicstaves.stafffinder.StafflineSkeleton.html

.. note:: Only works on computer generated images with perfectly
   horizontal staff lines.
"""
    category = "MusicStaves/Deformation"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([])
    return_type = Class('images_and_skel')
    author = "Bastian Czerwinski"
    
    def __call__(self):
        from gamera.toolkits.musicstaves.stafffinder import StafflineSkeleton
        [first_x, last_x, stafflines, thickness]=find_stafflines_int(self)

        # construct skeletons (in fact they didn't change)
        staffline_skel=[]
        for y in stafflines:
            skel=StafflineSkeleton()
            skel.left_x=first_x
            # all stafflines are completely straight
            skel.y_list=(last_x-first_x+1)*[y]
            staffline_skel.append(skel)

        return staffline_skel



class DeformationModule(PluginModule):
    cpp_headers = ["staffdeformation.hpp"]
    category = None
    functions = [degrade_kanungo_parallel, degrade_kanungo_single_image,
                 white_speckles_parallel, white_speckles_parallel_noskel,
                 typeset_emulation, rotation, curvature,
                 staffline_thickness_ratio,
                 staffline_thickness_variation,
                 staffline_y_variation,
                 staffline_interruptions,
                 find_stafflines]
    author = "Christoph Dalitz"

module = DeformationModule()

#----------------------------------------------------------------
