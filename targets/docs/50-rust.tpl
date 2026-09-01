@config banner =
@typemap Vector<*> = Vec&lt;*&gt;
@typemap Map<*,*> = HashMap&lt;*, *&gt;
@typemap long = i64
@typemap ulong = u64
@typemap int = i32
@typemap uint = u32
@typemap int8 = i8
@typemap uint8 = u8
@typemap int16 = i16
@typemap uint16 = u16
@typemap bool = bool
@typemap double = f64
@typemap string = String
@typemap bytes = Vec&lt;u8&gt;
@typemap Void = ()
@typemap media.* = *
@typemap models.* = *
@typemap p2p.* = *
@typemap e2e.* = *
@typemap instances.* = *
@typemap wrtc.* = *
@typemap webrtc.* = *
@typemap * = *
@config reserve_prefix = r#
@config reserved = as break const continue dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return static struct trait true type union unsafe use where while async await box do final macro override priv typeof unsized virtual yield
@for m in docpages
@file @{config.frag_dir}/@{m.docid}.rust.xml
    <lang-block language="rust">
        <category-title noref="true">
@if m.async
@if m.isvoid
            <shi>pub async fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>() <shi>-&gt; Result&lt;()&gt;</shi>
@else
@if m.retmap
            <shi>pub async fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>() <shi>-&gt; Result&lt;HashMap&lt;@{m.retmapkey|type}, @{m.retmapval|type}&gt;&gt;</shi>
@else
            <shi>pub async fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>() <shi>-&gt; Result&lt;@{m.ret|type}&gt;</shi>
@end
@end
@else
@if m.isvoid
            <shi>pub fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>()
@else
@if m.retmap
            <shi>pub fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>() <shi>-&gt; Result&lt;HashMap&lt;@{m.retmapkey|type}, @{m.retmapval|type}&gt;&gt;</shi>
@else
            <shi>pub fn</shi> <ref src="method">NTgCalls::<sb>@{m.name|snake|reserve}</sb></ref>() <shi>-&gt; Result&lt;@{m.ret|type}&gt;</shi>
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
                    <category-title><ref><sb>@{a.name|snake|reserve}</sb></ref>: <shi>@{a.type|type}</shi></category-title>
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
                    <category-title><ref><sb>@{p.name|snake|reserve}</sb></ref>: <shi>Option&lt;@{p.type|type}&gt;</shi></category-title>
@else
                    <category-title><ref><sb>@{p.name|snake|reserve}</sb></ref>: <shi>@{p.type|type}</shi></category-title>
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
@file @{config.frag_dir}/type-@{s.docid}.rust.xml
    <lang-block language="rust">
        <category-title noref="true">
            <shi>pub struct</shi> <ref src="class"><sb>@{s.name}</sb></ref>
        </category-title>
        <subtext>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for f in s.fields
@if f.optional
                    <category-title><ref><sb>@{f.name|snake|reserve}</sb></ref>: <shi>Option&lt;@{f.type|type}&gt;</shi></category-title>
@else
                    <category-title><ref><sb>@{f.name|snake|reserve}</sb></ref>: <shi>@{f.type|type}</shi></category-title>
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
@file @{config.frag_dir}/enum-@{e.docid}.rust.xml
    <lang-block language="rust">
        <category-title noref="true">
            <shi>pub enum</shi> <ref src="class"><sb>@{e.name}</sb></ref>
        </category-title>
        <subtext>
            <pg-title>ENUMERATION MEMBERS</pg-title>
            <subtext>
@for mem in e.members
                <category-title><ref>@{e.name}::<sb>@{mem.disp|pascal}</sb></ref></category-title>
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
@file @{config.frag_dir}/example-@{m.docid}.rust.xml
use ntgcalls::NTgCalls;

@if m.iscb
let app = NTgCalls::new();

@@mark@@
app.@{m.name|snake|reserve}(|
@for a in m.cb.cbargs
    @{a.name|snake|reserve}@{a.sep}
@end
| {
    // ...
});
@else
@if m.static
@ifany p in m.params : name
@@mark@@
NTgCalls::@{m.name|snake|reserve}(
@for p in m.params
    @{p.name|snake|reserve}@{p.sep}
@end
)?;
@else
@@mark@@
NTgCalls::@{m.name|snake|reserve}()?;
@end
@else
let app = NTgCalls::new();

@ifany p in m.params : name
@@mark@@
@if m.async
app.@{m.name|snake|reserve}(
@else
app.@{m.name|snake|reserve}(
@end
@for p in m.params
    @{p.name|snake|reserve}@{p.sep}
@end
@if m.async
).await?;
@else
)?;
@end
@else
@@mark@@
@if m.async
app.@{m.name|snake|reserve}().await?;
@else
app.@{m.name|snake|reserve}()?;
@end
@end
@end
@end
@endfile
@end
