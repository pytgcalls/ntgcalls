//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <api/async_dns_resolver.h>
#include <p2p/base/port.h>
#include <p2p/client/basic_port_allocator.h>
#include <p2p/client/relay_port_factory_interface.h>
#include <rtc_base/async_packet_socket.h>
#include <wrtc/utils/binary.hpp>

namespace wrtc::interfaces {

    class ReflectorPort final: public webrtc::Port {
    public:
        enum class State {
            Connecting,
            Connected,
            Ready,
            Receiveonly,
            Disconnected,
        };

        [[nodiscard]] bool ready() const;

        [[nodiscard]] bool connected() const;

        ~ReflectorPort() override;

        static std::unique_ptr<ReflectorPort> create(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::SocketFactory* underlying_socket_factory,
            webrtc::AsyncPacketSocket* s,
            uint8_t server_id,
            int server_priority,
            bool standalone_reflector_mode,
            uint32_t standalone_reflector_role_id
        );

        static std::unique_ptr<ReflectorPort> create(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::SocketFactory* underlying_socket_factory,
            uint16_t min_port,
            uint16_t max_port,
            uint8_t server_id,
            int server_priority,
            bool standalone_reflector_mode,
            uint32_t standalone_reflector_role_id
        );

        [[nodiscard]] webrtc::SocketAddress get_local_address() const;

        [[nodiscard]] bool SupportsProtocol(absl::string_view protocol) const override;

        void PrepareAddress() override;

        webrtc::Connection* CreateConnection(const webrtc::Candidate& remote_candidate, CandidateOrigin origin) override;

        bool HandleIncomingPacket(webrtc::AsyncPacketSocket* s, const webrtc::ReceivedIpPacket& packet) override;

        int SetOption(webrtc::Socket::Option opt, int value) override;

        int GetOption(webrtc::Socket::Option opt, int* value) override;

        int GetError() override;

        webrtc::ProtocolType GetProtocol() const override;

        int SendTo(std::span<const bytes::byte> data, const webrtc::SocketAddress& addr, const webrtc::AsyncSocketPacketOptions& options, bool payload) override;

        void OnSentPacket(webrtc::AsyncPacketSocket* s, const webrtc::SentPacketInfo& sent_packet) override;

        bool CanHandleIncomingPacketsFrom(const webrtc::SocketAddress& addr) const override;

        void close();

        static int get_relay_preference(webrtc::ProtocolType proto);

    protected:
        ReflectorPort(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::SocketFactory* underlying_socket_factory,
            webrtc::AsyncPacketSocket* socket,
            uint8_t server_id,
            int server_priority,
            bool standalone_reflector_mode,
            uint32_t standalone_reflector_role_id
        );

        ReflectorPort(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::SocketFactory* underlying_socket_factory,
            uint16_t min_port,
            uint16_t max_port,
            uint8_t server_id,
            int server_priority,
            bool standalone_reflector_mode,
            uint32_t standalone_reflector_role_id
        );

        webrtc::DiffServCodePoint StunDscpValue() const override;

        void HandleConnectionDestroyed(webrtc::Connection* conn) override;

    private:
        typedef std::map<webrtc::Socket::Option, int> SocketOptionsMap;
        typedef std::set<webrtc::SocketAddress> AttemptedServerSet;

        webrtc::CopyOnWriteBuffer peer_tag_;
        uint32_t random_tag_ = 0;
        webrtc::ProtocolAddress server_address_;
        uint8_t server_id_ = 0;
        webrtc::ScopedTaskSafety task_safety_;
        std::unique_ptr<webrtc::AsyncPacketSocket> socket_;
        webrtc::SocketFactory* underlying_socket_factory_;
        SocketOptionsMap socket_options_;
        std::unique_ptr<webrtc::AsyncDnsResolverInterface> resolver_;
        int error_;
        webrtc::CallbackList<ReflectorPort*> signal_reflector_port_closed_;
        webrtc::CallbackList<ReflectorPort*, const webrtc::SocketAddress&, const webrtc::SocketAddress&> signal_resolved_server_address_;
        State state_;
        AttemptedServerSet attempted_server_addresses_;
        bool is_running_ping_task_ = false;
        bool standalone_reflector_mode_;
        uint32_t standalone_reflector_role_id_;

        webrtc::DiffServCodePoint stun_dscp_value_;
        std::map<std::string, uint32_t> resolved_peer_tags_by_hostname_;
        webrtc::RelayCredentials credentials_;
        int server_priority_;

        void on_allocate_error(int error_code, const std::string& reason);

        std::string reconstructed_server_url(bool use_hostname) const;

        void resolve_turn_address(const webrtc::SocketAddress& address);

        void on_send_stun_packet(const void* data, size_t size, webrtc::StunRequest* _);

        int send(const void* data, size_t size, const webrtc::AsyncSocketPacketOptions& options) const;

        bool create_reflector_client_socket();

        void on_socket_connect(webrtc::AsyncPacketSocket* s);

        void on_socket_close(webrtc::AsyncPacketSocket* s, int e) const;

        void release();

        void send_reflector_hello();

        void dispatch_packet(const webrtc::ReceivedIpPacket& packet);

        static webrtc::CopyOnWriteBuffer parse_hex(const std::string& string);

        static int bind_socket(
            webrtc::Socket* socket,
            const webrtc::SocketAddress& local_address,
            uint16_t min_port,
            uint16_t max_port
        );

        static std::unique_ptr<webrtc::AsyncPacketSocket> create_client_raw_tcp_socket(
            webrtc::SocketFactory* socket_factory,
            const webrtc::SocketAddress& local_address,
            const webrtc::SocketAddress& remote_address
        );

        bool fail_and_prune_connection(const webrtc::SocketAddress& address);
    };

} // wrtc::interfaces
