
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "skeleton_utilities.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_skeleton_utilities(void);
#endif
                  static PyObject* call_split_skeleton(PyObject* self, PyObject* args);
                        static PyObject* call_get_corner_points(PyObject* self, PyObject* args);
                        static PyObject* call_get_corner_points_rj(PyObject* self, PyObject* args);
                        static PyObject* call_remove_spurs_from_skeleton(PyObject* self, PyObject* args);
                        static PyObject* call_distance_precentage_among_points(PyObject* self, PyObject* args);
                        static PyObject* call_create_skeleton_segment(PyObject* self, PyObject* args);
                        static PyObject* call_remove_vruns_around_points(PyObject* self, PyObject* args);
                        static PyObject* call_extend_skeleton(PyObject* self, PyObject* args);
                        static PyObject* call_parabola(PyObject* self, PyObject* args);
                        static PyObject* call_lin_parabola(PyObject* self, PyObject* args);
                        static PyObject* call_estimate_next_point(PyObject* self, PyObject* args);
            }

          static PyMethodDef _skeleton_utilities_methods[] = {
                  { CHAR_PTR_CAST "split_skeleton",
          call_split_skeleton, METH_VARARGS,
          CHAR_PTR_CAST "**split_skeleton** (``Image`` [Float] *distance_transform*, int *cornerwidth*, ``Choice`` [chessboard|manhattan|euclidean] *norm* = chessboard)\n\nSplits a skeleton image at branching and corner points and returns\na list of `SkeletonSegments`__.\n\n.. __: gamera.toolkits.musicstaves.plugins.skeleton_utilities.SkeletonSegment.html\n\nThe input image must be a skeleton, the `FloatImage` must be a distance\ntransform obtained from the original image. The splitting procedure\nconsists of two steps:\n\n1) Split at \"branching\" (more than two connected) points.\n\n2) In the remaining branches, corners are detected with get_corner_points_rj,\n   which requires a parameter *cornerwidth*. A reasonable value for\n   *cornerwidth* could be *max(staffspace_height/2,5)*; setting *cornerwidth*\n   to zero will skip the corner detection.\n\nIn both steps, a number of pixels is removed around each splitting point.\nThis number is taken from the *distance_transform* image. If you\ndo not want this, simply pass as *distance_transform* an image with all\npixel values equal to zero.\nThe parameter *norm* should determine the removal shape (diamond for\nchessboard, square for manhatten and circle for euclidean distance), but\nis currently ignored.\n"        },
                        { CHAR_PTR_CAST "get_corner_points",
          call_get_corner_points, METH_VARARGS,
          CHAR_PTR_CAST "**get_corner_points** ([object *points*], int *cornerwidth* = 1)\n\nReturns the corner points of the skeleton segment consisting of the\nprovided points by looking for angles below a certain threshold.\n\nCorners are detected by measuring the angle at each point plusminus\n*cornerwidth* neighbor points (see the figure below). The maximum angle is\ncurrently hard coded to 135 degrees. Apart from an angle threshold we\nalso check whether the branches are approximately straight.\n\n.. image:: images/split_skeleton.png\n\nFor an overview of more sophisticated corner detection algorithms, see\nC.H. Teh and R.T. Chin:\n*On the Detection of Dominant points on Digital Curves.* IEEE Transactions\non Pattern Analysis and Machine Intelligence, vol. 11, no. 8, pp. 859-872\n(1989).\n"        },
                        { CHAR_PTR_CAST "get_corner_points_rj",
          call_get_corner_points_rj, METH_VARARGS,
          CHAR_PTR_CAST "**get_corner_points_rj** ([object *points*], int *cornerwidth* = 1)\n\nReturns the corner points (maximum curvature points) of the skeleton\nsegment consisting of the provided points according to an algorithm by\nRosenfeld and Johnston.\n\nSee A. Rosenfeld and E. Johnston: *Angle detection on digital curves.* IEEE\nTrans. Comput., vol. C-22, pp. 875-878, Sept. 1973\nCompared to the algorithm in the paper the following modifications have been\nmade:\n\n - While Rosenfeld and Johnston's algorithm only works on closed curves,\n   this function operates on an open curve. The first checked point is the\n   one with the index *conerwidth* + 1 (*cornerwidth* corresponds to the\n   parameter *m* in the original paper).\n\n - This function does not allow adjacent dominant points, with a (chessboard)\n   distance less than *cornerwidth*. When this happens, the algorithm decides\n   based upon the angle found (it takes the sharper one).\n\n - Dominant points are considered corner points only if the angle of\n   the branches (see figure below) is less than *maxangle* (measured\n   in degrees; currently hard coded to 140 degrees).\n\n.. image:: images/split_skeleton.png\n"        },
                        { CHAR_PTR_CAST "remove_spurs_from_skeleton",
          call_remove_spurs_from_skeleton, METH_VARARGS,
          CHAR_PTR_CAST "**remove_spurs_from_skeleton** (int *length*, ``Choice`` [remove|keep|extrapolate] *endtreatment*)\n\nRemoves short branches (\"spurs\") from a skeleton image.\n\nParameters:\n\n  *length*:\n    only end branches not longer than length are removed\n  *endtreatment*:\n    determines how end points (branching points with two spurs and one\n    branch longer than *length*) are treated: 0 = spurs are removed,\n    1 = spurs are kept, 2 = end point is interpolated between the two\n    end branches.\n\n.. note:: Setting *endtreatment* = 0 will result in shortened\n          skeletons that no longer extend to the corners. The same\n          may happen for *endtreatment* = 2, though to a lesser extent.\n"        },
                        { CHAR_PTR_CAST "distance_precentage_among_points",
          call_distance_precentage_among_points, METH_VARARGS,
          CHAR_PTR_CAST "**distance_precentage_among_points** ([object *points*], int *mindistance*, int *maxdistance*)\n\nReturns the fraction of the given points that have pixel value\nbetween *mindistance* and *maxdistance* in the image *distance_transform*.\n"        },
                        { CHAR_PTR_CAST "create_skeleton_segment",
          call_create_skeleton_segment, METH_VARARGS,
          CHAR_PTR_CAST "**create_skeleton_segment** ([object *points*], [object *branching_points*])\n\nConstructor for a SkeletonSegment__ from a list of points.\n\n.. __: gamera.toolkits.musicstaves.plugins.skeleton_utilities.SkeletonSegment.html\n"        },
                        { CHAR_PTR_CAST "remove_vruns_around_points",
          call_remove_vruns_around_points, METH_VARARGS,
          CHAR_PTR_CAST "**remove_vruns_around_points** ([object *points*], int *threshold* = 0)\n\nRemoves all vertical runs to which any of the given points belongs.\nWhen the parameter *threshold* is greater than zero, only vertical runlengths\nshorter than *threshold* are removed.\n"        },
                        { CHAR_PTR_CAST "extend_skeleton",
          call_extend_skeleton, METH_VARARGS,
          CHAR_PTR_CAST "**extend_skeleton** (``Image`` [Float] *distance_transform*, ``ChoiceString(strict)`` [horizontal|linear|parabolic] *extrapolation_scheme* = linear, int *n_points* = 3)\n\nExtends a skeleton image at end points until a white\npixel in the provided unskeletonized image is found.\n\nThis is typically useful only for thinned out skeletons, because these do\nnot extend to the edges. For medial axis transform skeletons use\n``remove_spurs_from_skeleton`` instead.\n\nEach skeleton segment in *self* will be extended, until the skeleton segment\nis as long as the segment in *orig_image*. In case that the skeleton segment\nconsists of 3 or more points, an estimating parabola will be calculated using\nthe function parabola_. In case of a skeleton segment consisting of 2 points,\nthe segment will be extended using the fitting line.\n\nArguments:\n\n  *self*\n    The skeleton image to be extended\n\n  *distance_transform*\n    Distance transform of the original unskeletonized image. Determines\n    how far the skeleton is extrapolated at each end point.\n\n  *extrapolation_scheme*, *n_points*\n    Determines how new points are calculated. For 'linear' or 'parabolic'\n    extrapolation a line or parabola is fitted through *n_points* end\n    points of the skeleton branch. For 'horizontal' extrapolation the\n    skeleton branch is not investigated.\n\n    As horizontal extrapolation can lead to errenous results,\n    no more pixels are added in horizontal extrapolation than the distance\n    transform value at the skeleton end point.\n"        },
                        { CHAR_PTR_CAST "parabola",
          call_parabola, METH_VARARGS,
          CHAR_PTR_CAST "**parabola** ([object *points*])\n\nCalculate an estimating parabola for the given points. The first given\npoint (*points[0]*) is considered to be within the segment, whereas the last\ngiven point (*points[-1]*) is considered to be the end point of the segment.\n\nSince the segment (consisting of the given points) does not necessarily have to be a function, the values of *x* and *y* are described using a parameter *t*\nwith 2 functions:\n\n    x = x(t)\n\n    y = y(t)\n\nwhere\n\n    x(t) = ax*t^2 + bx*t + cx\n\n    y(t) = ay*t^2 + by*t + cy\n\nThe distance *t* is the sum of the distances between all points and is\nestimated with the eclidean distance:\n\n    t = sum(sqrt(dx^2 + dy^2))\n\nThus, the distance for *points[0]* is 0 and the distance for *points[-1]* is\n*t*.\n\nThe return value is the FloatVector [ax, ay, bx, by, cx, cy]. The values can\nbe used to calculate further points using the function estimate_next_point_.\n\n.. note:: Using the calculated parameters it is always possible to\n          re-calculate the last given point (+-1). If the parabola of all\n          given points does not match this criteria, the number of used points\n          will be reduced and the parabola will be calculated again (see the\n          image below).\n\n.. image:: images/parabola.png\n"        },
                        { CHAR_PTR_CAST "lin_parabola",
          call_lin_parabola, METH_VARARGS,
          CHAR_PTR_CAST "**lin_parabola** ([object *points*])\n\nCalculate an estimating parabola for the given points. The first given\npoint (*points[0]*) is considered to be within the segment, whereas the last\ngiven point (*points[-1]*) is considered to be the end point of the segment.\n\nSince the segment (consisting of the given points) does not necessarily have to be a function, the values of *x* and *y* are described using a parameter *t*\nwith 2 functions:\n\n    x = x(t)\n\n    y = y(t)\n\nwhere\n\n    x(t) = ax*t^2 + bx*t + cx\n\n    y(t) = by*t + cy\n\nThe distance *t* is the sum of the distances between all points and is\nestimated with the eclidean distance:\n\n    t = sum(sqrt(dx^2 + dy^2))\n\nThus, the distance for *points[0]* is 0 and the distance for *points[-1]* is\n*t*.\n\nThe return value is the FloatVector [ax, ay, bx, by, cx, cy] of which ay is\nalways 0. The values can be used to calculate further points using the\nfunction estimate_next_point_.\n\n.. note:: Using the calculated parameters it is always possible to\n          re-calculate the last given point (+-1). If the parabola of all\n          given points does not match this criteria, the number of used points\n          will be reduced and the parabola will be calculated again (see the\n          image below).\n\n.. image:: images/parabola.png\n"        },
                        { CHAR_PTR_CAST "estimate_next_point",
          call_estimate_next_point, METH_VARARGS,
          CHAR_PTR_CAST "**estimate_next_point** (``Point`` *current*, ``FloatVector`` *parameters*, float *t_end*, float *dt_end* = 0.00, int *direction* = 1)\n\nEstimates the next point on a given parabola.\n\nArguments:\n  *current*\n    The current point, which has to lie on the parabola.\n\n  *parameters*\n    The parameters [ax, ay, bx, by, cx, cy] that describe a parametric\n    parabola (see parabola_).\n\n  *t_end*\n    The distance of the current point to the first point (see parabola_ for\n    a possible computation).\n\n  *dt_end*\n    The distance of the current point to the previous one. When set to\n    the default value, it will be estimated automatically.\n\n  *direction*\n    The direction where to go. This option is not supported yet.\n\nThe return value is the vector [p, t_end, dt_end] where\n\n  *p* is the estimated point,\n\n  *t_end* is the distance of the estimated point to the first point,\n\n  *dt_end* is the distance of the estimated point to the current point.\n\n.. code:: Python\n\n    # calculate the parameters of the parabola and estimate the following point\n    parameters, t_end = parabola(points)\n    next_point, t_new, dt_new = estimate_next_point(points[-1], parameters, t_end)\n"        },
              { NULL }
  };

                static PyObject* call_split_skeleton(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                PyObject* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      Image* distance_transform_arg;
PyObject* distance_transform_pyarg;
                      int cornerwidth_arg;
                      int norm_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOii:split_skeleton"
                        ,
             &self_pyarg                        ,
             &distance_transform_pyarg                        ,
             &cornerwidth_arg                        ,
             &norm_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      if (!is_ImageObject(distance_transform_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'distance_transform' must be an image");
          return 0;
        }
        distance_transform_arg = ((Image*)((RectObject*)distance_transform_pyarg)->m_x);
        image_get_fv(distance_transform_pyarg, &distance_transform_arg->features, &distance_transform_arg->features_len);
                                          
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = split_skeleton(*((OneBitImageView*)self_arg), *((FloatImageView*)distance_transform_arg), cornerwidth_arg, norm_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = split_skeleton(*((Cc*)self_arg), *((FloatImageView*)distance_transform_arg), cornerwidth_arg, norm_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = split_skeleton(*((OneBitRleImageView*)self_arg), *((FloatImageView*)distance_transform_arg), cornerwidth_arg, norm_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = split_skeleton(*((RleCc*)self_arg), *((FloatImageView*)distance_transform_arg), cornerwidth_arg, norm_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = split_skeleton(*((MlCc*)self_arg), *((FloatImageView*)distance_transform_arg), cornerwidth_arg, norm_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'split_skeleton' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                  if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = return_arg;              return return_pyarg;
            }
                              }
                static PyObject* call_get_corner_points(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        PointVector* return_arg;
PyObject* return_pyarg;
                                          PointVector* points_arg;
PyObject* points_pyarg;
                      int cornerwidth_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oi:get_corner_points"
                        ,
             &points_pyarg                        ,
             &cornerwidth_arg                      ) <= 0)
           return 0;
               
              
      points_arg = PointVector_from_python(points_pyarg);
                          
              try {
                      return_arg = get_corner_points(points_arg, cornerwidth_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                                        if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              
      return_pyarg = PointVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_get_corner_points_rj(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        PointVector* return_arg;
PyObject* return_pyarg;
                                          PointVector* points_arg;
PyObject* points_pyarg;
                      int cornerwidth_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oi:get_corner_points_rj"
                        ,
             &points_pyarg                        ,
             &cornerwidth_arg                      ) <= 0)
           return 0;
               
              
      points_arg = PointVector_from_python(points_pyarg);
                          
              try {
                      return_arg = get_corner_points_rj(points_arg, cornerwidth_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                                        if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              
      return_pyarg = PointVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_remove_spurs_from_skeleton(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int length_arg;
                      int endtreatment_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oii:remove_spurs_from_skeleton"
                        ,
             &self_pyarg                        ,
             &length_arg                        ,
             &endtreatment_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                                          
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
return_arg = remove_spurs_from_skeleton(*((OneBitImageView*)self_arg), length_arg, endtreatment_arg);
break;
case CC:
return_arg = remove_spurs_from_skeleton(*((Cc*)self_arg), length_arg, endtreatment_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = remove_spurs_from_skeleton(*((OneBitRleImageView*)self_arg), length_arg, endtreatment_arg);
break;
case RLECC:
return_arg = remove_spurs_from_skeleton(*((RleCc*)self_arg), length_arg, endtreatment_arg);
break;
case MLCC:
return_arg = remove_spurs_from_skeleton(*((MlCc*)self_arg), length_arg, endtreatment_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'remove_spurs_from_skeleton' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
                static PyObject* call_distance_precentage_among_points(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                double return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      PointVector* points_arg;
PyObject* points_pyarg;
                      int mindistance_arg;
                      int maxdistance_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOii:distance_precentage_among_points"
                        ,
             &self_pyarg                        ,
             &points_pyarg                        ,
             &mindistance_arg                        ,
             &maxdistance_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      
      points_arg = PointVector_from_python(points_pyarg);
                                        
              try {
                      switch(get_image_combination(self_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = distance_precentage_among_points(*((FloatImageView*)self_arg), points_arg, mindistance_arg, maxdistance_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'distance_precentage_among_points' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                                                          return_pyarg = PyFloat_FromDouble((double)return_arg);            return return_pyarg;
                              }
                static PyObject* call_create_skeleton_segment(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        PyObject* return_arg;
PyObject* return_pyarg;
                                          PointVector* points_arg;
PyObject* points_pyarg;
                      PointVector* branching_points_arg;
PyObject* branching_points_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO:create_skeleton_segment"
                        ,
             &points_pyarg                        ,
             &branching_points_pyarg                      ) <= 0)
           return 0;
               
              
      points_arg = PointVector_from_python(points_pyarg);
                    
      branching_points_arg = PointVector_from_python(branching_points_pyarg);
            
              try {
                      return_arg = create_skeleton_segment(points_arg, branching_points_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                  delete branching_points_arg;                                      if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = return_arg;              return return_pyarg;
            }
                              }
                static PyObject* call_remove_vruns_around_points(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                          Image* self_arg;
PyObject* self_pyarg;
                      PointVector* points_arg;
PyObject* points_pyarg;
                      int threshold_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOi:remove_vruns_around_points"
                        ,
             &self_pyarg                        ,
             &points_pyarg                        ,
             &threshold_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      
      points_arg = PointVector_from_python(points_pyarg);
                          
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
remove_vruns_around_points(*((OneBitImageView*)self_arg), points_arg, threshold_arg);
break;
case CC:
remove_vruns_around_points(*((Cc*)self_arg), points_arg, threshold_arg);
break;
case ONEBITRLEIMAGEVIEW:
remove_vruns_around_points(*((OneBitRleImageView*)self_arg), points_arg, threshold_arg);
break;
case RLECC:
remove_vruns_around_points(*((RleCc*)self_arg), points_arg, threshold_arg);
break;
case MLCC:
remove_vruns_around_points(*((MlCc*)self_arg), points_arg, threshold_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'remove_vruns_around_points' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                            Py_INCREF(Py_None);
          return Py_None;
                    }
                static PyObject* call_extend_skeleton(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      Image* distance_transform_arg;
PyObject* distance_transform_pyarg;
                      char* extrapolation_scheme_arg;
                      int n_points_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOsi:extend_skeleton"
                        ,
             &self_pyarg                        ,
             &distance_transform_pyarg                        ,
             &extrapolation_scheme_arg                        ,
             &n_points_arg                      ) <= 0)
           return 0;
               
              if (!is_ImageObject(self_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'self' must be an image");
          return 0;
        }
        self_arg = ((Image*)((RectObject*)self_pyarg)->m_x);
        image_get_fv(self_pyarg, &self_arg->features, &self_arg->features_len);
                      if (!is_ImageObject(distance_transform_pyarg)) {
          PyErr_SetString(PyExc_TypeError, "Argument 'distance_transform' must be an image");
          return 0;
        }
        distance_transform_arg = ((Image*)((RectObject*)distance_transform_pyarg)->m_x);
        image_get_fv(distance_transform_pyarg, &distance_transform_arg->features, &distance_transform_arg->features_len);
                                          
              try {
                      switch(get_image_combination(self_pyarg)) {
case ONEBITIMAGEVIEW:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = extend_skeleton(*((OneBitImageView*)self_arg), *((FloatImageView*)distance_transform_arg), extrapolation_scheme_arg, n_points_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case CC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = extend_skeleton(*((Cc*)self_arg), *((FloatImageView*)distance_transform_arg), extrapolation_scheme_arg, n_points_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case ONEBITRLEIMAGEVIEW:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = extend_skeleton(*((OneBitRleImageView*)self_arg), *((FloatImageView*)distance_transform_arg), extrapolation_scheme_arg, n_points_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case RLECC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = extend_skeleton(*((RleCc*)self_arg), *((FloatImageView*)distance_transform_arg), extrapolation_scheme_arg, n_points_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
case MLCC:
switch(get_image_combination(distance_transform_pyarg)) {
case FLOATIMAGEVIEW:
return_arg = extend_skeleton(*((MlCc*)self_arg), *((FloatImageView*)distance_transform_arg), extrapolation_scheme_arg, n_points_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'distance_transform' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable value is FLOAT.", get_pixel_type_name(distance_transform_pyarg));
return 0;
}
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'extend_skeleton' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                  if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
                static PyObject* call_parabola(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        FloatVector* return_arg;
PyObject* return_pyarg;
                                          PointVector* points_arg;
PyObject* points_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O:parabola"
                        ,
             &points_pyarg                      ) <= 0)
           return 0;
               
              
      points_arg = PointVector_from_python(points_pyarg);
            
              try {
                      return_arg = parabola(points_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                      if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              
      return_pyarg = FloatVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_lin_parabola(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        FloatVector* return_arg;
PyObject* return_pyarg;
                                          PointVector* points_arg;
PyObject* points_pyarg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O:lin_parabola"
                        ,
             &points_pyarg                      ) <= 0)
           return 0;
               
              
      points_arg = PointVector_from_python(points_pyarg);
            
              try {
                      return_arg = lin_parabola(points_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                        delete points_arg;                                      if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              
      return_pyarg = FloatVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
                static PyObject* call_estimate_next_point(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                        FloatVector* return_arg;
PyObject* return_pyarg;
                                          Point current_arg;
PyObject* current_pyarg;
                      FloatVector* parameters_arg;
PyObject* parameters_pyarg;
                      double t_end_arg;
                      double dt_end_arg;
                      int direction_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OOddi:estimate_next_point"
                        ,
             &current_pyarg                        ,
             &parameters_pyarg                        ,
             &t_end_arg                        ,
             &dt_end_arg                        ,
             &direction_arg                      ) <= 0)
           return 0;
               
              
      try {
         current_arg = coerce_Point(current_pyarg);
      } catch (std::invalid_argument e) {
         PyErr_SetString(PyExc_TypeError, "Argument 'current' must be a Point, or convertible to a Point");
         return 0;
      }
                    
      parameters_arg = FloatVector_from_python(parameters_pyarg);
      if (parameters_arg == NULL) return 0;
                                                      
              try {
                      return_arg = estimate_next_point(current_arg, parameters_arg, t_end_arg, dt_end_arg, direction_arg);
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                          delete parameters_arg;                                                                                            if (return_arg== NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              
      return_pyarg = FloatVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
      
  DL_EXPORT(void) init_skeleton_utilities(void) {
    Py_InitModule(CHAR_PTR_CAST "_skeleton_utilities", _skeleton_utilities_methods);
  }
  

