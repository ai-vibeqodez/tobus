#include <aw/tobus/transports/http_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#error "HTTPBus currently requires POSIX sockets"
#endif

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace aw::tobus {
namespace {

constexpr std::size_t max_http_body = 16 * 1024 * 1024;
constexpr std::size_t max_http_header = 16 * 1024;
constexpr std::size_t max_queue_size = 4096;
constexpr std::string_view hex_digits = "0123456789abcdef";

struct Address {
    std::string host;
    std::string service;
};

Address parse_address(std::string_view address, bool url) {
    if (url) {
        constexpr std::string_view scheme = "http://";
        if (!address.starts_with(scheme)) {
            throw std::invalid_argument("HTTPBus client endpoint must start with http://");
        }
        address.remove_prefix(scheme.size());
        const auto slash = address.find('/');
        if (slash != std::string_view::npos) {
            if (address.substr(slash) != "/") {
                throw std::invalid_argument("HTTPBus client endpoint must not contain a path");
            }
            address = address.substr(0, slash);
        }
    }
    const auto colon = address.rfind(':');
    if (colon == std::string_view::npos || colon == 0 || colon + 1 == address.size()) {
        throw std::invalid_argument("HTTPBus endpoint must be host:port");
    }
    return {std::string(address.substr(0, colon)), std::string(address.substr(colon + 1))};
}

void close_socket(int fd) noexcept {
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
}

int connect_to(const std::string& host, const std::string& service) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* results = nullptr;
    const int lookup = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &results);
    if (lookup != 0) {
        throw std::runtime_error("HTTPBus: address lookup failed: " + std::string(::gai_strerror(lookup)));
    }
    int fd = -1;
    for (auto* item = results; item != nullptr; item = item->ai_next) {
        fd = ::socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd >= 0 && ::connect(fd, item->ai_addr, item->ai_addrlen) == 0) {
            break;
        }
        close_socket(fd);
        fd = -1;
    }
    ::freeaddrinfo(results);
    if (fd < 0) {
        throw std::runtime_error("HTTPBus: could not connect to " + host + ":" + service);
    }
    timeval timeout{2, 0};
    (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    return fd;
}

bool send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        const auto sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
        if (sent <= 0) {
            return false;
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
    return true;
}

bool receive_all(int fd, std::string& output, std::size_t count) {
    std::array<char, 8192> buffer{};
    while (count != 0) {
        const auto received = ::recv(fd, buffer.data(), std::min(count, buffer.size()), 0);
        if (received <= 0) {
            return false;
        }
        output.append(buffer.data(), static_cast<std::size_t>(received));
        count -= static_cast<std::size_t>(received);
    }
    return true;
}

std::string hex_encode(std::string_view bytes) {
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const unsigned char byte : bytes) {
        result.push_back(hex_digits[byte >> 4]);
        result.push_back(hex_digits[byte & 0x0f]);
    }
    return result;
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string hex_decode(std::string_view text) {
    if ((text.size() & 1U) != 0U || text.size() / 2 > max_http_body) {
        throw std::runtime_error("HTTPBus: malformed frame encoding");
    }
    std::string result(text.size() / 2, '\0');
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int high = hex_value(text[i]);
        const int low = hex_value(text[i + 1]);
        if (high < 0 || low < 0) {
            throw std::runtime_error("HTTPBus: malformed frame encoding");
        }
        result[i / 2] = static_cast<char>((high << 4) | low);
    }
    return result;
}

void append_u64(std::string& output, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<char>((value >> static_cast<unsigned>(shift)) & 0xffU));
    }
}

