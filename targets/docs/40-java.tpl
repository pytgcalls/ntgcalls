@config banner =
@typemap Vector<*> = java.util.List&lt;*&gt;
@typemap Map<*,*> = java.util.Map&lt;*, *&gt;
@typemap bytes = byte[]
@typemap long = long
@typemap ulong = long
@typemap int = int
@typemap uint = int
@typemap int8 = int
@typemap uint8 = int
@typemap int16 = int
@typemap uint16 = int
@typemap bool = boolean
@typemap double = double
@typemap string = String
@typemap Void = void
@typemap media.* = *
@typemap models.* = *
@typemap p2p.* = *
@typemap e2e.* = *
@typemap instances.* = *
@typemap wrtc.* = *
@typemap webrtc.* = *
@typemap * = *
@for m in docpages
@file @{config.frag_dir}/@{m.docid}.java.xml
    <lang-block language="java">
        <category-title noref="true">
@if m.static
@if m.isvoid
            <shi>static</shi> <shi>void</shi> <ref src="method">NTgCalls.<sb>@{m.name}</sb></ref>()
@else
            <shi>static</shi> <shi>@{m.ret|type}</shi> <ref src="method">NTgCalls.<sb>@{m.name}</sb></ref>()
@end
@else
@if m.isvoid
            <shi>void</shi> <ref src="method">NTgCalls.<sb>@{m.name}</sb></ref>()
@else
            <shi>@{m.ret|type}</shi> <ref src="method">NTgCalls.<sb>@{m.name}</sb></ref>()
@end
@end
        </category-title>
        <subtext>
@if m.hasdesc2
            <config id="@{m.desc2id}"/>
@end
            <br/>
@if m.iscb
            <category>
                <pg-title>CALLBACK ARGUMENTS</pg-title>
                <subtext>
@for a in m.cb.cbargs
                    <category-title><shi>@{a.type|type}</shi> <ref><sb>@{a.name|camel}</sb></ref></category-title>
@if a.hasdesc
                    <config id="@{a.descid}"/>
@end
@end
                </subtext>
            </category>
@else
@ifany p in m.params : name
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for p in m.params
@if p.optional
                    <category-title><shi>@{p.type|type}</shi> <ref><sb>@{p.name|camel}</sb></ref></category-title>
@else
                    <category-title><shi>@{p.type|type}</shi> <ref><sb>@{p.name|camel}</sb></ref></category-title>
@end
@if p.hasdesc
                    <config id="@{p.descid}"/>
@end
@end
                </subtext>
            </category>
@end
@end
@ifany r in m.raises : id
            <br/>
            <category>
                <pg-title>RAISES</pg-title>
                <subtext>
@for r in m.raises
                    <config id="@{r.id}"/>
@end
                </subtext>
            </category>
@end
        </subtext>
    </lang-block>
@endfile
@end
@for s in structs
@file @{config.frag_dir}/type-@{s.docid}.java.xml
    <lang-block language="java">
        <category-title noref="true">
            <shi>class</shi> <ref src="class"><sb>@{s.name}</sb></ref>
        </category-title>
        <subtext>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for f in s.fields
@if f.optional
                    <category-title><shi>@{f.type|type}</shi> <ref><sb>@{f.cpp}</sb></ref></category-title>
@else
                    <category-title><shi>@{f.type|type}</shi> <ref><sb>@{f.cpp}</sb></ref></category-title>
@end
@if f.hasdesc
                    <config id="@{f.descid}"/>
@end
@end
                </subtext>
            </category>
        </subtext>
    </lang-block>
@endfile
@end
@for e in enums
@if e.emit
@file @{config.frag_dir}/enum-@{e.docid}.java.xml
    <lang-block language="java">
        <category-title noref="true">
            <shi>enum</shi> <ref src="class"><sb>@{e.name}</sb></ref>
        </category-title>
        <subtext>
            <pg-title>ENUMERATION MEMBERS</pg-title>
            <subtext>
@for mem in e.members
                <category-title><ref>@{e.name}.<sb>@{mem.disp}</sb></ref></category-title>
@if mem.hasdesc
                <config id="@{mem.descid}"/>
@end
@end
            </subtext>
        </subtext>
    </lang-block>
@endfile
@end
@end
@for m in docpages
@file @{config.frag_dir}/example-@{m.docid}.java.xml
import io.github.pytgcalls.NTgCalls;

@if m.iscb
NTgCalls app = new NTgCalls();

@@mark@@
app.@{m.name}((
@for a in m.cb.cbargs
    @{a.name|camel}@{a.sep}
@end
) -> {
    // ...
});
@else
@if m.static
@ifany p in m.params : name
@@mark@@
NTgCalls.@{m.name}(
@for p in m.params
    @{p.name|camel}@{p.sep}
@end
);
@else
@@mark@@
NTgCalls.@{m.name}();
@end
@else
NTgCalls app = new NTgCalls();

@ifany p in m.params : name
@@mark@@
app.@{m.name}(
@for p in m.params
    @{p.name|camel}@{p.sep}
@end
);
@else
@@mark@@
app.@{m.name}();
@end
@end
@end
@endfile
@end
