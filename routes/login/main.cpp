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
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

int main() {
    crow::SimpleApp app;

    wsDbConfig dbConfig("samplex", "msl", "BK221409", "127.0.0.1", 5432);
    wsLogin creds;

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)([&dbConfig, &creds](const crow::request& req) {
        long long timestamp = current_timestamp();

        if (req.method == crow::HTTPMethod::POST) {
            try {
                auto json_data = nlohmann::json::parse(req.body);
                std::string username = json_data.at("username");
                std::string password = json_data.at("password");
                std::string ip_address = req.remote_ip_address;

                int responseCode;
                std::string message;
                std::string content;
                // Verify credentials
            // Verify credentials and get token
std::string token = creds.verify_credentials(dbConfig, username, password, ip_address);
if (!token.empty()) { // Check if the token is not empty
    responseCode = 1;
    message = "Login successful";
    content = token;
} else {
    responseCode = 0;
    message = "Account not found";
    content = "";
}

                // Generate and return the response JSON
                nlohmann::json response_json = create_response(responseCode, message, 1, timestamp, content);
                crow::response res;
                add_cors_headers(res);
                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(response_json.dump(4));
                return res;

            } catch (const std::exception& e) {
                nlohmann::json response_json = create_response(0, "Request failed: " + std::string(e.what()), 0, 0, "");
                crow::response res;
                add_cors_headers(res);
                res.code = 400;
                res.set_header("Content-Type", "application/json");
                res.write(response_json.dump(4));
                return res;
            }
        } else {
            nlohmann::json response_json = create_response(0, "Method Not Allowed", 0, 0, "");
            crow::response res;
            add_cors_headers(res);
            res.code = 405;
            res.set_header("Content-Type", "application/json");
            res.write(response_json.dump(4));
            return res;
        }
    });

    // Handle OPTIONS preflight requests
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::OPTIONS)([]() {
        crow::response res(204);
        add_cors_headers(res);
        return res;
    });

    app.port(18081).multithreaded().run();
    return 0;
}