# Install script for directory: D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/out/install/x64-Debug (default)")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/out/build/x64-Debug (default)/LandmarkDetector.lib")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenFace" TYPE FILE FILES
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/CCNF_patch_expert.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/CEN_patch_expert.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/CNN_utils.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/FaceDetectorMTCNN.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkCoreIncludes.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkDetectionValidator.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkDetectorFunc.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkDetectorModel.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkDetectorParameters.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/LandmarkDetectorUtils.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/Patch_experts.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/PAW.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/PDM.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/SVR_patch_expert.h"
    "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/include/stdafx.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/File Store/Blob Documents/Visual Studio 2019/Projects/FaceSegTest/lib/local/LandmarkDetector/out/build/x64-Debug (default)/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
