#pragma once

#ifndef _DCLPROCESSOR_CPP_H_
#define _DCLPROCESSOR_CPP_H_

#include <storage/ObjectDescriptor.h>
#include <processor/dclprocessor.h>

using namespace tmk;
using namespace tmk::storage;

// Object -- main unit in the system
// PMT_CAPTURE -- process typr for streaming sources and splitting raw data into 'Object's with default or custom 'ObjectDetectionCallback'
// PMT_ALGO -- for reading Object and work with it in the 'WorkWithObjectCallback', possible create new  in the Object 
// PMT_RESULT -- for reading Object 
enum class ProcessModuleType { PMT_MANAGER = -1, PMT_SERVER = 0, PMT_CAPTURE = 1, PMT_ALGO = 2, PMT_RESULT = 3 };

namespace tmk::dclprocessor {

class DCLProcess {
public:
    DCLProcess(ProcessModuleType _type = ProcessModuleType::PMT_ALGO, void *user_data_ptr = NULL, void (*callbackFunc)(void *user_data_ptr, ObjectDescriptor &obj) = NULL);
    DCLProcess(ProcessModuleType _type = ProcessModuleType::PMT_ALGO, void (*callbackFunc)(ObjectDescriptor &obj, char *boost_path, char *obj_name) = NULL);
private:
    ProcessModuleType mType;
    struct dclprocessor *dclProc;
};

} //namespace tmk::dclprocessor

#endif //_DCLPROCESSOR_CPP_H_

