/*
 *
 * Copyright (C) 2000-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef mgd01042005_findandremovestaves
#define mgd01042005_findandremovestaves

#include <math.h> // for tan

#include <gamera.hpp>
#include <plugins/logical.hpp>
#include <plugins/segmentation.hpp>
#include <plugins/image_utilities.hpp>
#include <plugins/runlength.hpp>
#include <plugins/projections.hpp>
#include <plugins/morphology.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
// DEBUGGING OUTPUT

#ifdef M_DEBUG
#include <iostream>
#include <plugins/tiff_support.hpp>
#define debug_message(a) std::cerr << a << std::endl;
inline void print_rect(char* msg, Rect& rect) {
  debug_message(msg << " <Rect " << rect.ul_x() << " " << rect.ul_y() << 
		" " << rect.lr_x() << " " << rect.lr_y() << ">");
}
inline void print_array(char* msg, const IntVector* array) {
  std::cerr << msg << " (length " << array->size() << ") ";
  for (size_t i = 0; i < array->size(); ++i)
    std::cerr << (*array)[i] << " ";
  std::cerr << std::endl;
}
#else
#define debug_message(a)
inline void print_rect(char* msg, Rect& rect) {}
inline void print_array(char* msg, IntVector* array) {}
#endif

namespace Aomr {

  typedef TypeIdImageFactory<ONEBIT, DENSE>::data_type data_type;
  typedef TypeIdImageFactory<ONEBIT, DENSE>::image_type view_type;

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // BASIC DATA STRUCTURES
  
  // Staff objects store information about a single staff.
  // The line weight is assumed to be the same for all lines.
  class Staff : public Rect {
  public:
    Staff() : staffline_h(-1.0) {};
    IntVector staffline_pos;
    inline size_t n_stafflines() { return staffline_pos.size(); }
    double staffline_h;
  };
  typedef std::vector<Staff> StaffVector;

  // Page objects store a page full of staves and keep track
  // of the total number of stafflines on the page
  class Page {
  public:
    Page() : total_stafflines(0) {};
    void add_staff(Staff& staff) {
      staves.push_back(staff);
      total_stafflines += staff.n_stafflines();
    }
    size_t total_stafflines;
    StaffVector staves;
  };

  template<class T> void deskew(T&, IntVector *, int);
  struct DeskewData {
    IntVector offsets;
    Rect rect;
    template<class T> void undo_deskew(T& image, int width)
    {
      if(rect.width()==0)
        deskew(image, &offsets, width);
      else
      {
        view_type subimage(image, rect);
        deskew(subimage, &offsets, width);
      }
    }
  };

  // Param objects simply encapsulate any parameters to the algorithm as a whole.
  // This makes it easier to pass all the different parameters around.
  class Param {
  public:
    Param(size_t crossing_symbols_ = 0, size_t n_stafflines_ = 0, double staffline_h_ = 0, 
	  double staffspace_h_ = 0.0, size_t skew_strip_width_ = 0, double max_skew_ = 8.0, 
	  bool deskew_only_ = false, bool find_only_ = false, bool undo_deskew_ = false) {
      n_stafflines = n_stafflines_;
      
      if (crossing_symbols_ > 2)
	throw std::range_error("Crossing symbols must be in range 0-2.");
      crossing_symbols = crossing_symbols_;
      
      staffline_h = staffline_h_;
      staffspace_h = staffspace_h_;
      skew_strip_width = skew_strip_width_;
      max_skew = max_skew_;
      deskew_only = deskew_only_;
      find_only = find_only_;
      undo_deskew = undo_deskew_;
    }
    size_t crossing_symbols;
    size_t n_stafflines;
    double staffline_h;
    double staffspace_h;
    size_t skew_strip_width;
    double max_skew;
    bool deskew_only;
    bool find_only;
    bool undo_deskew;
    vector<DeskewData> deskew_data;
    void add_deskew_info(IntVector *offsets, Rect rect)
    {
      if(!undo_deskew)
        return;
      DeskewData dd;
      for(IntVector::iterator i=offsets->begin();i<offsets->end();++i)
        dd.offsets.push_back(-*i);
      dd.rect=rect;
      deskew_data.push_back(dd);
    }
    void add_deskew_info(IntVector *offsets)
    {
      if(!undo_deskew)
        return;
      Rect rect;
      rect.offset_x(0);
      rect.offset_y(0);
      rect.width(0);
      rect.height(0);
      add_deskew_info(offsets,rect);
    }
    template<class T> void undo_deskews(T& image)
    {
      if(!undo_deskew)
        return;
      for(typeof(deskew_data.rend()) i=deskew_data.rbegin();i<deskew_data.rend();++i)
        i->undo_deskew(image, skew_strip_width);
    }
  };

  struct Peak {
    size_t index;
    int value;
  };

  typedef std::vector<Peak> PeakVector;

  typedef std::vector<Rect> RectVector;

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // UTILITY FUNCTIONS

  bool _peak_comparator(const Peak& a, const Peak& b) { return a.value > b.value; }

  // Sort peak indices by their values in descending order
  //   indices: a set of indexes into the *values*
  //   values: a set of values references by *indicies*
  // The indices are sorted in place
  void peaks_sort(IntVector* indicies, IntVector* values) {
    PeakVector peaks(indicies->size());
    
    PeakVector::iterator p_it = peaks.begin();
    IntVector::iterator i_it = indicies->begin();		
    for (; p_it != peaks.end(); ++p_it, ++i_it) {
      if (*i_it > (int)values->size())
	throw std::runtime_error("Indices are out of range");
      (*p_it).index = *i_it;
      (*p_it).value = (*values)[*i_it];
    }

    std::stable_sort(peaks.begin(), peaks.end(), &_peak_comparator);
    
    p_it = peaks.begin();
    i_it = indicies->begin();
    for (; p_it != peaks.end(); ++p_it, ++i_it)
      (*i_it) = (*p_it).index;
  }

  // Returns the sum of an IntVector
  inline int array_sum(IntVector* array) {
    int total = 0;
    for (IntVector::iterator i = array->begin(); i != array->end(); i++)
      total += *i;
    return total;
  }

  // Adds the elements of proj2 into proj1
  // offset is proj2's position relative to proj1, i.e. offset > 0 if p2 is lower than p1
  // proj1[0] += proj2[offset], ..., proj1[n - 1 - offset] += proj2[n - 1];
  // if offset < 0 proj1[offset] += proj2[0], ..., proj1[n - 1] += proj2[n - 1 - offset];
  // proj1 is modified
  void array_add(IntVector* proj1, IntVector* proj2, int offset) {
    if (proj1->size() != proj2->size())
	throw std::runtime_error("array_add arrays are of different size");
		
    int n = std::min(proj1->size(), proj2->size());
    if (offset >= 0)
      for (int i = 0, j = offset; j < n; i++, j++)
	(*proj1)[i] += (*proj2)[j];
    else
      for (int i = -offset, j = 0; i < n; i++, j++)
	(*proj1)[i] += (*proj2)[j];
  }

  // Performs the "filtering" operation in-place
  // This seems to be a "smoothing" operation
  // QUESTION: What exactly is happening here?
  void array_filter(IntVector* array, int wid) {
    int total = 0;
    for (int j = 0; j < wid; j++)
      total += (*array)[j];

    int temp = (*array)[0];
    (*array)[0] = total / wid;
    for (int i = 1, j = wid; i < (int)array->size() - wid - 1; i++, j++) {
      total -= temp;
      total += (*array)[j];
      temp = (*array)[i];
      (*array)[i] = total / wid;
    }

    // QUESTION: This doesn't seem to filter the whole array.  Is that a problem?
  }

  // Calculates the derivative of a subarray [begin, end) and returns the result in a newly-
  // allocated IntVector
  IntVector* array_deriv(IntVector::iterator begin, IntVector::iterator end) {
    size_t size = end - begin;
    IntVector* deriv = new IntVector(size, 0);

    IntVector::iterator d, pd, d1, d2;
    d1 = begin;
    d2 = begin + 2;
    d = deriv->begin();

    for (pd = d + 1; pd != deriv->end() - 1; pd++, d2++, d1++)
      *pd = (*d2 - *d1) / 2;
    // Handle the edge cases
    d[0] = d[1];
    d[size - 1] = d[size - 2];
    return deriv;
  }

  // Finds all the local maxima in subarray [begin, end) and places the results
  // in deriv (which must already be allocated), and returns the number of maxima.
  // Maxima with values below *min* threshold are ignored.
  // (was 'array_maximum')
  // MGD TODO: The interface to this function is really kludgy.
  int array_local_maxima(IntVector* deriv, IntVector::iterator begin, const IntVector::iterator end, 
			 int min) {
    int dd;
    int i, j = 0;
    IntVector::iterator data = begin;
    IntVector::iterator pdd = deriv->begin();
    int size = end - begin;
    IntVector maxi(size);
		
    dd = *pdd++;
    for (i = 1; i < size - 1 && j < size; i++) {
      if (*pdd++ > 0 && *pdd <= 0 
	  && (data[i] > min || data[i - 1] > min || data[i + 1] > min))
	{
	  if (data[i] >= data[i - 1])
	    {
	      if (data[i] >= data[i + 1])
		maxi[j++] = i;
	      else
		maxi[j++] = i + 1;
	    }
	  else
	    {
	      if (data[i + 1] > data[i - 1])
		maxi[j++] = i + 1;
	      else
		maxi[j++] = i - 1;
	    }
	}
    }
    pdd = deriv->begin();
    for (i = 0; i < j; i++)
      pdd[i] = maxi[i];
    deriv->resize(j);
    return j;
  }

  // Returns the average value of an IntVector range [begin, end)
  double array_avg(IntVector::iterator begin, const IntVector::iterator end) {
    int total = 0;
    int count = 0;
		
    for (; begin != end; ++begin)
      if (*begin != 0) {
	  total += *begin;
	  count++;
	}
    if (count == 0)
      return(0.);
    return((double)total / (double)count);
  }

  // cross-correlates two sets of projections
  // QUESTION: This code is a bit of a mystery to me
  int cross_correlate(IntVector* proj1, IntVector* proj2, int offset, int max_shift) {
    if (proj1->size() != proj2->size())
      throw std::length_error("cross_correlate proj1.size != proj2.size");

    int i, j;
    IntVector::iterator old = proj1->begin();
    IntVector::iterator new_data = proj2->begin();
    int n = int(proj1->size());
    IntVector::iterator new_limit;
    int size = max_shift * 2 + 1;

    IntVector total(size);

    if (offset >= 0) {
      new_limit = new_data + (n - max_shift - offset - 1);
      new_data += max_shift + offset;
    } else {
      // This looks a lot scarier than it is.  If -offset > proj1.size(), then
      // new_limit ends up being < new_data, so the loop is never run anyway.
      old -= offset;
      new_limit = new_data + (n - max_shift + offset - 1);
      new_data += max_shift;
    }

    for (; new_data < new_limit; old++, new_data++) {
      if (*new_data != 0) {
	IntVector::iterator pold = old;
	int newv = *new_data;
				
	for (IntVector::iterator ptot = total.begin(); ptot < total.end(); pold++, ptot++) {
	  if (*pold != 0)
	    *ptot += *pold * newv;
	}
      }
    }
    
    j = (std::max_element(total.begin(), total.end()) - total.begin()) - max_shift;
    old = proj1->begin();

    if (j == max_shift || -j == max_shift) {	/* ignore */
      return (offset);
    }
    for (i = 0; i < size; i++)
      if (total[i] != 0)
	break;
    if (i == size)
      j = 0;
    return (-j + offset);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // STAFF LINE FINDING AND DESKEWING

  // Returns true if the rect is so small that it is very unlikely to contain a staff
  // The threshold is based on the passed in staffspace_h
  inline bool rect_too_small_to_be_staff(Param& param, Rect& rect, double staffspace_h) {
    // was: return (rect.ncols() < 200 || rect.nrows() < 20);
    return (rect.ncols() < staffspace_h * param.n_stafflines * 2 ||
	    rect.nrows() < staffspace_h * (param.n_stafflines / 2.0));
  }

  inline bool is_staffline_and_staffspace_reasonable(double staffline_h, double staffspace_h) {
    return !(staffline_h > staffspace_h / 2.0 || staffspace_h <= 6.0 || staffline_h < 1.0);
  }

  // Finds a rough value for staffline_h and staffspace_h
  // The results are returned by reference
  template<class T>
  void find_rough_staffline_and_staffspace_height(T& original, double& staffline_h, double& staffspace_h) {
    /*
      staffline_h is the height of each line of the staff.
      staffspace_h is the space between staff lines.
      Throughout the algorithm these parameters are calculated with
      increasing accuracy (by looking at more and more local features). Here
      we make a rough estimate by looking at the histogram of vertical image.
      Th2e staffline_h is the most frequent black vertical run and the
      staffspace_h is the most frequen white vertical run.
    */
    debug_message("find_rough_staffline_and_staffspace_height");
    staffline_h = (double) most_frequent_run(original, runs::Black(), runs::Vertical());
    staffspace_h = (double) most_frequent_run(original, runs::White(), runs::Vertical());
    // MGD: This was (staffline_h > staffspace_h), but this was changed because
    // some images with lots of hashing patterns would get misrecognized as
    // staves.
    debug_message("staffline_h: " << staffline_h << " staffspace_h: " << staffspace_h);
    if (!is_staffline_and_staffspace_reasonable(staffline_h, staffspace_h)) {
      char temp[256];
      sprintf(temp, "Staffline height (%.02f) and staffspace height (%.02f) are not reasonable.", staffline_h, staffspace_h);
      throw std::runtime_error(temp);
    }
  }

  // Creates the basic images that will be used throughout
  // The resulting images *original* and *image_hfilter* are returned by reference
  template<class T, class U>
  void basic_filtering_step(T& original, U& image_hfilter,
			    double staffline_h, double staffspace_h) {
    debug_message("basic_filtering_step");

    debug_message("despeckling");
    despeckle(original, std::max(5, (int)(staffline_h)));

    // filter vertically thick things
    debug_message("filtering tall runs");
    image_copy_fill(original, image_hfilter);
    filter_tall_runs(image_hfilter, int((staffline_h * 2)), runs::Black());

    // filter horizontally short things using connected-components - this is so that
    // this will work if the image is skewed.
    debug_message("filtering horizontally short things");

    typedef typename ImageFactory<T>::ccs_type ccs_type;
    ccs_type* cc;
    cc = (ccs_type*)cc_analysis(image_hfilter);
    ccs::filter_narrow(*cc, (int)staffspace_h);
    delete_connected_components(cc);

    /* ccs_filter_tall() can remove tall things, (hairpins, slurs) 
     * but it's not a good idea to do this now because the page may be 
     * crooked so that as a connected component, a staffline may have a 
     * considerable height. We'll remove the tall things (slurs, wedges) 
     * _after_ we straighten the page a bit.
     */
  }
  
  // find_skew finds the amount of skew in an image in strips of a given
  // width. (MGD: was runs_shear)
  // Returns the offsets necessary to deskew in a new IntVector*.  Each offset represents
  // a strip *wid* pixels wide.
  // Also returns the y-projections of the image by reference
  // *max_offset* is the maximum number of pixels difference between each adjacent offset
  // This keeps the image from being horrifically deskewed, and also places an upper limit
  // on runtime.
  template<class T>
  inline void _find_skew_step(T& image, IntVector* last_yproj, IntVector* offsets, int i, 
			      int last_i, int wid, int max_offset) {
    IntVector* typroj = yproj_vertical_strip(image, i * wid, wid);
    try {
      if (array_sum(typroj) < wid) {
	(*offsets)[i] = (*offsets)[last_i];
      } else {
	(*offsets)[i] = cross_correlate(last_yproj, typroj, (*offsets)[last_i], max_offset);
      }
      array_add(last_yproj, typroj, (*offsets)[i]);
    } catch (std::exception) {
      delete typroj;
      throw;
    }
    delete typroj;
  }

  template<class T>
  IntVector *find_skew(T& image, IntVector** yproj, int wid, int max_offset)
  {
    debug_message("find_skew");
    
    IntVector* offsets = new IntVector((image.ncols() + wid - 1) / wid + 1);

    int halfway = image.ncols() / 2 / wid;
    IntVector* last_yproj = yproj_vertical_strip(image, halfway * wid, wid);

    try {
      (*offsets)[halfway] = 0;
		
      // Right half
      int i;
      for (i = halfway + 1; i < (int)image.ncols() / wid; i++)
	_find_skew_step(image, last_yproj, offsets, i, i - 1, wid, max_offset);

      // Right edge
      int last_offset = (*offsets)[i - 1];
      for (; i < (int)offsets->size(); i++)
	(*offsets)[i] = last_offset;

      // Left half
      for (i = halfway - 1; i >= 0; i--)
	_find_skew_step(image, last_yproj, offsets, i, i + 1, wid, max_offset);
    } catch (std::exception) {
      delete last_yproj;
      delete offsets;
      throw;
    }
    *yproj = last_yproj;
    return offsets;
  }

  // This function takes an IntVector of shear distances, and deskews the image (in place)
  // precondition: offsets->size() == image.ncols() / width
  // MGD: was "runs_adjust"
  template<class T>
  void deskew(T& image, IntVector *offsets, int width) {
    debug_message("deskew");
    typename IntVector::iterator offset = offsets->begin();

    for (size_t col = 0; col < image.ncols(); col++) {
      double inc;   /* interpolation inc per pixel */
      int distance; /* offset value after interpolation for the column */
      size_t index = col / width;
      // Be careful not to read off the end of offset list
      if (index >= offsets->size() - 1) {
	distance = offset[offsets->size() - 1];
      } else {
	inc = (offset[index] - offset[index + 1]) / (double)width; /* linear interpolation */
	distance = (int)::round((col % width) * inc) - offset[index];
      }
      // Threshold the distance so we don't over-adjust (beyond the edge of the image).
      // Frankly, if we're deskewing by that much there's something seriously wrong elsewhere, 
      // but it's better than crasing <wink>
      if (distance >= (int)image.nrows())
	distance = image.nrows() - 1;
      else if (distance <= -(int)image.nrows())
	distance = -(int)image.nrows() + 1;
      shear_column(image, col, distance);
    }
  }

  // remove tall ccs, such as slurs and dynamic wedges.
  // all other vertically thick things have already been removed
  template<class T>
  void remove_tall_ccs(T& image, int size) {
    debug_message("removing tall things");
    typedef typename ImageFactory<T>::ccs_type ccs_type;
    ccs_type* cc;
    cc = (ccs_type*)cc_analysis(image);
    ccs::filter_tall(*cc, (int)(size));
    delete_connected_components(cc);
  }
  
  // finds the individual stafflines within a staff and deskews the staff
  template<class T>
  Staff find_stafflines(Param& param, Page& page, T& image_original, T& image_hfilter, 
			Rect& rect, IntVector* yproj) {
    print_rect("find_stafflines", rect);

    double staffline_h, staffspace_h;
    Rect orig_rect = rect; // copy
    Staff staff;

    // The goal of this section is to get the y-projection, which will be used
    // to find the location of stafflines.
    // To deal with extensive ledger line samples
    // find_skew is called to obtain the y-projection (local_yproj)
    // subruns is useless to shear the entire page, since it does not span the 
    // entire width of the page
    
    {
      IntVector *deriv;
      int n_peaks;
      {
	IntVector *local_yproj;
	{
	  view_type subimage(image_original, rect); // image_original is most reliable for staffspace_h
	  if (param.staffspace_h <= 0)
	    staffspace_h = most_frequent_run(subimage, runs::White(), runs::Vertical());
	  else
	    staffspace_h = param.staffspace_h;
	  
	  data_type image_hhfilter_data(rect.size(), rect.ul());
	  view_type image_hhfilter(image_hhfilter_data);
	  image_copy_fill(view_type(image_hfilter, rect), image_hhfilter);
	  
	  if (param.staffline_h <= 0)
	    staffline_h = most_frequent_run(image_hhfilter, runs::Black(), runs::Vertical());
	  else
	    staffline_h = param.staffline_h;

	  if (!is_staffline_and_staffspace_reasonable(staffline_h, staffspace_h)) {
	    staff.ncols(0);
	    staff.nrows(0);
	    return staff;
	  }
	  
	  typedef typename ImageFactory<T>::ccs_type ccs_type;
	  ccs_type* cc;
	  cc = (ccs_type*)cc_analysis(image_hhfilter);
	  ccs::filter_narrow(*cc, (int)(staffspace_h * 2) + 1);
	  delete_connected_components(cc);
	  
	  IntVector* offsets = find_skew(subimage, &local_yproj, param.skew_strip_width, 
					 int(std::min(param.max_skew, staffspace_h)));
	  deskew(image_hhfilter, offsets, param.skew_strip_width);
	  
	  IntVector *hh_yproj = projection_rows(image_hhfilter);
	  try {
	    array_add(local_yproj, hh_yproj, 0);
	  } catch (std::exception) {
	    delete hh_yproj;
	    delete offsets;
	    throw;
	  }
	  
	  delete hh_yproj;
	  delete offsets;
	}

	// Find the positions of the stafflines (deriv)
	int avg = (int)array_avg(local_yproj->begin(), local_yproj->end());
	array_filter(local_yproj, (int)staffline_h);
	deriv = array_deriv(local_yproj->begin(), local_yproj->end());
	n_peaks = array_local_maxima(deriv, local_yproj->begin(), local_yproj->end(), avg);
	
	if (n_peaks > (int)param.n_stafflines) {
	  try {
	    peaks_sort(deriv, local_yproj);
	  } catch (std::exception) {
	    delete deriv;
	    delete local_yproj;
	    throw;
	  }
	  deriv->resize(param.n_stafflines);
	  std::sort(deriv->begin(), deriv->end());
	  n_peaks = param.n_stafflines;
	} else if (n_peaks < (int)param.n_stafflines) {
	  // TODO: For autodetecting the number of stafflines, this should not be an exception
	  delete deriv;
	  delete local_yproj;
// 	  if (param.find_only)
// 	    throw std::runtime_error("Less than the expected number of stafflines.");
// 	  else {
	    std::cerr << "Warning: Less than " << param.n_stafflines << " stafflines." << std::endl;
	    staff.ncols(0);
	    staff.nrows(0);
	    return staff;
// 	  }
	}
	delete local_yproj;
      }

      rect.ul_y(rect.ul_y() + (*deriv)[0]);
      rect.nrows((*deriv)[n_peaks - 1] - (*deriv)[0] + 1);

      staff.staffline_pos.resize(n_peaks);
      for (int i = 0; i < n_peaks; i++)
	staff.staffline_pos[i] = rect.ul_y() + (*deriv)[i] - (*deriv)[0];
      staff.staffline_h = staffline_h;

      delete deriv;
    }

    print_array("stafflines ", &(staff.staffline_pos));

    // Find the bounding box of the staff
    // Find the true left & right margins 
    // For the left take the current left margin and move left using the 
    // known staff positions & xprojection of narrow vertical slices 
    //
    // the xproj is slightly thinner to get around braces 

    if (rect.nrows() < staffline_h * 2)
      throw std::runtime_error("Proposed staff is too short.");

    { 
      IntVector *xproj = xproj_horizontal_strip(image_original, (int)(rect.ul_y() + staffline_h),
						(int)(rect.nrows() - staffline_h * 2));
      
      int width = 3;
      int column;
      // Find the left margin 
      for (column = std::min(int(rect.ul_x()), int(xproj->size()) - width); 
	   column >= 0; column--) {
	int sum = 0;
	for (int i = 0; i < width; i++)
	  sum += (*xproj)[column + i];
	if (sum < staffline_h)
	  break;
      }
      rect.ul_x(column + 1);
      
      // Find the right margin
      for (column = rect.lr_x(); column < (int)xproj->size(); column++) {
	int sum = 0;
	for (int i = 0; i < std::min(width, int(xproj->size() - column)); i++)
	  sum += (*xproj)[column + i];
	if (sum < staffline_h)
	  break;
      }
      rect.lr_x(column - 1);
      print_rect("side margins adjusted", rect);

      delete xproj;
    }

    staff.rect_set(Point(rect.ul_x(), *(staff.staffline_pos.begin())),
		   Point(rect.lr_x(), *(staff.staffline_pos.end() - 1)));

    if (param.find_only)
      return staff;
    
    // We're alomost ready to straighten out the stave (shear)
    // 1. put a little more room to the top and bottom of the stave bounding box 
    // 2. cut out that box from the original
    // 3. shear

    rect.ul_y(std::max(int((int)rect.ul_y() - 2 * staffline_h), (int)0));
    rect.lr_y(std::min(int((int)rect.lr_y() + 2 * staffline_h), (int)image_original.lr_y()));

    // QUESTION: Why do you quantize?
    orig_rect.ul_x((((int)rect.ul_x() + param.skew_strip_width - 1) / param.skew_strip_width) 
		   * param.skew_strip_width);
    orig_rect.ncols(rect.ncols() - (orig_rect.ul_x() - rect.ul_x()));
    
    // TODO: new_offset could be allocated on the stack
    {
      IntVector new_offset(image_original.ncols() / param.skew_strip_width + 1, 0);
      {
	int top, bot, new_top, new_bot, min_proj, left_margin;

	IntVector* local_yproj;
	view_type subimage(image_original, orig_rect);
	IntVector* offset_array = find_skew(subimage, &local_yproj, param.skew_strip_width,
					    int(std::min(param.max_skew * 3, staffspace_h)));
	delete local_yproj;
      
	// since the entire width of the original image must be sheared, offset_array
	// must be extended on both sides
	left_margin = orig_rect.ul_x() / param.skew_strip_width;
	for (int i = 0; i < std::min(left_margin, int(new_offset.size())); i++)
	  new_offset[i] = (*offset_array)[0];
	for (int i = left_margin; i < std::min(int(offset_array->size()) + left_margin, int(new_offset.size())); i++)
	  new_offset[i] = (*offset_array)[i - left_margin];
	for (int i = int(offset_array->size()) + left_margin; i < int(new_offset.size()); i++)
	  new_offset[i] = (*offset_array)[offset_array->size() - 1];
      
	delete offset_array;

	/* find minimum above the staff */
	if (page.staves.size() == 0)
	  top = 0;
	else
	  top = (*(page.staves.end() - 1)).ul_y();
	new_top = rect.ul_y();
	min_proj = (*yproj)[new_top];
	for (int i = new_top; i > top; i--) {
	  if ((*yproj)[i] < min_proj) {
	    min_proj = (*yproj)[i];
	    new_top = i;
	  }
	}
      
	/* find minimum below the staff */
	new_bot = rect.lr_y();
	bot = std::min(int(new_bot + rect.nrows()), (int)image_original.nrows());
	min_proj = (*yproj)[new_bot];
	for (int i = new_bot; i < bot; i++) {
	  if ((*yproj)[i] < min_proj) {
	    min_proj = (*yproj)[i];
	    new_bot = i;
	  }
	}
	rect.ul_x(0);
	rect.lr_x(image_original.lr_x());
	rect.ul_y(new_top);
	rect.lr_y(new_bot);
      }

      print_rect("expanded", rect);

      view_type subimage(image_original, rect);
      deskew(subimage, &new_offset, param.skew_strip_width);
      view_type subimage1(image_hfilter, rect);
      deskew(subimage1, &new_offset, param.skew_strip_width);
      param.add_deskew_info(&new_offset, rect);
    }   
  
    debug_message("  ] find_stafflines");
    return (staff);
  }


  // Removes side margins from the rect by cropping the ends where the x projection < size
  template<class T>
  void remove_side_margins(T& image, Rect& rect, int size) {
    IntVector* xproj = projection_cols(view_type(image, rect));

    int left_margin = 0;
    for (int i = 0; i < (int)xproj->size(); i++) {
      if ((*xproj)[i] > size) {
	left_margin = i - 1;
	break;
      }
    }

    int right_margin = image.ncols() - 1;
    for (int i = xproj->size() - 1; i > left_margin; i--) {
      if ((*xproj)[i] > size) {
	right_margin = i;
	break;
      }
    }

    delete xproj;
    
    rect.ul_x(left_margin);
    rect.lr_x(right_margin);
    print_rect("side margins", rect);
  }

  // Finds a single set of stafflines
  template<class T>
  void find_staff(Param& param, Page& page, T& image_original, T& image_hfilter, Rect& rect, 
		  double staffline_h, int space) {
    debug_message(" find_staff");

    remove_side_margins(image_hfilter, rect, (size_t)(staffline_h * 4));

    // MGD: removed a chunk (now gone) to deal with David Lewis' problem for not finding
    // all of the staff lines.  Will break the ability to find multiple staves with a break 
    // in the middle.

    // add some vertical margin -- taking caution not to go beyond bounds of data
    rect.ul_y(std::max((int)rect.ul_y() - space, 0)); // use space as margins to allow for minor skews
    rect.lr_y(std::min((int)rect.lr_y() + space, (int)image_hfilter.lr_y()));

    if (rect_too_small_to_be_staff(param, rect, space))
      return;

    /// MGD: removed a chunk (now gone) to deal with David Lewis' problem for not finding
    /// all of the staff lines.  Will break the ability to find
    /// multiple staves with a break in the middle.

    // MGD: In the old version, these projections were on the original *un-deskewed* image,
    //      but that just seems wrong, since it no longer reflects what we're working with.
    //      I changed it to the deskewed version of the original, and it seems to make only 
    //      superficial differences from the old algorithm.  I would argue this is probably more
    //      correct.
    // yproj = yproj_vertical_strip(image_original_unskewed, rect.ul_x(), rect.ncols());
    IntVector* yproj = yproj_vertical_strip(image_original, rect.ul_x(), rect.ncols());
    try {
      Staff staff = find_stafflines(param, page, image_original, image_hfilter, rect, yproj);
      if (staff.nrows() > 0 and staff.ncols() > 0)
	page.add_staff(staff);
    } catch (std::exception) {
      delete yproj;
      throw;
    }
    delete yproj;
  }

  // Finds all of the staves on the page
  template<class T>
  void locate_staves(Param& param, Page& page, IntVector* yproj, double staffline_h, 
		     double staffspace_h, T& image_original, T& image_hfilter) {

    debug_message("locate_staves");
    
    IntVector *deriv;
    IntVector::iterator data;	/* *data1; */
    int i, j, last, last_i;	/* staff_no = 0; */
    int space = (int)(staffspace_h + staffline_h);
    
    // On the whole page, find the peaks in y-projections
    array_filter(yproj, (int)staffline_h);
    deriv = array_deriv(yproj->begin(), yproj->end());
    // TODO: Fix this 100 threshold.  Dalitz and Karsten use max(array) / 5.0 -- is that
    //       a reasonable thing to do?
    array_local_maxima(deriv, yproj->begin(), yproj->end(), 100);
    data = deriv->begin();		/* peaks in yproj */
    
    last = data[0];
    last_i = 0;

    print_rect("Whole image", image_hfilter);
    
    for (i = 1; i <= (int)deriv->size(); i++) {
      if ((i == (int)deriv->size() && i - last_i >= (int)param.n_stafflines)
	  || (data[i] - data[i - 1] > (space * 2))) {
	int n_peaks = i - last_i;

	Rect rect(Point(0, last - 1), Point(image_hfilter.lr_x(), data[i-1] + 1));
       
	print_rect("candidate", rect);

	last = data[i];
	
	if (rect_too_small_to_be_staff(param, rect, staffspace_h)) {	/* too small */
	  debug_message("rect too small");
	  last_i = i;
	  continue;
	}
	
	// was (n_peaks < 10)
	if (n_peaks < int(param.n_stafflines * 2)) {
	  debug_message("n_peaks < 10");
	  if (n_peaks < (int)param.n_stafflines - 1) {
	    debug_message("n_peaks < param.n_stafflines");
	    last_i = i;
	    continue;
	  }	
	  try {
	    find_staff(param, page, image_original, image_hfilter, rect, staffline_h, space);
	  } catch (std::exception) {
	    delete deriv;
	    throw;
	  }
	} else {
	  int avg, n1;
	  IntVector* deriv1;
	  IntVector::iterator subyp; /* pointer to yproj do NOT free */
	  IntVector::iterator ydata = yproj->begin();

	  int offset = data[last_i] - 5;
	  if (offset < 0)
	    throw std::runtime_error("This should never happen");
	  subyp = ydata + offset;	/* was - 1 IF 94/11/07 */
	  if (i < 1)
	    throw std::runtime_error("This should never happen");
	  int subyp_size = data[i - 1] - data[last_i] + 10;	/* was + 2 */
	  avg = (int)array_avg(subyp, subyp + subyp_size);
	  deriv1 = array_deriv(subyp, subyp + subyp_size);
	  n1 = array_local_maxima(deriv1, subyp, subyp + subyp_size, avg);

	  // was (n1 < 10)
	  if (n1 < int(param.n_stafflines * 2)) {
	    debug_message("n1 < 10");
	    try {
	      find_staff(param, page, image_original, image_hfilter, rect, staffline_h, space);
	    } catch (std::exception) {
	      delete deriv1; delete deriv;
	      throw;
	    }
	  } else {
	    debug_message("n1 >= 10");
	    int last_j = 0;
	    IntVector::iterator data1 = deriv1->begin();
	    int orig = data[last_i] - 1;
	    int last1 = data1[0] + orig;

	    for (j = 1; j <= (int)deriv1->size(); j++)
	      if ((j == (int)deriv1->size() && j - last_j >= (int)param.n_stafflines)
		  || data1[j] - data1[j - 1] > (int) (space * 2)) {
		Rect rect(Point(0, last1), Point(image_hfilter.lr_x(), data1[j - 1] + orig - 1));
		print_rect("candidate 2", rect);
		if (rect_too_small_to_be_staff(param, rect, staffspace_h)) {
		  debug_message("rect too small");
		  last_j = j;
		  continue;
		}
		try {
		  find_staff(param, page, image_original, image_hfilter, rect, staffline_h, space);
		} catch (std::exception) {
		  delete deriv; delete deriv1;
		  throw;
		}
		last1 = data1[j] + orig;
		last_j = j;
	      }
	  }
	  delete deriv1;
	}
	last_i = i;
      }
    }
    delete deriv;
  }

  //////////////////////////////////////////////////
  // STAFFLINE REMOVAL

  template<class T>
  void find_staffline_height_for_staves(Param& param, Page& page, T& image_hfilter) {
    for (size_t i = 0; i < page.staves.size(); ++i) {
      Staff& staff = page.staves[i];
      if (staff.staffline_h <= 0) {
	if (param.staffline_h <= 0) {
	  view_type subimage(image_hfilter, staff);
	  staff.staffline_h = most_frequent_run(subimage, runs::Black(), runs::Vertical());
	} else
	  staff.staffline_h = param.staffline_h;
      }
    }
  }

  bool _staffline_comparator(const Rect& a, const Rect& b) {
    return a.lr_y() < b.ul_y();
  }

  // Removes any Ccs that are between stafflines or outside of the given staff
  // This function is intended to be run on the staffline image (short runs)
  // and will attempt to remove everything that isn't a staffline
  //  ccs: a set of ccs making up a staff
  //  staff: a Staff object defining where the lines are
  template<class U>
  void ccs_filter_between_and_outside_stafflines(U* ccs, const Staff& staff) {
    int n_lines = staff.staffline_pos.size();
    IntVector top(n_lines);
    IntVector bot(n_lines);

    for (int k = 0; k < n_lines - 1; k++) {
      top[k] = staff.staffline_pos[k] + (int)staff.staffline_h;
      bot[k] = staff.staffline_pos[k + 1] - (int)staff.staffline_h;
    }
    top[n_lines - 1] = staff.staffline_pos[0] - (int)staff.staffline_h;           /* very top */
    bot[n_lines - 1] = staff.staffline_pos[n_lines - 1] + (int)staff.staffline_h; /* very bottom */
    
    typename U::iterator it = ccs->begin();
    for (; it != ccs->end(); it++) {
      int max_y = (*it)->lr_y();
      int min_y = (*it)->ul_y();
      int mid_point = int(max_y + min_y) / 2;
      int k;
			
      if (mid_point < top[n_lines - 1] || mid_point > bot[n_lines - 1])
	k = 0;
      else {
	for (k = 0; k < n_lines - 1; k++)
	  if (mid_point < bot[k] && mid_point > top[k])
	    break;
      }
      if (k != n_lines - 1) 
	std::fill((*it)->vec_begin(), (*it)->vec_end(), 0);
    }
  }

  template<class T>
  void filter_staff(Param& param, Page& page, int staff_no, T& image_original,
		    T& image_hfilter, T& image_removed) {
    debug_message("filter_staff");
    
    Staff& staff = page.staves[staff_no];
    
    Rect rect = staff;
    rect.ul_y(std::max(int((int)rect.ul_y() - 2 * staff.staffline_h), (int)0));
    rect.lr_y(std::min(int((int)rect.lr_y() + 2 * staff.staffline_h), (int)image_original.lr_y()));

    {
      int top, bot, new_top, new_bot, min_proj;

      IntVector* yproj = yproj_vertical_strip(image_original, rect.ul_x(), rect.ncols());
      
      /* find minimum above the staff */
      if (staff_no == 0)
	top = 0;
      else
	top = page.staves[staff_no - 1].ul_y();
      new_top = rect.ul_y();
      min_proj = (*yproj)[new_top];
      for (int i = new_top; i > top; i--) {
	if ((*yproj)[i] < min_proj) {
	  min_proj = (*yproj)[i];
	  new_top = i;
	}
      }
    
      /* find minimum below the staff */
      new_bot = rect.lr_y();
      if (staff_no == (int)page.staves.size() - 1)
	bot = image_original.nrows();
      else
	bot = page.staves[staff_no + 1].ul_y();
      min_proj = (*yproj)[new_bot];
      for (int i = new_bot; i < bot; i++) {
	if ((*yproj)[i] < min_proj) {
	  min_proj = (*yproj)[i];
	  new_bot = i;
	}
      }
      rect.ul_x(0);
      rect.lr_x(image_original.lr_x());
      rect.ul_y(new_top);
      rect.lr_y(new_bot);

      delete yproj;
    }

    print_rect("filter_staff rect", rect);
    debug_message(staff.staffline_h);
    print_array("stafflines", &(staff.staffline_pos));

    data_type subimage_data(rect.size(), rect.ul());
    view_type subimage(subimage_data, rect);
    image_copy_fill(view_type(image_hfilter, rect), subimage);

    typedef typename ImageFactory<T>::ccs_type ccs_type;
    ccs_type* cc;
    cc = (ccs_type*)cc_analysis(subimage);
    /* filter out all but the stafflines */
    ccs::filter_tall(*cc, (int)(staff.staffline_h * 2) + 1);
    ccs_filter_between_and_outside_stafflines(cc, staff);
    delete_connected_components(cc);

    xor_image(subimage, view_type(image_original, rect));
    view_type subimage1(image_removed, rect);
    image_copy_fill(subimage, subimage1);
  }

  // was sl_create
  RectVector* create_staffline_table(Page& page) {
    debug_message("create_staffline_table");
    assert(page.total_stafflines > 0);

    debug_message("total_stafflines" << page.total_stafflines);

    RectVector* sl_table = new RectVector(page.total_stafflines);
    RectVector::iterator sl_it = sl_table->begin();

    for (StaffVector::iterator staff = page.staves.begin(); 
	 staff != page.staves.end(); staff++) {
      int half_linew = int((staff->staffline_h / 2.0) + 0.5);

      for (IntVector::iterator staffline = staff->staffline_pos.begin(); 
	   staffline != staff->staffline_pos.end(); staffline++) {
	Rect& rect = *(sl_it++);
	// QUESTION: It seems odd to be off-center of the staff line like this...
	rect.rect_set(Point(staff->ul_x(), (*staffline - half_linew)),
		      Point(staff->lr_x(), (*staffline + (half_linew * 3))));
      }
    }

    std::stable_sort(sl_table->begin(), sl_table->end(), &_staffline_comparator);
    return sl_table;
  }

  template<class T>
  void filter_stafflines(Param& param, Page& page, T& image_removed) {
    debug_message("filter_stafflines");

    // go through the image as vertical run-length data
    typename T::col_iterator i = image_removed.col_begin();
    typedef typename T::row_iterator Iter;
    int curr_col = 0;

    RectVector* sl_table = create_staffline_table(page);

    double max_staffline_h = 0.0;
    for (StaffVector::iterator staff = page.staves.begin();
         staff != page.staves.end(); staff++)
      if (staff->staffline_h > max_staffline_h)
        max_staffline_h = staff->staffline_h;

    for (size_t c = 0; i != image_removed.col_end(); ++i, ++curr_col, ++c) {
      Iter j = i.begin();
      Iter end = i.end();
      while (j != end) {
        if (is_black(*j)) {
          Iter beg = j;
          run_end(j, end, runs::Black());
          if (j - beg < max_staffline_h * 2) {
            Rect sl(Point(curr_col, beg - i.begin()),
                    Point(curr_col, j - i.begin()));
            std::pair<RectVector::iterator, RectVector::iterator> lines =
              std::equal_range(sl_table->begin(), sl_table->end(), sl, &_staffline_comparator);
            if (lines.first < lines.second) {
              bool fill = false;
              size_t mid_point = (sl.lr_y() + sl.ul_y()) / 2;
              // MGD: Added -- will only remove stafflines if in the right x position as well
              //      That *may* fix the bug with multiple staves on one line
              for (RectVector::iterator k = lines.first;
                   k != lines.second; ++k) {
                if (!(mid_point > k->lr_y() || mid_point < k->ul_y()) &&
                    (c >= k->ul_x() && c <= k->lr_x())) { // MGD: Now cognisant of staffline x-position
                  fill = true;
                  break;
                }
              }
              if (fill)
                std::fill(beg, j, white(image_removed));
            }
          }
        } else {
          run_end(j, end, runs::White());
        }
      }
    }
    delete sl_table;
  }

  template<class T>
  void filter_staves(Param& param, Page& page, T& image_original, T& image_hfilter,
		     T& image_removed) {
    debug_message("filter_staves");
    find_staffline_height_for_staves(param, page, image_hfilter);
    for (size_t i = 0; i < page.staves.size(); ++i)
      filter_staff(param, page, i, image_original, image_hfilter, image_removed);
    filter_stafflines(param, page, image_removed);
  }

  double calculate_max_skew(double max_skew, size_t deskew_strip_width) {
    static const double degrees_to_radians = (2 * 3.14159265358979323846) / 360.0;
    return abs((double)deskew_strip_width * tan(max_skew * degrees_to_radians));
  }

  //////////////////////////////////////////////////
  // TOP-LEVEL FUNCTIONS

  // Performs a deskewing by trying to cross-correlate the entire image (not
  // one staff at a time).  This generally doesn't produce very good results.
  template<class T>
  view_type* global_staffline_deskew(T& original, Param& param) {
    double staffline_h, staffspace_h;
    if (param.staffline_h <= 0.0 || param.staffspace_h <= 0.0)
      find_rough_staffline_and_staffspace_height(original, staffline_h, staffspace_h);
    else {
      staffline_h = param.staffline_h; staffspace_h = param.staffspace_h;
    }

    if (param.skew_strip_width <= 0)
      param.skew_strip_width = int(staffspace_h * 2);

    param.max_skew = calculate_max_skew(param.max_skew, param.skew_strip_width);

    IntVector* yproj;
    IntVector* offset_array;

    {
      data_type image_hfilter_data(Dim(original.ncols(), original.nrows()),
                                   Point(original.offset_x(), original.offset_y()));
      view_type image_hfilter(image_hfilter_data, original);

      basic_filtering_step(original, image_hfilter, staffline_h, staffspace_h);

      offset_array =
        find_skew(image_hfilter, &yproj, param.skew_strip_width,
                  int(std::min(param.max_skew, staffspace_h)));
    }

    data_type* result_data = new data_type(Dim(original.ncols(), original.nrows()),
                                           Point(original.offset_x(), original.offset_y()));
    view_type* result = new view_type(*result_data);
    image_copy_fill(original, *result);
    deskew(*result, offset_array, param.skew_strip_width);
    param.add_deskew_info(offset_array);

    delete offset_array;
    delete yproj;

    return result;
  }

  template<class T>
  std::pair<view_type*, view_type*> find_and_remove_staves_fujinaga(T& original, Param& param, Page& page) {
    debug_message("find_and_remove_staves");

    if (param.n_stafflines <= 3) {
      throw std::runtime_error
	("The number of stafflines must be greater than 2.\nAutodetection of the number of stafflines is not implemented.");
    }

    //////////////////////////////////////////////////
    // FIND ROUGH STAFFLINE AND STAFFSPACE HEIGHT

    double staffline_h, staffspace_h;
    if (param.staffline_h <= 0.0 || param.staffspace_h <= 0.0)
      find_rough_staffline_and_staffspace_height(original, staffline_h, staffspace_h);
    else {
      staffline_h = param.staffline_h;
      staffspace_h = param.staffspace_h;
    }

    if (param.skew_strip_width <= 0)
      param.skew_strip_width = int(staffspace_h * 2);

    param.max_skew = calculate_max_skew(param.max_skew, param.skew_strip_width);

    //////////////////////////////////////////////////
    // CREATE RUNLENGTH-FILTERED COPIES OF THE ORIGINAL IMAGE

    // Create the temporaries that we need
    // Remove offsets, which makes all the processing much more straightforward
    // (The offset will be put back at the end right before returning)
    data_type* image_original_data = new data_type(Dim(original.ncols(), original.nrows()), Point(0, 0));
    data_type image_hfilter_data(Dim(original.ncols(), original.nrows()), Point(0, 0));

    view_type* image_original = new view_type(*image_original_data);
    view_type image_hfilter(image_hfilter_data);

    image_copy_fill(original, *image_original);
    basic_filtering_step(*image_original, image_hfilter, staffline_h, staffspace_h);

    //////////////////////////////////////////////////
    // GLOBAL DESKEWING
    IntVector* yproj;
    IntVector* offset_array;
    try {
      offset_array = find_skew(image_hfilter, &yproj, param.skew_strip_width, 
			       (int)(std::min(param.max_skew, staffspace_h)));
    } catch (std::exception) {
      delete image_original->data(); delete image_original;
      throw;
    }
    deskew(*image_original, offset_array, param.skew_strip_width);
    deskew(image_hfilter, offset_array, param.skew_strip_width);
    param.add_deskew_info(offset_array);
    delete offset_array;

    remove_tall_ccs(image_hfilter, (size_t)(staffline_h * 8));

    // image_hfilter now consists of mostly staffline segments, some flat slurs and flat beams. 
    // yproj is the yproj of image_hfilter 

    try {
      locate_staves(param, page, yproj, staffline_h, staffspace_h, *image_original, image_hfilter);
    } catch (std::exception) {
      delete image_original->data(); delete image_original;
      delete yproj;
      throw;
    }
    delete yproj;

    if (param.find_only) {
      delete image_original->data(); delete image_original;
      return pair<view_type*, view_type*>(NULL, NULL);
    } else if (param.deskew_only) {
      param.undo_deskews(*image_original);
      return pair<view_type*, view_type*>(image_original, NULL);
    } else {
      data_type* image_removed_data = new data_type(Dim(original.ncols(), original.nrows()), Point(0, 0));
      view_type* image_removed = new view_type(*image_removed_data);
      image_copy_fill(*image_original, *image_removed);

      try {
	filter_staves(param, page, *image_original, image_hfilter, *image_removed);
      } catch (std::exception) {
	delete image_original->data(); delete image_original;
	delete image_removed->data(); delete image_removed;
	throw;
      }

      param.undo_deskews(*image_original);
      param.undo_deskews(*image_removed);
      return pair<view_type*, view_type*>(image_original, image_removed);
    }
  }

  template<class T>
  view_type* remove_staves_fujinaga(T& original, Page& page, Param& param) {
    debug_message("remove_staves");

    //////////////////////////////////////////////////
    // FIND ROUGH STAFFLINE AND STAFFSPACE HEIGHT

    double staffline_h, staffspace_h;
    if (param.staffline_h <= 0.0 || param.staffspace_h <= 0.0)
      find_rough_staffline_and_staffspace_height(original, staffline_h, staffspace_h);
    else {
      staffline_h = param.staffline_h;
      staffspace_h = param.staffspace_h;
    }

    //////////////////////////////////////////////////
    // CREATE RUNLENGTH-FILTERED COPIES OF THE ORIGINAL IMAGE

    // Create the temporaries that we need
    // Remove offsets, which makes all the processing much more straightforward
    // (The offset will be put back at the end right before returning)
    data_type image_original_data(Dim(original.ncols(), original.nrows()), Point(0, 0));
    data_type image_hfilter_data(Dim(original.ncols(), original.nrows()), Point(0, 0));
    data_type* image_removed_data = new data_type(Dim(original.ncols(), original.nrows()), Point(0, 0));

    view_type image_original(image_original_data);
    view_type image_hfilter(image_hfilter_data);
    view_type* image_removed = new view_type(*image_removed_data);

    image_copy_fill(original, image_original);
    basic_filtering_step(image_original, image_hfilter, staffline_h, staffspace_h);

    //////////////////////////////////////////////////
    // GLOBAL DESKEWING
    remove_tall_ccs(image_hfilter, (size_t)(staffline_h * 8));
    // Everything up to this point is just "catching up" with where find_and_deskew_staves_fujinaga
    // left off

    image_copy_fill(image_original, *image_removed);
    
    try {
      filter_staves(param, page, image_original, image_hfilter, *image_removed);
    } catch (std::exception) {
      delete image_removed->data(); delete image_removed;
      throw;
    }
    
    return image_removed;
  }

}

#endif
