//
// Created by Laky64 on 29/03/2024.
//

#include <wrtc/interfaces/reflector_port.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <random>
#include <ranges>
#include <sstream>
#include <absl/memory/memory.h>

#include <absl/algorithm/container.h>
#include <absl/strings/match.h>
#include <absl/types/optional.h>
#include <api/transport/stun.h>
#include <p2p/base/connection.h>
#include <rtc_base/async_packet_socket.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/net_helper.h>
#include <rtc_base/net_helpers.h>
#include <rtc_base/socket_address.h>
#include <rtc_base/strings/string_builder.h>

namespace wrtc {
    ReflectorPort::ReflectorPort(const webrtc::CreateRelayPortArgs& args,
        webrtc::AsyncPacketSocket* socket,
        const uint8_t serverId,
        const int serverPriority,
        const bool standaloneReflectorMode,
        const uint32_t standaloneReflectorRoleId):
    Port(
        PortParametersRef{
            args.env,
            args.network_thread,
            args.socket_factory,
            args.network,
            args.username,
            args.password,
        },
        webrtc::IceCandidateType::kRelay
    ),
    serverAddress(*args.server_address),
    serverId(serverId),
    socket(socket),
    error(0),
    state(STATE_CONNECTING),
    standaloneReflectorMode(standaloneReflectorMode),
    standaloneReflectorRoleId(standaloneReflectorRoleId),
    stunDscpValue(webrtc::DSCP_NO_CHANGE),
    credentials(args.config->credentials),
    serverPriority(serverPriority) {
        const auto rawPeerTag = parseHex(args.config->credentials.password);
        peerTag.AppendData(rawPeerTag.data(), rawPeerTag.size() - 4);
        if (standaloneReflectorMode) {
            randomTag = standaloneReflectorRoleId;
        } else {
            auto generator = std::mt19937(std::random_device()());
            do {
                std::uniform_int_distribution<uint32_t> distribution;
                randomTag = distribution(generator);
            } while (!randomTag);
        }
        peerTag.AppendData(reinterpret_cast<uint8_t*>(&randomTag), 4);
    }

    ReflectorPort::ReflectorPort(const webrtc::CreateRelayPortArgs& args,
        const uint16_t min_port,
        const uint16_t max_port,
        const uint8_t serverId,
        const int serverPriority,
        const bool standaloneReflectorMode,
        const uint32_t standaloneReflectorRoleId):
    Port(
        PortParametersRef{
            args.env,
            args.network_thread,
            args.socket_factory,
            args.network,
            args.username,
            args.password,
        },
        webrtc::IceCandidateType::kRelay,
        min_port,
        max_port
    ),
    serverAddress(*args.server_address),
    serverId(serverId),
    socket(nullptr),
    error(0),
    state(STATE_CONNECTING),
    standaloneReflectorMode(standaloneReflectorMode),
    standaloneReflectorRoleId(standaloneReflectorRoleId),
    stunDscpValue(webrtc::DSCP_NO_CHANGE),
    credentials(args.config->credentials),
    serverPriority(serverPriority) {
        const auto rawPeerTag = parseHex(args.config->credentials.password);
        peerTag.AppendData(rawPeerTag.data(), rawPeerTag.size() - 4);
        if (standaloneReflectorMode) {
            randomTag = standaloneReflectorRoleId;
        } else {
            auto generator = std::mt19937(std::random_device()());
            do {
                std::uniform_int_distribution<uint32_t> distribution;
                randomTag = distribution(generator);
            } while (!randomTag);
        }
        peerTag.AppendData(reinterpret_cast<uint8_t*>(&randomTag), 4);
    }

    bool ReflectorPort::ready() const {
        return state == STATE_READY;
    }

    bool ReflectorPort::connected() const {
        return state == STATE_READY || state == STATE_CONNECTED;
    }

    ReflectorPort::~ReflectorPort() {
        if (ready()) {
            Release();
        }
        delete socket;
    }

