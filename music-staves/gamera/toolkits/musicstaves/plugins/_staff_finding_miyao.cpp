
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "staff_finding_miyao.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_staff_finding_miyao(void);
#endif
                  static PyObject* call_miyao_candidate_points(PyObject* self, PyObject* args);
                        static PyObject* call_miyao_distance(PyObject* self, PyObject* args);
                        static PyObject* call_miyao_find_edge(PyObject* self, PyObject* args);
                      }

          static PyMethodDef _staff_finding_miyao_methods[] = {
                  { CHAR_PTR_CAST "miyao_candidate_points",
          call_miyao_candidate_points, METH_VARARGS,
          CHAR_PTR_CAST "**miyao_candidate_points** (int(4, 100) *scanline_count* = 35, int(0, 100) *tolerance* = 2, int(0, 100) *staffline_height* = 0)\n\nReturns middle points of blackruns along *scanline_count*\nequidistant vertical scan lines, that are *staffline_height* plusminus\n*tolerance* pixels high.\n\nWhen *staffline_height* is given as zero (default), it is automatically\ncomputed as most frequent black vertical runlength.\n\nReturns a list of *scanline_count* lists, each representing a vertical\nscanline. The first value in each list is the *x* position, the subsequent\nentries are the *y* positions of candidate points for staffline anchors.\n"        },
                        { CHAR_PTR_CAST "miyao_distance",
          call_miyao_distance, METH_VARARGS,
          CHAR_PTR_CAST "**miyao_distance** (int *xa*, int *ya*, int *xb*, int *yb*, float(0.00, 1.00) *blackness* = 0.80, float(-1.57, 1.57) *pr_angle* = 3.14, int *penalty* = 3)\n\nReturns 0 when two points *a* and *b* ly on the same staff line\ncandidate and *penalty* if not (note that Miyao uses 2 for a mismatch instead).\nThe distance zero is only returned when the following two conditions hold:\n\n- The line angle is *pr_angle* plusminus one degree. When *pr_angle*\n  is greater than pi/2, the line angle must be within (-20,+20)\n  degrees. *pr_angle* must be given in radian.\n\n- the average blackness of the connection line is at least *blackness*\n"        },
                        { CHAR_PTR_CAST "miyao_find_edge",
          call_miyao_find_edge, METH_VARARGS,
          CHAR_PTR_CAST "**miyao_find_edge** (int *x*, int *y*, float *angle*, ``bool`` *direction*)\n\nLooks for a staff line edge point near (*x*,*y*).\n\nThe point (*x*,*y*) is extrapolated to the left (*direction* = True)\nor right (*direction* = False) with angle *angle* (in radian) until the\npixel and its two vertical neighbors are white. When (*x*,*y*) itself\nis white, the edge point is searched in the opposite direction as the\nfirst black pixel among the three vertical neighbors.\n\nReturns the new edge point as a vector [x,y]\n"        },
                        { NULL }
  };

                static PyObject* call_miyao_candidate_points(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                PyObject* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int scanline_count_arg;
                      int tolerance_arg;
                      int staffline_height_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiii:miyao_candidate_points"
                        ,
             &self_pyarg                        ,
             &scanline_count_arg                        ,
             &tolerance_arg                        ,
             &staffline_height_arg                      ) <= 0)
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
return_arg = miyao_candidate_points(*((OneBitImageView*)self_arg), scanline_count_arg, tolerance_arg, staffline_height_arg);
break;
case CC:
return_arg = miyao_candidate_points(*((Cc*)self_arg), scanline_count_arg, tolerance_arg, staffline_height_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = miyao_candidate_points(*((OneBitRleImageView*)self_arg), scanline_count_arg, tolerance_arg, staffline_height_arg);
break;
case RLECC:
return_arg = miyao_candidate_points(*((RleCc*)self_arg), scanline_count_arg, tolerance_arg, staffline_height_arg);
break;
case MLCC:
return_arg = miyao_candidate_points(*((MlCc*)self_arg), scanline_count_arg, tolerance_arg, staffline_height_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'miyao_candidate_points' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
                static PyObject* call_miyao_distance(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                int return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int xa_arg;
                      int ya_arg;
                      int xb_arg;
                      int yb_arg;
                      double blackness_arg;
                      double pr_angle_arg;
                      int penalty_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiiiiddi:miyao_distance"
                        ,
             &self_pyarg                        ,
             &xa_arg                        ,
             &ya_arg                        ,
             &xb_arg                        ,
             &yb_arg                        ,
             &blackness_arg                        ,
             &pr_angle_arg                        ,
             &penalty_arg                      ) <= 0)
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
return_arg = miyao_distance(*((OneBitImageView*)self_arg), xa_arg, ya_arg, xb_arg, yb_arg, blackness_arg, pr_angle_arg, penalty_arg);
break;
case CC:
return_arg = miyao_distance(*((Cc*)self_arg), xa_arg, ya_arg, xb_arg, yb_arg, blackness_arg, pr_angle_arg, penalty_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = miyao_distance(*((OneBitRleImageView*)self_arg), xa_arg, ya_arg, xb_arg, yb_arg, blackness_arg, pr_angle_arg, penalty_arg);
break;
case RLECC:
return_arg = miyao_distance(*((RleCc*)self_arg), xa_arg, ya_arg, xb_arg, yb_arg, blackness_arg, pr_angle_arg, penalty_arg);
break;
case MLCC:
return_arg = miyao_distance(*((MlCc*)self_arg), xa_arg, ya_arg, xb_arg, yb_arg, blackness_arg, pr_angle_arg, penalty_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'miyao_distance' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
                  } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      
                                                                                                                                                                          return_pyarg = PyInt_FromLong((long)return_arg);            return return_pyarg;
                              }
                static PyObject* call_miyao_find_edge(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                IntVector* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      int x_arg;
                      int y_arg;
                      double angle_arg;
                      int direction_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oiidi:miyao_find_edge"
                        ,
             &self_pyarg                        ,
             &x_arg                        ,
             &y_arg                        ,
             &angle_arg                        ,
             &direction_arg                      ) <= 0)
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
return_arg = miyao_find_edge(*((OneBitImageView*)self_arg), x_arg, y_arg, angle_arg, direction_arg);
break;
case CC:
return_arg = miyao_find_edge(*((Cc*)self_arg), x_arg, y_arg, angle_arg, direction_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = miyao_find_edge(*((OneBitRleImageView*)self_arg), x_arg, y_arg, angle_arg, direction_arg);
break;
case RLECC:
return_arg = miyao_find_edge(*((RleCc*)self_arg), x_arg, y_arg, angle_arg, direction_arg);
break;
case MLCC:
return_arg = miyao_find_edge(*((MlCc*)self_arg), x_arg, y_arg, angle_arg, direction_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'miyao_find_edge' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
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
              
      return_pyarg = IntVector_to_python(return_arg);
      delete return_arg;
                    return return_pyarg;
            }
                              }
            
  DL_EXPORT(void) init_staff_finding_miyao(void) {
    Py_InitModule(CHAR_PTR_CAST "_staff_finding_miyao", _staff_finding_miyao_methods);
  }
  

