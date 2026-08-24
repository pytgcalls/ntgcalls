//
// Created by Lauren on 29/03/24.
//

#include <wrtc/interfaces/reflector_port.hpp>

#include <functional>
#include <memory>
#include <random>
#include <ranges>
#include <sstream>
#include <utility>
#include <absl/algorithm/container.h>
#include <absl/memory/memory.h>
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
#include <wrtc/interfaces/raw_tcp_socket.hpp>
#include <wrtc/utils/binary.hpp>

namespace wrtc::interfaces {
    ReflectorPort::ReflectorPort(const webrtc::CreateRelayPortArgs& args, webrtc::SocketFactory* underlying_socket_factory, webrtc::AsyncPacketSocket* socket, const uint8_t server_id, const int server_priority, const bool standalone_reflector_mode, const uint32_t standalone_reflector_role_id):
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
    server_address_(*args.server_address),
    server_id_(server_id),
    socket_(socket),
    underlying_socket_factory_(underlying_socket_factory),
    error_(0),
    state_(State::Connecting),
    standalone_reflector_mode_(standalone_reflector_mode),
    standalone_reflector_role_id_(standalone_reflector_role_id),
    stun_dscp_value_(webrtc::DSCP_NO_CHANGE),
    credentials_(args.config->credentials),
    server_priority_(server_priority) {
        if (standalone_reflector_mode) {
            random_tag_ = standalone_reflector_role_id;
        } else {
            auto generator = std::mt19937(std::random_device()());
            do {
                std::uniform_int_distribution<uint32_t> distribution;
                random_tag_ = distribution(generator);
            } while (!random_tag_);
        }
        if (const auto raw_peer_tag = parse_hex(args.config->credentials.password); raw_peer_tag.size() == 16) {
            peer_tag_.AppendData(raw_peer_tag.data(), raw_peer_tag.size() - 4);
        } else {
            for (int i = 0; i < 16; i++) {
                peer_tag_.AppendData(bytes::array<1>{});
            }
        }
        peer_tag_.AppendData(reinterpret_cast<bytes::byte*>(&random_tag_), 4);
    }

    ReflectorPort::ReflectorPort(const webrtc::CreateRelayPortArgs& args, webrtc::SocketFactory* underlying_socket_factory, const uint16_t min_port, const uint16_t max_port, const uint8_t server_id, const int server_priority, const bool standalone_reflector_mode, const uint32_t standalone_reflector_role_id):
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
    server_address_(*args.server_address),
    server_id_(server_id),
    socket_(nullptr),
    underlying_socket_factory_(underlying_socket_factory),
    error_(0),
    state_(State::Connecting),
    standalone_reflector_mode_(standalone_reflector_mode),
    standalone_reflector_role_id_(standalone_reflector_role_id),
    stun_dscp_value_(webrtc::DSCP_NO_CHANGE),
    credentials_(args.config->credentials),
    server_priority_(server_priority) {
        if (standalone_reflector_mode) {
            random_tag_ = standalone_reflector_role_id;
        } else {
            auto generator = std::mt19937(std::random_device()());
            do {
                std::uniform_int_distribution<uint32_t> distribution;
                random_tag_ = distribution(generator);
            } while (!random_tag_);
        }
        if (const auto raw_peer_tag = parse_hex(args.config->credentials.password); raw_peer_tag.size() == 16) {
            peer_tag_.AppendData(raw_peer_tag.data(), raw_peer_tag.size() - 4);
        } else {
            for (int i = 0; i < 16; i++) {
                peer_tag_.AppendData(bytes::array<1>{});
            }
        }
        peer_tag_.AppendData(reinterpret_cast<bytes::byte*>(&random_tag_), 4);
    }

    bool ReflectorPort::ready() const {
        return state_ == State::Ready;
    }

    bool ReflectorPort::connected() const {
        return state_ == State::Ready || state_ == State::Connected;
    }

