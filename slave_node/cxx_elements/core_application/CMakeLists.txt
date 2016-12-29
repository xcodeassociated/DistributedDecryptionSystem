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

add_library(mvc ${mvc_lib})

#target_link_libraries(mvc core_application)