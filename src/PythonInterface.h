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
 
#ifndef _PYTHON_INTERFACE_GUARD
#define _PYTHON_INTERFACE_GUARD 1

#include <Python.h>

#include <vector>
 
#include <openbabel/mol.h>
#include <openbabel/obconversion.h>
#include <openbabel/fingerprint.h>
 
//
// Singleton class implementation
//
class PythonInterface
{
  //
  // Singleton-based functionality (begin)
  //
  private:
    PythonInterface();
    static PythonInterface* _instance;
    static std::string _programName;
   
  public:
    static PythonInterface* getInstance()
    {
        if (!_instance) _instance = new PythonInterface();
        return _instance;
    }
    
    // Static destructor of sorts
    void cleanup()
    {
        pythonCleanup();
        if (_instance) delete _instance;
        _instance = 0;
    }
  //
  // Singleton-based functionality (end)
  //

  virtual ~PythonInterface() {}

  //
  // Python-related functionality (begin)
  //  
  public:
    //
    // External interface function predictor
    //
    bool filterOut(const std::vector<unsigned int>& fp) const;
    bool filterOut(OpenBabel::OBMol& mol) const;

  private:
    bool openbabelInit();
    bool pythonInit();
    void pythonCleanup();
    bool exceedsPredictionThreshold(double) const;
    PyArrayObject* createNumpyArray(const std::vector<unsigned int>& fp) const;
    
    // Open Babel
    OpenBabel::OBFingerprint* _fpType;

    // Python
    PyObject *_pModule, *_pFunc;
  //
  // Python-related functionality (end)
  //
};
 
#endif
