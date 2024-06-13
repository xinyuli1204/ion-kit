#include "ion/node.h"

#include "log.h"

namespace ion {


Node::Impl::Impl(const NodeID& id_, const std::string& name_, const Halide::Target& target_)
    : id(id_), name(name_), target(target_), params(), ports()
{
    auto bb(Halide::Internal::GeneratorRegistry::create(name_, Halide::GeneratorContext(target_)));
    if (!bb) {
        log::error("BuildingBlock {} is not found", name_);
        throw std::runtime_error("Failed to create building block object");
    }

    arginfos = bb->arginfos();
}

Node::Impl::Impl(const NodeID& id_, const std::string& name_, const Halide::Target& target_, const GraphID& graph_id_)
    : id(id_), name(name_), target(target_), params(), ports(), graph_id(graph_id_)
{
    auto bb(Halide::Internal::GeneratorRegistry::create(name_, Halide::GeneratorContext(target_)));
    if (!bb) {
        log::error("BuildingBlock {} is not found", name_);
        throw std::runtime_error("Failed to create building block object");
    }

    arginfos = bb->arginfos();
}

void Node::set_iport(const std::vector<Port>& ports) {

    impl_->ports.erase(std::remove_if(impl_->ports.begin(), impl_->ports.end(),
                                      [&](const Port &p) { return p.has_succ_by_nid(this->id());}),
                       impl_->ports.end());

    size_t i = 0;
    for (auto& port : ports) {
        // TODO: Validation is better to be done lazily after BuildingBlock::configure
        //
        // if (info.dir == Halide::Internal::ArgInfoDirection::Output) {
        //     continue;
        // }

        // if (i >= ports.size()) {
        //     log::error("Port {} is out of range", i);
        //     throw std::runtime_error("Failed to validate input port");
        // }

        // NOTE: Is succ_chans name OK to be just leave as it is?
        port.impl_->succ_chans.insert({id(), "_ion_iport_" + std::to_string(i)});
        port.impl_ ->graph_id = impl_->graph_id;
        impl_->ports.push_back(port);

        i++;
    }
}

void Node::set_iport(Port port) {
    port.impl_ ->graph_id = impl_->graph_id;
    port.impl_->succ_chans.insert({id(), port.pred_name()});
    impl_->ports.push_back(port);
}

void Node::set_iport(const std::string& name, Port port) {
    port.impl_ ->graph_id = impl_->graph_id;
    port.impl_->succ_chans.insert({id(), name});
    impl_->ports.push_back(port);
}

Port Node::operator[](const std::string& name) {
    auto it = std::find_if(impl_->ports.begin(), impl_->ports.end(),
                           [&](const Port& p){ return p.pred_id() == impl_->id && p.pred_name() == name; });
    if (it == impl_->ports.end()) {
        // This is output port which is never referenced.
        // Bind myself as a predecessor and register
        Port port(id(), name);
        port.impl_ ->graph_id = impl_->graph_id;
        impl_->ports.push_back(port);
        return port;
    } else {
        // Port is already registered
        return *it;
    }
}

Port Node::iport(const std::string& pn) {
    for (const auto& p: impl_->ports) {
        auto it = std::find_if(p.impl_->succ_chans.begin(), p.impl_->succ_chans.end(),
                               [&](const Port::Channel& c) { return std::get<0>(c) == impl_->id && std::get<1>(c) == pn; });
        if (it != p.impl_->succ_chans.end()) {
            return p;
        }
    }

    auto msg = fmt::format("BuildingBlock \"{}\" has no input \"{}\"", name(), pn);
    log::error(msg);
    throw std::runtime_error(msg);
}

std::vector<std::tuple<std::string, Port>> Node::iports() const {
    std::vector<std::tuple<std::string, Port>> iports;
    for (const auto& p: impl_->ports) {
        auto it = std::find_if(p.impl_->succ_chans.begin(), p.impl_->succ_chans.end(),
                               [&](const Port::Channel& c) { return std::get<0>(c) == impl_->id; });
        if (it != p.impl_->succ_chans.end()) {
            iports.push_back(std::make_tuple(std::get<1>(*it), p));
        }
    }
    return iports;
}


std::vector<std::tuple<std::string, Port>> Node::dynamic_iports() const {
   std::vector<std::tuple<std::string, Port>> dynamic_iports;

   int iports_size = 0;

//   impl_->ports.erase(std::remove_if(impl_->ports.begin(), impl_->ports.end(),[&](const Port &p) {
//                  auto it = std::find_if(p.impl_->succ_chans.begin(), p.impl_->succ_chans.end(),
//                                         [&](const Port::Channel& c) { return std::get<0>(c) == impl_->id; });
//                  return p.is_dnamic_port() &&  it != p.impl_->succ_chans.end() ;
//              }), impl_->ports.end());

    for (const auto& p: impl_->ports) {
        auto it = std::find_if(p.impl_->succ_chans.begin(), p.impl_->succ_chans.end(),
                               [&](const Port::Channel& c) { return std::get<0>(c) == impl_->id; });
        if (it != p.impl_->succ_chans.end()) {
            iports_size+=1;
        }
    }

   int iports_idx = 0;
   for (auto & arginfo: impl_->arginfos){
      if (arginfo.dir == Halide::Internal::ArgInfoDirection::Input) {
          if(iports_idx>=iports_size){

              Port port( arginfo.name, arginfo.types[0]);
              port.impl_ ->graph_id = impl_->graph_id;
              port.impl_->is_dynamic_port = true;
              port.impl_->dimensions = arginfo.dimensions;
              port.impl_->succ_chans.insert({id(), arginfo.name});
              dynamic_iports.push_back(std::make_tuple(arginfo.name, port));
          }
          iports_idx ++;
      }
   }
   return dynamic_iports;
}



std::vector<std::tuple<std::string, Port>> Node::dynamic_oports() const {
   std::vector<std::tuple<std::string, Port>> dynamic_oports;
   int oports_size = 0;
//   impl_->ports.erase(std::remove_if(impl_->ports.begin(), impl_->ports.end(),[&](const Port &p) {
//                  return   p.is_dnamic_port() && id() == p.pred_id();}), impl_->ports.end());

   for (const auto& p: impl_->ports) {
         if (id() == p.pred_id()) {
              oports_size +=1;
         }

   }
   int oports_idx = 0;
   for (auto & arginfo: impl_->arginfos){
      if (arginfo.dir == Halide::Internal::ArgInfoDirection::Output) {
          if(oports_idx>=oports_size){
              Port port(id(), arginfo.name);
              port.impl_ ->type = arginfo.types[0];
              port.impl_ ->graph_id = impl_->graph_id;
              port.impl_->is_dynamic_port = true;
              port.impl_->dimensions = arginfo.dimensions;
              dynamic_oports.push_back(std::make_tuple(arginfo.name, port));
          }
          oports_idx ++;
      }
   }
   return dynamic_oports;
}

Port Node::oport(const std::string& pn) {
    return this->operator[](pn);

    // TODO: It is better to just return exisitng output port?
    //
    // auto it = std::find_if(impl_->ports.begin(), impl_->ports.end(),
    //                        [&](const Port& p) { return p.pred_id() == id() && p.pred_name() == pn; });

    // if (it != impl_->ports.end()) {
    //     return *it;
    // }

    // auto msg = fmt::format("BuildingBlock \"{}\" has no output \"{}\"", name(), pn);
    // log::error(msg);
    // throw std::runtime_error(msg);
}

std::vector<std::tuple<std::string, Port>> Node::oports() const {
    std::vector<std::tuple<std::string, Port>> oports;
    for (const auto& p: impl_->ports) {
        if (id() == p.pred_id()) {
            oports.push_back(std::make_tuple(p.pred_name(), p));
        }
    }
    return oports;
}

void Node::set_dynamic_port(Port port) {
      impl_->ports.push_back(port);
}


void  Node::validate_port_address_alignment ()const {
    std::vector<std::tuple<std::string, Port>> dynamic_oports;
    std::vector<std::tuple<std::string, Port>> oports =  Node::oports() ;
    std::vector<std::tuple<std::string, Port>> iports =  Node::iports() ;
    std::tuple<const void *, bool>  output_tuple;
    std::set<std::tuple<const void *, bool>> setOfTuples;

    for (auto& [pn, port] :oports) {
        for(auto& [i, t] : port.impl_->bound_address){
            setOfTuples.insert(t);
        }
    }

    for (auto& [pn, port] :iports) {
        for(auto& [i, t] : port.impl_->bound_address){
            if (setOfTuples.find(t) != setOfTuples.end()) {
               std::get<1>(t) = true;
            }
        }
    }
};

} // namespace ion
