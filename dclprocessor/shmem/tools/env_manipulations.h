//
// Created by victor on 09.09.2021.
//

#ifndef TMK_DCL_ENV_MANIPULATIONS_H
#define TMK_DCL_ENV_MANIPULATIONS_H


namespace tmk {
    void storeAndCleanEnv(char **path_old, char **size_old);
    void restoreEnv(char **path_old, char **size_old);
};


#endif //TMK_DCL_ENV_MANIPULATIONS_H
