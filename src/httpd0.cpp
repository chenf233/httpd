#include <iostream>
#include <sstream>
#include <utility>  // std::pair
#include <vector>
#include <experimental/string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>
#include <functional>
#include <unordered_map>

enum HTTPMethod {
  GET = 0,
  POST
};

struct HTTPRequest {
  HTTPMethod method;
  std::string path;
  int http_version_major;
  int http_version_minor;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string data;
};

struct HTTPResponse {
  int status;
  int http_version_major;
  int http_version_minor;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string content;

  explicit HTTPResponse(int status_code = 200)
    : status{status_code},
      http_version_major{1},
      http_version_minor{1},
      headers{},
      content{}
  {}
};

std::experimental::string_view Trim(std::experimental::string_view str) {
  auto index = str.find_first_not_of(" ");
  std::experimental::string_view r{str.data()+index};
  index = str.find_last_not_of(" ");
  return index == std::experimental::string_view::npos ? r : r.substr(0, index+1);
}

HTTPRequest ParseHTTPRequest(std::experimental::string_view str) {
  HTTPRequest r;

  // METHOD
  auto index = str.find(" ");
  auto method_str = str.substr(0, index);
  if (method_str == "GET")
    r.method = GET;
  else if (method_str == "POST")
    r.method = POST;
  else
    throw std::runtime_error("Bad HTTP method");
  str.remove_prefix(index+1);

  // PATH
  index = str.find(" ");
  r.path = std::string(str.substr(0, index));
  str.remove_prefix(index+1);

  // HTTP VERSION
  if (str.substr(0, 5) != "HTTP/" || str[6] != '.' || !isdigit(str[5]) || !isdigit(str[7]))
    throw std::runtime_error("Bad HTTP version");
  r.http_version_major = str[5] - '0';
  r.http_version_minor = str[7] - '0';
  if (str[8] != '\r' || str[9] != '\n')
    throw std::runtime_error("Bad request line: should end with \"\\r\\n\"");
  str.remove_prefix(10);

  size_t content_length = 0;

  // HEADERS
  while (1) {
    index = str.find(':');
    auto key = str.substr(0, index);
    str.remove_prefix(index+1);

    index = str.find("\r\n");
    auto val = str.substr(0, index);
    std::cout << '\t' << key << ":" << val << "\n";
    str.remove_prefix(index+2);

    std::string k{Trim(key)};
    std::string v{Trim(val)};
    r.headers.emplace_back(std::pair<std::string, std::string>{k, v});
    if (k == "Content-Length")
      content_length = atoi(v.data());

    // END OF HEADERS
    if (str[0] == '\r' && str[1] == '\n') {
      str.remove_prefix(2);
      break;
    }
  }

  // CONTENT
  r.data = std::string{str.substr(0, content_length)};

/*
  // DEBUG PRINT
  std::cout << "METHOD: " << r.method << '\n'
            << "PATH: " << r.path << '\n'
            << "VERSION: " << r.http_version_major << '.' << r.http_version_minor << '\n'
            << "HEADERS:\n"
            ;
  for (auto h : r.headers)
    std::cout << '\t' << h.first << ':' << h.second << '\n';

  std::cout << "Content(" << content_length << "): \n" << r.data << '\n';
*/
  return r;
}

struct DispactchKey {
  HTTPMethod method;
  std::string path;

  DispactchKey(HTTPMethod m, const std::string &p) : method(m), path(p) {}
};

bool operator==(const DispactchKey &dk1, const DispactchKey &dk2) {
  return dk1.method == dk2.method && dk1.path == dk2.path;
}
// bool operator<(const DispactchKey &dk1, const DispactchKey &dk2) {
//   if (dk1.method != dk2.method)
//     return dk1.method < dk2.method;
//   return dk1.path < dk2.path;
//   //return true;
// }

struct MyHash {
  size_t operator()(const DispactchKey &dk) const {
    //return std::hash_val(dk.method, dk.path);
    std::cout << ">>>>> HASH: " << std::hash<int>()(dk.method) + std::hash<std::string>()(dk.path) << '\n';
    return std::hash<int>()(dk.method) + std::hash<std::string>()(dk.path);
  }
};

class RequestHandler {
public:
  typedef std::function<void(const HTTPRequest &req, HTTPResponse &res)> Handler;

  void Get(const std::string &path, Handler h) {
    RegisterHandler(GET, path, h);
  }
  void Post(const std::string &path, Handler h) {
    RegisterHandler(POST, path, h);
  }

  void Handle(const HTTPRequest &req, HTTPResponse &res) {
    std::cout << "Enter Handle()\n";
    std::cout << m_.size() << '\n';
    DispactchKey dk{req.method, req.path};
    auto f = m_[dk];
    if (f)
      f(req, res);
    else
      res.status = 404;
      //res.content = "NOT FOUND";
    //m_[dk](req, res);
    //m_[req.path](req, res);
    std::cout << "Leave Handle()\n";
  }

private:
  void RegisterHandler(HTTPMethod m, const std::string &path, Handler h) {
    std::cout << "Enter RegisterHandler\n";
    DispactchKey dk{m, path};
    m_[dk] = h;
    //m_[path] = h;
    std::cout << "Leave RegisterHandler\n";
  }

  std::unordered_map<DispactchKey, Handler, MyHash> m_;
  //std::unordered_map<std::string, Handler> m_;
};


int main(int argc, char **argv) {
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  struct sockaddr_in addr, c_addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(3000);
  bind(sock, (const sockaddr *)&addr, sizeof(addr));

  listen(sock, 5);
  socklen_t c_addr_len = sizeof(c_addr);
  int conn_sock = accept(sock, (sockaddr *)&c_addr, &c_addr_len);

  char buffer[1024];
  ssize_t n = recv(conn_sock, buffer, 1024, 0);
  buffer[n] = '\0';
  std::cout << buffer;

  auto request = ParseHTTPRequest(buffer);

  // ROUTES
  std::cout << "register\n";
  RequestHandler h;
  h.Get("/greeting", [](const HTTPRequest &req, HTTPResponse &res) {
    std::cout << "--- gretting ---\n";
    res.content = "Hello world!";
  });

  h.Post("/register", [](const HTTPRequest &req, HTTPResponse &res) {
    res.content = "Register";
  });
  std::cout << "end register\n";

  // HANDLE REQUEST
  HTTPResponse response;
  h.Handle(request, response);


  std::stringstream ss;
  if (response.status == 200) {
    const char *message = response.content.data();
    ss << "HTTP/1.1 200 OK\n"
      << "Content-Length: " << strlen(message) << "\r\n"
      << "\r\n"
      << message;
  } else {
    ss << "HTTP/1.1 404 NOT FOUND\n";
  }

  send(conn_sock, ss.str().data(), ss.str().size(), 0);


  close(conn_sock);
  close(sock);

  return 0;
}