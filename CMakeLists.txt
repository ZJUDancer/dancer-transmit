cmake_minimum_required(VERSION 3.5)
project(dtransmit)
add_definitions(-std=c++14 -Wall -O2 -Wextra -Wno-unused-result) #-Wfloat-equal -Weffc++ -pedantic

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

find_package(rclcpp_components REQUIRED)
find_package(rcutils)
find_package(rmw REQUIRED)
find_package(std_msgs REQUIRED)

find_package(Boost REQUIRED COMPONENTS system)

include_directories(
  include
  ${Boost_INCLUDE_DIRS}
)

add_library(dtransmit
  src/dtransmit.cpp
  )
ament_target_dependencies(dtransmit rclcpp)
ament_target_dependencies(dtransmit
    rclcpp_components
    rcutils
    std_msgs)
target_link_libraries(dtransmit ${Boost_LIBRARIES})

ament_export_include_directories(include)
install(
  DIRECTORY include/
  DESTINATION include
)


## Add gtest based cpp test target and link libraries
if (CATKIN_ENABLE_TESTING)
#     catkin_add_gtest(${PROJECT_NAME}_utest
#         test/utest.cpp)
#     target_link_libraries(${PROJECT_NAME}_utest
#         ${PROJECT_NAME}
#         ${catkin_LIBRARIES})

#     add_executable(test_dtransmit test/test_dtransmit.cpp)
#     target_link_libraries(test_dtransmit dtransmit)

#     add_executable(test_recv test/test_recv.cpp)
#     target_link_libraries(test_recv dtransmit)

#     add_executable(test_send test/test_send.cpp)
#     target_link_libraries(test_send dtransmit)
endif ()

add_executable(test_send test/test_send.cpp)
target_link_libraries(test_send dtransmit)

add_executable(test_recv test/test_recv.cpp)
target_link_libraries(test_recv dtransmit)

install(TARGETS
  test_send
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