    ReflectorPort::~ReflectorPort() {
        if (ready()) {
            socket_->UnsubscribeReadyToSend(this);
            socket_->UnsubscribeSentPacket(this);
            socket_->UnsubscribeSentPacket(this);
            release();
        }
        if (server_address_.proto == webrtc::PROTO_TCP) {
            socket_->UnsubscribeCloseEvent(this);
        }
        socket_ = nullptr;
    }

    std::unique_ptr<ReflectorPort> ReflectorPort::create(
        const webrtc::CreateRelayPortArgs& args,
        webrtc::SocketFactory* underlying_socket_factory,
        webrtc::AsyncPacketSocket* s,
        const uint8_t server_id,
        const int server_priority,
        const bool standalone_reflector_mode,
        const uint32_t standalone_reflector_role_id
    ) {
        if (args.config->credentials.username.size() > 32) {
            RTC_LOG(LS_ERROR) << "Attempt to use REFLECTOR with a too long username of length " << args.config->credentials.username.size();
            return nullptr;
        }
        return absl::WrapUnique(new ReflectorPort(args, underlying_socket_factory, s, server_id, server_priority, standalone_reflector_mode, standalone_reflector_role_id));
    }

    std::unique_ptr<ReflectorPort> ReflectorPort::create(
        const webrtc::CreateRelayPortArgs& args,
        webrtc::SocketFactory* underlying_socket_factory,
        const uint16_t min_port,
        const uint16_t max_port,
        const uint8_t server_id,
        const int server_priority,
        const bool standalone_reflector_mode,
        const uint32_t standalone_reflector_role_id
    ) {
        if (args.config->credentials.username.size() > 32) {
            RTC_LOG(LS_ERROR) << "Attempt to use TURN with a too long username of length " << args.config->credentials.username.size();
            return nullptr;
        }
        return absl::WrapUnique(new ReflectorPort(args, underlying_socket_factory, min_port, max_port, server_id, server_priority, standalone_reflector_mode, standalone_reflector_role_id));
    }

    webrtc::SocketAddress ReflectorPort::get_local_address() const {
        return socket_ ? socket_->GetLocalAddress() : webrtc::SocketAddress();
    }

    webrtc::ProtocolType ReflectorPort::GetProtocol() const {
        return server_address_.proto;
    }

    void ReflectorPort::PrepareAddress() {
        if (peer_tag_.size() != 16) {
            RTC_LOG(LS_ERROR) << "Allocation can't be started without setting the peer tag.";
            on_allocate_error(webrtc::STUN_ERROR_UNAUTHORIZED, "Missing REFLECTOR server credentials.");
            return;
        }
        if (server_id_ == 0) {
            RTC_LOG(LS_ERROR) << "Allocation can't be started without setting the server id.";
            on_allocate_error(webrtc::STUN_ERROR_UNAUTHORIZED, "Missing REFLECTOR server id.");
            return;
        }
        if (!server_address_.address.port()) {
            server_address_.address.SetPort(599);
        }
        if (server_address_.address.IsUnresolvedIP()) {
            resolve_turn_address(server_address_.address);
        } else {
            if (!IsCompatibleAddress(server_address_.address)) {
                RTC_LOG(LS_ERROR) << "IP address family does not match. server: " << server_address_.address.family() << " local: " << Network()->GetBestIP().family();
                on_allocate_error(webrtc::STUN_ERROR_GLOBAL_FAILURE, "IP address family does not match.");
                return;
            }
            attempted_server_addresses_.insert(server_address_.address);
            RTC_LOG(LS_VERBOSE) << ToString() << ": Trying to connect to REFLECTOR server via " << webrtc::ProtoToString(server_address_.proto) << " @ " << server_address_.address.ToSensitiveString();
            if (!create_reflector_client_socket()) {
                RTC_LOG(LS_ERROR) << "Failed to create REFLECTOR client socket";
                on_allocate_error(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "Failed to create REFLECTOR client socket.");
                return;
            }
            if (server_address_.proto == webrtc::PROTO_UDP) {
                send_reflector_hello();
            }
        }
    }

