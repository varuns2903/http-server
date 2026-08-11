#pragma once
#include "../routing/Router.hpp"
#include <vector>
#include <string>

namespace middleware {

enum class JsonType {
    STRING,
    NUMBER,
    BOOLEAN,
    OBJECT,
    ARRAY
};

struct SchemaField {
    std::string name;
    JsonType type;
    bool required = true;
};

// Returns a middleware that parses the request body as JSON and validates it against the schema.
// If valid, the parsed JSON is available in req.json_body (we will need to add this to HttpRequest).
// If invalid, it intercepts the request and responds with a 422 Unprocessable Entity.
routing::Middleware validate_json(const std::vector<SchemaField>& schema);

} // namespace middleware
