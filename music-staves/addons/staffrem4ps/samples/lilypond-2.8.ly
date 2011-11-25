\version "2.7.40"
\header {
	composer = "Cypriano de Rore (c. 1515-1565)"
	crossRefNumber = "1"
	footnotes = "\\\\Original clef, C on first line"
	origin = "Laura Conrad (www.laymusic.org)"
	tagline = "Lily was here 2.8.1 -- automatically converted from ABC"
	title = "Ancor che col partire â€” Cantus"
}
wordsdefaultVA = \lyricmode {
An-  cor che col par-  ti-  re Io mi sen-  ta 
mo-  ri-  re, Par-  tir vor-  rei ogn' hor, o-  gni mo-  men-  to,  Tant' 'e_il pia  
cer ch'io sen-  to, Tant' 'e_il pia-  cer ch'io sen-  to de la vi-  _   _   _ ta ch'a-    
qui-  sto nel ri-  tor-  _   _   _   _  no; E co-  sì mil-  l'e mil-  le volt' il gior-    
no, mil-  e mil-  le volt' il gior-  no Par-  tir da voi vor-  re-  i, _   
Tan-  to son dol-  ci gli ri-  tor-  _   _   _  ni mie-  _  i, E co-  sì   
mill-  e mil-  le volt' il gior-  no, mill' e mil-  le volt' il gior-  no Par tir da voi vor' re-  i, _   
Tan' to son dol' ci, gli ri-  tor-  ni  _   _   _   _   mie-  i.  
}
voicedefault =  {
\set Score.defaultBarType = "empty"

%  Copyright (C) 1999  Laura E. Conrad lconrad@world.std.com
 %  233 Broadway, Cambridge, MA 02139, USA
 %     This information is free; you can redistribute it and/or modify it
 %     under the terms of the GNU General Public License as published by
 %     the Free Software Foundation; either version 2 of the License, or
 %     (at your option) any later version.
     %     This work is distributed in the hope that it will be useful,
 %     but WITHOUT ANY WARRANTY; without even the implied warranty of
 %     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 %     GNU General Public License for more details.
     %     You should have received a copy of the GNU General Public License
 %     along with this work; if not, write to the Free Software
 %     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 \override Staff.TimeSignature #'style = #'C
 \time 2/2 % %MIDI nobarlines
 % ancient notation style
 %\clef "petrucci-c1"
  %\once \override Staff.TimeSignature #'style = #'neomensural
  %\override Voice.NoteHead #'style = #'neomensural
  %\override Voice.Rest #'style = #'neomensural
  %\override Stem #'flag-style = #'mensural
  \override Stem #'thickness = #1.0
  %\override Staff.Accidental  #'style = #'mensural
  \autoBeamOff
  \set Staff.printKeyCancellation = ##f
 % %MIDI gchord off
 \key e \phrygian   r2   a'1    g'2.    e'4    f'2    g'2    a'1    g'2    
r\breve   g'1    e'2    c''1    a'2      c''2    b'1    a'2    r2   g'2    a'2. 
   g'4    fis'2    g'2    e'2    a'1    g'2.    a'4    f'2      e'2   r2   
c''2    b'4    g'4      a'4    b'4    c''2    b'2    r2   r2   c''2    b'4    
g'4    a'4    b'4    c''2    b'2    r1   r2   a'2    d'2    a'2.    gis'8    
fis'!8    gis'!2    a'2    c''2      b'2    g'1    a'2    a'2    g'2.    fis'!8 
   e'8    fis'!4    e'4      g'\breve    r2   b'2    c''2    b'2    a'4.    
a'8    g'4    e'4    f'4    f'4    e'2      c'2    g'4.    g'8    f'4    d'4    
c'4    e'4    f'2    e'2    r2   c''2    b'2    c''2    a'2    g'2    f'1    
e'\breve    r1     g'2    fis'!4    g'4    e'2    d'2    r2   g'2 
   a'2    c''2.    b'8    a'8    b'2    e'2    a'1    gis'!2    a'1      r2 
   b'2    c''2    b'2      a'4.    a'8    g'4    e'4    f'4    f'4    e'2   
 c'2    g'4.    g'8    f'4    d'4    c'4    e'4    f'2    e'2    r2   c''2    
b'2    c''2    a'2    g'2    f'1    e'\breve     r1   g'2    
fis'!4    g'4    e'2    d'2    r2   g'2    a'2    c''1    b'2.    a'4    a'2.   
 gis'!4    g'2    a'1    gis'!\breve    \bar "|."       
}

\score{
         <<

	\context Staff="default"
	{
	    \voicedefault 
	}

	\addlyrics { 
 \wordsdefaultVA } 
    >>
	\layout {
	}
	\midi {}
}
