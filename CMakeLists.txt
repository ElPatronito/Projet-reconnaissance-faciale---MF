cmake_minimum_required(VERSION 3.16)
project(mon_app LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

qt_standard_project_setup()

qt_add_executable(mon_app
    mainIHM.cpp
    mainwindow.cpp
    mainwindow.h
    Reco.cpp
    RecognitionPipeline.h
    PCA.cpp
    PCA.h
    LDA.cpp
    LDA.h
    Prep.cpp
    Preprocesseur.h
)

include_directories(
    /usr/include/opencv4
    /home/cytech/Projet
    /home/cytech/anaconda3/lib/python3.13/site-packages/tensorflow/include
    /home/cytech/ING1/Projet_Reconnaissance_Faciale
)
target_link_libraries(mon_app PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
    mysqlcppconn
)