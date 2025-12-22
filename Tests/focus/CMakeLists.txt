SET( FocusTests_SRCS testfocus.cpp testfocusstars.cpp testdonutmetric.cpp )

ADD_EXECUTABLE( testfocus testfocus.cpp )
TARGET_LINK_LIBRARIES( testfocus ${TEST_LIBRARIES})
ADD_TEST( NAME FocusTest COMMAND testfocus )
SET_TESTS_PROPERTIES( FocusTest PROPERTIES LABELS "stable")

ADD_EXECUTABLE( testfocusstars testfocusstars.cpp )
TARGET_LINK_LIBRARIES( testfocusstars ${TEST_LIBRARIES})
ADD_TEST( NAME FocusStarsTest COMMAND testfocusstars )
SET_TESTS_PROPERTIES( FocusStarsTest PROPERTIES LABELS "stable")

ADD_EXECUTABLE( testdonutmetric testdonutmetric.cpp )
TARGET_LINK_LIBRARIES( testdonutmetric ${TEST_LIBRARIES})
ADD_TEST( NAME FocusDonutMetricTest COMMAND testdonutmetric )
SET_TESTS_PROPERTIES( FocusDonutMetricTest PROPERTIES LABELS "stable")