    std::unique_ptr<ReflectorPort> ReflectorPort::Create(const webrtc::CreateRelayPortArgs &args,
        webrtc::AsyncPacketSocket *s,
        const uint8_t serverId,
        const int serverPriority,
        const bool standaloneReflectorMode,
        const uint32_t standaloneReflectorRoleId) {
        if (args.config->credentials.username.size() > 32) {
            RTC_LOG(LS_ERROR) << "Attempt to use REFLECTOR with a too long username of length " << args.config->credentials.username.size();
            return nullptr;
        }
        return absl::WrapUnique(new ReflectorPort(args, s, serverId, serverPriority, standaloneReflectorMode, standaloneReflectorRoleId));
    }

    std::unique_ptr<ReflectorPort> ReflectorPort::Create(
        const webrtc::CreateRelayPortArgs &args,
        const uint16_t minPort,
        const uint16_t maxPort,
        const uint8_t serverId,
        const int serverPriority,
        const bool standaloneReflectorMode,
        const uint32_t standaloneReflectorRoleId
    ) {
        if (args.config->credentials.username.size() > 32) {
            RTC_LOG(LS_ERROR) << "Attempt to use TURN with a too long username of length " << args.config->credentials.username.size();
            return nullptr;
        }
        return absl::WrapUnique(new ReflectorPort(args, minPort, maxPort, serverId, serverPriority, standaloneReflectorMode, standaloneReflectorRoleId));
    }

    webrtc::SocketAddress ReflectorPort::GetLocalAddress() const {
        return socket ? socket->GetLocalAddress() : webrtc::SocketAddress();
    }

    webrtc::ProtocolType ReflectorPort::GetProtocol() const {
        return serverAddress.proto;
    }

    void ReflectorPort::PrepareAddress() {
        if (peerTag.size() != 16) {
            RTC_LOG(LS_ERROR) << "Allocation can't be started without setting the peer tag.";
            OnAllocateError(webrtc::STUN_ERROR_UNAUTHORIZED, "Missing REFLECTOR server credentials.");
            return;
        }
        if (serverId == 0) {
            RTC_LOG(LS_ERROR) << "Allocation can't be started without setting the server id.";
            OnAllocateError(webrtc::STUN_ERROR_UNAUTHORIZED, "Missing REFLECTOR server id.");
            return;
        }
        if (!serverAddress.address.port()) {
            serverAddress.address.SetPort(599);
        }
        if (serverAddress.address.IsUnresolvedIP()) {
            ResolveTurnAddress(serverAddress.address);
        } else {
            if (!IsCompatibleAddress(serverAddress.address)) {
                RTC_LOG(LS_ERROR) << "IP address family does not match. server: " << serverAddress.address.family() << " local: " << Network()->GetBestIP().family();
                OnAllocateError(webrtc::STUN_ERROR_GLOBAL_FAILURE, "IP address family does not match.");
                return;
            }
            attemptedServerAddresses.insert(serverAddress.address);
            RTC_LOG(LS_VERBOSE) << ToString() << ": Trying to connect to REFLECTOR server via " << webrtc::ProtoToString(serverAddress.proto) << " @ " << serverAddress.address.ToSensitiveString();
            if (!CreateReflectorClientSocket()) {
                RTC_LOG(LS_ERROR) << "Failed to create REFLECTOR client socket";
                OnAllocateError(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE,
                                "Failed to create REFLECTOR client socket.");
                return;
            }
            if (serverAddress.proto == webrtc::PROTO_UDP) {
                SendReflectorHello();
            }
        }
    }

