#include <memory>

class view {
    struct implementation;

    std::unique_ptr<implementation, void (*)(implementation*)> _self;
public:
    view();
};



