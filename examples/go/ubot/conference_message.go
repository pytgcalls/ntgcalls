package ubot

import (
	"bytes"
	"encoding/json"
	"strconv"
	"unicode/utf16"

	"github.com/mtgo-labs/mtgo/tg"
)

type conferenceEntity struct {
	Type       string `json:"_"`
	Offset     int32  `json:"offset"`
	Length     int32  `json:"length"`
	DocumentID string `json:"document_id,omitempty"`
}

type conferenceText struct {
	Type     string             `json:"_"`
	Text     string             `json:"text"`
	Entities []conferenceEntity `json:"entities"`
}

type conferenceMessage struct {
	Type     string         `json:"_"`
	RandomID string         `json:"random_id"`
	Message  conferenceText `json:"message"`
}

type rawConferenceEntity struct {
	Type       string      `json:"_"`
	Offset     json.Number `json:"offset"`
	Length     json.Number `json:"length"`
	DocumentID json.Number `json:"document_id"`
	MinLayer   json.Number `json:"_min_layer"`
}

type rawConferenceText struct {
	Type     string                `json:"_"`
	Text     *string               `json:"text"`
	Entities []rawConferenceEntity `json:"entities"`
	MinLayer json.Number           `json:"_min_layer"`
}

type rawConferenceMessage struct {
	Type     string             `json:"_"`
	RandomID json.Number        `json:"random_id"`
	Message  *rawConferenceText `json:"message"`
	MinLayer json.Number        `json:"_min_layer"`
}

func conferenceEntityName(entity tg.MessageEntityClass) (string, int32, int32, int64) {
	switch e := entity.(type) {
	case *tg.MessageEntityBold:
		return "messageEntityBold", e.Offset, e.Length, 0
	case *tg.MessageEntityItalic:
		return "messageEntityItalic", e.Offset, e.Length, 0
	case *tg.MessageEntityUnderline:
		return "messageEntityUnderline", e.Offset, e.Length, 0
	case *tg.MessageEntityStrike:
		return "messageEntityStrike", e.Offset, e.Length, 0
	case *tg.MessageEntitySpoiler:
		return "messageEntitySpoiler", e.Offset, e.Length, 0
	case *tg.MessageEntityCustomEmoji:
		return "messageEntityCustomEmoji", e.Offset, e.Length, e.DocumentID
	}
	return "", 0, 0, 0
}

func conferenceEntities(entities []tg.MessageEntityClass) []tg.MessageEntityClass {
	var result []tg.MessageEntityClass
	for _, entity := range entities {
		if name, _, _, _ := conferenceEntityName(entity); name != "" {
			result = append(result, entity)
		}
	}
	return result
}

func serializeConferenceMessage(randomID int64, text string, entities []tg.MessageEntityClass) ([]byte, error) {
	message := conferenceMessage{
		Type:     "groupCallMessage",
		RandomID: strconv.FormatInt(randomID, 10),
		Message: conferenceText{
			Type:     "textWithEntities",
			Text:     text,
			Entities: []conferenceEntity{},
		},
	}
	for _, entity := range entities {
		name, offset, length, documentID := conferenceEntityName(entity)
		if name == "" {
			continue
		}
		serialized := conferenceEntity{Type: name, Offset: offset, Length: length}
		if documentID != 0 {
			serialized.DocumentID = strconv.FormatInt(documentID, 10)
		}
		message.Message.Entities = append(message.Message.Entities, serialized)
	}
	var buffer bytes.Buffer
	encoder := json.NewEncoder(&buffer)
	encoder.SetEscapeHTML(false)
	if err := encoder.Encode(message); err != nil {
		return nil, err
	}
	return bytes.TrimRight(buffer.Bytes(), "\n"), nil
}

func unsupportedLayer(minLayer json.Number) bool {
	value, err := minLayer.Int64()
	return err == nil && value > tg.Layer
}

func parseConferenceEntity(entity rawConferenceEntity, size int64) tg.MessageEntityClass {
	if unsupportedLayer(entity.MinLayer) {
		return nil
	}
	offset, offsetErr := entity.Offset.Int64()
	length, lengthErr := entity.Length.Int64()
	if offsetErr != nil || lengthErr != nil || offset < 0 || length <= 0 || offset+length > size {
		return nil
	}
	switch entity.Type {
	case "messageEntityBold":
		return &tg.MessageEntityBold{Offset: int32(offset), Length: int32(length)}
	case "messageEntityItalic":
		return &tg.MessageEntityItalic{Offset: int32(offset), Length: int32(length)}
	case "messageEntityUnderline":
		return &tg.MessageEntityUnderline{Offset: int32(offset), Length: int32(length)}
	case "messageEntityStrike":
		return &tg.MessageEntityStrike{Offset: int32(offset), Length: int32(length)}
	case "messageEntitySpoiler":
		return &tg.MessageEntitySpoiler{Offset: int32(offset), Length: int32(length)}
	case "messageEntityCustomEmoji":
		if documentID, err := entity.DocumentID.Int64(); err == nil && documentID != 0 {
			return &tg.MessageEntityCustomEmoji{Offset: int32(offset), Length: int32(length), DocumentID: documentID}
		}
	}
	return nil
}

func deserializeConferenceMessage(data []byte) (int64, string, []tg.MessageEntityClass, bool) {
	var message rawConferenceMessage
	if err := json.Unmarshal(data, &message); err != nil {
		return 0, "", nil, false
	}
	randomID, err := message.RandomID.Int64()
	if message.Type != "groupCallMessage" || unsupportedLayer(message.MinLayer) || err != nil || randomID == 0 || message.Message == nil {
		return 0, "", nil, false
	}
	if message.Message.Type != "textWithEntities" || unsupportedLayer(message.Message.MinLayer) || message.Message.Text == nil {
		return 0, "", nil, false
	}
	text := *message.Message.Text
	size := int64(len(utf16.Encode([]rune(text))))
	var entities []tg.MessageEntityClass
	for _, raw := range message.Message.Entities {
		if entity := parseConferenceEntity(raw, size); entity != nil {
			entities = append(entities, entity)
		}
	}
	return randomID, text, entities, true
}