    void ReflectorPort::SendReflectorHello() {
        if (!(state == STATE_CONNECTED || state == STATE_READY)) {
            return;
        }
        RTC_LOG(LS_WARNING) << ToString() << ": REFLECTOR sending ping to " << serverAddress.address.ToString();
        webrtc::ByteBufferWriter bufferWriter;
        bufferWriter.Write(webrtc::MakeArrayView(peerTag.data(), peerTag.size()));
        for (int i = 0; i < 12; i++) {
            bufferWriter.WriteUInt8(0xffu);
        }
        bufferWriter.WriteUInt8(0xfeu);
        for (int i = 0; i < 3; i++) {
            bufferWriter.WriteUInt8(0xffu);
        }
        bufferWriter.WriteUInt64(123);
        while (bufferWriter.Length() % 4 != 0) {
            bufferWriter.WriteUInt8(0);
        }
        const webrtc::AsyncSocketPacketOptions options;
        (void) Send(bufferWriter.Data(), bufferWriter.Length(), options);
        if (!isRunningPingTask) {
            isRunningPingTask = true;
            int timeoutMs = 10000;
            if (state == STATE_CONNECTED) {
                timeoutMs = 500;
            }
            thread()->PostDelayedTask(SafeTask(taskSafety.flag(), [this] {
                isRunningPingTask = false;
                SendReflectorHello();
            }), webrtc::TimeDelta::Millis(timeoutMs));
        }
    }