void append_u32(std::string& output, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<char>((value >> static_cast<unsigned>(shift)) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view input, std::size_t& offset) {
    if (input.size() - offset < 8) throw std::runtime_error("HTTPBus: truncated frame");
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value = (value << 8) | static_cast<unsigned char>(input[offset++]);
    return value;
}

std::uint32_t read_u32(std::string_view input, std::size_t& offset) {
    if (input.size() - offset < 4) throw std::runtime_error("HTTPBus: truncated frame");
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
        value = (value << 8) | static_cast<unsigned char>(input[offset++]);
    return value;
}

std::string encode_frame(const FramedBus::Frame& frame) {
    if (frame.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("HTTPBus: event payload exceeds 4 GiB");
    }
    std::string result;
    result.reserve(20 + frame.payload.size());
    append_u64(result, frame.origin);
    append_u64(result, frame.type.value);
    append_u32(result, static_cast<std::uint32_t>(frame.payload.size()));
    result.append(reinterpret_cast<const char*>(frame.payload.data()), frame.payload.size());
    return hex_encode(result);
}

FramedBus::Frame decode_frame(std::string_view encoded) {
    const std::string bytes = hex_decode(encoded);
    std::size_t offset = 0;
    FramedBus::Frame frame;
    frame.origin = read_u64(bytes, offset);
    frame.type = EventTypeId{read_u64(bytes, offset)};
    const auto size = read_u32(bytes, offset);
    if (size != bytes.size() - offset) throw std::runtime_error("HTTPBus: invalid frame payload size");
    frame.payload.resize(size);
    if (size != 0) std::memcpy(frame.payload.data(), bytes.data() + offset, size);
    return frame;
}

struct HttpResponse {
    int status = 0;
    std::string body;
};

HttpResponse http_request(const std::string& host, const std::string& service, std::string_view method,
                          std::string_view path, std::string_view body = {}) {
    const int fd = connect_to(host, service);
    std::string request;
    request.reserve(256 + body.size());
    request.append(method)
        .append(" ")
        .append(path)
        .append(" HTTP/1.1\r\nHost: ")
        .append(host)
        .append("\r\nConnection: close\r\nContent-Length: ")
        .append(std::to_string(body.size()))
        .append("\r\n\r\n")
        .append(body);
    if (!send_all(fd, request)) {
        close_socket(fd);
        throw std::runtime_error("HTTPBus: failed to send HTTP request");
    }
    std::string response;
    std::array<char, 8192> buffer{};
    while (response.size() <= max_http_body + max_http_header) {
        const auto received = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (received == 0) break;
        if (received < 0) {
            close_socket(fd);
            throw std::runtime_error("HTTPBus: failed to receive HTTP response");
        }
        response.append(buffer.data(), static_cast<std::size_t>(received));
    }
    close_socket(fd);
    const auto header_end = response.find("\r\n\r\n");
    if (header_end == std::string::npos || header_end > max_http_header) {
        throw std::runtime_error("HTTPBus: malformed HTTP response");
    }
    const auto line_end = response.find("\r\n");
    const auto status_start = response.find(' ');
    if (line_end == std::string::npos || status_start == std::string::npos || status_start > line_end) {
        throw std::runtime_error("HTTPBus: malformed HTTP status");
    }
    int status = 0;
    const auto parsed = std::from_chars(response.data() + status_start + 1, response.data() + line_end, status);
    if (parsed.ec != std::errc{}) throw std::runtime_error("HTTPBus: malformed HTTP status");
    return {status, response.substr(header_end + 4)};
}

void send_response(int fd, int status, std::string_view body) {
    const std::string_view phrase = status == 200 ? "OK" : status == 400 ? "Bad Request" : "Not Found";
    std::string response = "HTTP/1.1 " + std::to_string(status) + " " + std::string(phrase) +
                           "\r\nConnection: close\r\nContent-Length: " + std::to_string(body.size()) +
                           "\r\nContent-Type: text/plain\r\n\r\n";
    response.append(body);
    (void)send_all(fd, response);
}

bool receive_http_request(int fd, std::string& method, std::string& path, std::string& body) {
    std::string data;
    std::array<char, 4096> buffer{};
    std::size_t header_end = std::string::npos;
    while ((header_end = data.find("\r\n\r\n")) == std::string::npos) {
        const auto received = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (received <= 0 || data.size() + static_cast<std::size_t>(received) > max_http_header) return false;
        data.append(buffer.data(), static_cast<std::size_t>(received));
    }
    const auto line_end = data.find("\r\n");
    if (line_end == std::string::npos) return false;
    const auto first_space = data.find(' ');
    const auto second_space = data.find(' ', first_space == std::string::npos ? first_space : first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos || second_space > line_end) return false;
    method = data.substr(0, first_space);
    path = data.substr(first_space + 1, second_space - first_space - 1);
    std::size_t content_length = 0;
    std::size_t line = line_end + 2;
    while (line < header_end) {
        const auto end = data.find("\r\n", line);
        if (end == std::string::npos || end > header_end) break;
        constexpr std::string_view key = "Content-Length:";
        if (std::string_view(data).substr(line, key.size()) == key) {
            auto value_start = line + key.size();
            while (value_start < end && (data[value_start] == ' ' || data[value_start] == '\t'))
                ++value_start;
            const auto parsed = std::from_chars(data.data() + value_start, data.data() + end, content_length);
            if (parsed.ec != std::errc{}) return false;
        }
        line = end + 2;
    }
    if (content_length > max_http_body) return false;
    body = data.substr(header_end + 4);
    if (body.size() > content_length) body.resize(content_length);
    return receive_all(fd, body, content_length - body.size());
}

std::uint64_t parse_cursor(std::string_view path) {
    constexpr std::string_view prefix = "/poll?after=";
    if (!path.starts_with(prefix)) throw std::runtime_error("HTTPBus: invalid poll path");
    std::uint64_t cursor = 0;
    const auto value = path.substr(prefix.size());
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), cursor);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        throw std::runtime_error("HTTPBus: invalid poll cursor");
    }
    return cursor;
}

} // namespace

