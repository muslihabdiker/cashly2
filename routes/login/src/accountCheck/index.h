#ifndef LOGIN_MANAGER_HPP
#define LOGIN_MANAGER_HPP

#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <ctime>
#include <curl/curl.h>
#include "../wsDbConfig/wsDbConfig.h"
#include <jwt-cpp/jwt.h>
std::string hash_password(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
    
    std::string hashed_password;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%02x", hash[i]);
        hashed_password += buffer;
    }
    return hashed_password;
}

class wsLogin {
public:
   std::string verify_credentials(wsDbConfig& dbConfig, const std::string& username, const std::string& password, const std::string& ip_address) {
    try {
        pqxx::work W(*dbConfig.getConnection());  // Start transaction

        std::string hashed_password = hash_password(password);
        std::string query = "SELECT password, email FROM fgg WHERE email = " + W.quote(username) + ";";
        pqxx::result R = W.exec(query);

        if (!R.empty()) {
            std::string stored_hashed_password = R[0][0].as<std::string>();
            std::string user_email = R[0][1].as<std::string>();
            bool valid_credentials = stored_hashed_password == hashed_password;

            if (valid_credentials) {
                send_login_notification(ip_address, user_email);
                return generateToken(user_email); // Return the generated token
            }
            W.commit(); // Commit transaction only if query is executed
        }
        return ""; // No user found or invalid credentials
    } catch (const std::exception &e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return ""; // Handle error case
    }
}
private:
    void send_login_notification(const std::string& ip_address, const std::string& email) {
        time_t now = time(0);
        std::string timestamp = ctime(&now);
        timestamp.erase(timestamp.find_last_not_of(" \n\r\t") + 1); // Trim newline

        std::string from = "auth@paswad.com"; // Replace with your email
        std::string to = email; // Set recipient email from the database
        std::string subject = "Login Notification";
          // Styled HTML body with more detailed information
        std::string body = R"(
        <html>
            <head>
                <style>
                    body {
                        font-family: Arial, sans-serif;
                        background-color: #f4f4f4;
                        padding: 20px;
                    }
                    .container {
                        background-color: #ffffff;
                        border: 1px solid #dddddd;
                        border-radius: 10px;
                        padding: 20px;
                        max-width: 600px;
                        margin: auto;
                    }
                    .header {
                        text-align: center;
                        background-color: #007bff;
                        color: white;
                        padding: 10px 0;
                        border-radius: 10px 10px 0 0;
                    }
                    .content {
                        margin: 20px 0;
                    }
                    .content p {
                        font-size: 16px;
                        color: #333333;
                    }
                    .footer {
                        text-align: center;
                        font-size: 12px;
                        color: #777777;
                        margin-top: 20px;
                    }
                    .footer a {
                        color: #007bff;
                        text-decoration: none;
                    }
                </style>
            </head>
            <body>
                <div class='container'>
                    <div class='header'>
                        <h2>Login Notification</h2>
                    </div>
                    <div class='content'>
                        <p>Dear User,</p>
                        <p>You have successfully logged into your account. Here are the login details:</p>
                        <p><strong>IP Address:</strong> )" + ip_address + R"(</p>
                        <p><strong>Login Time:</strong> )" + timestamp + R"(</p>
                        <p>If this wasn't you, please contact our support team immediately.</p>
                    </div>
                    <div class='footer'>
                        <p>Thank you for using our service!</p>
                        <p>Visit our website at <a href='https://paswad.com'>Paswad</a>.</p>
                    </div>
                </div>
            </body>
        </html>
        )";
 std::string payload = "To: " + to + "\r\n" +
                              "From: " + from + "\r\n" +
                              "Subject: " + subject + "\r\n" +
                              "MIME-Version: 1.0\r\n" +
                              "Content-Type: text/html; charset=UTF-8\r\n" +
                              "\r\n" + body + "\r\n";

        const char* smtp_server = "smtp://smtp.hostinger.com:587"; // SMTP server
        const char* smtp_username = "auth@paswad.com"; // SMTP username
        const char* password = "wp_vidco@BK221409@Muslih@Abdiker@Mohamed_^)$^)$"; // Hardcoded password

        CURL* curl = curl_easy_init();
        if (curl) {
            struct curl_slist* recipients = curl_slist_append(nullptr, to.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, smtp_server);
            curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_username);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

            // Set up email payload callback
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // For debugging

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "Failed to send email: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        } else {
            std::cerr << "Failed to initialize CURL." << std::endl;
        }
    }

    // Email payload source function for CURL
    static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
        std::string* payload = static_cast<std::string*>(userp);
        size_t len = payload->length();
        size_t max_to_copy = std::min(size * nmemb, len);

        if (max_to_copy > 0) {
            memcpy(ptr, payload->c_str(), max_to_copy);
            payload->erase(0, max_to_copy); // Remove the copied part
            return max_to_copy;
        }

        return 0; // No more data
    }
  // Function to generate a JWT token with the provided email
    std::string generateToken(const std::string& email) {
        // Create a JWT token
        auto token = jwt::create()
                         .set_issuer("cashly")
                         .set_subject(email)
                         .set_payload_claim("dashboard-token", jwt::claim(std::string("true"))) // Change here
                         .sign(jwt::algorithm::hs256{"your-256-bit-secret"});

        return token;
    }
};

#endif // LOGIN_MANAGER_HPP
