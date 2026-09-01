#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <py_async.hpp>
#include <py_bytes.hpp>
#include <py_holder.hpp>
#include <py_log.hpp>
#include <py_str.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/ntgcalls.hpp>

namespace py = pybind11;
using namespace ntgcalls::support;

PYBIND11_MODULE(ntgcalls, m, py::mod_gil_not_used()) {
@for c in classes
    {
        py::class_<@{c.cpp}, py_holder<@{c.cpp}>> w(m, "@{c.name}");
        w.def(py::init<>());
@for m in c.methods
@if m.static
        w.def_static("@{m.name|snake}", &@{c.cpp}::@{m.name|snake},
@for p in m.params
            py::arg("@{p.name|snake}"),
@end
            py::call_guard<py::gil_scoped_release>()
        );
@else
@if m.async
        w.def("@{m.name|snake}", [](@{c.cpp}& self@{m.argsep}
@for a in m.args
@if a.byval
            @{a.cpptype} @{a.name}@{a.sep}
@else
            const @{a.cpptype}& @{a.name}@{a.sep}
@end
@end
        ) {
            return async_run([&self@{m.argsep}
@for a in m.args
                @{a.name}@{a.sep}
@end
            ] {
@if m.isvoid
                self.@{m.name|snake}(
@else
                return self.@{m.name|snake}(
@end
@for a in m.args
                    @{a.name}@{a.sep}
@end
                );
            });
        }@{m.argsep}
@for p in m.params
            py::arg("@{p.name|snake}")@{p.sep}
@end
        );
@else
        w.def("@{m.name|snake}", &@{c.cpp}::@{m.name|snake},
@for p in m.params
            py::arg("@{p.name|snake}"),
@end
            py::call_guard<py::gil_scoped_release>()
        );
@end
@end
@end
    }
@end
@for e in enums
@if e.emit
    py::enum_<@{e.cpp}>(m, "@{e.name}")
@for mem in e.members
        .value("@{mem.disp}", @{e.cpp}::@{mem.cpp})
@end
        .export_values();
@end
@end
@for s in structs
    {
        py::class_<@{s.cpp}> w(m, "@{s.name}");
@if s.ctorable
        w.def(py::init<>());
@end
@if s.ctorargs
        w.def(py::init<
@for a in s.ctorargs
            @{a.cpptype}@{a.sep}
@end
        >(),
@for a in s.ctorargs
            py::arg("@{a.name|snake}")@{a.sep}
@end
        );
@end
@for f in s.fields
@if f.intenum
        w.def_property("@{f.name|snake}", [](const @{s.cpp}& self) { return static_cast<int>(self.@{f.cpp}); }, [](@{s.cpp}& self, const int v) { self.@{f.cpp} = static_cast<@{f.intenum}>(v); });
@else
        w.def_readwrite("@{f.name|snake}", &@{s.cpp}::@{f.cpp});
@end
@end
    }
@end
    const py::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
@for er in errors
    py::register_exception<@{er.cpp}>(m, "@{er.cpp|base}", baseExc);
@end
    install_log_bridge();
    Py_AtExit(&ntgcalls::utils::ShutdownHook::runAll);
    m.attr("__version__") = NTG_VERSION;
}
