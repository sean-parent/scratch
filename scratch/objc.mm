#include <iostream>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

auto f(annotate x) {
    return x;
}

struct my_class {
  my_class(annotate x) : _data(std::move(x)) {}
  annotate _data;

  my_class(my_class &&) noexcept = default;
  my_class &operator=(my_class &&) noexcept = default;
};

int f(int&& x);

template <class T>
void f(T x) {
    foo(move(x));
}

constexpr float convert(double x) { return float(x); }

int main() {
    int a[] = { 0, 1, 2, 3, 4 };
    for (const auto& e : a) {
        cout << e << endl;
    }

    auto x = convert(1.0);
    auto y = float(1.0);




    my_class w = annotate();
    #if 0
    annotate z = f(annotate());
    annotate x = f(std::move(z));
    #endif
}










































#if 0

#import <AppKit/NSView.h>

#include <string>
#include <memory>
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <functional>


using namespace std;

namespace framework {
class view {};
} // namespace framework

namespace ps_mobile {

} // namespace ps_mobile

class my_class {
};

@interface PSMMyClass : NSClassDescription
@end

@implementation PSMMyClass
@end

template <class Function, class... Args>
[[nodiscard]] auto async_dispatch(Function&& f, Args&&... args )
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;

    auto p = new packaged_type(std::bind([_f = std::forward<Function>(f)](Args&... args) {
        return std::invoke(_f, std::move(args)...);
    }, std::forward<Args>(args)...));

    auto result = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* f_) {
                packaged_type* f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

typedef NS_ENUM(NSInteger, UITableViewCellStyle) {
    UITableViewCellStyleDefault,
    UITableViewCellStyleValue1,
    UITableViewCellStyleValue2,
    UITableViewCellStyleSubtitle
};

enum class table_view_cell_style : size_t {
    standard,
    value1,
    value2,
    subtitle
};

class any {
  struct concept {
    virtual ~concept() = default;
    virtual ostream& print(ostream& out) const = 0;
  };
  template <class T> struct model final : concept {
    model(T x) : _data(std::move(x)) { }
    ostream& print(ostream& out) const override { return out << _data; }
    T _data;
  };
  shared_ptr<const concept> _self;

public:
  template <class T>
  any(T&& x) : _self(make_shared<model<T>>(forward<T>(x))) { }

  template <class T> any &operator=(T&& x) { return *this = any(forward<T>(x)); }

  friend inline ostream& operator<<(ostream& out, const any& x) { return x._self->print(out); }
};

struct silly {
    void member(int x) { cout << "silly: " << x << endl; }
};

int main() {
    {
        auto x = async_dispatch(&silly::member, silly(), 42);
        x.wait();
    }

vector<any> x = { 10, "Hello"s, 42.5 };

for (const auto& e : x) {
    cout << e << endl;
}

    {
    UITableViewCellStyle x = UITableViewCellStyleDefault;
    }

    {
    auto x = table_view_cell_style::standard;
    }



    {
      __block int x = 10;
      void (^f)(int) = ^(int y) {
        x += y;
      };
      f(5);
      NSLog(@"%d", x);

      void (^_f)(int) = [f copy];
    }

    {
      int x = 10;
      auto f = [&x](int y) {
        x += y;
      };
      f(5);
      cout << x << endl;

      function<void(int)> _f = f;
    }

#if 1

    {

    int y = 0;

    future<int> x = async_dispatch([y]{
        if (y == 0) throw runtime_error("bad y!");
        else return y + 42;
    });

    // do stuff

    try {
        cout << x.get() << endl;
    } catch(const std::exception& error) {
        cout << error.what() << endl;
    }

    }




#endif

    cout << u8"cube root: \u221b" << endl;

    cout << u8R"delimiter(
        This is a multi-line utf-8 string.
        as raw text so without \escape\ sequences.
    )delimiter" << endl;

    //cout << str << endl;

    {

      PSMMyClass *obj = [PSMMyClass new];
      __weak PSMMyClass *weak = obj;

      if (PSMMyClass *strong = weak) {
        // do stuff with strong
      }
    }

    {
      auto obj = make_shared<my_class>();
      weak_ptr<my_class> weak = obj;

      if (auto strong = weak.lock()) {
        // do stuff with strong
      }
    }


}

#endif

#if 0

#import <Foundation/Foundation.h>

#include <type_traits>
#include <iostream>

