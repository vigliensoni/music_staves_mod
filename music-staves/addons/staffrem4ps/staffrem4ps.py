#!/usr/bin/python
# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

##############################################################################
# staffrem4ps.py
#   - remove staff lines from postscript files generated with music software
#
# History:
#   2005-02-22  Christoph Dalitz  first creation
#   2005-02-22  Christoph Dalitz  halved memory footprint
#                                 added support for ps2ps
#   2005-02-23  Christoph Dalitz  ps2ps: ignored lines with trailing commas
#                                 added support for acroread
#   2005-03-16  Christoph Dalitz  fixed file open bug
#   2006-01-19  Christoph Dalitz  added support for Wayne Cripps' tab
#   2006-08-17  Christoph Dalitz  added support for lilypond 2.8
#                                 acroread renamed to arobat
#   2006-08-21  Christoph Dalitz  added option -m for marking staffs in red
#                                 gzip compressed infiles now possible
#   2006-09-19  Christoph Dalitz  added support for musixtex
#   2006-09-21  Christoph Dalitz  fixed detection of musixtex/lilypond-2.2
##############################################################################

import sys
import os
import re

# supported creators as [name, PSCreatorCode, homepage URL, -m supported]
creators = [ \
#	["abc2ps", "abc2ps", "http://www.ihp-ffo.de/~msm/", "yes"], \ # (untested)
	["abcm2ps", "abcm2ps", "http://moinejf.free.fr/", "yes"], \
	["abctab2ps", "abctab2ps", "http://www.lautengesellschaft.de/cdmm/", "yes"], \
	["lilypond-2.2", "does not set DSC comment", "http://www.lilypond.org/web/", "yes"], \
	["lilypond-2.8", "LilyPond 2\.8", "http://www.lilypond.org/web/", "yes"], \
	["musixtex", "does not set DSC comment", "http://www.ctan.org/tex-archive/macros/musixtex/taupin/", "yes"], \
	["mup", "Mup", "http://www.arkkra.com/", "yes"], \
	["pmw", "Philip's Music", "http://www.quercite.com/pmw.html", "yes"], \
	["tab", "tab, ", "http://www.cs.dartmouth.edu/~wbc/lute/AboutTab.html", "yes"], \
	["ps2ps", "Ghostscript.*(pswrite)", "http://www.cs.wisc.edu/~ghost/", "yes"], \
	["acrobat", "PScript.*\.dll|PSCRIPT\.DRV|PDFCreator ", "Acrobat Distiller", "yes"] \
]

#
# parse command line arguments
#
class c_opt:
    infile = ""
    outfile = ""
    creator = ""
    markred = False
    compressed = False
    def error_exit(self):
        usage = "Usage:\n" + \
                "\t" + sys.argv[0] + " [options] <infile>\n" + \
                "Options:\n" + \
                "\t-o outfile output file name. When omitted, the\n" + \
                "\t           infile stem gets the extension '-nostaff'\n" + \
                "\t           (or '-redstaff' for option -m)\n" + \
                "\t-c creator creater of the postscript file.\n" + \
                "\t           Use '-h' for a list of supported creators\n" + \
                "\t           When omitted, the value is guessed from the\n" + \
                "\t-m         mark staffs in red, but do not remove them\n" + \
                "\t-z         infile is compressed with gzip\n" + \
                "\t           (automatically set when infile ends with \".gz\")\n" + \
                "\t-h         print supported creators and exit\n"
        sys.stderr.write(usage)
        sys.exit(1)
    def supported_creators(self, clist):
        ret = sys.argv[0] + \
              " supported creators as options for '-c':\n\n" + \
              "\t%-15s (homepage)\n" % "(name)"
        for c in clist:
            ret += ("\t%-15s " % c[0]) + c[2] + "\n"
        ret += "\nFor other creators try to process the PS or PDF file\n" + \
               "with ps2ps or pdf2ps and then use the creator option ps2ps\n"
        return ret
opt = c_opt()

