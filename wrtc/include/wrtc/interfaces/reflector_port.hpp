//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <p2p/base/port.h>
#include <p2p/client/relay_port_factory_interface.h>
#include <p2p/client/basic_port_allocator.h>
#include <api/async_dns_resolver.h>
#include <rtc_base/async_packet_socket.h>

namespace wrtc {


    class ReflectorPort final : public webrtc::Port {
    public:
        enum PortState {
            STATE_CONNECTING,
            STATE_CONNECTED,
            STATE_READY,
            STATE_RECEIVEONLY,
            STATE_DISCONNECTED,
        };

        bool ready() const;

        bool connected() const;

        ~ReflectorPort() override;

        static std::unique_ptr<ReflectorPort> Create(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::AsyncPacketSocket* socket,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        static std::unique_ptr<ReflectorPort> Create(
            const webrtc::CreateRelayPortArgs& args,
            uint16_t minPort,
            uint16_t maxPort,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
       );

        webrtc::SocketAddress GetLocalAddress() const;

        bool SupportsProtocol(absl::string_view protocol) const override;

        void PrepareAddress() override;

        void OnReadyToSend(webrtc::AsyncPacketSocket* socket);

        void OnReadPacket(webrtc::AsyncPacketSocket* socket, const webrtc::ReceivedIpPacket& packet);

        webrtc::Connection* CreateConnection(const webrtc::Candidate& remote_candidate, CandidateOrigin origin) override;

        bool HandleIncomingPacket(webrtc::AsyncPacketSocket* socket, const webrtc::ReceivedIpPacket& packet) override;

        int SetOption(webrtc::Socket::Option opt, int value) override;

        int GetOption(webrtc::Socket::Option opt, int* value) override;

        int GetError() override;

        webrtc::ProtocolType GetProtocol() const override;

        int SendTo(const void* data, size_t size, const webrtc::SocketAddress& addr, const webrtc::AsyncSocketPacketOptions& options, bool payload) override;

        void OnSentPacket(webrtc::AsyncPacketSocket* socket, const webrtc::SentPacketInfo& sent_packet) override;

        bool CanHandleIncomingPacketsFrom(const webrtc::SocketAddress& addr) const override;

        void Close();

        static int GetRelayPreference(webrtc::ProtocolType proto);

        void HandleConnectionDestroyed(webrtc::Connection* conn) override;

    protected:
        ReflectorPort(
            const webrtc::CreateRelayPortArgs& args,
            webrtc::AsyncPacketSocket* socket,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        ReflectorPort(
            const webrtc::CreateRelayPortArgs& args,
            uint16_t min_port,
            uint16_t max_port,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        webrtc::DiffServCodePoint StunDscpValue() const override;

    private:
        typedef std::map<webrtc::Socket::Option, int> SocketOptionsMap;
        typedef std::set<webrtc::SocketAddress> AttemptedServerSet;

        webrtc::CopyOnWriteBuffer peerTag;
        uint32_t randomTag = 0;
        webrtc::ProtocolAddress serverAddress;
        uint8_t serverId = 0;
        webrtc::ScopedTaskSafety taskSafety;
        webrtc::AsyncPacketSocket* socket;
        SocketOptionsMap socketOptions;
        std::unique_ptr<webrtc::AsyncDnsResolverInterface> resolver;
        int error;
        sigslot::signal1<ReflectorPort*> SignalReflectorPortClosed;
        sigslot::signal3<ReflectorPort*, const webrtc::SocketAddress&, const webrtc::SocketAddress&> SignalResolvedServerAddress;
        PortState state;
        AttemptedServerSet attemptedServerAddresses;
        bool isRunningPingTask = false;
        bool standaloneReflectorMode;
        uint32_t standaloneReflectorRoleId;

        webrtc::DiffServCodePoint stunDscpValue;
        std::map<std::string, uint32_t> resolvedPeerTagsByHostname;
        webrtc::RelayCredentials credentials;
        int serverPriority;

        void OnAllocateError(int error_code, const std::string& reason);

        std::string ReconstructedServerUrl(bool useHostname) const;

        void ResolveTurnAddress(const webrtc::SocketAddress& address);

        void OnSendStunPacket(const void *data, size_t size, webrtc::StunRequest *_);

        int Send(const void* data, size_t size, const webrtc::AsyncSocketPacketOptions& options) const;

        bool CreateReflectorClientSocket();

        void OnSocketConnect(webrtc::AsyncPacketSocket* socket);

        void OnSocketClose(webrtc::AsyncPacketSocket* socket, int error);

        void Release();

        void SendReflectorHello();

        void DispatchPacket(const webrtc::ReceivedIpPacket& packet);

        static webrtc::CopyOnWriteBuffer parseHex(std::string const &string);

        bool FailAndPruneConnection(const webrtc::SocketAddress& address);
    };

} // wrtc