#include <boost/signals2/signal.hpp>

using namespace std;



int main() {


    auto url = [NSURL URLWithString:@"file:///Users/sean-parent/Projects/boost.org/boost_1_63_0/boost-build.jam"
        relativeToURL:[NSURL URLWithString:@"file:///Users/sean-parent/"]];
    NSLog(@"%@", url.relativeString);
}
#endif

#if 0

#include <memory>

namespace apollo {

#if defined(__OBJC__)
namespace objc {

template <class T> // T* is convertible to id
class weak_ptr {
    __weak id _p = nullptr;

    template <class Y>
    friend class weak_ptr;

    weak_ptr(T* p, int) : _p(p) { }

  public:
    constexpr weak_ptr() = default;

    // construct

    template <class Y>
    weak_ptr(Y* p) : weak_ptr(p, 0) { }

    weak_ptr(const weak_ptr&) = default;
    weak_ptr(weak_ptr&&) noexcept = default;

    template <class Y>
    weak_ptr(const weak_ptr<Y>& r) : weak_ptr(r.lock()) { }

    template <class Y>
    weak_ptr(weak_ptr<Y>&& r) noexcept : weak_ptr(std::move(r).lock()) { }

    // assign

    template <class Y>
    weak_ptr& operator=(Y* p) { return *this = weak_ptr(p); }

    weak_ptr& operator=(const weak_ptr&) = default;
    weak_ptr& operator=(weak_ptr&&) noexcept = default;

    template <class Y>
    weak_ptr& operator=( const weak_ptr<Y>& r ) { return *this = weak_ptr(r); }

    template <class Y>
    weak_ptr& operator=(weak_ptr<Y>&& r) noexcept { return *this = weak_ptr(std::move(r)); }

    T* lock() && { return std::move(_p); }
    T* lock() const & { return _p; }

    void reset() { _p = nullptr; }
    bool expired() const { return _p == nullptr; }
};

} // namespace objc

template <class T>
auto make_weak(T* x) -> objc::weak_ptr<T> { return x; }

#endif

template <class T>
auto make_weak(const std::shared_ptr<T>& x) -> std::weak_ptr<T> { return x; }

} // namespace apollo


#include <boost/signals2/signal.hpp>
#include <type_traits>

template <class T>
struct boost::signals2::weak_ptr_traits<apollo::objc::weak_ptr<T>> {
    using shared_type = T*;
};

template <class T, class = std::enable_if_t<std::is_convertible<T, id>::value>>
using enable_if_id = T;

template <class T>
struct boost::signals2::shared_ptr_traits<enable_if_id<T*>> {
    using weak_type = apollo::objc::weak_ptr<T>;
};


#import <Foundation/Foundation.h>
#include <iostream>
#include <typeinfo>

using namespace std;

template <class F>
void run(F f) {
    @autoreleasepool {
        f();
    }
}

template <class T> struct specialize;

template <class T>
struct specialize<T*> {
    using type = T*;

};


#import <Foundation/NSString.h>

int main() {
    {
    __weak NSObject* x;
    specialize<decltype(x)>::type  b = nullptr;
    }

    boost::signals2::signal<void()> sig;
    NSString* x;

    boost::signals2::signal<void()>::slot_type slot([]{ cout << "Hello" << endl; });
    slot.track_foreign(apollo::make_weak(x));
    slot.track_foreign(x);

    NSObject* a;

    cout << std::is_base_of<NSString, NSObject>::value << endl;
    cout << std::is_base_of<NSObject, NSString>::value << endl;

    cout << std::is_convertible<NSString*, NSObject*>::value << endl;
    cout << std::is_convertible<NSObject*, NSString*>::value << endl;
    auto b = apollo::make_weak(a);

    apollo::objc::weak_ptr<NSObject> y = apollo::make_weak(x);
    apollo::objc::weak_ptr<NSString> z = apollo::make_weak(a);
    apollo::objc::weak_ptr<NSString> c = b;

    run([&, _x = apollo::make_weak([[NSString alloc] initWithFormat: @"Address: %p", (void*)x])]{
        x = _x.lock();
    });

    NSString* _x = x;
    if (_x) cout << "object" << endl;
    else cout << "no object" << endl;
};


#endif

#if 0
#include <memory>
#include <type_traits>
#include <tuple>

#import <Foundation/NSValue.h>

template <class T>
auto make_weak(const std::shared_ptr<T>& x) { return std::weak_ptr<T>(x); }

