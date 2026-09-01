@config banner =
@typemap Vector<*> = list[*]
@typemap Map<*,*> = dict[*, *]
@typemap long = int
@typemap ulong = int
@typemap int = int
@typemap uint = int
@typemap int8 = int
@typemap uint8 = int
@typemap int16 = int
@typemap uint16 = int
@typemap bool = bool
@typemap double = float
@typemap string = str
@typemap bytes = bytes
@typemap Void = None
@typemap media.* = ntgcalls.*
@typemap models.* = ntgcalls.*
@typemap p2p.* = ntgcalls.*
@typemap e2e.* = ntgcalls.*
@typemap instances.* = ntgcalls.*
@typemap wrtc.* = ntgcalls.*
@typemap webrtc.* = ntgcalls.*
@typemap * = ntgcalls.*
@for m in docpages
@file @{config.frag_dir}/@{m.docid}.python.xml
    <lang-block language="python" default="true">
        <category-title noref="true">
@if m.async
@if m.isvoid
            <shi>async</shi> <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>()
@else
@if m.retmap
            <shi>async</shi> <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>() <shi>-&gt; dict[@{m.retmapkey|type}, @{m.retmapval|type}]</shi>
@else
            <shi>async</shi> <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>() <shi>-&gt; @{m.ret|type}</shi>
@end
@end
@else
@if m.isvoid
            <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>()
@else
@if m.retmap
            <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>() <shi>-&gt; dict[@{m.retmapkey|type}, @{m.retmapval|type}]</shi>
@else
            <ref src="method">NTgCalls.<sb>@{m.name|snake}</sb></ref>() <shi>-&gt; @{m.ret|type}</shi>
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
                    <category-title><ref><sb>@{a.name|snake}</sb></ref>: <shi>@{a.type|type}</shi></category-title>
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
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi>Optional</shi>[<shi>@{p.type|type}</shi>]</category-title>
@else
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi>@{p.type|type}</shi></category-title>
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
@file @{config.frag_dir}/type-@{s.docid}.python.xml
    <lang-block language="python" default="true">
        <category-title noref="true">
            <shi>class</shi> <ref src="class">ntgcalls.<sb>@{s.name}</sb></ref>()
        </category-title>
        <subtext>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for f in s.fields
@if f.optional
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi>Optional</shi>[<shi>@{f.type|type}</shi>]</category-title>
@else
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi>@{f.type|type}</shi></category-title>
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
@file @{config.frag_dir}/enum-@{e.docid}.python.xml
    <lang-block language="python" default="true">
        <category-title noref="true">
            <shi>enum</shi> <ref src="class">ntgcalls.<sb>@{e.name}</sb></ref>
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
@file @{config.frag_dir}/example-@{m.docid}.python.xml
from ntgcalls import NTgCalls

@if m.iscb
app = NTgCalls()

@@mark@@
def @{m.name|snake}(
@for a in m.cb.cbargs
    @{a.name|snake}@{a.sep}
@end
):
    ...

app.@{m.name|snake}(@{m.name|snake})
@else
@if m.static
@ifany p in m.params : name
@@mark@@
NTgCalls.@{m.name|snake}(
@for p in m.params
    @{p.name|snake}@{p.sep}
@end
)
@else
@@mark@@
NTgCalls.@{m.name|snake}()
@end
@else
app = NTgCalls()

@ifany p in m.params : name
@@mark@@
@if m.async
await app.@{m.name|snake}(
@else
app.@{m.name|snake}(
@end
@for p in m.params
    @{p.name|snake}@{p.sep}
@end
)
@else
@@mark@@
@if m.async
await app.@{m.name|snake}()
@else
app.@{m.name|snake}()
@end
@end
@end
@end
@endfile
@end
