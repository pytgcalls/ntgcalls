@config banner = <!-- @generated from schema.ntl / DO NOT EDIT -->
@for m in docpages
@file @{config.docs_out}/NTgCalls/@{m.section}/@{m.title}.xml
<page>
    <h1>@{m.title}</h1>
@if m.hasdesc
    <config id="@{m.descid}"/>
@end
    <h3>Example</h3>
    <multisyntax id="languages">
        <tabs>
            <tab id="python">Python</tab>
            <tab id="c">C</tab>
            <tab id="node">Node.js</tab>
            <tab id="java">Java</tab>
            <tab id="rust">Rust</tab>
        </tabs>
@if m.hasexpython
        <syntax-highlight id="python"@{m.markpython} language="python"><config id="@{m.docid}_EXAMPLE_PYTHON"/></syntax-highlight>
@else
        <syntax-highlight id="python"@{m.markpython} language="python">
@insert @{config.frag_dir}/example-@{m.docid}.python.xml
</syntax-highlight>
@end
@if m.hasexc
        <syntax-highlight id="c"@{m.markc} language="c"><config id="@{m.docid}_EXAMPLE_C"/></syntax-highlight>
@else
        <syntax-highlight id="c"@{m.markc} language="c">
@insert @{config.frag_dir}/example-@{m.docid}.c.xml
</syntax-highlight>
@end
@if m.hasexnode
        <syntax-highlight id="node"@{m.marknode} language="javascript"><config id="@{m.docid}_EXAMPLE_NODE"/></syntax-highlight>
@else
        <syntax-highlight id="node"@{m.marknode} language="javascript">
@insert @{config.frag_dir}/example-@{m.docid}.node.xml
</syntax-highlight>
@end
@if m.hasexjava
        <syntax-highlight id="java"@{m.markjava} language="java"><config id="@{m.docid}_EXAMPLE_JAVA"/></syntax-highlight>
@else
        <syntax-highlight id="java"@{m.markjava} language="java">
@insert @{config.frag_dir}/example-@{m.docid}.java.xml
</syntax-highlight>
@end
@if m.hasexrust
        <syntax-highlight id="rust"@{m.markrust} language="rust"><config id="@{m.docid}_EXAMPLE_RUST"/></syntax-highlight>
@else
        <syntax-highlight id="rust"@{m.markrust} language="rust">
@insert @{config.frag_dir}/example-@{m.docid}.rust.xml
</syntax-highlight>
@end
    </multisyntax>
    <separator/>
    <h2>Details</h2>
@insert @{config.frag_dir}/@{m.docid}.python.xml
@insert @{config.frag_dir}/@{m.docid}.c.xml
@insert @{config.frag_dir}/@{m.docid}.node.xml
@insert @{config.frag_dir}/@{m.docid}.java.xml
@insert @{config.frag_dir}/@{m.docid}.rust.xml
</page>
@endfile
@end
@for s in structs
@file @{config.docs_out}/NTgCalls/@{s.section}/@{s.title}.xml
<page>
    <h1>@{s.title}</h1>
@if s.hasdesc
    <config id="@{s.descid}"/>
@end
    <separator/>
    <h2>Details</h2>
    <lang-tabs id="languages"/>
@insert @{config.frag_dir}/type-@{s.docid}.python.xml
@insert @{config.frag_dir}/type-@{s.docid}.c.xml
@insert @{config.frag_dir}/type-@{s.docid}.node.xml
@insert @{config.frag_dir}/type-@{s.docid}.java.xml
@insert @{config.frag_dir}/type-@{s.docid}.rust.xml
</page>
@endfile
@end
@for e in enums
@if e.emit
@file @{config.docs_out}/NTgCalls/@{e.section}/@{e.title}.xml
<page>
    <h1>@{e.title}</h1>
@if e.hasdesc
    <config id="@{e.descid}"/>
@end
    <separator/>
    <h2>Details</h2>
    <lang-tabs id="languages"/>
@insert @{config.frag_dir}/enum-@{e.docid}.python.xml
@insert @{config.frag_dir}/enum-@{e.docid}.c.xml
@insert @{config.frag_dir}/enum-@{e.docid}.node.xml
@insert @{config.frag_dir}/enum-@{e.docid}.java.xml
@insert @{config.frag_dir}/enum-@{e.docid}.rust.xml
</page>
@endfile
@end
@end
