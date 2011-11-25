/*
 * Copyright (C) 2006 Christoph Dalitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _Evaluation_HPP_
#define _Evaluation_HPP_

#include <gamera.hpp>
#include <plugins/segmentation.hpp>

using namespace Gamera;
using namespace std;


// for distinguishing Ccs from Gstaves and Sstaves
class CcLabel {
public:
  char image; // 'G' or 'S'
  int  cclabel;
  CcLabel(char i, int c) {image = i; cclabel = c;}
  friend int operator<(const CcLabel& c1, const CcLabel& c2) { 
    if (c1.image == c2.image) return (c1.cclabel < c2.cclabel);
    else return c1.image < c2.image;
  }
};


template<class T, class U>
IntVector* segment_error(T &Gstaves, U &Sstaves) {

  ImageList* Gccs = cc_analysis(Gstaves);
  ImageList* Sccs = cc_analysis(Sstaves);
  ImageList::iterator ccs_it;
  size_t x,y;
  int classlabel, Gclasslabel, Sclasslabel;
  CcLabel Gcclabel('G',0), Scclabel('S',0), cclabel('A',0);
  map<CcLabel,int> classoflabel; // cclabel -> classlabel
  multimap<int,CcLabel> labelsofclass; // classlabel -> cclabel
  typedef multimap<int,CcLabel>::iterator mm_iterator;
  mm_iterator mmit;
  pair<mm_iterator,mm_iterator> fromto;
  vector<CcLabel> tmplabels;
  vector<CcLabel>::iterator vit;

  // check for overlaps from Gstaves
  for (ccs_it = Gccs->begin(), classlabel = 0; ccs_it != Gccs->end(); ++ccs_it, ++classlabel) {
    Gclasslabel = classlabel;
    Cc* cc = static_cast<Cc*>(*ccs_it);
    Gcclabel.cclabel = cc->label();
    classoflabel[Gcclabel] = Gclasslabel;
    labelsofclass.insert(make_pair(Gclasslabel,Gcclabel));
    for (y=0; y < cc->nrows(); y++)
      for (x=0; x < cc->ncols(); x++) {
        Scclabel.cclabel = Sstaves.get(Point(cc->ul_x() + x, cc->ul_y() + y));
        // in case of overlap:
        if (Scclabel.cclabel) {
          // check whether segment from S is new
          if (classoflabel.find(Scclabel) == classoflabel.end()) {
            classoflabel[Scclabel] = Gclasslabel;
            labelsofclass.insert(make_pair(Gclasslabel,Scclabel));
          } else {
            Sclasslabel = classoflabel[Scclabel];
            if (Sclasslabel != Gclasslabel) {
              // unite both classes, i.e. move Sclasslabel into Gclasslabel
              tmplabels.clear();
              fromto = labelsofclass.equal_range(Sclasslabel);
              for (mmit = fromto.first; mmit != fromto.second; ++mmit) {
                cclabel = mmit->second;
                classoflabel[cclabel] = Gclasslabel;
                tmplabels.push_back(cclabel);
              }
              labelsofclass.erase(Sclasslabel);
              for (vit = tmplabels.begin(); vit != tmplabels.end(); ++vit)
                labelsofclass.insert(make_pair(Gclasslabel,*vit));
            }
          }
        }
      }
  }

  // check for CCs from Sstaves without overlap (false positives)
  for (ccs_it = Sccs->begin(); ccs_it != Sccs->end(); ++ccs_it) {
    Cc* cc = static_cast<Cc*>(*ccs_it);
    Scclabel.cclabel = cc->label();
    if (classoflabel.find(Scclabel) == classoflabel.end()) {
        classlabel++;
        classoflabel[Scclabel] = classlabel;
        labelsofclass.insert(make_pair(classlabel,Scclabel));
    }
  }

  // build up class population numbers and classify error types
  int n1,n2,n3,n4,n5,n6,nG,nS;
  n1 = n2 = n3 = n4 = n5 = n6 = 0;
  for (mmit = labelsofclass.begin(); mmit != labelsofclass.end(); ) {
    nG = nS = 0;
    fromto = labelsofclass.equal_range(mmit->first);
    for (mmit = fromto.first; mmit != fromto.second; ++mmit) {
      if (mmit->second.image == 'G') nG++; else nS++;
    }
    // determine error category
    if (nG == 1 && nS == 1) n1++;
    else if (nG == 1 && nS == 0) n2++;
    else if (nG == 0 && nS == 1) n3++;
    else if (nG == 1 && nS  > 1) n4++;
    else if (nG  > 1 && nS == 1) n5++;
    else if (nG  > 1 && nS  > 1) n6++;
    else printf("Plugin segment_error: empty equivalence"
                " constructed which should not happen\n");
  }

  // free memory from CC analysis
  for (ccs_it = Gccs->begin(); ccs_it != Gccs->end(); ++ccs_it) 
	delete (*ccs_it);
  delete Gccs;
  for (ccs_it = Sccs->begin(); ccs_it != Sccs->end(); ++ccs_it) 
	delete (*ccs_it);
  delete Sccs;


  // build return value
  IntVector* errors = new IntVector();
  errors->push_back(n1); errors->push_back(n2);
  errors->push_back(n3); errors->push_back(n4);
  errors->push_back(n5); errors->push_back(n6);
  return errors;
}

// describes a link between two interruptions for interruption_error()
struct linktype {int g_node, s_node;};

// for documentation, see gamera/toolkits/musicstaves/plugins/evaluation.py
// or doc/html/musicstaves.html#interruption-error
template<class T, class U, class V>
IntVector* interruption_error(T &Gstaves, U &Sstaves, V &Skeletons) {
  vector<linktype> links;
  if(!PyList_Check(Skeletons))
    throw std::runtime_error("interruption_error: Skeletons is no list");

  int interruptions_without_link=0,removed_links=0,ground_truth_interruptions=0;
  // we inspect the images using the skeletons
  for(int i=0;i<PyList_Size(Skeletons);++i) {
    PyObject *pyob;
    PyObject *skel=PyList_GetItem(Skeletons,i);
    pyob = PyObject_GetAttrString(skel,"left_x");
    int x=PyInt_AsLong(pyob);
    Py_DECREF(pyob);
    PyObject *y_list=PyObject_GetAttrString(skel,"y_list");
    bool linked=false;
    bool G_int=false,S_int=false;
    int nodes=0,g_nodes=0,s_nodes=0;
    // now lets walk along the skeleton to find interruptions
    for(int j=0;j<PyList_Size(y_list);++j,++x) {
      int y=PyInt_AsLong(PyList_GetItem(y_list,j));
      bool G_int_old = G_int, S_int_old = S_int;
      // the staffline has an interruption, if there is no pixel from
      // [x,y-2] to [x,y+2]
      G_int=true;
      S_int=true;
      for(int dy=-2;dy<=2;++dy) {
        Point p=Point(x,y+dy);
        if(Gstaves.get(p)!=0) G_int=false;
        if(Sstaves.get(p)!=0) S_int=false;
      }
      // this is the beginning of an interruption in the Gstaves image
      if(G_int&&!G_int_old) {
        nodes++;
        g_nodes++;
      }
      // this is the beginning of an interruption in the Sstaves image
      if(S_int&&!S_int_old) {
        nodes++;
        s_nodes++;
      }
      // two simultaneous interruptions need to be linked...
      if(G_int&&S_int) {
        // ...but only if they're not already
        if(!linked) {
          linked=true;
          linktype link;
          link.g_node=g_nodes-1;
          link.s_node=s_nodes-1;
          links.insert(links.end(),link);
        }
      } else {
        linked=false;
      }
      // a set of linked (or a single not-linked) interruptions is over
      // now take a closer look
      if(((!G_int&&!S_int)||(j==PyList_Size(y_list)-1)) && nodes>0) {
        if(nodes==1) {
          interruptions_without_link++;
        } else {
          bool removed=false;
          do {
            removed=false;

            // search a link, whose g_node is linked exactly once
            for(int k=0;k<(int)links.size();++k) {
              int l;
              for(l=0;l<(int)links.size();++l) {
                if(l!=k && links[k].g_node==links[l].g_node)
                  break;
              }
              // the for loop went through ==> found such a link (at index k)
              if(l==(int)links.size()) {
                // remove all links having to do with the corresponding s_node
                for(int l=links.size()-1;l>=k;--l) {
                  if(links[k].s_node==links[l].s_node) {
                    links.erase(links.begin()+l);
                    removed_links++;
                  }
                }
                // one of the removed links is the one we accepted, so we
                // don't count it
                removed_links--;
                removed=true;
                break;
              }
            }

            // now do all the same again, just swapping G and S
            // so...

            // search a link, whose s_node is linked exactly once
            for(int k=0;k<(int)links.size();++k) {
              int l;
              for(l=0;l<(int)links.size();++l) {
                if(l!=k && links[k].s_node==links[l].s_node)
                  break;
              }
              // the for loop went through ==> found such a link (at index k)
              if(l==(int)links.size()) {
                // remove all links having to do with the corresponding g_node
                for(int l=links.size()-1;l>=k;--l) {
                  if(links[k].g_node==links[l].g_node) {
                    links.erase(links.begin()+l);
                    removed_links++;
                  }
                }
                // one of the removed links is the one we accepted, so we
                // don't count it
                removed_links--;
                removed=true;
                break;
              }
            }

          } while(removed);
          // we now should have processed all links
          if(links.size()>0)
            throw std::runtime_error("interruption_error: internal error");
        }
        ground_truth_interruptions += g_nodes;
        nodes=0;
        g_nodes=0;
        s_nodes=0;
      }
    }
    Py_DECREF(y_list);
  }
  // build return value
  IntVector* errors = new IntVector();
  errors->push_back(interruptions_without_link);
  errors->push_back(removed_links);
  errors->push_back(ground_truth_interruptions);
  return errors;
}

#endif
