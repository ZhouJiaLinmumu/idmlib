INCLUDE_DIRECTORIES(

  ${izenelib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs

  ${idmlib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${izenelib_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${XML2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  ADD_EXECUTABLE(t_cc
    Runner.cpp
    t_CC.cpp
    )
  TARGET_LINK_LIBRARIES(t_cc ${libs})
  SET_TARGET_PROPERTIES(t_cc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/testbin
    )
  ADD_TEST(CC "${PROJECT_SOURCE_DIR}/testbin/t_cc")
ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
