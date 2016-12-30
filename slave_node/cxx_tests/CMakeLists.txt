project(slave_node_tests)

## only for gtests
#add_subdirectory(test_libs/googletest)

## gmock contains also gtests
add_subdirectory(test_libs/googlemock)
add_subdirectory(test_source)