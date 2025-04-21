#ifndef JWT_H
#define JWT_H

#include <string>
#include <map>

class JWT {
public:
    JWT();
    ~JWT();

    // Generate a new JWT token
    std::string generateToken(const std::map<std::string, std::string>& payload, 
                            const std::string& secret,
                            long expirationTime = 3600); // Default 1 hour

    // Verify and decode a JWT token
    bool verifyToken(const std::string& token, const std::string& secret);

    // Get payload from token after verification
    std::map<std::string, std::string> getPayload() const;

    // Check if token is expired
    bool isExpired() const;

private:
    std::map<std::string, std::string> payload_;
    bool isValid_;
    
    // Helper methods
    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);
    std::string generateSignature(const std::string& header, 
                                const std::string& payload, 
                                const std::string& secret);
};

#endif // JWT_H