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


    class ReflectorPort final : public cricket::Port {
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
            const cricket::CreateRelayPortArgs& args,
            rtc::AsyncPacketSocket* socket,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        static std::unique_ptr<ReflectorPort> Create(
            const cricket::CreateRelayPortArgs& args,
            uint16_t minPort,
            uint16_t maxPort,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
       );

        rtc::SocketAddress GetLocalAddress() const;

        bool SupportsProtocol(absl::string_view protocol) const override;

        void PrepareAddress() override;

        void OnReadyToSend(rtc::AsyncPacketSocket* socket);

        void OnReadPacket(rtc::AsyncPacketSocket* socket, const rtc::ReceivedPacket& packet);

        cricket::Connection* CreateConnection(const cricket::Candidate& remote_candidate, CandidateOrigin origin) override;

        bool HandleIncomingPacket(rtc::AsyncPacketSocket* socket, const rtc::ReceivedPacket& packet) override;

        int SetOption(rtc::Socket::Option opt, int value) override;

        int GetOption(rtc::Socket::Option opt, int* value) override;

        int GetError() override;

        cricket::ProtocolType GetProtocol() const override;

        int SendTo(const void* data, size_t size, const rtc::SocketAddress& addr, const rtc::PacketOptions& options, bool payload) override;

        void OnSentPacket(rtc::AsyncPacketSocket* socket, const rtc::SentPacket& sent_packet) override;

        bool CanHandleIncomingPacketsFrom(const rtc::SocketAddress& addr) const override;

        void Close();

        static int GetRelayPreference(cricket::ProtocolType proto);

        void HandleConnectionDestroyed(cricket::Connection* conn) override;

    protected:
        ReflectorPort(
            const cricket::CreateRelayPortArgs& args,
            rtc::AsyncPacketSocket* socket,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        ReflectorPort(
            const cricket::CreateRelayPortArgs& args,
            uint16_t min_port,
            uint16_t max_port,
            uint8_t serverId,
            int serverPriority,
            bool standaloneReflectorMode,
            uint32_t standaloneReflectorRoleId
        );

        rtc::DiffServCodePoint StunDscpValue() const override;

    private:
        typedef std::map<rtc::Socket::Option, int> SocketOptionsMap;
        typedef std::set<rtc::SocketAddress> AttemptedServerSet;

        rtc::CopyOnWriteBuffer peerTag;
        uint32_t randomTag = 0;
        cricket::ProtocolAddress serverAddress;
        uint8_t serverId = 0;
        webrtc::ScopedTaskSafety taskSafety;
        rtc::AsyncPacketSocket* socket;
        SocketOptionsMap socketOptions;
        std::unique_ptr<webrtc::AsyncDnsResolverInterface> resolver;
        int error;
        sigslot::signal1<ReflectorPort*> SignalReflectorPortClosed;
        sigslot::signal3<ReflectorPort*, const rtc::SocketAddress&, const rtc::SocketAddress&> SignalResolvedServerAddress;
        PortState state;
        AttemptedServerSet attemptedServerAddresses;
        bool isRunningPingTask = false;
        bool standaloneReflectorMode;
        uint32_t standaloneReflectorRoleId;

        rtc::DiffServCodePoint stunDscpValue;
        std::map<std::string, uint32_t> resolvedPeerTagsByHostname;
        cricket::RelayCredentials credentials;
        int serverPriority;

        void OnAllocateError(int error_code, const std::string& reason);

        std::string ReconstructedServerUrl(bool useHostname) const;

        void ResolveTurnAddress(const rtc::SocketAddress& address);

        void OnSendStunPacket(const void *data, size_t size, cricket::StunRequest *_);

        int Send(const void* data, size_t size, const rtc::PacketOptions& options) const;

        bool CreateReflectorClientSocket();

        void OnSocketConnect(rtc::AsyncPacketSocket* socket);

        void OnSocketClose(rtc::AsyncPacketSocket* socket, int error);

        void Release();

        void SendReflectorHello();

        void DispatchPacket(const rtc::ReceivedPacket& packet);

        static rtc::CopyOnWriteBuffer parseHex(std::string const &string);

        bool FailAndPruneConnection(const rtc::SocketAddress& address);
    };

} // wrtc