HTTPBus::HTTPBus(EventBusConfig config)
    : FramedBus(config) {
    const auto role = config.options.find("role");
    if (role == config.options.end() || (role->second != "server" && role->second != "client")) {
        throw std::invalid_argument("HTTPBus requires options[\"role\"] to be server or client");
    }
    server_ = role->second == "server";
    const auto address = parse_address(config.endpoint, !server_);
    host_ = address.host;
    service_ = address.service;
    if (server_) {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        addrinfo* results = nullptr;
        const char* host = host_ == "*" ? nullptr : host_.c_str();
        const int lookup = ::getaddrinfo(host, service_.c_str(), &hints, &results);
        if (lookup != 0) throw std::runtime_error("HTTPBus: bind address lookup failed");
        for (auto* item = results; item != nullptr; item = item->ai_next) {
            listen_socket_ = ::socket(item->ai_family, item->ai_socktype, item->ai_protocol);
            if (listen_socket_ < 0) continue;
            int reuse = 1;
            (void)::setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            if (::bind(listen_socket_, item->ai_addr, item->ai_addrlen) == 0 && ::listen(listen_socket_, 32) == 0)
                break;
            close_socket(listen_socket_);
            listen_socket_ = -1;
        }
        ::freeaddrinfo(results);
        if (listen_socket_ < 0) throw std::runtime_error("HTTPBus: could not bind " + host_ + ":" + service_);
        worker_ = std::thread(&HTTPBus::run_server, this);
    } else {
        worker_ = std::thread(&HTTPBus::run_client, this);
    }
}

HTTPBus::~HTTPBus() {
    stopping_.store(true, std::memory_order_release);
    close_socket(listen_socket_);
    if (worker_.joinable()) worker_.join();
}

std::string_view HTTPBus::transport_name() const noexcept {
    return "http";
}