    bool ReflectorPort::CreateReflectorClientSocket() {
        RTC_DCHECK(!socket || SharedSocket());
        if (serverAddress.proto == webrtc::PROTO_UDP && !SharedSocket()) {
            if (standaloneReflectorMode && Network()->name() == "shared-reflector-network") {
                const webrtc::IPAddress ipv4_any_address(INADDR_ANY);
                socket = socket_factory()->CreateUdpSocket(webrtc::SocketAddress(ipv4_any_address, 12345), min_port(), max_port());
            } else {
                socket = socket_factory()->CreateUdpSocket(webrtc::SocketAddress(Network()->GetBestIP(), 0), min_port(), max_port());
            }
        } else if (serverAddress.proto == webrtc::PROTO_TCP) {
            RTC_DCHECK(!SharedSocket());
            constexpr int opts = webrtc::PacketSocketFactory::OPT_STUN;
            webrtc::PacketSocketTcpOptions tcp_options;
            tcp_options.opts = opts;
            socket = socket_factory()->CreateClientTcpSocket(
                webrtc::SocketAddress(Network()->GetBestIP(), 0),
                serverAddress.address,
                get_proxy(),
                get_user_agent(),
                tcp_options
            );
        }
        if (!socket) {
            error = SOCKET_ERROR;
            return false;
        }
        for (auto &[fst, snd] : socketOptions) {
            socket->SetOption(fst, snd);
        }
        if (!SharedSocket()) {
            socket->RegisterReceivedPacketCallback([this](webrtc::AsyncPacketSocket* s, const webrtc::ReceivedIpPacket& packet) {
                OnReadPacket(s, packet);
            });
        }
        socket->SignalReadyToSend.connect(this, &ReflectorPort::OnReadyToSend);
        socket->SignalSentPacket.connect(this, &ReflectorPort::OnSentPacket);
        if (serverAddress.proto == webrtc::PROTO_TCP || serverAddress.proto == webrtc::PROTO_TLS) {
            socket->SignalConnect.connect(this, &ReflectorPort::OnSocketConnect);
            socket->SubscribeCloseEvent(this, [this](webrtc::AsyncPacketSocket* s, const int e) {
                OnSocketClose(s, e);
            });
        } else {
            state = STATE_CONNECTED;
        }
        return true;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void ReflectorPort::OnSocketConnect(webrtc::AsyncPacketSocket* s) {
        RTC_DCHECK(serverAddress.proto == webrtc::PROTO_TCP || serverAddress.proto == webrtc::PROTO_TLS);
        if (const webrtc::SocketAddress& socket_address = s->GetLocalAddress(); absl::c_none_of(Network()->GetIPs(), [socket_address](const webrtc::InterfaceAddress& addr) {
            return socket_address.ipaddr() == addr;
        })) {
            if (s->GetLocalAddress().IsLoopbackIP()) {
                RTC_LOG(LS_WARNING) << "Socket is bound to the address:" << socket_address.ipaddr().ToSensitiveString()
                << ", rather than an address associated with network:"
                << Network()->ToString()
                << ". Still allowing it since it's localhost.";
            } else if (IPIsAny(Network()->GetBestIP())) {
                RTC_LOG(LS_WARNING)
                << "Socket is bound to the address:"
                << socket_address.ipaddr().ToSensitiveString()
                << ", rather than an address associated with network:"
                << Network()->ToString()
                << ". Still allowing it since it's the 'any' address"
                ", possibly caused by multiple_routes being disabled.";
            } else {
                RTC_LOG(LS_WARNING) << "Socket is bound to the address:"
                << socket_address.ipaddr().ToSensitiveString()
                << ", rather than an address associated with network:"
                << Network()->ToString() << ". Discarding REFLECTOR port.";
                OnAllocateError(
                    webrtc::STUN_ERROR_GLOBAL_FAILURE,
                    "Address not associated with the desired network interface."
                );
                return;
            }
        }
        state = STATE_CONNECTED;
        if (serverAddress.address.IsUnresolvedIP()) {
            serverAddress.address = s->GetRemoteAddress();
        }
        RTC_LOG(LS_VERBOSE) << "ReflectorPort connected to " << s->GetRemoteAddress().ToSensitiveString() << " using tcp.";
    }

    void ReflectorPort::OnSocketClose(webrtc::AsyncPacketSocket* s, const int e) {
        RTC_LOG(LS_WARNING) << ToString() << ": Connection with server failed with error: " << e;
        RTC_DCHECK(s == socket);
        Close();
    }

    webrtc::Connection* ReflectorPort::CreateConnection(const webrtc::Candidate& remote_candidate, CandidateOrigin origin) {
        if (!SupportsProtocol(remote_candidate.protocol())) {
            return nullptr;
        }
        const auto remoteHostname = remote_candidate.address().hostname();
        if (remoteHostname.empty()) {
            return nullptr;
        }
        const auto ipFormat = "reflector-" + std::to_string(static_cast<uint32_t>(serverId)) + "-";
        if (!absl::StartsWith(remoteHostname, ipFormat) || !absl::EndsWith(remoteHostname, ".reflector")) {
            return nullptr;
        }
        if (remote_candidate.address().port() != serverAddress.address.port()) {
            return nullptr;
        }
        if (state == STATE_DISCONNECTED || state == STATE_RECEIVEONLY) {
            return nullptr;
        }
        auto* conn = new webrtc::ProxyConnection(env(), NewWeakPtr(), 0, remote_candidate);
        AddOrReplaceConnection(conn);
        return conn;
    }

    bool ReflectorPort::FailAndPruneConnection(const webrtc::SocketAddress& address) {
        if (webrtc::Connection* conn = GetConnection(address); conn != nullptr) {
            conn->FailAndPrune();
            return true;
        }
        return false;
    }

    int ReflectorPort::SetOption(const webrtc::Socket::Option opt, int value) {
        if (opt == webrtc::Socket::OPT_DSCP) {
            stunDscpValue = static_cast<webrtc::DiffServCodePoint>(value);
        }
        if (!socket) {
            socketOptions[opt] = value;
            return 0;
        }
        return socket->SetOption(opt, value);
    }

    int ReflectorPort::GetOption(const webrtc::Socket::Option opt, int* value) {
        if (!socket) {
            const auto it = socketOptions.find(opt);
            if (it == socketOptions.end()) {
                return -1;
            }
            *value = it->second;
            return 0;
        }
        return socket->GetOption(opt, value);
    }

    int ReflectorPort::GetError() {
        return error;
    }

    int ReflectorPort::SendTo(const void* data, size_t size, const webrtc::SocketAddress& addr, const webrtc::AsyncSocketPacketOptions& options, bool payload) {
        webrtc::CopyOnWriteBuffer targetPeerTag;
        auto syntheticHostname = addr.hostname();
        uint32_t resolvedPeerTag = 0;
        if (auto resolvedPeerTagIt = resolvedPeerTagsByHostname.find(syntheticHostname); resolvedPeerTagIt != resolvedPeerTagsByHostname.end()) {
            resolvedPeerTag = resolvedPeerTagIt->second;
        } else {
            const auto prefixFormat = "reflector-" + std::to_string(static_cast<uint32_t>(serverId)) + "-";
            std::string suffixFormat = ".reflector";
            if (!absl::StartsWith(syntheticHostname, prefixFormat) || !absl::EndsWith(syntheticHostname, suffixFormat)) {
                RTC_LOG(LS_ERROR) << ToString() << ": Discarding SendTo request with destination " << addr.ToString();
                return -1;
            }
            auto startPosition = prefixFormat.size();
            auto tagString = syntheticHostname.substr(startPosition, syntheticHostname.size() - suffixFormat.size() - startPosition);
            std::stringstream tagStringStream(tagString);
            tagStringStream >> resolvedPeerTag;
            if (resolvedPeerTag == 0) {
                RTC_LOG(LS_ERROR) << ToString() << ": Discarding SendTo request with destination " << addr.ToString() << " (could not parse peer tag)";
                return -1;
            }
            resolvedPeerTagsByHostname.insert(std::make_pair(syntheticHostname, resolvedPeerTag));
        }
        targetPeerTag.AppendData(peerTag.data(), peerTag.size() - 4);
        targetPeerTag.AppendData(reinterpret_cast<uint8_t*>(&resolvedPeerTag), 4);

        webrtc::ByteBufferWriter bufferWriter;
        bufferWriter.Write(webrtc::MakeArrayView(targetPeerTag.data(), targetPeerTag.size()));
        bufferWriter.Write(webrtc::MakeArrayView(reinterpret_cast<const uint8_t*>(&randomTag), 4));

        bufferWriter.WriteUInt32(static_cast<uint32_t>(size));
        bufferWriter.Write(webrtc::MakeArrayView(static_cast<const uint8_t*>(data), size));
        while (bufferWriter.Length() % 4 != 0) {
            bufferWriter.WriteUInt8(0);
        }
        webrtc::AsyncSocketPacketOptions modified_options(options);
        CopyPortInformationToPacketInfo(&modified_options.info_signaled_after_sent);
        modified_options.info_signaled_after_sent.turn_overhead_bytes = bufferWriter.Length() - size;
        (void) Send(bufferWriter.Data(), bufferWriter.Length(), modified_options);
        return static_cast<int>(size);
    }

    bool ReflectorPort::CanHandleIncomingPacketsFrom(const webrtc::SocketAddress& addr) const {
        return serverAddress.address == addr;
    }

    bool ReflectorPort::HandleIncomingPacket(webrtc::AsyncPacketSocket* s, webrtc::ReceivedIpPacket const &packet) {
        if (s != socket) {
            return false;
        }
        uint8_t const *data = packet.payload().begin();
        size_t size = packet.payload().size();
        webrtc::SocketAddress const &remote_addr = packet.source_address();
        auto packet_time_us = packet.arrival_time();

        if (remote_addr != serverAddress.address) {
            RTC_LOG(LS_WARNING) << ToString()
            << ": Discarding REFLECTOR message from unknown address: "
            << remote_addr.ToSensitiveString()
            << " server_address_: "
            << serverAddress.address.ToSensitiveString();
            return false;
        }
        if (size < 16) {
            RTC_LOG(LS_WARNING) << ToString()
            << ": Received REFLECTOR message that was too short (" << size << ")";
            return false;
        }
        if (state == STATE_DISCONNECTED) {
            RTC_LOG(LS_WARNING)
            << ToString()
            << ": Received REFLECTOR message while the REFLECTOR port is disconnected";
            return false;
        }

        uint8_t receivedPeerTag[16];
        memcpy(receivedPeerTag, data, 16);

        if (memcmp(receivedPeerTag, peerTag.data(), 16 - 4) != 0) {
            RTC_LOG(LS_WARNING)
            << ToString()
            << ": Received REFLECTOR message with incorrect peer_tag";
            return false;
        }
        if (state != STATE_READY) {
            state = STATE_READY;

            RTC_LOG(LS_VERBOSE) << ToString() << ": REFLECTOR " << serverAddress.address.ToString() << " is now ready";

            const auto ipFormat = "reflector-" + std::to_string(static_cast<uint32_t>(serverId)) + "-" + std::to_string(randomTag) + ".reflector";
            webrtc::SocketAddress candidateAddress(ipFormat, serverAddress.address.port());
            if (standaloneReflectorMode) {
                candidateAddress.SetResolvedIP(serverAddress.address.ipaddr());
            }
            AddAddress(
                candidateAddress,
                serverAddress.address,
                webrtc::SocketAddress(),
                webrtc::UDP_PROTOCOL_NAME,
                webrtc::ProtoToString(serverAddress.proto),
                "",
                webrtc::IceCandidateType::kRelay,
                GetRelayPreference(serverAddress.proto),
                serverPriority,
                ReconstructedServerUrl(false),
                true
            );
        }

        if (size > 16 + 4 + 4) {
            bool isSpecialPacket = false;
            if (size >= 16 + 12) {
                uint8_t specialTag[12];
                memcpy(specialTag, data + 16, 12);

                uint8_t expectedSpecialTag[12];
                memset(expectedSpecialTag, 0xff, 12);

                if (memcmp(specialTag, expectedSpecialTag, 12) == 0) {
                    isSpecialPacket = true;
                }
            }

            if (!isSpecialPacket) {
                uint32_t senderTag = 0;
                memcpy(&senderTag, data + 16, 4);

                uint32_t dataSize = 0;
                memcpy(&dataSize, data + 16 + 4, 4);
                dataSize = be32toh(dataSize);
                if (dataSize > size - 16 - 4 - 4) {
                    RTC_LOG(LS_WARNING)
                    << ToString()
                    << ": Received data packet with invalid size tag";
                } else {
                    const auto ipFormat = "reflector-" + std::to_string(static_cast<uint32_t>(serverId)) + "-" + std::to_string(senderTag) + ".reflector";
                    webrtc::SocketAddress candidateAddress(ipFormat, serverAddress.address.port());
                    candidateAddress.SetResolvedIP(serverAddress.address.ipaddr());
                    int64_t packet_timestamp = -1;
                    if (packet_time_us.has_value()) {
                        packet_timestamp = packet_time_us->us_or(-1);
                    }
                    DispatchPacket(webrtc::ReceivedIpPacket::CreateFromLegacy(data + 16 + 4 + 4, dataSize, packet_timestamp, candidateAddress));
                }
            }
        }
        return true;
    }

    void ReflectorPort::OnReadPacket(webrtc::AsyncPacketSocket* s, const webrtc::ReceivedIpPacket& packet) {
        HandleIncomingPacket(s, packet);
    }

    void ReflectorPort::OnSentPacket(webrtc::AsyncPacketSocket* s, const webrtc::SentPacketInfo& sent_packet) {
        SignalSentPacket(sent_packet);
    }

    void ReflectorPort::OnReadyToSend(webrtc::AsyncPacketSocket*) {
        if (ready()) {
            Port::OnReadyToSend();
        }
    }

    bool ReflectorPort::SupportsProtocol(const absl::string_view protocol) const {
        return protocol == webrtc::UDP_PROTOCOL_NAME;
    }

    void ReflectorPort::ResolveTurnAddress(const webrtc::SocketAddress& address) {
        if (resolver)
            return;
        RTC_LOG(LS_VERBOSE) << ToString() << ": Starting TURN host lookup for " << address.ToSensitiveString();
        resolver = socket_factory()->CreateAsyncDnsResolver();
        resolver->Start(address, [this] {
            auto& result = resolver->result();
            if (result.GetError() != 0 && (serverAddress.proto == webrtc::PROTO_TCP || serverAddress.proto == webrtc::PROTO_TLS)) {
                if (!CreateReflectorClientSocket()) {
                    OnAllocateError(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "TURN host lookup received error.");
                }
                return;
            }
            webrtc::SocketAddress resolved_address = serverAddress.address;
            if (result.GetError() != 0 || !result.GetResolvedAddress(Network()->GetBestIP().family(), &resolved_address)) {
                RTC_LOG(LS_WARNING) << ToString() << ": TURN host lookup received error " << result.GetError();
                error = result.GetError();
                OnAllocateError(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "TURN host lookup received error.");
                return;
            }
            SignalResolvedServerAddress(this, serverAddress.address, resolved_address);
            serverAddress.address = resolved_address;
            PrepareAddress();
        });
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void ReflectorPort::OnSendStunPacket(const void* data, const size_t size, webrtc::StunRequest* _) {
        RTC_DCHECK(connected());
        webrtc::AsyncSocketPacketOptions options(StunDscpValue());
        options.info_signaled_after_sent.packet_type = webrtc::PacketType::kTurnMessage;
        CopyPortInformationToPacketInfo(&options.info_signaled_after_sent);
        if (Send(data, size, options) < 0) {
            RTC_LOG(LS_ERROR) << ToString() << ": Failed to send TURN message, error: "
            << socket->GetError();
        }
    }

    void ReflectorPort::OnAllocateError(const int error_code, const std::string& reason) {
        thread()->PostTask(SafeTask(taskSafety.flag(), [this] {
            SignalPortError(this);
        }));
        std::string address = GetLocalAddress().HostAsSensitiveURIString();
        int port = GetLocalAddress().port();
        if (serverAddress.proto == webrtc::PROTO_TCP && serverAddress.address.IsPrivateIP()) {
            address.clear();
            port = 0;
        }
        SendCandidateError(webrtc::IceCandidateErrorEvent(address, port, ReconstructedServerUrl(true), error_code, reason));
    }

    void ReflectorPort::Release() {
        state = STATE_RECEIVEONLY;
    }

    void ReflectorPort::Close() {
        if (!ready()) {
            OnAllocateError(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "");
        }
        state = STATE_DISCONNECTED;
        for (const auto connection : connections() | std::views::values) {
            connection->Destroy();
        }
        SignalReflectorPortClosed(this);
    }

    int ReflectorPort::GetRelayPreference(const webrtc::ProtocolType proto) {
        switch (proto) {
            case webrtc::PROTO_TCP:
                return webrtc::ICE_TYPE_PREFERENCE_RELAY_TCP;
            case webrtc::PROTO_TLS:
                return webrtc::ICE_TYPE_PREFERENCE_RELAY_TLS;
            default:
                RTC_DCHECK(proto == webrtc::PROTO_UDP);
            return webrtc::ICE_TYPE_PREFERENCE_RELAY_UDP;
        }
    }

    webrtc::DiffServCodePoint ReflectorPort::StunDscpValue() const {
        return stunDscpValue;
    }

    void ReflectorPort::DispatchPacket(const webrtc::ReceivedIpPacket& packet) {
        if (webrtc::Connection* conn = GetConnection(packet.source_address())) {
            conn->OnReadPacket(packet);
        } else {
            Port::OnReadPacket(packet, webrtc::ProtocolType::PROTO_UDP);
        }
    }

    webrtc::CopyOnWriteBuffer ReflectorPort::parseHex(std::string const &string) {
        webrtc::CopyOnWriteBuffer result;
        for (size_t i = 0; i < string.length(); i += 2) {
            std::string byteString = string.substr(i, 2);
            char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
            result.AppendData(&byte, 1);
        }
        return result;
    }

    int ReflectorPort::Send(const void* data, const size_t size, const webrtc::AsyncSocketPacketOptions& options) const {
        return socket->SendTo(data, size, serverAddress.address, options);
    }

    void ReflectorPort::HandleConnectionDestroyed(webrtc::Connection* conn) {}

    std::string ReflectorPort::ReconstructedServerUrl(const bool useHostname) const {
        std::string scheme = "turn";
        std::string transport = "tcp";
        switch (serverAddress.proto) {
            case webrtc::PROTO_SSLTCP:
            case webrtc::PROTO_TLS:
                scheme = "turns";
                break;
            case webrtc::PROTO_UDP:
                transport = "udp";
                break;
            case webrtc::PROTO_TCP:
                break;
        }
        webrtc::StringBuilder url;
        url << scheme << ":"
        << (useHostname ? serverAddress.address.hostname() : serverAddress.address.ipaddr().ToString())
        << ":" << serverAddress.address.port() << "?transport=" << transport;
        return url.Release();
    }
}  // namespace cricket