# parse command line
i = 1
while i < len(sys.argv):
    if sys.argv[i] == "-o":
        i += 1; opt.outfile = sys.argv[i]
    elif sys.argv[i] == '-c':
        i += 1; opt.creator = sys.argv[i]
        knowncreator = 0
        for c in creators:
            if c[0] == opt.creator:
                knowncreator = 1
                break
        if not knowncreator:
            sys.stderr.write("Unsupported creator '" + opt.creator + "'\n")
            sys.stderr.write(opt.supported_creators(creators))
            sys.exit(1)
    elif sys.argv[i] == '-m':
        opt.markred = True
    elif sys.argv[i] == '-z':
        opt.compressed = True
    elif sys.argv[i] == '-h':
        print opt.supported_creators(creators)
        sys.exit(0)
    elif sys.argv[i][0] == '-':
        opt.error_exit()
    else:
        opt.infile = sys.argv[i]
    i += 1

if (opt.infile == "") or not os.path.isfile(opt.infile):
    sys.stderr.write("File '" + opt.infile + "' not found.\n")
    opt.error_exit()
if (opt.outfile == ""):
    if opt.markred:
        postfix = "-redstaff"
    else:
        postfix = "-nostaff"
    segs = opt.infile.split(".")
    if len(segs) > 1:
        if segs[-1] == "gz":
            del segs[-1]
        segs[-2] += postfix
        opt.outfile = ".".join(segs)
    else:
        opt.outfile = opt.infile + postfix

# try to autodetect whether infile is compressed
if opt.infile.endswith(".gz"):
    opt.compressed = True

# read entire file into memory
# this is not strictly necessary for all creators, but simplifies things
if opt.compressed:
    fin = os.popen("gunzip -c '" + opt.infile + "'", 'r')
else:
    fin = open(opt.infile,'r')
lines = fin.readlines()
fin.close()

# try to guess creator
if (opt.creator == ""):
    couldbelilypond22 = 0
    couldbemusixtex = 0
    for x in lines:
        if x.startswith("%%Creator:"):
            for c in creators:
                if re.search(c[1],x[10:]):
                    opt.creator = c[0]
                    break
                elif re.search("dvips",x[10:]):
                    couldbelilypond22 = 1
                    couldbemusixtex = 1
                    break
            if (opt.creator != ""):
                break
        if couldbelilypond22 and \
           (x.startswith("/lyscale") or x.startswith("/lilypond")):
            opt.creator = "lilypond-2.2"
            break
        if couldbemusixtex and x.startswith("%DVIPSSource:  TeX"):
            opt.creator = "musixtex"
            #break # do not break, because it could still be lilypond22
    if (opt.creator == ""):
        sys.stderr.write("Cannot identify PS creator. Use option '-f'\n")
        sys.stderr.write(opt.supported_creators(creators))
        sys.exit(2)
    else:
        sys.stdout.write("Creator autoguessed as " + opt.creator + "\n")

# when option -m, check whether creator supports this
if opt.markred:
    for c in creators:
        if opt.creator == c[0] and c[3] == "no":
            sys.stderr.write("Option -m not supported for creator " + \
                             opt.creator + "\n")
            sys.exit(1)

# file for modified output
print "Write output to " + opt.outfile
fout = open(opt.outfile,'w')

