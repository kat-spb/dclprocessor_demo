#pragma once

#ifndef _MIDDLEBLOCK_CPP_H_
#define _MIDDLEBLOCK_CPP_H_

#include <storage/IObject.h>
#include <storage/storage_inteface.h>
#include <dclprocessor/DCLProcessor.hpp>

using namespace tmk;
using namespace tmk::middleblock;
using namespace tmk::storage;
using namespace tmk::dclprocessor;

void analyze_frame(FrameRef &frame) {
    auto img = frame().image();
    int imgSize = img.total() * img.elemSize();
    std::cout<< "frame[" << frame().id << "] imgSize: " << imgSize << std::endl;
    fflush(stdout);
}

void analyze_frameset(FramesetRef &fs) {
    int number_frames = fs().framesCount();
    std::cout << "number frames in framesets[" << fs().id << "]: " << number_frames << std::endl;
    fflush(stdout);
#if 0
    int count = 0;
    for(auto &frameID : fs().framesIDs())     {
        std::cout<< "frame" << "["<< count <<"].name: "<< frameID.name << std::endl;
        std::cout<< "frame" << "["<< count <<"].id: "  << frameID.id << std::endl;
        auto frame = fs().frame(frameID);
        analyze_frame(frame);
        ++count;
    }
#else
    if (number_frames > 0) {
        auto frame = fs().frame(fs().framesIDs()[0]);
        analyze_frame(frame);
    }
#endif
}

void analyze_object(ObjectRef<ObjectDescriptor> &object) {
    std::cout << "number framesets in object: " << object().framesetsCount() << std::endl;
    auto framesets = object().framesetsIDs();
    int count = 0;
    for(auto &framesetID : framesets)
    {
        std::cout<< "frameset" << "["<< count <<"].name: "<< framesetID.name << std::endl;
        std::cout<< "frameset" << "["<< count <<"].id:"  << framesetID.id << std::endl;
        auto fs = object().frameset(framesetID);
        analyze_frameset(fs);
        ++count;
    }
}

void WorkWithObjectDebug(ObjectDescriptor &obj, char *boost_path, char *obj_name) {
#if 1
    std::cout << "[MiddleBlock in C]: Hello, object " << obj_name
              << " from " << boost_path << std::endl;
    void *obj_ptr;
    fprintf(stdout, "[MiddleBlock]: %s from %s\n", obj_name, boost_path);
    fflush(stdout);
    storage_interface_get_object(&obj_ptr, dclproc->boost_path, obj_name);
    ObjectDescriptor &obj = *(ObjectDescriptor*)obj_ptr;
    storage_interface_analyze_object_with_framesets(dclproc->boost_path, obj_name);
#else
    auto framesets = obj.framesetsIDs();
    for(auto &framesetID : framesets) {
        auto fs = obj.frameset(framesetID);
        for(auto &frameID : fs().framesIDs()) {
            auto frame = fs().frame(frameID);
            auto img = frame().image(); //cv::Mat
            cv::imshow("image", img);
            cv::waitKey(0);
        }
    }
#endif
}

void TestUserData(void *user_data_ptr) {
    MiddleBlockData *mb = reinterpret_cast<MiddleBlockData*>user_data_ptr;
    //MiddleBlockData *mb = static_cast<MiddleBlockData*>user_data_ptr; 
    //MiddleBlock = *(MiddleBlockData*)user_data_ptr;
    std::cout << "mb.n = " << mb.n << std::endl;
}

void WorkWithObject(void *user_data_ptr, ObjectDescriptor &obj) {
#if 0
    (void)user_data_ptr;
#else
    (void)obj;
    MiddleBlockData *mb = reinterpret_cast<MiddleBlockData*>user_data_ptr;
    //MiddleBlockData *mb = static_cast<MiddleBlockData*>user_data_ptr; 
    //MiddleBlock = *(MiddleBlockData*)user_data_ptr;
    std::cout << "mb.n = " << mb.n << std::endl;
#endif

#if 1
    (void)obj;
#else
    auto framesets = obj.framesetsIDs();
    for(auto &framesetID : framesets) {
        auto fs = obj.frameset(framesetID);
        for(auto &frameID : fs().framesIDs()) {
            auto frame = fs().frame(frameID);
            auto img = frame().image(); //cv::Mat
            cv::imshow("image", img);
            cv::waitKey(0);
        }
    }
#endif
}

void main()
{
  MiddleBlockData mb(3);
  DCLProcessor dclproc(PMT_ALGO); //or PMT_RESULT

  // global block data for usage in the callback with new interface:
  // void WorkWithObject(void *user_data_ptr, ObjectDescriptor &obj, char *boost_path, char *obj_name)
  dclproc.setUserData((void*)&mb); // --> &mb return as user_data_ptr in the callback ??? C++ impementatin
  dclproc.registerDebugCallback(&WorkWithObject);
  dclproc.registerCallback(&WorkWithObjectAndUserData);
}

} //namespace tmk::dclprocessor

#endif //_DCLPROCESSOR_CPP_H_

