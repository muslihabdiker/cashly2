#include <chrono> // For timestamp generation
#include <ctime>  // For converting timestamp to string format
#include "src/libs/crow_all.h"
#include "src/libs/json.hpp"
#include "src/accountCheck/index.h"
#include <iostream>

// Function to get current timestamp
long long current_timestamp() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

// Function to create a JSON response
nlohmann::json create_response(int responseCode, const std::string& message, int affectedRows, long long timestamp, const std::string& content) {
    return {
        {"response", {
            {"code", responseCode},
            {"payload", {
                {"message", message},
                {"timestamp", std::to_string(timestamp)},
                {"resource", content}
            }}
        }}
    };
}

// Function to add CORS headers
void add_cors_headers(crow::response &res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main() {
    crow::SimpleApp app;

    wsDbConfig dbConfig("samplex", "msl", "BK221409", "127.0.0.1", 5432);
    wsLogin creds;

    // Endpoint for forgot password
    CROW_ROUTE(app, "/forgot").methods(crow::HTTPMethod::POST)([&creds, &dbConfig](const crow::request& req) {
        long long timestamp = current_timestamp();

        try {
            // Check if the request body is empty
            if (req.body.empty()) {
                throw std::logic_error("Request body is empty.");
            }

            auto json_data = nlohmann::json::parse(req.body);

            // Check for the presence of the email field
            if (!json_data.contains("email")) {
                throw std::logic_error("Missing email field in request body.");
            }

            std::string email = json_data.at("email");
            int affectedRows = 1; // Assuming one row is affected for sending reset link
            std::string content;

            // Process forgot password
            if (creds.forgot_password_manager(dbConfig, email)) {
                content = ""; // No specific content needed for success
                nlohmann::json response_json = create_response(1, "Reset password link sent", affectedRows, timestamp, content);
                crow::response res;
                add_cors_headers(res);
                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(response_json.dump(4));
                return res;
            } else {
                content = "";
                nlohmann::json response_json = create_response(0, "Email not found", 0, timestamp, content);
                crow::response res;
                add_cors_headers(res);
                res.code = 404; // Not Found
                res.set_header("Content-Type", "application/json");
                res.write(response_json.dump(4));
                return res;
            }
        } catch (const std::exception& e) {
            nlohmann::json response_json = create_response(0, "Request failed: " + std::string(e.what()), 0, timestamp, "");
            crow::response res;
            add_cors_headers(res);
            res.code = 400; // Bad Request
            res.set_header("Content-Type", "application/json");
            res.write(response_json.dump(4));
            return res;
        }
    });

    // Handle OPTIONS preflight requests
    CROW_ROUTE(app, "/forgot").methods(crow::HTTPMethod::OPTIONS)([](const crow::request&) {
        crow::response res(204);
        add_cors_headers(res);
        return res; // Return the response here
    });

    app.port(18088).multithreaded().run();
    return 0;
}
