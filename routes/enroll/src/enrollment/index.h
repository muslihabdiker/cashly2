#ifndef ENROLL_MANAGER_HPP
#define ENROLL_MANAGER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include "../wsDbConfig/wsDbConfig.h"

class wsEnroll {
public:
    // Insert new user credentials into the database
    bool wsEnrollUser(wsDbConfig& dbConfig, const std::string& email, const std::string& password) {
        try {
            pqxx::work W(*dbConfig.getConnection());
            // Hash the password before inserting it into the database
            std::string hashed_password = hash_password(password);
            std::string query = "INSERT INTO fgg (email, password) VALUES (" + W.quote(email) + ", " + W.quote(hashed_password) + ");";
            W.exec(query);
            W.commit();

            // Send confirmation email
            send_registration_email(email);
            return true;
        } catch (const std::exception &e) {
            std::cerr << "Database error: " << e.what() << std::endl;
            return false;
        }
    }

private:
    // Hash the password using SHA-256
    std::string hash_password(const std::string& password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);

        std::stringstream ss;
        for (auto byte : hash) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }

    // Send registration email using SMTP
    void send_registration_email(const std::string& email) {
        CURL *curl;
        CURLcode res;

        const std::string smtp_server = "smtp://smtp.hostinger.com:587"; // Replace with your SMTP server
        const std::string username = "auth@paswad.com"; // Replace with your email
        const std::string password = "wp_vidco@BK221409@Muslih@Abdiker@Mohamed_^)$^)$"; // Replace with your email password

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if(curl) {
            std::string from = "auth@paswad.com"; // Replace with your email
            std::string to = email;
            std::string subject = "Welcome to Our Service!";
            std::string body = "Thank you for registering with us. We're glad to have you on board!";

            std::string payload = "To: " + to + "\r\n" +
                                  "From: " + from + "\r\n" +
                                  "Subject: " + subject + "\r\n" +
                                  "\r\n" + body + "\r\n";

            curl_easy_setopt(curl, CURLOPT_URL, smtp_server.c_str());
            curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());

            struct curl_slist *recipients = NULL;
            recipients = curl_slist_append(recipients, to.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Optional: for debugging

            res = curl_easy_perform(curl);

            if(res != CURLE_OK) {
                std::cerr << "Failed to send email: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }
};

#endif
