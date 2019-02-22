/*
 *  This file is part of esynth.
 *
 *  esynth is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  esynth is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with esynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
 
#include <iostream>
#include <vector>
 
#include <Python.h>
#include <numpy/arrayobject.h>
 
#include <openbabel/mol.h>
#include <openbabel/obconversion.h>
#include <openbabel/fingerprint.h>
 
#include "PythonInterface.h"
#include "Options.h"
 
// Static allocation
PythonInterface* PythonInterface::_instance = 0;
std::string PythonInterface::_programName = "esynth extension";

//
// Initializes this object to interface with python code.
//
PythonInterface::PythonInterface() :
  _fpType(0),
  _pModule(0),
  _pFunc(0)
{
    if (!openbabelInit())
    {
        std::cerr << "Open Babel failed to initialize for prediction." << std::endl;
        throw "Open Babel failed to initialize for prediction.";
    }

    if (!pythonInit())
    {
        std::cerr << "Python failed to initialize for prediction." << std::endl;
        throw "Python failed to initialize for prediction.";
    }
}

//
// Initialize all python related functionality.
//
bool PythonInterface::openbabelInit()
{
    _fpType = OpenBabel::OBFingerprint::FindFingerprint("");
    
    return _fpType;
}

//
// Initialize all python related functionality.
//
bool PythonInterface::pythonInit()
{
    Py_SetProgramName((char*)_programName.c_str());
    
    // Initialize the python interpreter
    Py_Initialize();
    PyObject* pName = PyString_FromString(Options::PYTHON_MODULE_NAME.c_str());

    _pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    import_array();

    if (!_pModule)
    {
        PyErr_Print();
        std::cerr << "Python predictor module "
                  << Options::PYTHON_MODULE_NAME
                  << " failed to load" << std::endl;
        return false;
    }

    // pFunc is a new reference
    _pFunc = PyObject_GetAttrString(_pModule, Options::PYTHON_FUNCTION_NAME.c_str());

    if (!_pFunc)
    {
        if (PyErr_Occurred()) PyErr_Print();
        std::cerr << "Python predictor function "
                  << Options::PYTHON_FUNCTION_NAME
                  << " failed to initialize" << std::endl;
        return false;
    }

    // Check for callability of the module.function
    if (!PyCallable_Check(_pFunc))
    {
        std::cerr << "Python predictor function is not callable" << std::endl;
        return false;
    }

    // Success!
    std::cout << "Python predictor module (" << Options::PYTHON_MODULE_NAME
              << ") and function (" << Options::PYTHON_FUNCTION_NAME
              << ") initialized properly";

    return true;
}

//
// Clean up all python related functionality.
//
void PythonInterface::pythonCleanup()
{
    // Kill the module and function python objects
    Py_XDECREF(_pFunc);
    Py_DECREF(_pModule);

    // Kill the python interpreter
    Py_Finalize();
}

//
// External interface function predictor
// @param fp--1024 binary vector fingerprint representing a molecule
// @return a boolean whether smi meets the parameters of being acceptable 
//
bool PythonInterface::filterOut(const std::vector<unsigned int>& fp) const
{
    // Set up input arguments
    PyObject* pArgs = reinterpret_cast<PyObject*>(createNumpyArray(fp));

    // Call the python predictor function
    PyObject* pValue = PyObject_CallObject(_pFunc, pArgs);
    
    // Kill the arguments
    Py_DECREF(pArgs);

    //
    if (!pValue)
    {
        PyErr_Print();
        std::cerr << "Python call failed" << std::endl;
    }
    
    // We expect a floating point value being returned from the predictor
    if (!PyFloat_Check(pValue))
    {
        std::cerr << "Predictor did not return a float data type as expected" << std::endl;
        throw "Predictor did not return a float data type as expected";
    }
    
    double sa = PyFloat_AsDouble(pValue);
    if (PyErr_Occurred())
    {
        PyErr_Print();
        std::cerr << "An error occured converting the predicted value to double ("
                  << sa << ")" << std::endl;
        throw "An error occured converting the predicted value to double ";
    }

    // Kill the value object    
    Py_DECREF(pValue);

    // Does this value meet the threshold?
    return exceedsPredictionThreshold(sa);
}

//
// External interface function predictor
// @param mol--OpenBabel molecule representation
// @return a boolean whether smi meets the parameters of being acceptable:
//         converts the openbabel representation to a fingerprint
//
bool PythonInterface::exceedsPredictionThreshold(double sa) const
{
    return sa > Options::SA_THRESHOLD;
}

//
// External interface function predictor
// @param mol--OpenBabel molecule representation
// @return a boolean whether smi meets the parameters of being acceptable:
//         converts the openbabel representation to a fingerprint
//
bool PythonInterface::filterOut(OpenBabel::OBMol& mol) const
{
    // Create and acquire the fingerprint
    std::vector<unsigned> fp;
    _fpType->GetFingerprint(&mol, fp);
    
    // Predict based on the fingerprint
    return filterOut(fp);
}

//
// @param fp--molecule fingerprint (as a vector)
// @return numpy array representation (1024-bit a boolean whether smi meets the parameters of being acceptable:
//         converts the openbabel representation to a fingerprint
//
PyArrayObject* PythonInterface::createNumpyArray(const std::vector<unsigned int>& fp) const
{
    const int FP_SIZE = 1024;
    if (fp.size() * sizeof(unsigned int) * CHAR_BIT != FP_SIZE)
    {
        std::cout << "createNumpyArray:: fingerprint size discrepancy: "
                  << "fp vector size (" << fp.size() << ")"
                  << " * " << sizeof(unsigned int) << " * " << CHAR_BIT
                  << " = " << fp.size() * sizeof(unsigned int) * CHAR_BIT
                  << " != " << FP_SIZE
                  << std::endl;
    }
            
    // http://stackoverflow.com/questions/30357115/pyarray-simplenewfromdata-example
    // Convert it to a NumPy array.
    npy_intp dimensions[1];
    dimensions[0] = fp.size(); // 1024; // Exact fingerprint size
    PyObject* pArray = PyArray_SimpleNewFromData(1, dimensions, NPY_UINT32, reinterpret_cast<const void*>(&fp[0]));
    if (!pArray)
    {
        std::cerr << "createNumpyArray::Failed to create Numpy array from fingerprint " << std::endl;
        return 0;
    }
        
    return reinterpret_cast<PyArrayObject*>(pArray);
}



/* int _tmain(int argc, _TCHAR* argv[])
{
    int result = EXIT_FAILURE;

    Py_SetProgramName(argv[0]);
    Py_Initialize();
    import_array();

    // Build the 2D array in C++
    const int SIZE = 10;
    npy_intp dims[2]{SIZE, SIZE};
    const int ND = 2;
    long double(*c_arr)[SIZE]{ new long double[SIZE][SIZE] };
    if (!c_arr) {
        std::cerr << "Out of memory." << std::endl;
        goto fail_c_array;
    }
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            c_arr[i][j] = i * SIZE + j;

    // Convert it to a NumPy array.
    PyObject *pArray = PyArray_SimpleNewFromData(
        ND, dims, NPY_LONGDOUBLE, reinterpret_cast<void*>(c_arr));
    if (!pArray)
        goto fail_np_array;
    PyArrayObject *np_arr = reinterpret_cast<PyArrayObject*>(pArray);

    // import mymodule.array_tutorial
    const char *module_name = "mymodule";
    PyObject *pName = PyUnicode_FromString(module_name);
    if (!pName)
        goto fail_name;
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (!pModule)
        goto fail_import;
    const char *func_name = "array_tutorial";
    PyObject *pFunc = PyObject_GetAttrString(pModule, func_name);
    if (!pFunc)
        goto fail_getattr;
    if (!PyCallable_Check(pFunc)){
        std::cerr << module_name << "." << func_name
                  << " is not callable." << std::endl;
        goto fail_callable;
    }

    // np_ret = mymodule.array_tutorial(np_arr)
    PyObject *pReturn = PyObject_CallFunctionObjArgs(pFunc, pArray, NULL);
    if (!pReturn)
        goto fail_call;
    if (!PyArray_Check(pReturn)) {
        std::cerr << module_name << "." << func_name
                  << " did not return an array." << std::endl;
        goto fail_array_check;
    }
    PyArrayObject *np_ret = reinterpret_cast<PyArrayObject*>(pReturn);
    if (PyArray_NDIM(np_ret) != ND - 1) {
        std::cerr << module_name << "." << func_name
                  << " returned array with wrong dimension." << std::endl;
        goto fail_ndim;
    }

    // Convert back to C++ array and print.
    int len = PyArray_SHAPE(np_ret)[0];
    c_out = reinterpret_cast<long double*>(PyArray_DATA(np_ret));
    std::cout << "Printing output array" << std::endl;
    for (int i = 0; i < len; i++)
        std::cout << c_out[i] << ' ';
    std::cout << std::endl;
    result = EXIT_SUCCESS;

fail_ndim:
fail_array_check:
    Py_DECREF(pReturn);
fail_call:
fail_callable:
    Py_DECREF(pFunc);
fail_getattr:
    Py_DECREF(pModule);
fail_import:
fail_name:
    Py_DECREF(pArray);
fail_np_array:
    delete[] c_arr;
fail_c_array:
    if (PyErr_Check())
        PyErr_Print();  
    PyFinalize();
    return result;
} */
