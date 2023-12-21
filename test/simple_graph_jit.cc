#include "ion/ion.h"

using namespace ion;

int main()
{
    Halide::Type t = Halide::type_of<int32_t>();
    Port min0{"min0", t}, extent0{"extent0", t}, min1{"min1", t}, extent1{"extent1", t}, v{"v", t};
    Param v41{"v", "41"};
    Builder b;
    b.with_bb_module("ion-bb-test");
    b.set_target(Halide::get_host_target());
    Node n;
    n = b.add("test_producer").set_param(v41);
    n = b.add("test_consumer")(n["output"], min0, extent0, min1, extent1, v);

    PortMap pm;
    pm.set(min0, 0);
    pm.set(extent0, 2);
    pm.set(min1, 0);
    pm.set(extent1, 2);
    pm.set(v, 1);

    ion::Buffer<int32_t> r = ion::Buffer<int32_t>::make_scalar();
    pm.set(n["output"], r);

    for (int i=0; i<5; ++i) {
        std::cout << i << "'th loop" << std::endl;
        b.run(pm);
    }

    return 0;
}
