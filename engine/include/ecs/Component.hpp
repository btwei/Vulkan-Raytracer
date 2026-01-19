#ifndef VKRT_COMPONENT_HPP
#define VKRT_COMPONENT_HPP

namespace vkrt {
    
/**
 * @class Component
 * @brief Base component class
 * 
 * This class acts as a base class for components in the ECS
 * system. The philosophy is that components contain state but
 * no behaviour ideally.
 */
class Component {
public:
    virtual ~Component() = default;
};

} // namespace vkrt

#endif // VKRT_COMPONENT_HPP