find_path(CppRestSdk_INCLUDE_DIR NAMES cpprest.hpp
		HINTS
		/usr/local/include/
		/usr/include/
	}
)
 
find_library(CppRestSdk_LIBRARY NAMES cpprest)
find_library(CppRestSdk_LIBRARY_CRYPTO NAMES crypto)
find_library(CppRestSdk_LIBRARY_SSL NAMES ssl)

if(CppRestSdk_INCLUDE_DIR AND CppRestSdk_LIBRARY AND CppRestSdk_LIBRARY_CRYPTO AND CppRestSdk_LIBRARY_SSL)
  set(ntcore_FOUND TRUE)
endif()

if(CppRestSdk_LIBRARY)
	set(CppRestSdk_LIBRARIES ${CppRestSdk_LIBRARY} ${CppRestSdk_LIBRARY_CRYPTO} ${CppRestSdk_LIBRARY_SSL})
endif()

if (CppRestSdk_FOUND)
	mark_as_advanced(CppRestSdk_INCLUDE_DIR CppRestSdk_LIBRARY CppRestSdk_LIBRARIES)
endif()