# abc*2ps, mup
# replace /staff macro with empty code
if (opt.creator == "abc2ps") or \
       (opt.creator == "abcm2ps") or \
       (opt.creator == "abctab2ps") or \
       (opt.creator == "mup"): 
    instaffbraces = 0  # indicator whether we are inside /staff macro
    firstbracefound = 0
    for x in lines:
        if instaffbraces == 0:
            if not (x.startswith("/staff") or x.startswith("/tabN")):
                fout.write(x)
            else:
                if not opt.markred:
                    if x.startswith("/staff"):
                        if (opt.creator == "abcm2ps"):
                            fout.write("/staff {pop pop pop} ")
                        elif (opt.creator == "mup"):
                            fout.write("/staff {pop pop pop pop pop pop} ")
                        else: # abc2ps or abctab2ps
                            fout.write("/staff {pop} ")
                    else:
                        # (only in abctab2ps)
                        fout.write("/tabN {pop pop} ")
                else: # opt.markred
                    fout.write(re.sub("/(staff|tabN)(\\s*\\{)","/\\1\\2 1 0 0 setrgbcolor ", x))
                # overread everything between the next braces
                instaffbraces = 1
                string4print = ""
                for c in x[6:]:
                    cisfirstbrace = False
                    if instaffbraces == 0:
                        string4print += c
                    else:
                        if (c == '{'):
                            instaffbraces += 1
                            if not firstbracefound:
                                firstbracefound = 1
                                cisfirstbrace = True
                        elif (c == '}'):
                            instaffbraces -= 1
                        if (firstbracefound) and (instaffbraces == 1):
                            instaffbraces = 0
                            firstbracefound = 0
                            if opt.markred:
                                string4print += " 0 setgray "
                        if opt.markred and not cisfirstbrace:
                            string4print += c
                fout.write(string4print)
        else: # instaffbraces
            string4print = ""
            for c in x:
                cisfirstbrace = False
                if instaffbraces == 0:
                    string4print += c
                else:
                    if (c == '{'):
                        instaffbraces += 1
                        if not firstbracefound:
                            firstbracefound = 1
                            cisfirstbrace = True
                    elif (c == '}'):
                        instaffbraces -= 1
                    if (firstbracefound) and (instaffbraces == 1):
                        instaffbraces = 0
                        firstbracefound = 0
                        if opt.markred:
                            string4print += " 0 setgray "
                    if opt.markred and not cisfirstbrace:
                        string4print += c
            fout.write(string4print)


# lilypond 2.2 output is created by dvips and not very pretty
# look for v|V followed by V macros and remove it
tokenlist = []
if (opt.creator == "lilypond-2.2"):
    inpsbody = 0
    for x in lines:
        if not inpsbody:
            fout.write(x)
            if x.startswith("%%EndSetup"):
                inpsbody = 1
        else:
            # dvips creates random line breaks
            # thus we need to store tokens across line breaks
            tokenlist += x.split(" ")
    # look for x1 x2 x3 x4 v y1 y2 V ... patterns
    range2delete = {} # indices to delete are stored as keys
    for i, tok in enumerate(tokenlist):
        if tok == "v" or tok == "v\n" or tok == "V" or tok == "V\n":
            if tokenlist[i+3] == "V" or tokenlist[i+3] == "V\n":
                # mark everything until last V for removal
                j = i + 6
                while tokenlist[j] == "V" or tokenlist[j] == "V\n":
                    j += 3
                if tok[0] == "v":
                    start = i-4
                else: # tok[0] == "V"
                    start = i-2
                for k in range(start,j-2):
                    range2delete[k] = "x"

    # rejoin tokenlist without marked patterns
    for i, tok in enumerate(tokenlist):
        if not range2delete.has_key(i):
            fout.write(tok)
            if tok and tok[-1] != "\n":
                fout.write(" ")
        elif opt.markred:
            if not range2delete.has_key(i-1):
                fout.write("1 0 0 setrgbcolor ")
            fout.write(tok)
            fout.write(" ")
            if not range2delete.has_key(i+1):
                fout.write("0 setgray ")