    void ReflectorPort::send_reflector_hello() {
        if (!(state_ == State::Connected || state_ == State::Ready)) {
            return;
        }
        RTC_LOG(LS_WARNING) << ToString() << ": REFLECTOR sending ping to " << server_address_.address.ToString();
        webrtc::ByteBufferWriter buffer_writer;
        if (server_address_.proto == webrtc::PROTO_TCP) {
            buffer_writer.Write(std::span(peer_tag_.data(), peer_tag_.size()));
            buffer_writer.WriteUInt32(0);
            while (buffer_writer.Length() % 4 != 0) {
                buffer_writer.WriteUInt8(0);
            }
        } else {
            buffer_writer.Write(std::span(peer_tag_.data(), peer_tag_.size()));
            for (int i = 0; i < 12; i++) {
                buffer_writer.WriteUInt8(0xffu);
            }
            buffer_writer.WriteUInt8(0xfeu);
            for (int i = 0; i < 3; i++) {
                buffer_writer.WriteUInt8(0xffu);
            }
            buffer_writer.WriteUInt64(123);
            while (buffer_writer.Length() % 4 != 0) {
                buffer_writer.WriteUInt8(0);
            }
        }
        const webrtc::AsyncSocketPacketOptions options;
        (void) send(buffer_writer.Data(), buffer_writer.Length(), options);
        if (!is_running_ping_task_) {
            is_running_ping_task_ = true;
            int timeout_ms = 10000;
            if (state_ == State::Connected) {
                timeout_ms = 500;
            }
            thread()->PostDelayedTask(
                SafeTask(task_safety_.flag(), [this] {
                    is_running_ping_task_ = false;
                    send_reflector_hello();
                }),
                webrtc::TimeDelta::Millis(timeout_ms)
            );
        }
    }

    bool ReflectorPort::create_reflector_client_socket() {
        RTC_DCHECK(!socket_ || SharedSocket());
        if (server_address_.proto == webrtc::PROTO_UDP && !SharedSocket()) {
            if (standalone_reflector_mode_ && Network()->name() == "shared-reflector-network") {
                const webrtc::IPAddress ipv4_any_address(INADDR_ANY);
                socket_ = socket_factory()->CreateUdpSocket(env(), webrtc::SocketAddress(ipv4_any_address, 12345), min_port(), max_port());
            } else {
                socket_ = socket_factory()->CreateUdpSocket(env(), webrtc::SocketAddress(Network()->GetBestIP(), 0), min_port(), max_port());
            }
        } else if (server_address_.proto == webrtc::PROTO_TCP) {
            RTC_DCHECK(!SharedSocket());
            constexpr int kOpts = 0;
            webrtc::PacketSocketTcpOptions tcp_options;
            tcp_options.opts = kOpts;
            socket_ = create_client_raw_tcp_socket(
                underlying_socket_factory_,
                webrtc::SocketAddress(Network()->GetBestIP(), 0),
                server_address_.address
            );
        }
        if (!socket_) {
            error_ = SOCKET_ERROR;
            return false;
        }
        for (auto& [fst, snd] : socket_options_) {
            socket_->SetOption(fst, snd);
        }
        if (!SharedSocket()) {
            socket_->RegisterReceivedPacketCallback([this](webrtc::AsyncPacketSocket* s, const webrtc::ReceivedIpPacket& packet) {
                HandleIncomingPacket(s, packet);
            });
        }
        socket_->SubscribeReadyToSend(this, [this](webrtc::AsyncPacketSocket*) {
            if (ready()) {
                OnReadyToSend();
            }
        });
        socket_->SubscribeSentPacket(this, [this](webrtc::AsyncPacketSocket* s, const webrtc::SentPacketInfo& packet) {
            OnSentPacket(s, packet);
        });
        if (server_address_.proto == webrtc::PROTO_TCP || server_address_.proto == webrtc::PROTO_TLS) {
            socket_->SubscribeConnect(this, [this](webrtc::AsyncPacketSocket* s) {
                on_socket_connect(s);
            });
            socket_->SubscribeCloseEvent(this, [this](webrtc::AsyncPacketSocket* s, const int e) {
                on_socket_close(s, e);
            });
        } else {
            state_ = State::Connected;
        }
        return true;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void ReflectorPort::on_socket_connect(webrtc::AsyncPacketSocket* s) {
        RTC_DCHECK(server_address_.proto == webrtc::PROTO_TCP || server_address_.proto == webrtc::PROTO_TLS);
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
                on_allocate_error(
                    webrtc::STUN_ERROR_GLOBAL_FAILURE,
                    "Address not associated with the desired network interface."
                );
                return;
            }
        }
        state_ = State::Connected;
        if (server_address_.address.IsUnresolvedIP()) {
            server_address_.address = s->GetRemoteAddress();
        }
        RTC_LOG(LS_VERBOSE) << "ReflectorPort connected to " << s->GetRemoteAddress().ToSensitiveString() << " using tcp.";

