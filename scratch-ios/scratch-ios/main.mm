//
//  main.m
//  scratch-ios
//
//  Created by Sean Parent on 7/16/14.
//  Copyright (c) 2014 Adobe. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "STLAppDelegate.h"

#include <utility>
#include <iostream>

using namespace std;

namespace adobe {

template <typename T>
class weak_arc {
    __weak T* _weak;

  public:
    constexpr weak_arc() : _weak(nil) { }
    weak_arc(const weak_arc&) = default;
    template <typename Y>
    weak_arc(const weak_arc<Y>& r) : _weak(r._weak) { }
    template <typename Y>
    weak_arc(Y* r) : _weak(r) { }

    weak_arc& operator=(const weak_arc&) = default;
    template <typename Y>
    weak_arc& operator=(const weak_arc<Y>& r) { _weak = r._weak; return *this; }
    template <typename Y>
    weak_arc& operator=(Y* r) { _weak = r; }

    void reset() { _weak = nil; }
    void swap(weak_arc& r) { std::swap(_weak, r._weak); }

    bool expired() const { return _weak; }
    T* lock() const { return _weak; }
};

} // namespace adobe

struct annotate {
    static int _source;
    int _id{_source++};

    annotate() { cout << "annotate ctor: " << _id << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor: "  << _id << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor: " << _id << endl; }

    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor:" << _id << endl; }
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

int annotate::_source;

struct hold {
    annotate _x;
    void no_sink(const annotate& x) {
        _x = x;
    }
};

@interface STLTest : NSObject

- (void)test;

@end

@implementation STLTest
{
    annotate _x;
    hold _hold;
}

- (void)test
{
    auto x = [self pass:_x];
    [self consume:x];
}


- (void)consume:(annotate)x
{
    _hold.no_sink(move(x));
}

- (annotate)pass:(annotate)x;
{
    return x;
}

@end

int main(int argc, char * argv[])
{
    @autoreleasepool {
    {
        auto p = [[STLTest alloc]init];
        [p test];
    }
    }

    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([STLAppDelegate class]));
    }
}
