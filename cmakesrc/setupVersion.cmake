if(COMMAND cmake_policy)
  cmake_policy(set CMP0003 NEW)
endif(COMMAND cmake_policy)


# project version
set( PROJECT_MAJOR_VERSION 5 )
set( PROJECT_MINOR_VERSION 00 )
set( PROJECT_PATCH_LEVEL 07 )



if ( NOT PROJECT_PATCH_LEVEL EQUAL 00 )
 message( "Building a development version" )
 set (BINARY_POSTFIX "_dev")
 set (PKG_POSTFIX "-dev")
endif ( NOT PROJECT_PATCH_LEVEL EQUAL 00 )
