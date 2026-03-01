#ifndef TESTSYSTEM_HPP
#define TESTSYSTEM_HPP

#include "System.hpp"

/**
 * @class TestSystem
 * @brief A user defined ECS system to test certain engine features
 */
class TestSystem : public vkrt::System {
public:
    TestSystem();
    ~TestSystem();
    virtual void update(std::vector<std::unique_ptr<vkrt::Entity>>& entityList, const vkrt::GlobalSingletons globalSingletons) override;
};

#endif