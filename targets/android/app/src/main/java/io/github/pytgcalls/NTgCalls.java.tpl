@config java_pkg = io.github.pytgcalls
@config java_dir = @{config.self_dir}
@config java_exc_base = RuntimeException
@config import_sep = .
@config import_line = import $;
@config import_anchor = package
@importroot io.github.pytgcalls
@importroot java.util
@typemap Vector<*> = java.util.List<*>
@typemap Map<*, *> = java.util.Map<*, *>
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
@typemap media.* = io.github.pytgcalls.media.*
@typemap p2p.* = io.github.pytgcalls.p2p.*
@typemap e2e.* = io.github.pytgcalls.e2e.*
@typemap * = io.github.pytgcalls.*
@boxmap int = Integer
@boxmap uint = Integer
@boxmap int8 = Integer
@boxmap uint8 = Integer
@boxmap int16 = Integer
@boxmap uint16 = Integer
@boxmap long = Long
@boxmap ulong = Long
@boxmap double = Double
@boxmap bool = Boolean
@for e in enums
@if e.emit
@if e.ns
@file @{config.java_dir}/@{e.ns}/@{e.name}.java
package @{config.java_pkg}.@{e.ns};
@else
@file @{config.java_dir}/@{e.name}.java
package @{config.java_pkg};
@end

public enum @{e.name} {
@for mem in e.members
    @{mem.disp},
@end
}
@endfile
@end
@end
@for s in structs
@if s.ns
@file @{config.java_dir}/@{s.ns}/@{s.name}.java
package @{config.java_pkg}.@{s.ns};
@else
@file @{config.java_dir}/@{s.name}.java
package @{config.java_pkg};
@end

public class @{s.name} {
@for f in s.fields
    public final @{f.type|type} @{f.cpp};
@end

    public @{s.name}(
@for f in s.fields
        @{f.type|type} @{f.cpp}@{f.sep}
@end
    ) {
@for f in s.fields
        this.@{f.cpp} = @{f.cpp};
@end
    }
}
@endfile
@end
@for e in excs
@file @{config.java_dir}/exceptions/@{e.name}Exception.java
package @{config.java_pkg}.exceptions;

@if e.parent
public class @{e.name}Exception extends @{e.parent}Exception {
@else
public class @{e.name}Exception extends @{config.java_exc_base} {
@end
    public @{e.name}Exception(String message) {
        super(message);
    }
}
@endfile
@end
@for cb in callbacks
@file @{config.java_dir}/@{cb.name}.java
package @{config.java_pkg};

public interface @{cb.name} {
    void @{cb.method|camel}(
@for p in cb.params
        @{p.type|type} @{p.name|camel}@{p.sep}
@end
    );
}
@endfile
@end
@for c in classes
@file @{config.java_dir}/@{c.name}.java
package @{config.java_pkg};

public class @{c.name} {
    @SuppressWarnings("unused")
    private long nativePointer;

    static {
        System.loadLibrary("ntgcalls");
    }

    private native void init();

    private native void destroy();

    public @{c.name}() {
        init();
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

@for m in c.methods
@skip m.name in enableGlibLoop
@if m.static
    public static native @{m.ret|type} @{m.name}(
@else
    public native @{m.ret|type} @{m.name}(
@end
@for p in m.params
        @{p.type|type} @{p.name|camel}@{p.sep}
@end
    );
@end
}
@endfile
@end