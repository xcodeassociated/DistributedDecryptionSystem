set(mvc_lib
        controller/Controller.cpp
        controller/Controller.hpp
        model/Model.cpp
        model/Model.hpp
        view/View.cpp
        view/View.hpp)

include_directories(controller)
include_directories(model)
include_directories(view)

## keyword STATIC for static lib linking
add_library(mvc SHARED
        ${mvc_lib})
