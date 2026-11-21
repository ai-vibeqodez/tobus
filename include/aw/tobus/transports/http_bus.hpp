#ifndef AW_TOBUS_TRANSPORTS_HTTP_BUS_HPP
#define AW_TOBUS_TRANSPORTS_HTTP_BUS_HPP

/**
 * @file http_bus.hpp
 * @brief HTTP transport with a small relay server and HTTP client polling.
 *
 * Configure @c options["role"] as @c "server" or @c "client". A server endpoint is @c host:port; a client endpoint
 * is @c http://host:port. The server accepts @c POST /publish and @c GET /poll requests.
 */

#include <aw/tobus/transports/framed_bus.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace aw::tobus {

/**
 * @class HTTPBus
 * @brief HTTP relay transport, enabled with @c TOBUS_ENABLE_HTTP.
 */
class HTTPBus final : public FramedBus {
public:
    /// Creates a relay server or connects a polling client according to @c options["role"].
    explicit HTTPBus(EventBusConfig config);
    ~HTTPBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;

private:
    void run_server();
    void run_client();

    bool server_ = false;
    std::atomic<bool> stopping_{false};
    int listen_socket_ = -1;
    std::thread worker_;
    std::atomic<std::uint64_t> cursor_{0};
    std::mutex queue_mtx_;
    std::vector<Frame> queued_frames_;
    std::mutex outgoing_mtx_;
    std::vector<Frame> outgoing_frames_;
    std::string host_;
    std::string service_;
};

} // namespace aw::tobus

#endif  // AW_TOBUS_TRANSPORTS_HTTP_BUS_HPP
