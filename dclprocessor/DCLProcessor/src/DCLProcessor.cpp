#include <iostream>

#include <processor/dclprocessor_old.h>
#include <processor/dclprocessor.h>
#include <DCLProcessor/DCLProcessor.hpp>

namespace tmk::dclprocessor {

extern "C" char **environ;

DCLProcess::DCLProcess(ProcessModuleType _type, void *user_data_ptr, void (*callbackFunc)(void *user_data_ptr, ObjectDescriptor &obj)) {
    struct dclprocessor *proc = get_dclprocessor();
    enum PC_TYPE cType;
    //check enviroments and read process info from map in the shared memory
    //struct dclprocessor *p = get_dclprocessor();
    //dclProc = *p; 
    switch (_type) {
        case ProcessModuleType::PMT_SERVER:
            cType = PC_FIRST;
            break;
        default:
            cType = PC_MIDDLE;
    }
    //? cType = get_dclprocessor_type(&p->pipeline);
    attach_to_pipeline(proc, NULL, work_with_object);
}

//OLD version, will be deleted after testing the new one
DCLProcess::DCLProcess(ProcessModuleType _type, void (*callbackFunc)(ObjectDescriptor &obj, char *boost_path, char *obj_name)):
    //check enviroments and read process info from map in the shared memory
    struct dclprocessor *p = get_dclprocessor();
    dclProc = *p;
    switch (_type) {
        case ProcessModuleType::PMT_SERVER:
            cType = PC_FIRST;
            break;
        default:
            cType = PC_MIDDLE;
    }
    connect_to_pipeline(cType, user_data_ptr, callbackFunc);
}

} //namespace tmk::dclprocessor
