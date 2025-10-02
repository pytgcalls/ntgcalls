package ubot

import (
	"gotgcalls/ntgcalls"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func parseVideoSources(sources []*tg.GroupCallParticipantVideoSourceGroup) []ntgcalls.SsrcGroup {
	var ssrcGroups []ntgcalls.SsrcGroup
	for _, source := range sources {
		var sourceIds []uint32
		for _, sourceId := range source.Sources {
			sourceIds = append(sourceIds, uint32(sourceId))
		}
		ssrcGroups = append(ssrcGroups, ntgcalls.SsrcGroup{
			Semantics: source.Semantics,
			Ssrcs:     sourceIds,
		})
	}
	return ssrcGroups
}
