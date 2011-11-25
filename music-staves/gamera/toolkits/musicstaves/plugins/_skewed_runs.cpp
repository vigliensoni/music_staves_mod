
        
    
  #include "gameramodule.hpp"
  #include "knnmodule.hpp"

        #include "skewed_runs.hpp"
  
    #include <string>
  #include <stdexcept>
  #include "Python.h"
  #include <list>

  using namespace Gamera;
  
        
      extern "C" {
#ifndef _MSC_VER
    void init_skewed_runs(void);
#endif
                  static PyObject* call_keep_tall_skewed_runs(PyObject* self, PyObject* args);
            }

          static PyMethodDef _skewed_runs_methods[] = {
                  { CHAR_PTR_CAST "keep_tall_skewed_runs",
          call_keep_tall_skewed_runs, METH_VARARGS,
          CHAR_PTR_CAST "**keep_tall_skewed_runs** (float(-85.00, 85.00) *minangle*, float(-85.00, 85.00) *maxangle*, int *height*, [object *points*], ``ChoiceString(strict)`` [both|up|down] *direction*)\n\nRemoves all black pixels except those belonging to black runs taller\nthan a given height in a given angle range.\n\nArguments:\n\n.. image:: images/skewed_tall_runs.png\n\n*minangle*, *maxangle*:\n  Angle range that is scanned for tall runs. Angles are measured in degrees\n  and relative to the vertical axis; thus in the image above *minangle*\n  is about -30 and *maxangle* about 45. When both angles are zero, the\n  function behaves like ``filter_short_runs``.\n\n*height*:\n  When *points* is not given, only black runs taller than *2 * height* are kept.\n  Otherwise the condition depends on the parameter *direction* (see below).\n\n*points*:\n  When given, only tall runs crossing these points are kept.\n  Additionally they must run at least *height* in the given *direction*.\n  For performance reasons, of each skewed run\n  only the part up to *2 * height* around the given points is kept.\n  When empty or omitted, all tall skewed runs are kept.\n\n*direction*:\n  When ``both``, skewed runs must run at least *height* both in the upper\n  and lower vertical direction. When ``up`` or ``down``, the only need to\n  run in one direction starting from the given points. Only has effect\n  when additionally *poins* is given.\n    "        },
              { NULL }
  };

                static PyObject* call_keep_tall_skewed_runs(PyObject* self, PyObject* args) {
            
      PyErr_Clear();
                                                                                Image* return_arg;
PyObject* return_pyarg;
                                          Image* self_arg;
PyObject* self_pyarg;
                      double minangle_arg;
                      double maxangle_arg;
                      int height_arg;
                      PointVector* points_arg;
PyObject* points_pyarg;
                      char* direction_arg;
      
                                      if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OddiOs:keep_tall_skewed_runs"
                        ,
             &self_pyarg                        ,
             &minangle_arg                        ,
             &maxangle_arg                        ,
             &height_arg                        ,
             &points_pyarg                        ,
             &direction_arg                      ) <= 0)
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
return_arg = keep_tall_skewed_runs(*((OneBitImageView*)self_arg), minangle_arg, maxangle_arg, height_arg, points_arg, direction_arg);
break;
case CC:
return_arg = keep_tall_skewed_runs(*((Cc*)self_arg), minangle_arg, maxangle_arg, height_arg, points_arg, direction_arg);
break;
case ONEBITRLEIMAGEVIEW:
return_arg = keep_tall_skewed_runs(*((OneBitRleImageView*)self_arg), minangle_arg, maxangle_arg, height_arg, points_arg, direction_arg);
break;
case RLECC:
return_arg = keep_tall_skewed_runs(*((RleCc*)self_arg), minangle_arg, maxangle_arg, height_arg, points_arg, direction_arg);
break;
case MLCC:
return_arg = keep_tall_skewed_runs(*((MlCc*)self_arg), minangle_arg, maxangle_arg, height_arg, points_arg, direction_arg);
break;
default:
PyErr_Format(PyExc_TypeError,"The 'self' argument of 'keep_tall_skewed_runs' can not have pixel type '%s'. Acceptable values are ONEBIT, ONEBIT, ONEBIT, ONEBIT, and ONEBIT.", get_pixel_type_name(self_pyarg));
return 0;
}
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
              return_pyarg = create_ImageObject(return_arg);              return return_pyarg;
            }
                              }
      
  DL_EXPORT(void) init_skewed_runs(void) {
    Py_InitModule(CHAR_PTR_CAST "_skewed_runs", _skewed_runs_methods);
  }
  