template <class T>
struct weak_wrapper {
    T value;
};

template <class T>
auto make_weak(T* x) -> weak_wrapper<__weak T*> {
    return {x};
}

#include <iostream>
#include <future>
#include <typeinfo>

using namespace std;

struct weak_t {
    __weak NSNumber* value;
};

auto f() {
    auto a = [NSNumber numberWithDouble: 42.0];
    return weak_t{a};
}

int main() {
    NSObject* obj = [NSObject new];
    // weak_wrapper<NSObject> weak;
    weak_wrapper<__weak NSObject*> weak;
    @autoreleasepool {
        weak = make_weak(obj);
        cout << typeid(weak).name() << endl;
        obj = nil;
    }
    obj = weak.value;
    if (obj) cout << "object" << endl;
    else cout << "no object" << endl;


}
#endif


#if 0

#include <type_traits>
#include <iostream>

using namespace std;

constexpr bool BOOL_CONTROLLER = 1;

template <typename T, bool B = BOOL_CONTROLLER>
inline auto foo(T x) -> enable_if_t<B>
{
    cout << x << endl;
}

template <typename T, bool B = BOOL_CONTROLLER>
inline auto foo(T x) -> enable_if_t<!B>
{
    // Not an error because bar(x) depends on type T
    // but will not be instantiated.
    // If you replace T with int, becomes an error.
    bar(x); // function bar not defined
};

int main() {
    foo(10);

    int i = 5;
    void (^b)() = [=]{ cout << i << endl; };
}

#endif

#if 0

#include <AppKit/AppKit.h>
#include <iostream>

#include <vector>
#include <utility>

namespace adobe {

/**************************************************************************************************/

/*!

Initial state of out is assumed to be op(s). Final state of out is op(result.first);

*/

template <typename I,   // models InputIterator
          typename F,   // models UnaryLogicalOperation
          typename O>   // models OutputIterator
auto region_operation(bool s, I f, I l, F op, O out) -> std::pair<bool, O>
// requires: out accepts value_type(I1)
// returns final state of s at l and position of out
{
    if (op(s) == op(!s)) return { s, out };

    while (f != l) {
        s = !s;
        *out = *f;
        ++out;
        ++f;
    }
    return { s, out };
}

/**************************************************************************************************/

/*!

Initial state of out is assumed to be op(s1, s2). Final state of out is
op(result.first, result.second);

*/

template <typename I1,  // models InputIterator
          typename I2,  // models InputIterator
          typename F,   // models BinaryLogicalOperation
          typename O>   // O models OutputIterator
auto region_operation(bool s1, I1 f1, I1 l1,
                      bool s2, I2 f2, I2 l2,
                      F op, O out) -> std::tuple<bool, bool, O>
// requires: [f1, l1) and [f2, l2) are sorted ranges
//  and value_type(I1) == value_type(I2)
//  and out accepts value_type(I1)
// returns final state of s1 at l1 and s2 at l2 and position of out
{
    using namespace std;
    using namespace std::placeholders;

    bool s = op(s1, s2);

    while (f1 != l1 && f2 != l2) {
        auto p = min(*f1, *f2);

        if (p == *f1) { s1 = !s1; ++f1; }
        if (p == *f2) { s2 = !s2; ++f2; }

        if (s != op(s1, s2)) { s = !s; *out = p; ++out; }
    }
    tie(s1, out) = region_operation(s1, f1, l1, bind(op, _1, s2), out);
    tie(s2, out) = region_operation(s2, f2, l2, bind(op, s1, _1), out);

    return { s1, s2, out };
}

/**************************************************************************************************/

template <typename I1,  // models InputIterator
          typename I2,  // models InputIterator
          typename F,   // models BinaryLogicalOperation
          typename O>   // O models OutputIterator
auto region_operation_(bool s1, I1 f1, I1 l1,
                      bool s2, I2 f2, I2 l2,
                      F op, O out) -> std::tuple<bool, bool, O>
// requires: [f1, l1) and [f2, l2) are sorted by comp
//  and value_type(I1) == value_type(I2)
//  and out accepts value_type(I1)
// returns final state of s1 at l1 and s2 at l2 and position of out
{
    bool s_ = op(s1, s2);

    while (f1 != l1 && f2 != l2) {
        auto p = min(*f1, *f2);

        if (p == *f1) { s1 = !s1; ++f1; }
        if (p == *f2) { s2 = !s2; ++f2; }

        bool ns = op(s1, s2);
        if (s_ != ns) { *out = p; ++out; s_ = ns; }
    }
    std::tie(s1, out) = region_operation(s1, f1, l1, std::bind(op, std::placeholders::_1, s2), out);
    std::tie(s2, out) = region_operation(s2, f2, l2, std::bind(op, s1, std::placeholders::_1), out);

    return { s1, s2, out };
}

/**************************************************************************************************/


class region {
  public:
    region() = default;
    region(std::initializer_list<std::size_t> x) : points_(x) { }

