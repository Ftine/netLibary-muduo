set(http_SRCS
        HttpServer.cpp
        HttpResponse.cpp
        HttpContext.cpp
        )

add_library(muduo_http ${http_SRCS})
target_link_libraries(muduo_http muduo_net)

install(TARGETS muduo_http DESTINATION lib)
set(HEADERS
        HttpContext.h
        HttpRequest.h
        HttpResponse.h
        HttpServer.h
        )
install(FILES ${HEADERS} DESTINATION include/muduo/net/http)

add_executable(httpserver_test test/HttpServer_test.cpp)
target_link_libraries(httpserver_test muduo_http)

# 查找Boost库

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
message("Boost include directories: ${Boost_INCLUDE_DIRS}")

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
add_executable(httprequest_unittest test/HttpRequest_unittest.cpp)
target_link_libraries(httprequest_unittest muduo_http ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})




