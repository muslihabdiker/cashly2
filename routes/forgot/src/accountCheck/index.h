#ifndef LOGIN_MANAGER_HPP
#define LOGIN_MANAGER_HPP

#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include "../wsDbConfig/wsDbConfig.h"
#include <random>


class wsLogin {
public:
    bool forgot_password_manager(wsDbConfig& dbConfig, const std::string& username) {
        try {
            pqxx::work W(*dbConfig.getConnection());  // Start transaction

            std::string query = "SELECT email FROM fgg WHERE email = " + W.quote(username) + ";";
            pqxx::result R = W.exec(query);

            if (!R.empty()) {
                std::string user_email = R[0][0].as<std::string>(); // Fetch email from the database

                // Send reset password email in a separate thread
                std::thread(&wsLogin::send_reset_password_notification, this, user_email).detach();
                W.commit(); // Commit transaction only if query is executed
                return true;
            }
            return false; // No user found, do not commit
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return false; // Transaction will automatically be aborted
        }
    }

private:
    void send_reset_password_notification(const std::string& email) {
        CURL *curl;
        CURLcode res;

        const std::string smtp_server = "smtp://smtp.hostinger.com:587"; // SMTP server
        const std::string smtp_username = "auth@paswad.com"; // SMTP username
        const std::string password = "wp_vidco@BK221409@Muslih@Abdiker@Mohamed_^)$^)$"; // Replace with actual password

        std::string from = "auth@paswad.com"; // Replace with your email
        std::string subject = "Reset Password Request";
        std::string reset_link = "https://paswad.com/reset_password?token=" + generate_reset_token(email);

        std::string body = "Dear User,\n"
                           "We received a request to reset your password for your account.\n"
                           "If you did not make this request, you can ignore this email. "
                           "Otherwise, click the link below to reset your password:\n"
                           + reset_link + "\n"
                           "Thank you for using our service!\n"
                           "Visit our website at https://paswad.com.";

        std::string payload = "To: " + email + "\r\n" +
                              "From: " + from + "\r\n" +
                              "Subject: " + subject + "\r\n" +
                              "MIME-Version: 1.0\r\n" +
                              "Content-Type: text/plain; charset=UTF-8\r\n" +
                              "\r\n" + body + "\r\n";

        curl = curl_easy_init();

        if (curl) {
            struct curl_slist *recipients = curl_slist_append(nullptr, email.c_str());
            curl_easy_setopt(curl, CURLOPT_URL, smtp_server.c_str());
            curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_username.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_read_callback);
            curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Optional: for debugging

            res = curl_easy_perform(curl); // Capture the result
            if (res != CURLE_OK) { // Check for errors
                std::cerr << "Email send failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        }
    }

    static size_t payload_read_callback(void *ptr, size_t size, size_t nmemb, std::string *payload) {
        if (payload->empty()) {
            return 0; // No more data to send
        }

        size_t len = std::min(size * nmemb, payload->size());
        std::memcpy(ptr, payload->c_str(), len);
        payload->erase(0, len); // Remove sent data
        return len; // Return the number of bytes sent
    }

std::string generate_reset_token(const std::string& email) {
    std::mt19937 gen(std::random_device{}()); // Seed the random number generator
    std::uniform_int_distribution<int> distr(100000, 999999); // Define the distribution
    return "token_for_" + email + "_" + std::to_string(distr(gen)); // Generate the token
}
};

#endif // LOGIN_MANAGER_HPP
