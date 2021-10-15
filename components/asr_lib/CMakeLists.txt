################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "include_private")
###############################################

############## Add source files ###############
# list(APPEND ADD_SRCS  "src/lib1.c"
#     )
# aux_source_directory(src ADD_SRCS)  # collect all source file in src dir, will set var ADD_SRCS
append_srcs_dir(ADD_SRCS "src")     # append source file in src dir to var ADD_SRCS
# list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS asound pthread)
###############################################

############ Add static libs ##################
list(APPEND ADD_STATIC_LIB "lib/x86/libms_asr.a" "lib/x86/libMNN_x86.a")
###############################################

############ Add dynamic libs ##################
# list(APPEND ADD_DYNAMIC_LIB "lib/arch/v831/libmaix_nn.so"
#                             "lib/arch/v831/libmaix_cam.so"
# )
###############################################

#### Add compile option for this component ####
#### Just for this component, won't affect other 
#### modules, including component that depend 
#### on this component
# list(APPEND ADD_DEFINITIONS_PRIVATE -DAAAAA=1)

#### Add compile option for this component
#### and components denpend on this component
list(APPEND ADD_DEFINITIONS -DINFER_TYPE=2)
###############################################

############ Add static libs ##################
#### Update parent's variables like CMAKE_C_LINK_FLAGS
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group libmaix/libtest.a -ltest2 -Wl,--end-group" PARENT_SCOPE)
###############################################

# register component, DYNAMIC or SHARED flags will make component compiled to dynamic(shared) lib
register_component()

