#ifndef ION_NODE_H
#define ION_NODE_H

#include <memory>
#include <string>
#include <vector>

#include <Halide.h>

#include "param.h"
#include "port.h"

namespace ion {

/**
 * Node class is used to manage node which consists graph structure.
 */
class Node {
    friend class Builder;

public:
    struct Impl {
        std::string id;
        std::string name;
        std::string graph_id;
        Halide::Target target;
        std::vector<Param> params;
        std::vector<Port> ports;
        std::vector<Halide::Internal::AbstractGenerator::ArgInfo> arginfos;

        Impl(): id(), name(), target(), params(), ports() {}
        Impl(const std::string& id_, const std::string& name_, const Halide::Target& target_);
        Impl(const std::string& id_, const std::string& name_, const Halide::Target& target_, const std::string& graph_id_);
    };

public:
    Node() : impl_(new Impl) {};

    Node(const std::shared_ptr<Impl>& impl) : impl_(impl) {};

    /**
     * Set the target of the node.
     * @arg target: The target ofject which consists of OS, Architecture, and sets of Features.
     * See https://halide-lang.org/docs/struct_halide_1_1_target.html for more details.
     * This target object can be retrieved by calling BuildingBlock::get_target from BuildingBlock::generate and BuildingBlock::schedule.
     * @return Node object whose target is set.
     */
    Node set_target(const Halide::Target& target) {
        impl_->target = target;
        return *this;
    }

    /**
     * Set the static parameters of the node.
     * @arg args: Variadic arguments of ion::Param.
     * Each of the arguments is applied and evaluated at compile time as a GeneratorParam declared in user-defined class deriving BuildingBlock.
     * @return Node object whose parameter is set.
     */
    template<typename... Args>
    Node set_param(Args ...args) {
        impl_->params = std::vector<Param>{args...};
        return *this;
    }

    void set_param(const std::vector<Param>& params) {
        impl_->params = params;
    }

    /**
     * Set the dynamic port of the node.
     * @arg args: Variadic arguments of ion::Port.
     * Each of the arguments is connected to each of the Input of this Node.
     * Each of the arguemnts should be one of the following:
     * 1. input port of the pipeline (Created by calling constructor of ion::Port).
     * 2. output port of another node (Retrieved by calling Node::operator[]).
     * @return Node object whose port is set.
     */
    template<typename... Args>
    Node operator()(Args ...args) {
        set_iport(create_ports_vector(args...));
        return *this;
    }

    void set_iport(const std::vector<Port>& ports);

    void set_iport(Port port);

    void set_iport(const std::string& name, Port port);

    /**
     * Retrieve relevant port of the node.
     * @arg name: The name of port name which is matched with first argument of Input/Output declared in user-defined class deriving BuildingBlock.
     * @return Port object which is specified by name.
     */
    Port operator[](const std::string& name);

    // Getter
    const std::string& id() const {
        return impl_->id;
    }

    const std::string& name() const {
        return impl_->name;
    }

    const Halide::Target& target() const {
        return impl_->target;
    }

    const std::vector<Param>& params() const {
        return impl_->params;
    }

    const std::vector<Port>& ports() const {
        return impl_->ports;
    }

    Port iport(const std::string& pn);
    std::vector<std::tuple<std::string, Port>> iports() const;

    Port oport(const std::string& pn);
    std::vector<std::tuple<std::string, Port>> oports() const;

private:
    Node(const std::string& id, const std::string& name, const Halide::Target& target)
        : impl_(new Impl{id, name, target})
    {
    }

    Node(const std::string& id, const std::string& name, const Halide::Target& target, const std::string& graph_id)
        : impl_(new Impl{id, name, target, graph_id})
    {
    }


    template<typename... Args>
    std::vector<Port> create_ports_vector(Args... args) const {
        return {get_iport(args)...};
    }

    Port get_iport(Port arg) const {
        // if input is port
        return arg;
    }

   template<typename T>
   Port get_iport(Halide::Buffer<T>&  arg) const {
        // if input is halide buffer
        if (impl_->graph_id.empty())
            return Port(arg);
        else
            return Port(arg, impl_->graph_id);
   }

    std::shared_ptr<Impl> impl_;
};

} // namespace ion

#endif // ION_NODE_H
