package ubot

import (
	"html"
	"regexp"
	"slices"
	"strconv"
	"strings"
	"unicode/utf16"

	"github.com/mtgo-labs/mtgo/tg"
)

type htmlEvent struct {
	offset  int32
	opening bool
	index   int
	tag     string
}

type openTag struct {
	tag        string
	start      int32
	documentID int64
}

var htmlTagRegex = regexp.MustCompile(`<(/?)([a-zA-Z-]+)([^>]*)>`)
var emojiIDRegex = regexp.MustCompile(`emoji-id="?(\d+)`)
var htmlEscaper = strings.NewReplacer("&", "&amp;", "<", "&lt;", ">", "&gt;")

var htmlAliases = map[string]string{
	"strong": "b",
	"em":     "i",
	"ins":    "u",
	"strike": "s",
	"del":    "s",
}

func utf16Length(text string) int32 {
	return int32(len(utf16.Encode([]rune(text))))
}

func entityTag(entity tg.MessageEntityClass) (int32, int32, string, string) {
	switch e := entity.(type) {
	case *tg.MessageEntityBold:
		return e.Offset, e.Length, "<b>", "</b>"
	case *tg.MessageEntityItalic:
		return e.Offset, e.Length, "<i>", "</i>"
	case *tg.MessageEntityUnderline:
		return e.Offset, e.Length, "<u>", "</u>"
	case *tg.MessageEntityStrike:
		return e.Offset, e.Length, "<s>", "</s>"
	case *tg.MessageEntitySpoiler:
		return e.Offset, e.Length, "<tg-spoiler>", "</tg-spoiler>"
	case *tg.MessageEntityCode:
		return e.Offset, e.Length, "<code>", "</code>"
	case *tg.MessageEntityCustomEmoji:
		return e.Offset, e.Length, `<tg-emoji emoji-id="` + strconv.FormatInt(e.DocumentID, 10) + `">`, "</tg-emoji>"
	}
	return 0, 0, "", ""
}

func compareHTMLEvents(a, b htmlEvent) int {
	if a.offset != b.offset {
		return int(a.offset - b.offset)
	}
	if a.opening != b.opening {
		if a.opening {
			return 1
		}
		return -1
	}
	if a.opening {
		return a.index - b.index
	}
	return b.index - a.index
}

func compareEntities(a, b tg.MessageEntityClass) int {
	aOffset, aLength, _, _ := entityTag(a)
	bOffset, bLength, _, _ := entityTag(b)
	if aOffset != bOffset {
		return int(aOffset - bOffset)
	}
	return int(bLength - aLength)
}

func unparseHTML(text string, entities []tg.MessageEntityClass) string {
	encoded := utf16.Encode([]rune(text))
	sorted := slices.Clone(entities)
	slices.SortStableFunc(sorted, compareEntities)
	var events []htmlEvent
	for index, entity := range sorted {
		offset, length, opening, closing := entityTag(entity)
		if opening == "" {
			continue
		}
		events = append(events, htmlEvent{offset, true, index, opening}, htmlEvent{offset + length, false, index, closing})
	}
	slices.SortStableFunc(events, compareHTMLEvents)
	var result strings.Builder
	position := int32(0)
	for _, event := range events {
		result.WriteString(htmlEscaper.Replace(string(utf16.Decode(encoded[position:event.offset]))))
		result.WriteString(event.tag)
		position = event.offset
	}
	result.WriteString(htmlEscaper.Replace(string(utf16.Decode(encoded[position:]))))
	return result.String()
}

func newEntity(tag string, offset, length int32, documentID int64) tg.MessageEntityClass {
	switch tag {
	case "b":
		return &tg.MessageEntityBold{Offset: offset, Length: length}
	case "i":
		return &tg.MessageEntityItalic{Offset: offset, Length: length}
	case "u":
		return &tg.MessageEntityUnderline{Offset: offset, Length: length}
	case "s":
		return &tg.MessageEntityStrike{Offset: offset, Length: length}
	case "tg-spoiler":
		return &tg.MessageEntitySpoiler{Offset: offset, Length: length}
	case "code":
		return &tg.MessageEntityCode{Offset: offset, Length: length}
	case "tg-emoji":
		if documentID != 0 {
			return &tg.MessageEntityCustomEmoji{Offset: offset, Length: length, DocumentID: documentID}
		}
	}
	return nil
}

func parseHTML(text string) (string, []tg.MessageEntityClass) {
	var result strings.Builder
	var length int32
	var stack []openTag
	var entities []tg.MessageEntityClass
	position := 0
	for _, match := range htmlTagRegex.FindAllStringSubmatchIndex(text, -1) {
		segment := html.UnescapeString(text[position:match[0]])
		result.WriteString(segment)
		length += utf16Length(segment)
		position = match[1]
		closing := text[match[2]:match[3]] == "/"
		tag := strings.ToLower(text[match[4]:match[5]])
		attributes := text[match[6]:match[7]]
		if alias, ok := htmlAliases[tag]; ok {
			tag = alias
		}
		if tag == "span" && !closing && strings.Contains(attributes, "tg-spoiler") {
			tag = "tg-spoiler"
		}
		if tag != "span" && newEntity(tag, 0, 1, 1) == nil {
			literal := text[match[0]:match[1]]
			result.WriteString(literal)
			length += utf16Length(literal)
			continue
		}
		if !closing {
			var documentID int64
			if emoji := emojiIDRegex.FindStringSubmatch(attributes); emoji != nil {
				documentID, _ = strconv.ParseInt(emoji[1], 10, 64)
			}
			stack = append(stack, openTag{tag, length, documentID})
			continue
		}
		for i := len(stack) - 1; i >= 0; i-- {
			if stack[i].tag != tag && (tag != "span" || (stack[i].tag != "span" && stack[i].tag != "tg-spoiler")) {
				continue
			}
			opened := stack[i]
			stack = append(stack[:i], stack[i+1:]...)
			if length > opened.start {
				if entity := newEntity(opened.tag, opened.start, length-opened.start, opened.documentID); entity != nil {
					entities = append(entities, entity)
				}
			}
			break
		}
	}
	result.WriteString(html.UnescapeString(text[position:]))
	slices.SortStableFunc(entities, compareEntities)
	return result.String(), entities
}
