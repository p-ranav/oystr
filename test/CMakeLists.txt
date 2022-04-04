# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(searchTests LANGUAGES CXX)

# ---- Tests ----

add_executable(search_test source/search_test.cpp)
target_link_libraries(search_test PRIVATE search_lib)
target_compile_features(search_test PRIVATE cxx_std_17)

add_test(NAME search_test COMMAND search_test)

# ---- End-of-file commands ----

add_folders(Test)
