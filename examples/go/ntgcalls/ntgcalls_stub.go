//go:build !cgo

package ntgcalls

import "errors"

var errCgoDisabled = errors.New("cgo is disabled; ntgcalls requires cgo to function")

func NTgCalls() *Client {
	return nil
}

func GetProtocol() Protocol {
	return Protocol{}
}

func GetMediaDevices() MediaDevices {
	return MediaDevices{}
}

func Version() string {
	return ""
}

func (ctx *Client) OnStreamEnd(callback StreamEndCallback)                          {}
func (ctx *Client) OnUpgrade(callback UpgradeCallback)                              {}
func (ctx *Client) OnConnectionChange(callback ConnectionChangeCallback)            {}
func (ctx *Client) OnSignal(callback SignalCallback)                                {}
func (ctx *Client) OnFrame(callback FrameCallback)                                  {}
func (ctx *Client) OnRemoteSourceChange(callback RemoteSourceCallback)              {}
func (ctx *Client) OnRequestBroadcastTimestamp(callback BroadcastTimestampCallback) {}
func (ctx *Client) OnRequestBroadcastPart(callback BroadcastPartCallback)           {}
func (ctx *Client) OnUpdateEmojis(callback EmojisCallback)                          {}
func (ctx *Client) OnRequestParticipants(callback RequestParticipantsCallback)      {}
func (ctx *Client) OnOutboundBlock(callback OutboundBlockCallback)                  {}
func (ctx *Client) OnSubchainRequest(callback SubchainRequestCallback)              {}

func (ctx *Client) GetState(chatId int64) (MediaState, error) {
	return MediaState{}, errCgoDisabled
}

func (ctx *Client) GetConnectionMode(chatId int64) (ConnectionMode, error) {
	return ConnectionMode(0), errCgoDisabled
}

func (ctx *Client) GetEmojisFingerprint(chatId int64) (string, error) {
	return "", errCgoDisabled
}

func (ctx *Client) GetCallType(chatId int64) (CallType, error) {
	return CallType(0), errCgoDisabled
}

func (ctx *Client) CreateCall(chatId int64) (string, error) {
	return "", errCgoDisabled
}

func (ctx *Client) InitPresentation(chatId int64) (string, error) {
	return "", errCgoDisabled
}

func (ctx *Client) InitConference(chatId int64, userId int64, lastBlock []byte) (ConferenceJoinParams, error) {
	return ConferenceJoinParams{}, errCgoDisabled
}

func (ctx *Client) StopPresentation(chatId int64) error {
	return errCgoDisabled
}

func (ctx *Client) AddIncomingVideo(chatId, userId int64, endpoint string, ssrcGroups []SsrcGroup) (uint32, error) {
	return 0, errCgoDisabled
}

func (ctx *Client) RemoveIncomingVideo(chatId int64, endpoint string) error {
	return errCgoDisabled
}

func (ctx *Client) CreateP2PCall(chatId int64) error {
	return errCgoDisabled
}

func (ctx *Client) InitExchange(chatId int64, dhConfig DhConfig, gAHash []byte) ([]byte, error) {
	return nil, errCgoDisabled
}

func (ctx *Client) ExchangeKeys(chatId int64, gAB []byte, fingerprint int64) (AuthParams, error) {
	return AuthParams{}, errCgoDisabled
}

func (ctx *Client) SkipExchange(chatId int64, encryptionKey []byte, isOutgoing bool) error {
	return errCgoDisabled
}

func (ctx *Client) ConnectP2P(chatId int64, rtcServers []RTCServer, versions []string, P2PAllowed bool, customParameters string) error {
	return errCgoDisabled
}

func (ctx *Client) SendSignalingData(chatId int64, data []byte) error {
	return errCgoDisabled
}

func (ctx *Client) Connect(chatId int64, params string, isPresentation bool) error {
	return errCgoDisabled
}

func (ctx *Client) SetStreamSources(chatId int64, streamMode StreamMode, desc MediaDescription) error {
	return errCgoDisabled
}

func (ctx *Client) SendExternalFrame(chatId int64, streamDevice StreamDevice, data []byte, frameData FrameData) error {
	return errCgoDisabled
}

func (ctx *Client) SendBroadcastTimestamp(chatId int64, timestamp int64) error {
	return errCgoDisabled
}

func (ctx *Client) SendBroadcastPart(chatId int64, segmentID int64, partID int32, status MediaSegmentStatus, qualityUpdate bool, data []byte) error {
	return errCgoDisabled
}

func (ctx *Client) UpdateAudioSsrcMappings(chatId int64, mappings []SsrcMapping) error {
	return errCgoDisabled
}

func (ctx *Client) ApplyBlocks(chatId int64, subchain int32, nextOffset int32, blocks [][]byte, fromShortPoll bool) error {
	return errCgoDisabled
}

func (ctx *Client) FinishSubchainRequest(chatId int64, subchain int32) error {
	return errCgoDisabled
}

func (ctx *Client) Pause(chatId int64) (bool, error) {
	return false, errCgoDisabled
}

func (ctx *Client) Resume(chatId int64) (bool, error) {
	return false, errCgoDisabled
}

func (ctx *Client) Mute(chatId int64) (bool, error) {
	return false, errCgoDisabled
}

func (ctx *Client) UnMute(chatId int64) (bool, error) {
	return false, errCgoDisabled
}

func (ctx *Client) Stop(chatId int64) error {
	return errCgoDisabled
}

func (ctx *Client) Time(chatId int64, streamMode StreamMode) (uint64, error) {
	return 0, errCgoDisabled
}

func (ctx *Client) CpuUsage() (float64, error) {
	return 0, errCgoDisabled
}

func (ctx *Client) EnableGLibLoop(enable bool) {}

func (ctx *Client) Calls() map[int64]*CallInfo {
	return nil
}

func (ctx *Client) Free() {}
