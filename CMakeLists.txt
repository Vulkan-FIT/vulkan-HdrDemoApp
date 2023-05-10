cmake_minimum_required(VERSION 3.11.0)

set(NAME hdrDemoApp)

project(${NAME})

# 1. Set VULKAN_SDK_PATH in .env.cmake to target specific vulkan version
# first branch is irrelevant, because .env.cmake file doesnt exist
if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include") # 1.1 Make sure this include path is correct
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib") # 1.2 Make sure lib path is correct
  set(Vulkan_FOUND "True")
  message(STATUS "Defined VulkanSDK")
else()
  message(STATUS "In else")
  find_package(Vulkan REQUIRED)
  set(Vulkan_INCLUDE_DIRS "/VulkanSDK/Include") # 1.1 Make sure this include path is correct
  set(Vulkan_LIBRARIES "/VulkanSDK/Lib") # 1.2 Make sure lib path is correct
  message(STATUS "Not defined VulkanSDK")
endif()



# append source dir (hda_hdrDemoApp) in list of folders where find to cmake modules (FindGLWF.cmake, FindVulkan.cmake etc.)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")



if (DEFINED GLFW_PATH)
  message(STATUS "Using GLFW path specified in .env")
  set(GLFW_INCLUDE_DIRS "${GLFW_PATH}/include")
  if (MSVC)
    set(GLFW_LIB "${GLFW_PATH}/lib-vc2022") # 2.1 Update lib-vc2019 to use same version as your visual studio
  elseif (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    message(STATUS "USING MINGW")
    set(GLFW_LIB "${GLFW_PATH}/lib-mingw-w64") # 2.1 make sure matches glfw mingw subdirectory
  endif()
else()
  message(STATUS "In else")
  find_package(glfw3 REQUIRED)
  set(GLFW_LIB "/glfw-3.3.8.bin.WIN64/lib-vc2022")
  set(GLFW_INCLUDE_DIRS "/glfw-3.3.8.bin.WIN64/include")
  message(STATUS "Found GLFW")
endif()




set(SOURCES main.cpp hda_window.cpp hda_instancegpu.cpp hda_hdrdemoapp.cpp builder.cpp hda_swapchain.cpp hda_pipeline.cpp hda_model.cpp)
set(INCLUDES hda_window.hpp hda_instancegpu.hpp hda_hdrdemoapp.hpp builder.hpp hda_swapchain.hpp hda_pipeline.hpp hda_model.hpp)
set(APP_SHADERS shader.vert shader.frag skybox.vert skybox.frag)



#######################
# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS 
  ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
  /usr/bin 
  /usr/local/bin 
  ${VULKAN_SDK_PATH}/Bin
  ${VULKAN_SDK_PATH}/Bin32
  $ENV{VULKAN_SDK}/Bin/ 
  $ENV{VULKAN_SDK}/Bin32/
)

macro(add_shaders nameList depsList)
	foreach(name ${nameList})
		get_filename_component(directory ${name} DIRECTORY)
		if(directory)
			file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${directory}")
		endif()
		add_custom_command(COMMENT "Converting ${name} to spir-v..."
		                   MAIN_DEPENDENCY ${name}
		                   OUTPUT ${name}.spv
		                   COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} --target-env vulkan1.0 -x ${CMAKE_CURRENT_SOURCE_DIR}/${name} -o ${name}.spv)
		source_group("Shaders" FILES ${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
		list(APPEND ${depsList} ${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
	endforeach()
endmacro()
#xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${PROJECT_NAME} ${SOURCES} ${INCLUDES} ${APP_SHADER_DEPS})

#target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

if (WIN32)
  message(STATUS "CREATING BUILD FOR WINDOWS")


  target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${TINYOBJ_PATH}
    ${GLFW_INCLUDE_DIRS}
    ${GLM_PATH}
    )

  target_link_directories(${PROJECT_NAME} PUBLIC
    ${Vulkan_LIBRARIES}
    ${GLFW_LIB}
  )

  target_link_libraries(${PROJECT_NAME} glfw3 Vulkan::Vulkan) # driv tam bylo vulkan-1
elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${PROJECT_SOURCE_DIR}/src
      ${TINYOBJ_PATH}
    )
    target_link_libraries(${PROJECT_NAME} glfw3 ${Vulkan_LIBRARIES})
endif()



############## Build SHADERS #######################

