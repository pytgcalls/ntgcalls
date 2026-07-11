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
@out @{config.self_dir}/index.d.ts
@for e in enums
@if e.emit
export enum @{e.name} {
@for mem in e.members
    @{mem.disp},
@end
}

@end
@end
@for s in structs
export interface @{s.name} {
@for f in s.fields
@if f.intenum
    @{f.name|camel}: number;
@else
@if f.optional
    @{f.name|camel}?: @{f.type|type};
@else
    @{f.name|camel}: @{f.type|type};
@end
@end
@end
}

@end
@for c in classes
export class @{c.name} {
    constructor();
@for m in c.methods
@if m.iscb
@else
@if m.static
    static @{m.name|camel}(
@else
    @{m.name|camel}(
@end
@for p in m.params
        @{p.name|camel}: @{p.type|type}@{p.sep}
@end
@if m.async
@if m.retmap
    ): Promise<Map<@{m.retmapkey|type}, @{m.retmapval|type}>>;
@else
    ): Promise<@{m.ret|type}>;
@end
@else
@if m.isvoid
    ): void;
@else
    ): @{m.ret|type};
@end
@end
@end
@end
@for cb in callbacks
    @{cb.method|camel}(callback: (
@for a in cb.cbargs
        @{a.name|camel}: @{a.type|type}@{a.sep}
@end
    ) => void): void;
@end
}

@end
