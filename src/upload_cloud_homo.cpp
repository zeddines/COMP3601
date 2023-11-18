#include <curl/curl.h>
#include <string>
#include <iostream>
#include <unordered_map>
#include <tuple>
#include <limits>
//#include <nlohmann/json.hpp>


//using JSON = nlohmann::json;

constexpr auto AUTH_CODE_URL = "https://oauth2.googleapis.com/device/code";
constexpr auto ACCESS_TOKEN_URL = "https://oauth2.googleapis.com/token";
constexpr auto UPLOAD_URL = "https://www.googleapis.com/upload/drive/v3/files";
constexpr auto AUTH_CONTENT_TYPE = "Content-Type: application/x-www-form-urlencoded";
constexpr auto SCOPE = "scope=https://www.googleapis.com/auth/drive.file";
constexpr auto GRANT_TYPE = "grant_type=urn:ietf:params:oauth:grant-type:device_code";
constexpr auto UPLOAD_TYPE = "uploadType=multipart";


auto parseJson(const std::string &json) {
    std::unordered_map<std::string, std::string> result;
    std::string key, value;
    bool isKey = true;

    for (size_t i = 0; i < json.length(); ++i) {
        if (json[i] == '{' || json[i] == '}' || json[i] == ' ' || json[i] == ',') {
            continue;
        }

        if (json[i] == ':') {
            isKey = false;
            continue;
        }

        if (json[i] == '"') {
            size_t nextQuote = json.find('"', i + 1);
            if (isKey) {
                key = json.substr(i + 1, nextQuote - i - 1);
                isKey = false;
            } else {
                value = json.substr(i + 1, nextQuote - i - 1);
                result[key] = value;
                isKey = true;
            }
            i = nextQuote;
        }
    }

    return result;
}


void waitForAuthorization(const std::string &url, const std::string &userCode) {
    std::cout << "Waiting For Authentication ..." << std::endl;
    std::cout << "Please Go to URL Below and Enter the User Code." << std::endl;
    std::cout << "URL: " << url << std::endl; 
    std::cout << "User Code: " << userCode << std::endl; 
    std::cout << "Press Y to Continue ..." << std::endl; 

    char keyPressed;
    do {
        std::cin >> keyPressed;

        if (std::cin.fail() || std::cin.get() != '\n') {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    } while(keyPressed != 'y' && keyPressed != 'Y'); 
}


void cleanup(CURL *curl, struct curl_slist *headers){
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
}


size_t write_callback(char *buf, size_t size, size_t nmemb, void* up) {
  size_t num_bytes = size*nmemb;
  std::string* data = (std::string*) up;
  for(int i = 0; i < num_bytes; i++) {
    data->push_back(buf[i]);
  }
  return num_bytes;
}


auto getAuthCodes(const std::string &client_id){
    std::cout << "Obtaining Device Code ..." << std::endl;
    struct curl_slist *headers = NULL;
    CURL *curl = curl_easy_init();
    if (curl) {
        //JSON result;
        std::unordered_map<std::string, std::string> result; 
        CURLcode res;
        const auto postFields = client_id + "&" + SCOPE;
        std::string response;
        /* Set URL */
        curl_easy_setopt(curl, CURLOPT_URL, AUTH_CODE_URL);
        
        /* Set Post fields and headers */
        headers = curl_slist_append(headers, AUTH_CONTENT_TYPE);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
        /* Get Result */ 
        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            cleanup(curl, headers);
            std::cerr << "Fail to get result: " << curl_easy_strerror(res) << std::endl;
            exit(1);
        }
        
        std::cout << response << std::endl;
        
        //result = JSON::parse(response);
        result = parseJson(response); 
        cleanup(curl, headers);
    
        return std::make_tuple<std::string, 
                               std::string, 
                               std::string>("device_code=" + std::string(result["device_code"]),
                                            std::string(result["user_code"]), 
                                            std::string(result["verification_url"]));
    } else {
        cleanup(curl, headers);
        std::cerr << "Fail to create curl handle" << std::endl;
        exit(1);
    }
}


auto getAccessToken(const std::string &client_id, const std::string &client_secret, const std::string &device_code){
    struct curl_slist *headers = NULL;
    CURL *curl = curl_easy_init();
    if (curl) {
//        JSON result;
        std::unordered_map<std::string, std::string> result; 
        CURLcode res;
        const std::string postFields = client_id + "&" + client_secret + "&" + device_code + "&" + GRANT_TYPE;
        std::string response;
        std::cout << postFields << std::endl;
        /* Set URL */
        curl_easy_setopt(curl, CURLOPT_URL, ACCESS_TOKEN_URL);
        
        /* Set Post fields and headers */
        headers = curl_slist_append(headers, AUTH_CONTENT_TYPE);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);

        /* Get Result */ 
        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            cleanup(curl, headers);
            std::cerr << "Fail to get result: " << curl_easy_strerror(res) << std::endl;
            exit(1);
        }
        
        std::cout << response << std::endl;
        //result = JSON::parse(response);
        result = parseJson(response);
        cleanup(curl, headers);

        return "Authorization: Bearer " + result["access_token"];//.get<std::string>();
    } else {
        cleanup(curl, headers);
        std::cerr << "Fail to create curl handle" << std::endl;
        exit(1);
    }
}


void uploadFile(const std::string &accessToken, const std::string &filePath){
    struct curl_slist *headers = NULL;
    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode res;

        /* Set URL and Headers */
        curl_easy_setopt(curl, CURLOPT_URL, UPLOAD_URL);
        headers = curl_slist_append(headers, accessToken.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
       
        /* Set Mime */
        curl_mime *mime = curl_mime_init(curl);

        /* Set Metadata */
//        curl_mimepart *metaData = curl_mime_addpart(mime);
//        curl_mime_type(metaData, "application/json; charset=UTF-8");
//        curl_mime_data(metaData, "{\"mimeType\": \"audio/wav\", \"name\": \"test.wav\"}", CURL_ZERO_TERMINATED);

        
        /* Set Files */
        curl_mimepart *data = curl_mime_addpart(mime);
        curl_mime_name(data, "file");
        curl_mime_type(data, "audio/mpeg");
        curl_mime_filedata(data, filePath.c_str());

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);       
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            cleanup(curl, headers);
            curl_mime_free(mime);
            std::cerr << "fail to get result: " << curl_easy_strerror(res) << std::endl;
            exit(1);
        }
        cleanup(curl, headers);
        curl_mime_free(mime);
    } else {
        cleanup(curl, headers);
        std::cerr << "Fail to create handle" << std::endl;
        exit(1);
    }
}


int main(int argc, char *argv[]){
    const auto client_id = "client_id=" + std::string(argv[1]);
    const auto client_secret = "client_secret=" + std::string(argv[2]);
    const auto filePath = std::string(argv[3]);

    const auto &[device_code, user_code, verification_url] = getAuthCodes(client_id);

    waitForAuthorization(verification_url, user_code);
    
    const auto accessToken = getAccessToken(client_id, client_secret, device_code);
    
    uploadFile(accessToken, filePath);
    return 0;
}
