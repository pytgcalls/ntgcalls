@config banner =
@typemap Vector<bytes> = ntg_bytes
@typemap Vector<*> = *
@typemap long = int64_t
@typemap ulong = uint64_t
@typemap int = int32_t
@typemap uint = uint32_t
@typemap int8 = int8_t
@typemap uint8 = uint8_t
@typemap int16 = int16_t
@typemap uint16 = uint16_t
@typemap bool = bool
@typemap double = double
@typemap string = char*
@typemap bytes = uint8_t
@typemap Void = void
@typemap media.* = ntg_*
@typemap models.* = ntg_*
@typemap p2p.* = ntg_*
@typemap e2e.* = ntg_*
@typemap instances.* = ntg_*
@typemap * = ntg_*
@for m in docpages
@file @{config.frag_dir}/@{m.docid}.c.xml
    <lang-block language="c">
        <category-title noref="true">
@if m.iscb
            <ref><shi language="c">ntg_result</shi> <sb src="method">ntg_on_@{m.cbtype|snake}</sb></ref>()
@else
            <ref><shi language="c">ntg_result</shi> <sb src="method">ntg_@{m.name|snake}</sb></ref>()
@end
        </category-title>
        <subtext>
@if m.hasdesc2
            <config id="@{m.desc2id}"/>
@end
            <br/>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@if m.static
@else
                    <category-title><ref><sb>handle</sb></ref>: <shi language="c">ntg_instance*</shi></category-title>
                    <config id="C_HANDLE_DESC"/>
@end
@if m.iscb
                    <category-title><ref><sb>callback</sb></ref>: <shi language="c">ntg_@{m.cbtype|snake}_cb</shi></category-title>
                    <config id="C_CALLBACK_DESC"/>
                    <category-title><ref><sb>user_data</sb></ref>: <shi language="c">void*</shi></category-title>
                    <config id="C_USER_DATA_DESC"/>
