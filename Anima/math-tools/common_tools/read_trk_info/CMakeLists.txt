if(BUILD_TOOLS)

project(animaReadTRKInfo)

## #############################################################################
## Find VTK
## #############################################################################
find_package(VTK REQUIRED COMPONENTS
  CommonCore
  IOLegacy
)
if (VTK_VERSION VERSION_LESS "8.90.0")
  include(${VTK_USE_FILE})
endif()


## #############################################################################
## List Sources
## #############################################################################

list_source_files(${PROJECT_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

  

## #############################################################################
## add executable
## #############################################################################

add_executable(${PROJECT_NAME}
  ${${PROJECT_NAME}_CFILES}
  )

## #############################################################################
## Include Directories
## #############################################################################
target_include_directories(${PROJECT_NAME} PRIVATE
  ${VTK_INCLUDE_DIRS}
  # Ajoutez d'autres répertoires d'inclusion si nécessaire
)


## #############################################################################
## Link
## #############################################################################

target_link_libraries(${PROJECT_NAME}
  AnimaDataIO
  ${ITKIO_LIBRARIES}
  ${VTK_LIBRARIES} # Ajout des bibliothèques VTK
  )

## #############################################################################
## install
## #############################################################################

set_exe_install_rules(${PROJECT_NAME})

endif()