void HTTPBus::send_frame(const Frame& frame) {
    if (server_) {
        std::lock_guard lock(queue_mtx_);
        queued_frames_.emplace_back(frame);
        if (queued_frames_.size() > max_queue_size) queued_frames_.erase(queued_frames_.begin());
        cursor_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    std::lock_guard lock(outgoing_mtx_);
    outgoing_frames_.emplace_back(frame);
    if (outgoing_frames_.size() > max_queue_size) outgoing_frames_.erase(outgoing_frames_.begin());
}

void HTTPBus::run_server() {
    for (;;) {
        if (stopping_.load(std::memory_order_acquire)) return;
        const int fd = ::accept(listen_socket_, nullptr, nullptr);
        if (fd < 0) {
            if (stopping_.load(std::memory_order_acquire)) return;
            continue;
        }
        timeval timeout{2, 0};
        (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        std::string method, path, body;
        if (!receive_http_request(fd, method, path, body)) {
            send_response(fd, 400, "bad request");
            close_socket(fd);
            continue;
        }
        if (method == "POST" && path == "/publish") {
            try {
                auto frame = decode_frame(body);
                receive_frame(frame);
                {
                    std::lock_guard lock(queue_mtx_);
                    queued_frames_.emplace_back(std::move(frame));
                    if (queued_frames_.size() > max_queue_size) queued_frames_.erase(queued_frames_.begin());
                    cursor_.fetch_add(1, std::memory_order_relaxed);
                }
                send_response(fd, 200, "ok");
            } catch (...) {
                send_response(fd, 400, "invalid frame");
            }
        } else if (method == "GET" && path.starts_with("/poll?after=")) {
            try {
                const auto after = parse_cursor(path);
                std::lock_guard lock(queue_mtx_);
                const auto sequence = cursor_.load(std::memory_order_relaxed);
                std::string response_body = std::to_string(sequence) + "\n";
                const auto earliest = sequence >= queued_frames_.size() ? sequence - queued_frames_.size() : 0;
                if (after < earliest) response_body = std::to_string(earliest) + "\n";
                const auto first_sequence = std::max(after + 1, earliest + 1);
                const auto skip = first_sequence > earliest + queued_frames_.size()
                                      ? queued_frames_.size()
                                      : static_cast<std::size_t>(first_sequence - earliest - 1);
                for (std::size_t i = skip; i < queued_frames_.size(); ++i) {
                    response_body.append(std::to_string(earliest + i + 1)).push_back(' ');
                    response_body.append(encode_frame(queued_frames_[i])).push_back('\n');
                }
                send_response(fd, 200, response_body);
            } catch (...) {
                send_response(fd, 400, "invalid cursor");
            }
        } else {
            send_response(fd, 400, "unknown route");
        }
        close_socket(fd);
    }
}

void HTTPBus::run_client() {
    while (!stopping_.load(std::memory_order_acquire)) {
        try {
            std::vector<Frame> pending;
            {
                std::lock_guard lock(outgoing_mtx_);
                pending.swap(outgoing_frames_);
            }
            std::size_t sent_count = 0;
            try {
                for (; sent_count < pending.size(); ++sent_count) {
                    const auto published =
                        http_request(host_, service_, "POST", "/publish", encode_frame(pending[sent_count]));
                    if (published.status != 200) throw std::runtime_error("HTTPBus: relay rejected publication");
                }
            } catch (...) {
                report_transport_error(std::current_exception());
                std::lock_guard lock(outgoing_mtx_);
                outgoing_frames_.insert(
                    outgoing_frames_.begin(),
                    std::make_move_iterator(pending.begin() + static_cast<std::ptrdiff_t>(sent_count)),
                    std::make_move_iterator(pending.end()));
                throw;
            }
            const auto response = http_request(host_, service_, "GET", "/poll?after=" + std::to_string(cursor_.load()));
            if (response.status == 200) {
                std::size_t line_start = 0;
                const auto first_end = response.body.find('\n');
                if (first_end != std::string::npos) {
                    line_start = first_end + 1;
                    while (line_start < response.body.size()) {
                        const auto line_end = response.body.find('\n', line_start);
                        if (line_end == std::string::npos) break;
                        const auto line = std::string_view(response.body).substr(line_start, line_end - line_start);
                        const auto space = line.find(' ');
                        if (space != std::string_view::npos) {
                            std::uint64_t seq = 0;
                            const auto parsed_seq = std::from_chars(line.data(), line.data() + space, seq);
                            if (parsed_seq.ec == std::errc{}) {
                                try {
                                    receive_frame(decode_frame(line.substr(space + 1)));
                                } catch (...) {
                                }
                                const auto seen = cursor_.load(std::memory_order_relaxed);
                                if (seq > seen) cursor_.store(seq, std::memory_order_relaxed);
                            }
                        }
                        line_start = line_end + 1;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
}

} // namespace aw::tobus
