#include <gtest/gtest.h>
#include "../src/routing/Router.hpp"

TEST(RouterTest, RouteMatchAndNotFound) {
    routing::Router router;
    router.add_route(http::HttpMethod::GET, "/test", [](const http::HttpRequest&) {
        http::HttpResponse res;
        res.status_code = http::HttpStatus::OK;
        return res;
    });

    http::HttpRequest req1;
    req1.method = http::HttpMethod::GET;
    req1.uri = "/test";
    
    auto res1 = router.route(req1);
    EXPECT_EQ(res1.status_code, http::HttpStatus::OK);

    http::HttpRequest req2;
    req2.method = http::HttpMethod::GET;
    req2.uri = "/unknown";
    
    auto res2 = router.route(req2);
    EXPECT_EQ(res2.status_code, http::HttpStatus::NotFound);
}
