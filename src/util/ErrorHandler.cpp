#include "ErrorHandler.h"
#include <sstream>
#include <json/json.h>

Json::Value ErrorHandler::parseHttpRequest(const std::string& httpRequest) {
    HttpRequest request = HttpRequest::fromRawRequest(httpRequest);
    Json::Value requestJson;

    requestJson["method"] = request.method();
    requestJson["path"] = request.path();

    // Convert headers to JSON
    Json::Value headersJson;
    for (const auto& [key, value] : request.headers()) {
        headersJson[key] = value;
    }
    requestJson["headers"] = headersJson;

    // Parse body
    if (!request.body().empty()) {
        if (request.get_header("Content-Type").find("application/json") != std::string::npos) {
            Json::CharReaderBuilder builder;
            Json::Value bodyJson;
            std::istringstream bodyStream(request.body());
            std::string parseErrors;

            if (!Json::parseFromStream(builder, bodyStream, &bodyJson, &parseErrors)) {
                throw std::runtime_error("Failed to parse JSON body: " + parseErrors);
            }
            requestJson["body"] = bodyJson;
        } else {
            requestJson["body"] = request.body();
        }
    }

    // Parse query parameters
    Json::Value queryParams;
    for (const auto& [key, value] : request.parse_query_params()) {
        queryParams[key] = value;
    }
    requestJson["query"] = queryParams;

    return requestJson;
}

Json::Value ErrorHandler::extractJsonBody(const std::string& httpRequest) {
    HttpRequest request = HttpRequest::fromRawRequest(httpRequest);
    if (request.get_header("Content-Type").find("application/json") == std::string::npos) {
        throw std::runtime_error("Request does not contain JSON body");
    }

    Json::CharReaderBuilder builder;
    Json::Value bodyJson;
    std::istringstream bodyStream(request.body());
    std::string parseErrors;

    if (!Json::parseFromStream(builder, bodyStream, &bodyJson, &parseErrors)) {
        throw std::runtime_error("Failed to parse JSON body: " + parseErrors);
    }

    return bodyJson;
}

std::string ErrorHandler::generateErrorResponse(int statusCode, const std::string& message) {
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
    response << "Content-Type: application/json\r\n";
    
    Json::Value errorJson;
    errorJson["error"] = message;
    errorJson["status"] = statusCode;
    
    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, errorJson);
    
    response << "Content-Length: " << jsonStr.length() << "\r\n";
    response << "\r\n";
    response << jsonStr;
    
    return response.str();
}

std::string ErrorHandler::getStatusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 415: return "Unsupported Media Type";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}