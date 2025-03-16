package io.github.pytgcalls.media;

import java.util.List;

public class SsrcGroup {
    String semantics;
    List<Integer> ssrcGroups;

    public SsrcGroup(String semantics, List<Integer> ssrcGroups) {
        this.semantics = semantics;
        this.ssrcGroups = ssrcGroups;
    }
}