        // ReSharper disable once CppDFAConstantConditions
        if (server_address_.proto == webrtc::PROTO_TCP && state_ != State::Ready) {
            state_ = State::Ready;
            RTC_LOG(LS_INFO) << ToString() << ": REFLECTOR " << server_address_.address.ToString() << " is now ready";

            const auto ip_format = "reflector-" + std::to_string(static_cast<uint32_t>(server_id_)) + "-" + std::to_string(random_tag_) + ".reflector";
            webrtc::SocketAddress candidate_address(ip_format, server_address_.address.port());
            if (standalone_reflector_mode_) {
                candidate_address.SetResolvedIP(server_address_.address.ipaddr());
            }

            AddAddress(
                candidate_address,
                server_address_.address,
                webrtc::SocketAddress(),
                webrtc::UDP_PROTOCOL_NAME,
                webrtc::ProtoToString(server_address_.proto),
                "",
                webrtc::IceCandidateType::kRelay,
                get_relay_preference(server_address_.proto),
                server_priority_,
                reconstructed_server_url(false),
                true
            );
            send_reflector_hello();
        }
    }

    void ReflectorPort::on_socket_close(webrtc::AsyncPacketSocket* s, const int e) const {
        RTC_LOG(LS_WARNING) << ToString() << ": Connection with server failed with error: " << e;
        RTC_DCHECK(s == socket_.get());
    }

    webrtc::Connection* ReflectorPort::CreateConnection(const webrtc::Candidate& remote_candidate, CandidateOrigin origin) {
        if (!SupportsProtocol(remote_candidate.protocol())) {
            return nullptr;
        }
        const auto remote_hostname = remote_candidate.address().hostname();
        if (remote_hostname.empty()) {
            return nullptr;
        }
        const auto ip_format = "reflector-" + std::to_string(static_cast<uint32_t>(server_id_)) + "-";
        if (!absl::StartsWith(remote_hostname, ip_format) || !absl::EndsWith(remote_hostname, ".reflector")) {
            return nullptr;
        }
        if (remote_candidate.address().port() != server_address_.address.port()) {
            return nullptr;
        }
        if (state_ == State::Disconnected || state_ == State::Receiveonly) {
            return nullptr;
        }

        webrtc::Candidate updated_remote_candidate = remote_candidate;
        if (server_address_.proto == webrtc::PROTO_TCP) {
            webrtc::SocketAddress updated_address = updated_remote_candidate.address();
            updated_address.SetResolvedIP(server_address_.address.ipaddr());
            updated_remote_candidate.set_address(updated_address);
        }

        auto* conn = new webrtc::ProxyConnection(env(), NewWeakPtr(), 0, updated_remote_candidate);
        AddOrReplaceConnection(conn);
        return conn;
    }

    bool ReflectorPort::fail_and_prune_connection(const webrtc::SocketAddress& address) {
        if (webrtc::Connection* conn = GetConnection(address); conn != nullptr) {
            conn->FailAndPrune();
            return true;
        }
        return false;
    }

    int ReflectorPort::SetOption(const webrtc::Socket::Option opt, int value) {
        if (opt == webrtc::Socket::OPT_DSCP) {
            stun_dscp_value_ = static_cast<webrtc::DiffServCodePoint>(value);
        }
        if (!socket_) {
            socket_options_[opt] = value;
            return 0;
        }
        return socket_->SetOption(opt, value);
    }

    int ReflectorPort::GetOption(const webrtc::Socket::Option opt, int* value) {
        if (!socket_) {
            const auto it = socket_options_.find(opt);
            if (it == socket_options_.end()) {
                return -1;
            }
            *value = it->second;
            return 0;
        }
        return socket_->GetOption(opt, value);
    }

    int ReflectorPort::GetError() {
        return error_;
    }

    int ReflectorPort::SendTo(std::span<const bytes::byte> data, const webrtc::SocketAddress& addr, const webrtc::AsyncSocketPacketOptions& options, bool payload) {
        webrtc::CopyOnWriteBuffer target_peer_tag;
        auto synthetic_hostname = addr.hostname();
        uint32_t resolved_peer_tag = 0;
        if (auto resolved_peer_tag_it = resolved_peer_tags_by_hostname_.find(synthetic_hostname); resolved_peer_tag_it != resolved_peer_tags_by_hostname_.end()) {
            resolved_peer_tag = resolved_peer_tag_it->second;
        } else {
            const auto prefix_format = "reflector-" + std::to_string(static_cast<uint32_t>(server_id_)) + "-";
            constexpr std::string kSuffixFormat = ".reflector";
            if (!absl::StartsWith(synthetic_hostname, prefix_format) || !absl::EndsWith(synthetic_hostname, kSuffixFormat)) {
                RTC_LOG(LS_ERROR) << ToString() << ": Discarding SendTo request with destination " << addr.ToString();
                return -1;
            }
            auto start_position = prefix_format.size();
            auto tag_string = synthetic_hostname.substr(start_position, synthetic_hostname.size() - kSuffixFormat.size() - start_position);
            std::stringstream tag_string_stream(tag_string);
            tag_string_stream >> resolved_peer_tag;
            if (resolved_peer_tag == 0) {
                RTC_LOG(LS_ERROR) << ToString() << ": Discarding SendTo request with destination " << addr.ToString() << " (could not parse peer tag)";
                return -1;
            }
            resolved_peer_tags_by_hostname_.insert(std::make_pair(synthetic_hostname, resolved_peer_tag));
        }
        target_peer_tag.AppendData(peer_tag_.data(), peer_tag_.size() - 4);
        target_peer_tag.AppendData(reinterpret_cast<bytes::byte*>(&resolved_peer_tag), 4);

        webrtc::ByteBufferWriter buffer_writer;
        buffer_writer.Write(std::span(target_peer_tag.data(), target_peer_tag.size()));
        buffer_writer.Write(std::span(reinterpret_cast<const bytes::byte*>(&random_tag_), 4));

        buffer_writer.WriteUInt32(static_cast<uint32_t>(data.size()));
        buffer_writer.Write(data);
        while (buffer_writer.Length() % 4 != 0) {
            buffer_writer.WriteUInt8(0);
        }
        webrtc::AsyncSocketPacketOptions modified_options(options);
        CopyPortInformationToPacketInfo(&modified_options.info_signaled_after_sent);
        modified_options.info_signaled_after_sent.turn_overhead_bytes = buffer_writer.Length() - data.size();
        (void) send(buffer_writer.Data(), buffer_writer.Length(), modified_options);
        return static_cast<int>(data.size());
    }

    bool ReflectorPort::CanHandleIncomingPacketsFrom(const webrtc::SocketAddress& addr) const {
        return server_address_.address == addr;
    }

    bool ReflectorPort::HandleIncomingPacket(webrtc::AsyncPacketSocket* s, webrtc::ReceivedIpPacket const& packet) {
        if (s != socket_.get()) {
            return false;
        }
        const bytes::byte* data = packet.payload().data();
        const size_t size = packet.payload().size();
        webrtc::SocketAddress const& remote_addr = packet.source_address();
        auto packet_time_us = packet.arrival_time();

        if (remote_addr != server_address_.address) {
            RTC_LOG(LS_WARNING) << ToString()
                                << ": Discarding REFLECTOR message from unknown address: "
                                << remote_addr.ToSensitiveString()
                                << " server_address_: "
                                << server_address_.address.ToSensitiveString();
            return false;
        }
        if (size < 16) {
            RTC_LOG(LS_WARNING) << ToString()
                                << ": Received REFLECTOR message that was too short (" << size << ")";
            return false;
        }
        if (state_ == State::Disconnected) {
            RTC_LOG(LS_WARNING)
                << ToString()
                << ": Received REFLECTOR message while the REFLECTOR port is disconnected";
            return false;
        }

        bytes::byte received_peer_tag[16];
        std::memcpy(received_peer_tag, data, 16);

        if (std::memcmp(received_peer_tag, peer_tag_.data(), 16 - 4) != 0) {
            RTC_LOG(LS_WARNING)
                << ToString()
                << ": Received REFLECTOR message with incorrect peer_tag";
            return false;
        }
        if (state_ != State::Ready) {
            state_ = State::Ready;

            RTC_LOG(LS_VERBOSE) << ToString() << ": REFLECTOR " << server_address_.address.ToString() << " is now ready";

            const auto ip_format = "reflector-" + std::to_string(static_cast<uint32_t>(server_id_)) + "-" + std::to_string(random_tag_) + ".reflector";
            webrtc::SocketAddress candidate_address(ip_format, server_address_.address.port());
            if (standalone_reflector_mode_) {
                candidate_address.SetResolvedIP(server_address_.address.ipaddr());
            }
            AddAddress(
                candidate_address,
                server_address_.address,
                webrtc::SocketAddress(),
                webrtc::UDP_PROTOCOL_NAME,
                webrtc::ProtoToString(server_address_.proto),
                "",
                webrtc::IceCandidateType::kRelay,
                get_relay_preference(server_address_.proto),
                server_priority_,
                reconstructed_server_url(false),
                true
            );
        }

        if (size > 16 + 4 + 4) {
            bool is_special_packet = false;
            if (size >= 16 + 12) {
                bytes::byte special_tag[12];
                std::memcpy(special_tag, data + 16, 12);

                bytes::byte expected_special_tag[12];
                std::memset(expected_special_tag, 0xff, 12);

                if (std::memcmp(special_tag, expected_special_tag, 12) == 0) {
                    is_special_packet = true;
                }
            }

            if (!is_special_packet) {
                uint32_t sender_tag = 0;
                std::memcpy(&sender_tag, data + 16, 4);

                uint32_t data_size = 0;
                std::memcpy(&data_size, data + 16 + 4, 4);
                data_size = be32toh(data_size);
                if (data_size > size - 16 - 4 - 4) {
                    RTC_LOG(LS_WARNING)
                        << ToString()
                        << ": Received data packet with invalid size tag";
                } else {
                    const auto ip_format = "reflector-" + std::to_string(static_cast<uint32_t>(server_id_)) + "-" + std::to_string(sender_tag) + ".reflector";
                    webrtc::SocketAddress candidate_address(ip_format, server_address_.address.port());
                    candidate_address.SetResolvedIP(server_address_.address.ipaddr());
                    int64_t packet_timestamp = -1;
                    if (packet_time_us.has_value()) {
                        packet_timestamp = packet_time_us->us_or(-1);
                    }
                    dispatch_packet(webrtc::ReceivedIpPacket::CreateFromLegacy(data + 16 + 4 + 4, data_size, packet_timestamp, candidate_address));
                }
            }
        }
        return true;
    }

    void ReflectorPort::OnSentPacket(webrtc::AsyncPacketSocket* s, const webrtc::SentPacketInfo& sent_packet) {
        NotifySentPacket(sent_packet);
    }

    bool ReflectorPort::SupportsProtocol(const absl::string_view protocol) const {
        return protocol == webrtc::UDP_PROTOCOL_NAME;
    }

    void ReflectorPort::resolve_turn_address(const webrtc::SocketAddress& address) {
        if (resolver_)
            return;
        RTC_LOG(LS_VERBOSE) << ToString() << ": Starting TURN host lookup for " << address.ToSensitiveString();
        resolver_ = socket_factory()->CreateAsyncDnsResolver();
        resolver_->Start(address, [this] {
            auto& result = resolver_->result();
            if (result.GetError() != 0 && (server_address_.proto == webrtc::PROTO_TCP || server_address_.proto == webrtc::PROTO_TLS)) {
                if (!create_reflector_client_socket()) {
                    on_allocate_error(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "TURN host lookup received error.");
                }
                return;
            }
            webrtc::SocketAddress resolved_address = server_address_.address;
            if (result.GetError() != 0 || !result.GetResolvedAddress(Network()->GetBestIP().family(), &resolved_address)) {
                RTC_LOG(LS_WARNING) << ToString() << ": TURN host lookup received error " << result.GetError();
                error_ = result.GetError();
                on_allocate_error(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "TURN host lookup received error.");
                return;
            }
            signal_resolved_server_address_.Send(this, server_address_.address, resolved_address);
            server_address_.address = resolved_address;
            PrepareAddress();
        });
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void ReflectorPort::on_send_stun_packet(const void* data, const size_t size, webrtc::StunRequest* _) {
        RTC_DCHECK(connected());
        webrtc::AsyncSocketPacketOptions options(StunDscpValue());
        options.info_signaled_after_sent.packet_type = webrtc::PacketType::kTurnMessage;
        CopyPortInformationToPacketInfo(&options.info_signaled_after_sent);
        if (send(data, size, options) < 0) {
            RTC_LOG(LS_ERROR) << ToString() << ": Failed to send TURN message, error: "
                              << socket_->GetError();
        }
    }

    void ReflectorPort::on_allocate_error(const int error_code, const std::string& reason) {
        thread()->PostTask(SafeTask(task_safety_.flag(), [this] {
            NotifyPortError(this);
        }));
        std::string address = get_local_address().HostAsSensitiveURIString();
        int port = get_local_address().port();
        if (server_address_.proto == webrtc::PROTO_TCP && server_address_.address.IsPrivateIP()) {
            address.clear();
            port = 0;
        }
        SendCandidateError(webrtc::IceCandidateErrorEvent(address, port, reconstructed_server_url(true), error_code, reason));
    }

    void ReflectorPort::release() {
        state_ = State::Receiveonly;
    }

    void ReflectorPort::close() {
        if (!ready()) {
            on_allocate_error(webrtc::STUN_ERROR_SERVER_NOT_REACHABLE, "");
        }
        state_ = State::Disconnected;
        for (const auto connection : connections() | std::views::values) {
            connection->Destroy();
        }
        signal_reflector_port_closed_.Send(this);
    }

    int ReflectorPort::get_relay_preference(const webrtc::ProtocolType proto) {
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
        return stun_dscp_value_;
    }

    void ReflectorPort::dispatch_packet(const webrtc::ReceivedIpPacket& packet) {
        if (webrtc::Connection* conn = GetConnection(packet.source_address())) {
            conn->OnReadPacket(packet);
        } else {
            Port::OnReadPacket(packet, webrtc::ProtocolType::PROTO_UDP);
        }
    }

    webrtc::CopyOnWriteBuffer ReflectorPort::parse_hex(const std::string& string) {
        webrtc::CopyOnWriteBuffer result;
        for (size_t i = 0; i < string.length(); i += 2) {
            const std::string byte_string = string.substr(i, 2);
            const char byte = static_cast<char>(strtol(byte_string.c_str(), nullptr, 16));
            result.AppendData(&byte, 1);
        }
        return result;
    }

    int ReflectorPort::bind_socket(
        webrtc::Socket* socket,
        const webrtc::SocketAddress& local_address,
        // ReSharper disable once CppDFAConstantParameter
        const uint16_t min_port,
        // ReSharper disable once CppDFAConstantParameter
        const uint16_t max_port
    ) {
        int ret = -1;
        if (min_port == 0 && max_port == 0) {
            ret = socket->Bind(local_address);
        } else {
            for (int port = min_port; ret < 0 && port <= max_port; ++port) {
                ret = socket->Bind(webrtc::SocketAddress(local_address.ipaddr(), port));
            }
        }
        return ret;
    }

    std::unique_ptr<webrtc::AsyncPacketSocket> ReflectorPort::create_client_raw_tcp_socket(
        webrtc::SocketFactory* socket_factory,
        const webrtc::SocketAddress& local_address,
        const webrtc::SocketAddress& remote_address
    ) {
        auto socket = socket_factory->Create(local_address.family(), SOCK_STREAM);
        if (!socket) {
            return nullptr;
        }
        if (bind_socket(socket.get(), local_address, 0, 0) < 0) {
            if (local_address.IsAnyIP()) {
                RTC_LOG(LS_WARNING) << "TCP bind failed with error " << socket->GetError() << "; ignoring since socket is using 'any' address.";
            } else {
                RTC_LOG(LS_ERROR) << "TCP bind failed with error " << socket->GetError();
                return nullptr;
            }
        }

        if (socket->SetOption(webrtc::Socket::OPT_NODELAY, 1) != 0) {
            RTC_LOG(LS_ERROR) << "Setting TCP_NODELAY option failed with error "
                              << socket->GetError();
        }

        if (socket->Connect(remote_address) < 0) {
            RTC_LOG(LS_ERROR) << "TCP connect failed with error " << socket->GetError();
            return nullptr;
        }

        return std::make_unique<webrtc::RawTcpSocket>(std::move(socket));
    }

    int ReflectorPort::send(const void* data, const size_t size, const webrtc::AsyncSocketPacketOptions& options) const {
        return socket_->SendTo(data, size, server_address_.address, options);
    }

    void ReflectorPort::HandleConnectionDestroyed(webrtc::Connection* conn) {}

    std::string ReflectorPort::reconstructed_server_url(const bool use_hostname) const {
        std::string scheme = "turn";
        std::string transport = "tcp";
        switch (server_address_.proto) {
        case webrtc::PROTO_SSLTCP:
        case webrtc::PROTO_TLS:
            scheme = "turns";
            break;
        case webrtc::PROTO_UDP:
            transport = "udp";
            break;
        case webrtc::PROTO_TCP:
            break;
        case webrtc::PROTO_DTLS:
            scheme = "turns";
            transport = "udp";
            break;
        }
        webrtc::StringBuilder url;
        url << scheme << ":"
            << (use_hostname ? server_address_.address.hostname() : server_address_.address.ipaddr().ToString())
            << ":" << server_address_.address.port() << "?transport=" << transport;
        return url.Release();
    }
} // namespace cricket
