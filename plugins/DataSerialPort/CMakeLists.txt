
include_directories( ./ ../  ../../include  ../../common ${Qt5WebSockets_INCLUDE_DIRS})

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SERIALPORT)

find_package(Qt5 REQUIRED COMPONENTS SerialPort)

#QT5_WRAP_UI ( UI_SRC  ../common/selectlistdialog.ui  )


SET( SRC
    dataserialportreader.cpp
    ../../include/PlotJuggler/datastreamer_base.h
    )

add_library(DataSerialPort SHARED ${SRC} ${UI_SRC}  )
target_link_libraries(DataSerialPort  ${Qt5Widgets_LIBRARIES} Qt5::SerialPort Qt5::Xml)

if(COMPILING_WITH_CATKIN)
	install(TARGETS DataSerialPort
            ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
            LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
            RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
	install(TARGETS DataSerialPort DESTINATION bin  )
endif()

