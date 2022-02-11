# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(vch VERSION 1.0 DESCRIPTION "Virtual Console Host TCP")

if(UNIX AND NOT APPLE) # linux
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	set(CMAKE_HAVE_THREADS_LIBRARY 1)
	set(CMAKE_USE_WIN32_THREADS_INIT 0)
	set(CMAKE_USE_PTHREADS_INIT 1)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	set(HAVE_CXX_ATOMICS64_WITHOUT_LIB True)
	set(HAVE_CXX_ATOMICS_WITHOUT_LIB True)
endif()

file(GLOB_RECURSE source_list "app/*.cpp" "app/*.hpp")
add_executable(${PROJECT_NAME} ${source_list})
#file(GLOB ${PROJECT_NAME} CONFIGURE_DEPENDS "*.h" "*.cpp")

#set(INCLUDE_DIRECTORIES "libs/Lunaris")

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
  "libs"
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17 # or 20 if you want something more recent
    CXX_STANDARD_REQUIRED ON
 )
