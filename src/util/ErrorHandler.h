#pragma once

#include "../http/RequestHandler.h"
#include <string>
#include <json/json.h>

/**
 * Error Handler class
 * Handles parsing HTTP requests and generating error responses
 */
class ErrorHandler {
public:
    /**
     * Parse an HTTP request string into a JSON object
     * @param httpRequest Raw HTTP request string
     * @return JSON object representing the request
     */
    static Json::Value parseHttpRequest(const std::string& httpRequest);
    
    /**
     * Extract JSON body from an HTTP request
     * @param httpRequest Raw HTTP request string
     * @return JSON object from the request body
     * @throws runtime_error if the request doesn't contain a valid JSON body
     */
    static Json::Value extractJsonBody(const std::string& httpRequest);
    
    /**
     * Generate an HTTP error response
     * @param statusCode HTTP status code
     * @param message Error message
     * @return HTTP response string
     */
    static std::string generateErrorResponse(int statusCode, const std::string& message);
    
private:
    /**
     * Get the text description for an HTTP status code
     * @param statusCode HTTP status code
     * @return Status text
     */
    static std::string getStatusText(int statusCode);
};