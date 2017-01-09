
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