# lilypond 2.8
# remove more than three consecutive horzontal lines
# with same right end point
if (opt.creator == "lilypond-2.8"):
    re_hline = re.compile("[0-9] lineto stroke$")
    minlines = 4
    tolerance = 3
    lastlineindex = 0
    lastwidth = ""
    numequal = 0
    outbuf_all = ""
    outbuf_nol = ""
    for i, x in enumerate(lines):
        xstripped = x.strip()
        m = re_hline.search(xstripped)
        if m:
            lastlineindex = i
            thiswidth = xstripped.split(" ")[-4]
            height = float(xstripped.split(" ")[-3])
            if (numequal == 0 or lastwidth == thiswidth) and height == 0.0:
                numequal += 1
                outbuf_all += x
                if opt.markred:
                    outbuf_nol += "1 0 0 setrgbcolor " + \
                                  xstripped + " 0 setgray\n"
            else:
                if numequal < minlines:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
                outbuf_all = x
                if opt.markred:
                    outbuf_nol = "1 0 0 setrgbcolor " + \
                                  xstripped + " 0 setgray\n"
                else:
                    outbuf_nol = ""
                numequal = 1
            lastwidth = thiswidth
        else:
            if i - lastlineindex > tolerance:
                if numequal < minlines:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
                outbuf_all = x
                outbuf_nol = x
                lastwidth = ""
                numequal = 0
            else:
                outbuf_all += x
                outbuf_nol += x
    # do not forget to print last content of buffer
    if outbuf_all:
        if numequal < minlines:
            fout.write(outbuf_all)
        else:
            fout.write(outbuf_nol)

# musixtex output is created by dvips and not very pretty
# look for totals of 5 v|V macros and remove them
tokenlist = []
if (opt.creator == "musixtex"):
    inpsbody = 0
    for x in lines:
        if not inpsbody:
            fout.write(x)
            if x.startswith("%%EndSetup"):
                inpsbody = 1
        else:
            # dvips creates random line breaks
            # thus we need to store tokens across line breaks
            tokenlist += x.split(" ")
    # look for x1 x2 x3 x4 v (y1 y2 V){4} patterns
    range2delete = {} # indices to delete are stored as keys
    for i, tok in enumerate(tokenlist):
        if tok == "v" or tok == "v\n":
            if (tokenlist[i+3] == "V" or tokenlist[i+3] == "V\n") and \
               (tokenlist[i+6] == "V" or tokenlist[i+6] == "V\n") and \
               (tokenlist[i+9] == "V" or tokenlist[i+9] == "V\n") and \
               (tokenlist[i+12] == "V" or tokenlist[i+12] == "V\n") and \
               (tokenlist[i+15] != "V" and tokenlist[i+15] != "V\n"):
                # mark everything until last V for removal
                for k in range(i-4,i+13):
                    range2delete[k] = "x"

    # rejoin tokenlist without marked patterns
    for i, tok in enumerate(tokenlist):
        if not range2delete.has_key(i):
            fout.write(tok)
            if tok and tok[-1] != "\n":
                fout.write(" ")
        elif opt.markred:
            if not range2delete.has_key(i-1):
                fout.write("1 0 0 setrgbcolor ")
            fout.write(tok)
            fout.write(" ")
            if not range2delete.has_key(i+1):
                fout.write("0 setgray ")

# pmw
# remove all lines beginning with (FFF ie. staff lines wider than 100pt
if (opt.creator == "pmw"):
    for x in lines:
        if not x.startswith("(FFF"):
            fout.write(x)
        elif opt.markred:
            fout.write("1 0 0 setrgbcolor " + x + "0 setgray\n")

# tab
# remove more than four consecutive lines ending with Rule,
# start with the same number and are seperated by at most
# one line ending with RM
if (opt.creator == "tab"):
    re_ruleline = re.compile("\sRule$")
    re_rmline = re.compile("\sRM$")
    minlines = 4
    tolerance = 1
    lastllineindex = 0
    lastnum = ""
    numequal = 1
    outbuf_all = ""
    outbuf_nol = ""
    for i, x in enumerate(lines):
        xstripped = x.strip()
        m = re_ruleline.search(xstripped)
        if m:
            lastllineindex = i
            thisnum = xstripped.split(" ")[0]
            if lastnum == thisnum:
                numequal += 1
                outbuf_all += x
                if opt.markred:
                    outbuf_nol += "1 0 0 setrgbcolor " + \
                                  xstripped + " 0 setgray\n"
            else:
                if numequal < minlines:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
                outbuf_all = x
                if opt.markred:
                    outbuf_nol = "1 0 0 setrgbcolor " + \
                                  xstripped + " 0 setgray\n"
                else:
                    outbuf_nol = ""
                numequal = 1
                lastnum = thisnum
        else:
            m = re_rmline.search(xstripped)
            if not m or i - lastllineindex > tolerance:
                if numequal < minlines:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
                outbuf_all = x
                outbuf_nol = ""
                numequal = 1
                lastnum = ""
            else:
                outbuf_all += x
                outbuf_nol += x
    # do not forget to print last content of buffer
    if outbuf_all:
        if numequal < minlines:
            fout.write(outbuf_all)
        else:
            fout.write(outbuf_nol)


