#include <gtest/gtest.h>
#include "../src/routing/Router.hpp"

TEST(RouterTest, RouteMatchAndNotFound) {
    routing::Router router;
    router.add_route(http::HttpMethod::GET, "/test", [](const http::HttpRequest& /*req*/, http::HttpResponse& res) {
        res.status_code = http::HttpStatus::OK;
    });

    http::HttpRequest req1;
    req1.method = http::HttpMethod::GET;
    req1.uri = "/test";
    
    http::HttpResponse res1;
    router.route(req1, res1);
    EXPECT_EQ(res1.status_code, http::HttpStatus::OK);

    http::HttpRequest req2;
    req2.method = http::HttpMethod::GET;
    req2.uri = "/unknown";
    
    http::HttpResponse res2;
    router.route(req2, res2);
    EXPECT_EQ(res2.status_code, http::HttpStatus::NotFound);
}

TEST(RouterTest, DynamicRouteParameters) {
    routing::Router router;
    router.add_route(http::HttpMethod::GET, "/users/:id", [](const http::HttpRequest& req, http::HttpResponse& res) {
        res.status_code = http::HttpStatus::OK;
        // Verify params are extracted
        EXPECT_EQ(req.params.at("id"), "42");
    });

    http::HttpRequest req1;
    req1.method = http::HttpMethod::GET;
    req1.uri = "/users/42";
    
    http::HttpResponse res1;
    router.route(req1, res1);
    EXPECT_EQ(res1.status_code, http::HttpStatus::OK);
}

TEST(RouterTest, MiddlewareExecution) {
    routing::Router router;
    
    // Middleware that blocks requests
    router.use([](http::HttpRequest& req, http::HttpResponse& res) {
        if (req.headers["Authorization"] != "Bearer token") {
            res.status_code = http::HttpStatus::Forbidden;
            return false; // Stop pipeline
        }
        return true; // Continue
    });

    router.add_route(http::HttpMethod::GET, "/protected", [](const http::HttpRequest&, http::HttpResponse& res) {
        res.status_code = http::HttpStatus::OK;
    });

    // Request without auth
    http::HttpRequest req1;
    req1.method = http::HttpMethod::GET;
    req1.uri = "/protected";
    http::HttpResponse res1;
    router.route(req1, res1);
    EXPECT_EQ(res1.status_code, http::HttpStatus::Forbidden);

    // Request with auth
    http::HttpRequest req2;
    req2.method = http::HttpMethod::GET;
    req2.uri = "/protected";
    req2.headers["Authorization"] = "Bearer token";
    http::HttpResponse res2;
    router.route(req2, res2);
    EXPECT_EQ(res2.status_code, http::HttpStatus::OK);
}
