#include <stdlib.h>
#include <string.h>

#include <misc/log.h>
#include <storage/storage_interface.h>
#include <processor/dclprocessor.h>

//#include <storage/ObjectDescriptor.h>

void work_with_object(void *user_data_ptr, char *boost_path, char *obj_name) {
    if (!user_data_ptr) {
        LOG_FD_INFO("[middle]: Hello, object '%s' from '%s' and no user data", obj_name,  boost_path);
    }
    else {
        LOG_FD_INFO("[middle]: Hello, object '%s' from '%s' and user_data '%s'", obj_name,  boost_path, (char *)user_data_ptr);
    }
    //this is a place for maniplation with the object
    storage_interface_analyze_object_with_framesets(boost_path, obj_name);
    //ObjectDescriptor
    //storage_interface_get_object(void **obj_ptr, boost_path, obj_name);
}

int main(){
    char *user_data_ptr = strdup("user data");
    struct dclprocessor *proc = dclprocessor_get(2);
    attach_to_pipeline(proc, user_data_ptr, work_with_object);
    free(user_data_ptr);
    return 0;
}

