#pragma once
#ifndef TECOSG_COMMON_H
#define TECOSG_COMMON_H

#include <tecosg/Export.h>

// Define NULL pointer value
#ifndef NULL
    #ifdef  __cplusplus
        #define NULL	0
    #else
        #define NULL	((void *)0)
    #endif
#endif

#endif
