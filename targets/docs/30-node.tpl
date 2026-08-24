@config banner =
@typemap Vector<*> = *[]
@typemap long = bigint
@typemap ulong = bigint
@typemap int = number
@typemap uint = number
@typemap int8 = number
@typemap uint8 = number
@typemap int16 = number
@typemap uint16 = number
@typemap bool = boolean
@typemap double = number
@typemap string = string
@typemap bytes = Buffer
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
@file @{config.frag_dir}/@{m.docid}.node.xml
    <lang-block language="node">
        <category-title noref="true">
@if m.async
@if m.isvoid
            <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>() <shi>: Promise&lt;void&gt;</shi>
@else
@if m.retmap
            <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>() <shi>: Promise&lt;Map&lt;@{m.retmapkey|type}, @{m.retmapval|type}&gt;&gt;</shi>
@else
            <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>() <shi>: Promise&lt;@{m.ret|type}&gt;</shi>
@end
@end
@else
@if m.isvoid
            <shi>static</shi> <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>()
@else
@if m.retmap
            <shi>static</shi> <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>() <shi>: Promise&lt;Map&lt;@{m.retmapkey|type}, @{m.retmapval|type}&gt;&gt;</shi>
@else
            <shi>static</shi> <ref src="method">NTgCalls.<sb>@{m.name|camel}</sb></ref>() <shi>: Promise&lt;@{m.ret|type}&gt;</shi>
@end
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
                    <category-title><ref><sb>@{a.name|camel}</sb></ref>: <shi>@{a.type|type}</shi></category-title>
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
                    <category-title><ref><sb>@{p.name|camel}</sb></ref><shi>?</shi>: <shi>@{p.type|type}</shi></category-title>
@else
                    <category-title><ref><sb>@{p.name|camel}</sb></ref>: <shi>@{p.type|type}</shi></category-title>
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
@file @{config.frag_dir}/type-@{s.docid}.node.xml
    <lang-block language="node">
        <category-title noref="true">
            <shi>interface</shi> <ref src="class"><sb>@{s.name}</sb></ref>
        </category-title>
        <subtext>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for f in s.fields
@if f.optional
                    <category-title><ref><sb>@{f.name|camel}</sb></ref><shi>?</shi>: <shi>@{f.type|type}</shi></category-title>
@else
                    <category-title><ref><sb>@{f.name|camel}</sb></ref>: <shi>@{f.type|type}</shi></category-title>
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
@file @{config.frag_dir}/enum-@{e.docid}.node.xml
    <lang-block language="node">
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
@file @{config.frag_dir}/example-@{m.docid}.node.xml
const { NTgCalls } = require('ntgcalls');

@if m.iscb
const app = new NTgCalls();

@@mark@@
app.@{m.name|camel}((
@for a in m.cb.cbargs
    @{a.name|camel}@{a.sep}
@end
) => {
    // ...
});
@else
@if m.static
@ifany p in m.params : name
@@mark@@
NTgCalls.@{m.name|camel}(
@for p in m.params
    @{p.name|camel}@{p.sep}
@end
);
@else
@@mark@@
NTgCalls.@{m.name|camel}();
@end
@else
const app = new NTgCalls();

@ifany p in m.params : name
@@mark@@
@if m.async
await app.@{m.name|camel}(
@else
app.@{m.name|camel}(
@end
@for p in m.params
    @{p.name|camel}@{p.sep}
@end
);
@else
@@mark@@
@if m.async
await app.@{m.name|camel}();
@else
app.@{m.name|camel}();
@end
@end
@end
@end
@endfile
@end
