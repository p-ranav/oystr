# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(oystrTests LANGUAGES CXX)

# ---- Tests ----

add_executable(oystr_test source/oystr_test.cpp)
target_link_libraries(oystr_test PRIVATE oystr_lib)
target_compile_features(oystr_test PRIVATE cxx_std_17)

add_test(NAME oystr_test COMMAND oystr_test)

# ---- End-of-file commands ----

add_folders(Test)