@else
@for p in m.params
@if p.bytes
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi language="c">const uint8_t*</shi>, <ref><sb>@{p.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
@if p.string
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi language="c">const char*</shi></category-title>
@else
@if p.vector
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi language="c">const @{p.type|type|snake}*</shi>, <ref><sb>@{p.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
@if p.optional
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi language="c">const @{p.type|type|snake}*</shi></category-title>
@else
                    <category-title><ref><sb>@{p.name|snake}</sb></ref>: <shi language="c">@{p.type|type|snake}</shi></category-title>
@end
@end
@end
@end
@if p.hasdesc
                    <config id="@{p.descid}"/>
@end
@end
@if m.isvoid
@else
@if m.retstring
                    <category-title><ref><sb>out</sb></ref>: <shi language="c">char**</shi></category-title>
                    <config id="C_OUT_DESC"/>
@else
@if m.retbytes
                    <category-title><ref><sb>out</sb></ref>: <shi language="c">uint8_t**</shi>, <ref><sb>out_len</sb></ref>: <shi language="c">size_t*</shi></category-title>
                    <config id="C_OUT_LEN_DESC"/>
@else
@if m.retvector
                    <category-title><ref><sb>out</sb></ref>: <shi language="c">@{m.ret|type|snake}**</shi>, <ref><sb>out_len</sb></ref>: <shi language="c">size_t*</shi></category-title>
                    <config id="C_OUT_LEN_DESC"/>
@else
@if m.retmap
                    <category-title><ref><sb>out</sb></ref>: <shi language="c">@{m.retmapval|type|snake}_entry**</shi>, <ref><sb>out_len</sb></ref>: <shi language="c">size_t*</shi></category-title>
                    <config id="C_OUT_LEN_DESC"/>
@else
                    <category-title><ref><sb>out</sb></ref>: <shi language="c">@{m.ret|type|snake}*</shi></category-title>
                    <config id="C_OUT_DESC"/>
@end
@end
@end
@end
@end
@end
                </subtext>
            </category>
@if m.iscb
            <br/>
            <category>
                <pg-title>CALLBACK ARGUMENTS</pg-title>
                <subtext>
                    <category-title><ref><sb>handle</sb></ref>: <shi language="c">ntg_instance*</shi></category-title>
                    <config id="C_HANDLE_DESC"/>
@for a in m.cb.cbargs
@if a.bytes
                    <category-title><ref><sb>@{a.name|snake}</sb></ref>: <shi language="c">const uint8_t*</shi>, <ref><sb>@{a.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
@if a.string
                    <category-title><ref><sb>@{a.name|snake}</sb></ref>: <shi language="c">const char*</shi></category-title>
@else
@if a.vector
                    <category-title><ref><sb>@{a.name|snake}</sb></ref>: <shi language="c">const @{a.type|type|snake}*</shi>, <ref><sb>@{a.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
                    <category-title><ref><sb>@{a.name|snake}</sb></ref>: <shi language="c">@{a.type|type|snake}</shi></category-title>
@end
@end
@end
@if a.hasdesc
                    <config id="@{a.descid}"/>
@end
@end
                    <category-title><ref><sb>user_data</sb></ref>: <shi language="c">void*</shi></category-title>
                    <config id="C_USER_DATA_DESC"/>
                </subtext>
            </category>
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
@file @{config.frag_dir}/type-@{s.docid}.c.xml
    <lang-block language="c">
        <category-title noref="true">
            <ref><shi language="c">struct</shi> <sb src="class">ntg_@{s.name|snake}</sb></ref>
        </category-title>
        <subtext>
            <category>
                <pg-title>PARAMETERS</pg-title>
                <subtext>
@for f in s.fields
@if f.bytes
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi language="c">uint8_t*</shi>, <ref><sb>@{f.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
@if f.string
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi language="c">char*</shi></category-title>
@else
@if f.vector
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi language="c">@{f.type|type|snake}*</shi>, <ref><sb>@{f.name|snake}_len</sb></ref>: <shi language="c">size_t</shi></category-title>
@else
@if f.optional
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi language="c">@{f.type|type|snake}*</shi></category-title>
@else
                    <category-title><ref><sb>@{f.name|snake}</sb></ref>: <shi language="c">@{f.type|type|snake}</shi></category-title>
@end
@end
@end
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
@file @{config.frag_dir}/enum-@{e.docid}.c.xml
    <lang-block language="c">
        <category-title noref="true">
            <ref><shi language="c">enum</shi> <sb src="class">ntg_@{e.name|snake}</sb></ref>
        </category-title>
        <subtext>
            <pg-title>ENUMERATION MEMBERS</pg-title>
            <subtext>
@for mem in e.members
                <category-title><ref><sb>NTG_@{e.name|snake|upper}_@{mem.disp|snake|upper}</sb></ref></category-title>
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
@file @{config.frag_dir}/example-@{m.docid}.c.xml
#include "ntgcalls.h"

@if m.iscb
ntg_instance* handle = ntg_instance_create();

@@mark@@
ntg_on_@{m.cbtype|snake}(handle, &amp;@{m.name|snake}_callback, NULL);
@else
@if m.static
@@mark@@
ntg_result result = ntg_@{m.name|snake}(
@for p in m.params
@if p.bytes
    @{p.name|snake}, @{p.name|snake}_len,
@else
@if p.vector
    @{p.name|snake}, @{p.name|snake}_len,
@else
    @{p.name|snake},
@end
@end
@end
@if m.isvoid
@else
@if m.retbytes
    &amp;out, &amp;out_len
@else
@if m.retvector
    &amp;out, &amp;out_len
@else
@if m.retmap
    &amp;out, &amp;out_len
@else
    &amp;out
@end
@end
@end
@end
);
@else
ntg_instance* handle = ntg_instance_create();

@@mark@@
ntg_result result = ntg_@{m.name|snake}(
@if m.isvoid
    handle@{m.argsep}
@else
    handle,
@end
@for p in m.params
@if m.isvoid
@if p.bytes
    @{p.name|snake}, @{p.name|snake}_len@{p.sep}
@else
@if p.vector
    @{p.name|snake}, @{p.name|snake}_len@{p.sep}
@else
    @{p.name|snake}@{p.sep}
@end
@end
@else
@if p.bytes
    @{p.name|snake}, @{p.name|snake}_len,
@else
@if p.vector
    @{p.name|snake}, @{p.name|snake}_len,
@else
    @{p.name|snake},
@end
@end
@end
@end
@if m.isvoid
@else
@if m.retbytes
    &amp;out, &amp;out_len
@else
@if m.retvector
    &amp;out, &amp;out_len
@else
@if m.retmap
    &amp;out, &amp;out_len
@else
    &amp;out
@end
@end
@end
@end
);
@end
@end
@endfile
@end
