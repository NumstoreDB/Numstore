#pragma once

#define PY_ARRAY_UNIQUE_SYMBOL _pynumstore_ARRAY_API

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <smfile.h>

typedef struct txn txn_t;

// Serialize and deserialize
PyObject      *ns_dtype_serialize (PyArray_Descr *dtype);
PyArray_Descr *ns_dtype_deserialize (PyObject *s);

/** Method table appended to the module by numstore_module.c. */
extern PyMethodDef ns_dtype_methods[];
