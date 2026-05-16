add_library(scripting
  ScriptingEngine.cpp
  ScriptingEngine.h
  Python/PyScriptingBackend.cpp
  Python/PyScriptingBackend.h
  Python/Modules/controllermodule.cpp
  Python/Modules/controllermodule.h
  Python/Modules/doliomodule.cpp
  Python/Modules/doliomodule.h
  Python/Modules/dolphinmodule.cpp
  Python/Modules/dolphinmodule.h
  Python/Modules/eventmodule.cpp
  Python/Modules/eventmodule.h
  Python/Modules/emulationmodule.cpp
  Python/Modules/emulationmodule.h
  Python/Modules/guimodule.cpp
  Python/Modules/guimodule.h
  Python/Modules/memorymodule.cpp
  Python/Modules/memorymodule.h
  Python/Modules/registersmodule.cpp
  Python/Modules/registersmodule.h
  Python/Modules/savestatemodule.cpp
  Python/Modules/savestatemodule.h
  Python/Utils/as_py_func.h
  Python/Utils/convert.h
  Python/Utils/fmt.h
  Python/Utils/invoke.h
  Python/Utils/module.h
  Python/Utils/object_wrapper.cpp
  Python/Utils/object_wrapper.h
)

# On Windows, prefer the bundled Python from the Externals/python submodule so
# developers don't need a separate Python install. These hints are only applied
# when the submodule has been initialised; they can be overridden by passing
# -DPython3_ROOT_DIR (or the individual variables) to CMake.
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/Externals/python/x64/libs/python312.lib")
  set(Python3_ROOT_DIR    "${CMAKE_SOURCE_DIR}/Externals/python/x64" CACHE PATH     "" FORCE)
  set(Python3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Externals/python/x64/include"        CACHE PATH     "" FORCE)
  set(Python3_LIBRARY     "${CMAKE_SOURCE_DIR}/Externals/python/x64/libs/python312.lib" CACHE FILEPATH "" FORCE)
endif()

find_package(Python3 REQUIRED COMPONENTS Development)
include_directories(${Python3_INCLUDE_DIRS})
# TODO imgui shouldn't be here
target_link_libraries(scripting PRIVATE ${Python3_LIBRARIES} imgui core)