    friend inline region operator~(region x) {
        x.compliment_ = !x.compliment_;
        return x;
    }

    friend inline region operator&(const region& x, const region& y) {
        return x.operator_(y, [](bool x, bool y){ return x && y; });
    }

    friend inline region operator|(const region& x, const region & y) {
        return x.operator_(y, [](bool x, bool y){ return x || y; });
    }

    friend inline region operator-(const region& x, const region& y) {
        return x.operator_(y, [](bool x, bool y){ return x && !y; });
    }

    bool operator[](std::size_t x) const {
        return ((upper_bound(begin(points_), end(points_), x) - begin(points_)) & 1) ^ compliment_;
    }

    template <typename F>
    void operator()(F f, std::size_t l) {
        std::size_t p = 0;

        auto if_ = begin(points_), il_ = end(points_);

        if (if_ == il_) return;
        p = *if_;
        if (!compliment_) {
            ++if_;
            if (p >= l) return;
        }

        while (p < l) {
            f(p);
            ++p;
            if (if_ != il_ && *if_ == p) {
                ++if_;
                if (if_ == il_) return;
                p = *if_;
                ++if_;
            }
        }
    }

  private:
    template <typename F>
    region operator_(const region& x, F op) const
    {
        region result;
        result.points_.reserve(points_.size() + x.points_.size());

        result.compliment_ = op(compliment_, x.compliment_);

        region_operation(compliment_, begin(points_), end(points_),
                         x.compliment_, begin(x.points_), end(x.points_),
                         op, back_inserter(result.points_));

        return result;
    }

    bool compliment_ = false;
    std::vector<std::size_t> points_;  // invariant points_[0] != 0
};

} // namespace adobe

using namespace adobe;
using namespace std;

template <typename T>
struct pair_
{
    pair_(const T& x, const T& y) : a(x), b(y) { }
    template <typename U>
    pair_& operator=(initializer_list<U> x) {
        auto f = begin(x), l = end(x);
        if (f != l) a = *f++;
        if (f != l) b = *f++;
        return *this;
    }

    T a, b;
};

int main()
{
#if 0
    int xa = 10;
    int xb = 43;

//    tie(xa, xb) = { xb, xa };

    tuple<int&, int&> ac(xa, xb);
    tuple<int, int> ad;

    ad = { 3, 4 };

    cout << xa << " " << xb << endl;
#endif

    int a[] = { 3, 8, 12, 14 }; // 00011111000011000...
    int b[] = { 0, 6, 11 };     // 11111100000111...

    region_operation(false, begin(a), end(a),
                     false, begin(b), end(b),
                     logical_and<bool>(), ostream_iterator<int>(cout, " "));
    cout << endl;

    region_operation(false, begin(a), end(a),
                     false, begin(b), end(b),
                     logical_or<bool>(), ostream_iterator<int>(cout, " "));
    cout << endl;

    region_operation(false, begin(a), end(a),
                     false, begin(b), end(b),
                     [](bool x, bool y) { return x && !y; }, ostream_iterator<int>(cout, " "));
    cout << endl;


    region x = { 0, 5, 9 };
    region y = { 3, 5, 9, 11 };

    region z = x & y;
    z([](size_t x){ cout << x << endl; }, 15);

    cout << "--- or ---" << endl;
    region or_ = x | y;
    or_([](size_t x){ cout << x << endl; }, 15);

    cout << "--- index ---" << endl;
    cout << x[2] << endl;
    cout << x[5] << endl;
    cout << x[6] << endl;

    
    cout << "--- subtraction ---" << endl;
    region sub_ = x - y;
    sub_([](size_t x){ cout << x << endl; }, 15);

    cout << "--- compliment ---" << endl;
    x = ~x;
    cout << x[2] << endl;
    cout << x[6] << endl;
}
#endif