# ps2ps
# remove groups of at least five lines starting with the same number,
# but do not end with a comma (what is the comma good for?)
# (WARNING: this is not perfect, but seems to work in many cases)
if (opt.creator == "ps2ps"):
    re_numberline = re.compile("^-?[0-9]*\.?[0-9]+ .*[^,]$")
    lastnum = ""
    numequal = 1
    outbuf = ""
    for i, x in enumerate(lines):
        xstripped = x.strip()
        m = re_numberline.search(xstripped)
        if m:
            thisnum = xstripped.split(" ")[0]
            if lastnum == thisnum:
                numequal += 1
                outbuf += x
            else:
                if numequal < 5:
                    fout.write(outbuf)
                elif opt.markred:
                    fout.write("1 0 0 setrgbcolor\n")
                    fout.write(outbuf)
                    fout.write("0 setgray\n")
                outbuf = x
                numequal = 1
                lastnum = thisnum
        else:
            if outbuf and (numequal < 5):
                fout.write(outbuf)
            elif outbuf and opt.markred:
                fout.write("1 0 0 setrgbcolor\n")
                fout.write(outbuf)
                fout.write("0 setgray\n")
            outbuf = ""
            numequal = 1
            lastnum = ""
            fout.write(x)

# acroread
# lineto is mapped to l, so we look for consecutive lines ending with
# l and starting with the same number. As these can be interrupted by m
# lines, we need to ignore up to some tolerance intermitting non l lines
# moreover we need to keep l lines that are close to each other
# (almost same end point), because these cannot be staff lines
# (but possibly beams, hairpins etc.)
if (opt.creator == "acrobat"):
    re_lline = re.compile("^-?[0-9]*\.?[0-9]+ -?[0-9]*\.?[0-9]+ l$")
    tolerance = 6
    lastllineindex = 0
    lastxnum = ""
    lastynum = ""
    numequal = 1
    outbuf_all = ""
    outbuf_nol = ""
    for i, x in enumerate(lines):
        xstripped = x.strip()
        m = re_lline.search(xstripped)
        if m:
            lastllineindex = i
            thisxnum = xstripped.split(" ")[0]
            thisynum = xstripped.split(" ")[1]
            if lastxnum == thisxnum and \
                   abs(float(lastynum) - float(thisynum)) > 1.0:
                numequal += 1
                outbuf_all += x
                if opt.markred:
                    outbuf_nol += "1 0 0 setrgbcolor " + \
                                  xstripped + " stroke newpath 0 setgray\n"
            else:
                if numequal < 5:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
                outbuf_all = x
                if opt.markred:
                    outbuf_nol = "1 0 0 setrgbcolor " + \
                                  xstripped + " stroke newpath 0 setgray\n"
                else:
                    outbuf_nol = ""
                numequal = 1
                lastxnum = thisxnum
                lastynum = thisynum
        elif i - lastllineindex < tolerance:
            outbuf_all += x
            outbuf_nol += x
        else:
            if outbuf_all:
                if numequal < 5:
                    fout.write(outbuf_all)
                else:
                    fout.write(outbuf_nol)
            outbuf_all = ""
            outbuf_nol = ""
            numequal = 1
            lastxnum = ""
            lastynum = ""
            lastllineindex = i
            fout.write(x)
    # do not forget to print last content of buffer
    if outbuf_all:
        fout.write(outbuf_all)


# cleanup
fout.close()

