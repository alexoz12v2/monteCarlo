#inspired from Sascha Willelms vulkan examples
function(buildExec EXEC_NAME)
	set(EXEC_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXEC_NAME})
	message(STATUS "Time to build ${EXEC_FOLDER}")

	# file(GLOB <variable> globbingExpressions)
	# Generate a list of files that match the <globbing-expressions> and store it into the <variable>
	# where globbing expressions are simplified regular expressions
	file(GLOB SOURCE *.cpp )

	# set main file name
	set(EXEC_CPP ${EXEC_FOLDER}/${EXEC_NAME}.cpp)

	#add shader directories
	set(SHADER_DIR ${SHADER_PARENT_DIR}/${EXEC_NAME})
	message(STATUS ${CMAKE_CXX_FLAGS})

	#executable, libraries
	add_executable(${EXEC_NAME} ${EXEC_CPP})
	target_compile_definitions(${EXEC_NAME} PRIVATE SHADER_DIR="${SHADER_DIR}")
	target_compile_features(${EXEC_NAME} PRIVATE cxx_std_20)
	target_link_libraries(${EXEC_NAME} ${EXEC_LIBS})

endfunction(buildExec)

set(EXECS test cubeTest)

foreach(EXEC ${EXECS})
	buildExec(${EXEC})
endforeach(EXEC)
