struct move_only {
    move_only();
    move_only(move_only&&) { }
};

int main() {
    move_only x;
    move_only y;
    x = y;
}



#if 0

#include <iostream>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
};

struct object {
    object& operator=(const object&) & = default;
    object& operator=(object&&) & noexcept = default;
};

struct wrap {
    annotate a_;
    object b_;
};

void f(object& x);

int main() {
#if 0
    object x;
    // Assignment to rvalues will generate an error.
    object() = x;
    object() = object();
#endif
    wrap x;
    x = wrap();
    wrap() = x; // this is still allowed
}
#endif

#if 0

#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
};


struct wrap {
    ~wrap() noexcept(false) { throw 0; }
};

struct wrap2 {
    wrap m1_;
    annotate m2_;
};

#define ADOBE_NOEXCEPT noexcept

class example {
    example(string x, string y) : str1_(move(x)), str2_(move(y)) { }
    /* ... */

    // copy - if memberwise copy is not sufficient - provide custom copy
    example(const example&); // custom copy
    
    // move ctor equivalent to = default
    example(example&& x) ADOBE_NOEXCEPT : member_(move(x.member_)) { }
    
    // copy assignment is copy and move assign
    example& operator=(const example& x) { example tmp = x; *this = move(x); return *this; }
    
    // move assignment equivalent to = default
    example& operator=(example&& x) ADOBE_NOEXCEPT { member_ = move(x.member_); return *this; }
    
    /* ... */
    
 private:
    string str1_, str2_;
    int member_;
};

string append_file_suffix(string x)
{
    x += ".jpg";
    return x;
}

vector<int> deck()
{
    vector<int> x(52);
    iota(begin(x), end(x), 0);
    return x;
}

vector<int> shuffle(vector<int> x)
{
    random_shuffle(begin(x), end(x));
    return x;
}

auto multiply(int a, int b) -> int
{ return a * b; }

template <typename T>
auto multiply(T a, T b) -> decltype(a * b)
{ return a * b; }

declval<


vector<string> file_names()
{
    vector<string> x;
    x.push_back(append_file_suffix("best_image"));
    
    //...
    
    return x;
}




enum class color : int32_t { red, green, blue };

color operator+(color x, color y)
{
    return static_cast<color>(static_cast<underlying_type<color>::type>(x)
        + static_cast<underlying_type<color>::type>(y) % 3);
}


int main() {
    vector<string> file_names;
    
    //...
    
    file_names.push_back(append_file_suffix("best_image"));
    
    color x = color::red;
    cout << (x == color::red) << endl;
    
    x = x + color::blue;
    cout << static_cast<underlying_type<color>::type>(x) << endl;

#if 0
    vector<int> a = shuffle(deck());
    /* ... */
    
    for (const auto& e : a) cout << e << endl;
    
   // auto p = begin(a);
    
    auto& last = a.back();
    last += 10;
    
    
    for (auto& e : a) e += 3;
#endif
}

#endif


#if 0

#include <memory>

using namespace std;

template <typename C> // Concept base class
class any {
  public:
    template <typename T>
    any(T x) : self_(new model<T>(move(x))) { }
    
    template <typename T, typename F>
    any(unique_ptr<T>&& x, F cpy) : self_(new model_unique<T, F>(move(x), move(cpy))) { }
    
    any(const any& x) : self_(x.self_->copy()) { }
    any(any&&) noexcept = default;
    
    any& operator=(const any& x) { any tmp = x; *this = move(tmp); return *this; }
    any& operator=(any&&) noexcept = default;
    
    C* operator->() const { return &self_->dereference(); }
    C& operator*() const { return self_->dereference(); }
    
    template <typename T>
    any(any<T>& x) : self_(new model<T>) { }

  private:
    struct regular {
        virtual regular* copy() const = 0;
        virtual C& dereference() = 0;
    };
    
    template <typename T>
    struct model : regular {
        model(T x) : data_(move(x)) { }
        regular* copy() const { return new model(*this); }
        C& dereference() { return data_; }
        
        T data_;
    };
    
    template <typename T, typename F>
    struct model_unique : regular {
        model_unique(unique_ptr<T>&& x, F cpy) : data_(move(x)), copy_(move(cpy)) { }
        regular* copy() const { return new model_unique(copy_(*data_), copy_); }
        C& dereference() { return *data_; }
        
        unique_ptr<T> data_;
        F copy_;
    };
    
    unique_ptr<regular> self_;
};

/**************************************************************************************************/

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class shape {
  public:
    virtual void draw(ostream& out, size_t position) const = 0;
};

class polygon : public shape {
};

class square : public polygon {
    void draw(ostream& out, size_t position) const
    { out << string(position, ' ') << "square" << endl; }
};

class circle : public shape {
    void draw(ostream& out, size_t position) const
    { out << string(position, ' ') << "circle" << endl; }
};

int main() {
    thread_local int local = 5;

    any<shape> x = { unique_ptr<square>(new square()),
        [](const square& x) { return unique_ptr<square>(new square()); } };
    vector<any<shape>> a = { square(), circle(), x };
    for (const auto& e : a) e->draw(cout, 0);
}


#endif


#if 0

/*
    Copyright 2013 Adobe Systems Incorporated
    Distributed under the MIT License (see license at
    http://stlab.adobe.com/licenses.html)
    
    This file is intended as example code and is not production quality.
*/

#include <cassert>
#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

/******************************************************************************/
// Library

template <typename T>
void draw(const T& x, ostream& out, size_t position)
{ out << string(position, ' ') << x << endl; }

class object_t {
  public:
    template <typename T>
    object_t(T x) : self_(make_shared<model<T>>(move(x)))
    { }
    
    friend void draw(const object_t& x, ostream& out, size_t position)
    { x.self_->draw_(out, position); }
    
  private:
    struct concept_t {
        virtual ~concept_t() = default;
        virtual void draw_(ostream&, size_t) const = 0;
    };
    template <typename T>
    struct model : concept_t {
        model(T x) : data_(move(x)) { }
        void draw_(ostream& out, size_t position) const 
        { draw(data_, out, position); }
        
        T data_;
    };
    
   shared_ptr<const concept_t> self_;
};

using document_t = vector<object_t>;

void draw(const document_t& x, ostream& out, size_t position)
{
    out << string(position, ' ') << "<document>" << endl;
    for (const auto& e : x) draw(e, out, position + 2);
    out << string(position, ' ') << "</document>" << endl;
}

using history_t = vector<document_t>;

void commit(history_t& x) { assert(x.size()); x.push_back(x.back()); }
void undo(history_t& x) { assert(x.size()); x.pop_back(); }
document_t& current(history_t& x) { assert(x.size()); return x.back(); }

/******************************************************************************/
// Client

class my_class_t {
    /* ... */
};

void draw(const my_class_t&, ostream& out, size_t position)
{ out << string(position, ' ') << "my_class_t" << endl; }

int main()
{
    history_t h(1);

    current(h).emplace_back(0);
    current(h).emplace_back(string("Hello!"));
    
    draw(current(h), cout, 0);
    cout << "--------------------------" << endl;
    
    commit(h);
    
    current(h)[0] = 42.5;
    
    auto document = current(h);
    
    document.emplace_back(make_shared<my_class_t>());
    
    auto saving = async([=]() {
        this_thread::sleep_for(chrono::seconds(3));
        cout << "--------- 'save' ---------" << endl;
        draw(document, cout, 0);
    });

    current(h)[1] = string("World");
    
    current(h).emplace_back(current(h));
    current(h).emplace_back(my_class_t());
    
    draw(current(h), cout, 0);
    cout << "--------------------------" << endl;
    
    undo(h);
    
    draw(current(h), cout, 0);
}


#endif



#if 0
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;

struct person {
    string first_;
    string last_;
    string state_;
};

template <typename T>
void print(const T& a) {
    for (const auto& e: a) cout
        << setw(10) << e.first_
        << setw(10) << e.last_
        << setw(10) << e.state_ << endl;
}

int main() {
    person a[] = {
        { "John", "Smith", "New York" },
        { "John", "Doe", "Ohio" },
        { "John", "Stone", "Alaska" },
        { "Micheal", "Smith", "New York" },
        { "Micheal", "Doe", "Georgia" },
        { "Micheal", "Stone", "Alaska" }
    };
    
    random_shuffle(begin(a), end(a));
    
    cout << setw(10) << "- first -" << setw(10) << "- last -" << setw(10) << "- state -" << endl;
    print(a);
    
    cout << setw(10) << "- first -" << setw(10) << "- last -" << setw(10) << "* state *" << endl;
    stable_sort(begin(a), end(a), [](const person& x, const person& y){ return x.state_ < y.state_; });
    print(a);
    
    cout << setw(10) << "* first *" << setw(10) << "- last -" << setw(10) << "- state -" << endl;
    stable_sort(begin(a), end(a), [](const person& x, const person& y){ return x.first_ < y.first_; });
    print(a);
    
    cout << setw(10) << "- first -" << setw(10) << "* last *" << setw(10) << "- state -" << endl;
    stable_sort(begin(a), end(a), [](const person& x, const person& y){ return x.last_ < y.last_; });
    print(a);
    
    cout << setw(10) << "* first *" << setw(10) << "- last -" << setw(10) << "- state -" << endl;
    stable_sort(begin(a), end(a), [](const person& x, const person& y){ return x.first_ < y.first_; });
    print(a);
}


#endif



#if 0

#include <iostream>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
};

struct annotate_value_assign {
    annotate_value_assign() { cout << "annotate_value_assign ctor" << endl; }
    annotate_value_assign(const annotate_value_assign&) { cout << "annotate_value_assign copy-ctor" << endl; }
    annotate_value_assign(annotate_value_assign&&) noexcept { cout << "annotate_value_assign move-ctor" << endl; }
    annotate_value_assign& operator=(annotate_value_assign x) noexcept { cout << "annotate_value_assign value-assign" << endl; return *this; }
    ~annotate_value_assign() { cout << "annotate_value_assign dtor" << endl; }
};

struct wrap1 {
    annotate m_;
};

struct wrap2 {
    annotate_value_assign m_;
};

int main() {
    {
    wrap1 x, y;
    cout << "<move wrap1>" << endl;
    x = move(y);
    cout << "</move wrap1>" << endl;
    }
    {
    wrap2 x, y;
    cout << "<move wrap2>" << endl;
    x = move(y);
    cout << "</move wrap2>" << endl;
    }
    
    cout << boolalpha;
    cout << "wrap1 nothrow move assignable trait:" << is_nothrow_move_assignable<wrap1>::value << endl;
    cout << "wrap2 nothrow move assignable trait:" << is_nothrow_move_assignable<wrap2>::value << endl;
}

#endif



#if 0

template <typename T>
void swap(T& x, T& y)
{
    T t = x;
    x = y;
    y = t;
}

template <typename I>
void reverse(I f, I l)
{
    while (f != l) {
        --l;
        if (f == l) break;
        swap(*f, *l);
        ++f;
    }
}

template <typename I>
auto rotate(I f, I m, I l) -> I
{
    if (f == m) return l;
    if (m == l) return f;

    reverse(f, m);
    reverse(m, l);
    reverse(f, l);
    
    return f + (l - m);
}


template <typename I,
          typename P>
auto stable_partition(I f, I l, P p) -> I
{
    auto n = l - f;
    if (n == 0) return f;
    if (n == 1) return f + p(*f);
    
    auto m = f + (n / 2);

    return rotate(stable_partition(f, m, p),
                  m,
                  stable_partition(m, l, p));
}

#include <iostream>
#include <memory>
#include <vector>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate& x) { m_ = x.m_; cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&& x) noexcept { x.m_ = 13; cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { m_ = 255; cout << "annotate dtor" << endl; }
    
    int m_ = 42;
};



int main()
{
    // int a[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    
    std::vector<annotate> x;
    x.push_back(annotate());
    
    cout << "----" << endl;
    
    x.reserve(2);
    
    cout << "----" << endl;
    
    x.push_back(x.back());
    cout << "----" << endl;
    x.push_back(x.back());
    cout << "----" << endl;
    for (const auto& e : x) std::cout << e.m_ << "\n";
    
    #if 0
    stable_partition(&a[0], &a[10], [](const int& x) -> bool { return x & 1; });
    for (const auto& e : a) std::cout << e << "\n";
    
    std::cout << sizeof(std::shared_ptr<int>) << std::endl;
    #endif
}

#endif


#if 0

#include <iostream>
#include <memory>
#include <future>
#include <vector>
#include <dispatch/dispatch.h>

using namespace std;


// For illustration only
class group {
  public:
    template <typename F>
    void async(F&& f) {        
        auto then = then_;
        thread(bind([then](F& f){ f(); }, std::forward<F>(f))).detach();
    }
    
    template <typename F>
    void then(F&& f) {
        then_->f_ = forward<F>(f);
        then_.reset();
    }
    
  private:
    struct packaged {
        ~packaged() { thread(bind(move(f_))).detach(); }
        function<void ()> f_;
    };
    
    shared_ptr<packaged> then_ = make_shared<packaged>();
};


int main()
{
    group g;
    g.async([]() { 
        this_thread::sleep_for(chrono::seconds(2));
        cout << "task 1" << endl;
    });
    
    g.async([]() { 
        this_thread::sleep_for(chrono::seconds(1));
        cout << "task 2" << endl;
    });
    
    g.then([=](){
        cout << "done!" << endl;
    });
    
    this_thread::sleep_for(chrono::seconds(10));
}

#endif



#if 0

#include <iostream>
#include <memory>
#include <future>
#include <vector>
#include <dispatch/dispatch.h>

using namespace std;


namespace adobe {

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;
    
    auto p = new packaged_type(std::forward<F>(f), std::forward<Args>(args)...);
    auto result = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* f_) {
                packaged_type* f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

} // namespace adobe

// For illustration only
class group {
  public:
    template <typename F, typename ...Args>
    auto async(F&& f, Args&&... args)
        -> future<typename result_of<F (Args...)>::type>
    {
        using result_type = typename std::result_of<F (Args...)>::type;
        using packaged_type = std::packaged_task<result_type ()>;
        
        auto p = packaged_type(forward<F>(f), forward<Args>(args)...);
        auto result = p.get_future();
        
        auto then = then_;
        thread(bind([then](packaged_type& p){ p(); }, move(p))).detach();
        
        return result;
    }
    
    template <typename F, typename ...Args>
    auto then(F&& f, Args&&... args)
        -> future<typename result_of<F (Args...)>::type>
    {
        using result_type = typename std::result_of<F (Args...)>::type;
        using packaged_type = std::packaged_task<result_type ()>;
        
        auto p = packaged_type(forward<F>(f), forward<Args>(args)...);
        auto result = p.get_future();
        
        then_->reset(new packaged<packaged_type>(move(p)));
        then_ = nullptr;
        
        return result;
    }
    
  private:
    struct any_packaged {
        virtual ~any_packaged() = default;
    };
    
    template <typename P>
    struct packaged : any_packaged {
        packaged(P&& f) : f_(move(f)) { }
        ~packaged() { thread(bind(move(f_))).detach(); }
        
        P f_;
    };
    
    shared_ptr<unique_ptr<any_packaged>> then_ = make_shared<unique_ptr<any_packaged>>();
};


int main()
{
    group g;
    
    auto x = g.async([]() { 
        this_thread::sleep_for(chrono::seconds(2));
        cout << "task 1" << endl;
        return 10;
    });
    
    auto y = g.async([]() { 
        this_thread::sleep_for(chrono::seconds(1));
        cout << "task 2" << endl;
        return 5;
    });
    
    auto r = g.then(bind([](future<int>& x, future<int>& y) {
        cout << "done:" << (x.get() + y.get()) << endl;
    }, move(x), move(y)));
    
    r.get();
}

#endif


#if 0

/*
    Copyright 2013 Adobe Systems Incorporated
    Distributed under the MIT License (see license at
    http://stlab.adobe.com/licenses.html)
    
    This file is intended as example code and is not production quality.
*/

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <dispatch/dispatch.h>

using namespace std;

/******************************************************************************/
// Library

template <typename T>
void draw(const T& x, ostream& out, size_t position)
{ out << string(position, ' ') << x << endl; }

class object_t {
  public:
    template <typename T>
    object_t(T x) : self_(make_shared<model<T>>(move(x))) { }
        
    friend void draw(const object_t& x, ostream& out, size_t position)
    { x.self_->draw_(out, position); }
    
  private:
    struct concept_t {
        virtual ~concept_t() = default;
        virtual void draw_(ostream&, size_t) const = 0;
    };
    template <typename T>
    struct model : concept_t {
        model(T x) : data_(move(x)) { }
        void draw_(ostream& out, size_t position) const
        { draw(data_, out, position); }
        
        T data_;
    };
    
   shared_ptr<const concept_t> self_;
};

using document_t = vector<object_t>;

void draw(const document_t& x, ostream& out, size_t position)
{
    out << string(position, ' ') << "<document>" << endl;
    for (auto& e : x) draw(e, out, position + 2);
    out << string(position, ' ') << "</document>" << endl;
}

/******************************************************************************/
// Client

class my_class_t {
    /* ... */
};

void draw(const my_class_t&, ostream& out, size_t position)
{ out << string(position, ' ') << "my_class_t" << endl; }

int main()
{
    document_t document;
    
    document.emplace_back(my_class_t());
    document.emplace_back(string("Hello World!"));
    
    auto saving = async([=]() {
        this_thread::sleep_for(chrono::seconds(3));
        cout << "-- 'save' --" << endl;
        draw(document, cout, 0);
    });
    
    document.emplace_back(document);
    
    draw(document, cout, 0);
    saving.get();
}


#endif


/**************************************************************************************************/


//
//  main.cpp
//  scratch
//
//  Created by Sean Parent on 3/18/13.
//  Copyright (c) 2013 stlab. All rights reserved.

/**************************************************************************************************/

#if 0

#include <iostream>
#include <memory>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
};


int main() {

auto x = unique_ptr<int>(new int(5));
x = move(x);

cout << *x << endl;


}



#endif

/**************************************************************************************************/


#if 0
#include <list>
#include <future>
#include <iostream>
#include <boost/range/algorithm/find.hpp>
#include <cstddef>
#include <dispatch/dispatch.h>
#include <adobe/algorithm.hpp>
#include <list>

//using namespace std;
using namespace adobe;
using namespace std;


namespace adobe {

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;
    
    auto p = new packaged_type(std::forward<F>(f), std::forward<Args>(args)...);
    auto result = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* f_) {
                packaged_type* f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

} // namespace adobe

template <typename T>
class concurrent_queue
{
    mutex   mutex_;
    list<T> q_;
  public:
    void enqueue(T x)
    {
        list<T> tmp;
        tmp.push_back(move(x));
        {
        lock_guard<mutex> lock(mutex);
        q_.splice(end(q_), tmp);
        }
    }
    
    // ...
};

class join_task {
};

struct last_name {
    using result_type = const string&;
    
    template <typename T>
    const string& operator()(const T& x) const { return x.last; }
};


int main() {

#if 0
auto a = { 0, 1, 2, 3, 4, 5 };
auto x = 3;
{
auto p = find(begin(a), end(a), x);
cout << *p << endl;
}
{
auto p = find(a, x);
cout << *p << endl;
}
#endif

struct employee {
    string first;
    string last;
};

#if 0
int f(int x);
int g(int x);
int r[] = { 0, 1, 2, 3, 4, 5 };

for (const auto& e: r) f(g(e));
for (const auto& e: r) { f(e); g(e); };
for (auto& e: r) e = f(e) + g(e);
#endif

employee a[] = { { "Sean", "Parent" }, { "Jaakko", "Jarvi" }, { "Bjarne", "Stroustrup" } };

{

sort(a, [](const employee& x, const employee& y){ return x.last < y.last; });
auto p = lower_bound(a, "Parent",
    [](const employee& x, const string& y){ return x.last < y; });
    
cout << p->first << endl;
}

{
sort(a, less(), &employee::last);
auto p = lower_bound(a, "Parent", less(), &employee::last);

cout << p->first << endl;
}

{
sort(a, less(), &employee::last);

// ...
auto p = lower_bound(a, "Parent", less(), last_name());

cout << p->first << endl;
}

auto get_name = []{ return string("parent"); };

string s = get_name();
//auto p = find_if(a, [&](const string& e){ return e == "Sean"; });



}

#endif

#if 0

#include <iostream>
#include <vector>

using namespace std;

class object_t {
  public:
    template <typename T> // T models Drawable
    object_t(T x) : self_(make_shared<model<T>>(move(x)))
    { }
    
    void draw(ostream& out, size_t position) const
    { self_->draw_(out, position); }
    
  private:
    struct concept_t {
        virtual ~concept_t() = default;
        virtual void draw_(ostream&, size_t) const = 0;
    };
    
    template <typename T>
    struct model : concept_t {
        model(T x) : data_(move(x)) { }
        void draw_(ostream& out, size_t position) const 
        { data_.draw(out, position); }
        
        T data_;
    };
    
    shared_ptr<const concept_t> self_;
};

using document_t = vector<object_t>;

void draw(const document_t& x, ostream& out, size_t position)
{
    out << string(position, ' ') << "<document>" << endl;
    for (const auto& e : x) e.draw(out, position + 2);
    out << string(position, ' ') << "</document>" << endl;
}

class my_class_t
{
  public:
    void draw(ostream& out, size_t position) const
    { out << string(position, ' ') << "my_class_t" << endl; }
    
};

int main()
{
    document_t document;
    
    document.emplace_back(my_class_t());
    
    draw(document, cout, 0);
}

#endif

#if 0

#include <algorithm>
#include <iostream>

using namespace std;

int main()
{
    double a[] = { 2.5, 3.6, 4.7, 5.8, 6.3 };
    
    auto p = lower_bound(begin(a), end(a), 5, [](double e, int x) {  return e < x; });
    
    cout << *p << endl;
}

#endif

#if 0

#include <list>
#include <future>
#include <iostream>

#include <dispatch/dispatch.h>

using namespace std;

namespace adobe {

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;
    
    auto p = new packaged_type(std::forward<F>(f), std::forward<Args>(args)...);
    auto result = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* f_) {
                packaged_type* f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

} // namespace adobe



template <typename T>
class concurrent_queue
{
  public:
    using queue_type = list<T>;
    
    bool enqueue(T x)
    {
        bool was_dry;
        
        queue_type tmp;
        tmp.push_back(move(x));
    
        {
        lock_guard<mutex> lock(mutex);
        was_dry = was_dry_; was_dry_ = false;
        q_.splice(end(q_), tmp);
        }
        return was_dry;
    }
    
    queue_type deque()
    {
        queue_type result;
        
        {
        lock_guard<mutex> lock(mutex);
        if (q_.empty()) was_dry_ = true;
        else result.splice(end(result), q_, begin(q_));
        }
        
        return result;
    }

  private:
    mutex       mutex_;
    queue_type  q_;
    bool        was_dry_ = true;
};

class serial_task_queue
{
  public:
    template <typename F, typename... Args>
    auto async(F&& f, Args&&... args) -> future<typename result_of<F (Args...)>::type>
    {
        using result_type = typename result_of<F (Args...)>::type;
        using packaged_type = packaged_task<result_type ()>;
        
        auto p = make_shared<packaged_type>(bind(forward<F>(f), forward<Args>(args)...));
        auto result = p->get_future();
        
        if (shared_->enqueue([=](){
            (*p)();
            queue_type tmp = shared_->deque();
            if (!tmp.empty()) adobe::async(move(tmp.front()));
        })) adobe::async(move(shared_->deque().front()));
        
        return result;
    }
  private:
    using shared_type = concurrent_queue<function<void ()>>;
    using queue_type = shared_type::queue_type;
      
    shared_ptr<shared_type> shared_ = make_shared<shared_type>();
};

int main()
{
    serial_task_queue queue;
    
    cout << "begin async" << endl;
    future<int> x = queue.async([]{ cout << "task 1" << endl; sleep(3); cout << "end 1" << endl; return 42; });
    
    int y = 10;
    queue.async([](int x) { cout << "x = " << x << endl; }, move(y));
    
    #if 1
    queue.async(bind([](future<int>& x) {
        cout << "task 2: " << x.get() << endl; sleep(2); cout << "end 2" << endl;
    }, move(x)));
    
    #else
    
    #if 0
    queue.async([](future<int>&& x) {
        cout << "task 2: " << x.get() << endl; sleep(2); cout << "end 2" << endl;
    }, move(x));
    #endif
    #endif
    queue.async([]{ cout << "task 3" << endl; sleep(1); cout << "end 3" << endl; });
    cout << "end async" << endl;
    sleep(10);
}


#endif

#if 0

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

/*

With C++11 rotate now returns the new mid point.

*/

using namespace std;

template <typename I> // I models RandomAccessIterator
void slide_0(I f, I l, I p)
{
    if (p < f) rotate(p, f, l);
    if (l < p) rotate(f, l, p);
    // else do nothing
}

template <typename I> // I models RandomAccessIterator
auto slide(I f, I l, I p) -> pair<I, I>
{
    if (p < f) return { p, rotate(p, f, l) };
    if (l < p) return { rotate(f, l, p), p };
    return { f, l };
}

#if 0
template <typename I, // I models BidirectionalIterator
          typename S> // S models UnaryPredicate
auto gather(I f, I l, I p, S s) -> pair<I, I>
{
    using value_type = typename iterator_traits<I>::value_type;
    
    return { stable_partition(f, p, [&](const value_type& x){ return !s(x); }),
             stable_partition(p, l, s) };
}
#endif

namespace adobe {

template <typename P>
class unary_negate
{
  public:
    using result_type = bool;
    
    unary_negate(P p) : p_(move(p)) { }
    
    template <typename T>
    bool operator()(const T& x) const { return !p_(x); }

  private:
    P p_;
};

template <typename P>
auto not1(P p) -> unary_negate<P> { return unary_negate<P>(move(p)); }

} // namespace adobe

template <typename I, // I models BidirectionalIterator
          typename S> // S models UnaryPredicate
auto gather(I f, I l, I p, S s) -> pair<I, I>
{
    return { stable_partition(f, p, adobe::not1(s)),
             stable_partition(p, l, s) };
}

struct is_odd
{
    typedef bool result_type;
    typedef int  argument_type;
    template <typename T>
    bool operator()(const T& x) const { return x & 1; }
};


struct Panel {
    int cur_panel_center();
    int cur_panel_left();
    int cur_right();
    int width();
    template <typename T, typename U>
    int Move(T, U);
    int panel_width();
};

#define CHECK(x)
#define CHECK_LT(x, y)
#define CHECK_NE(x, y)
#define CHECK_GE(x, y)

const int kBarPadding = 0;
const int kAnimMs = 0;

struct PanelBar {
    void RepositionExpandedPanels(Panel* fixed_panel);
    template <typename T, typename U> int GetPanelIndex(T, U);
    
    vector<shared_ptr<Panel>> expanded_panels_;
    Panel* wm_;
};

using Panels = vector<shared_ptr<Panel>>;

template <typename T> using ref_ptr = shared_ptr<T>;

void PanelBar::RepositionExpandedPanels(Panel* fixed_panel) {
  CHECK(fixed_panel);

  // First, find the index of the fixed panel.
  int fixed_index = GetPanelIndex(expanded_panels_, *fixed_panel);
  CHECK_LT(fixed_index, expanded_panels_.size());
  
  {
  const int center_x = fixed_panel->cur_panel_center();
  
#if 0
  auto p = find_if(begin(expanded_panels_), end(expanded_panels_),
    [&](const ref_ptr<Panel>& e){ return center_x <= e->cur_panel_center(); });
#endif

  auto f = begin(expanded_panels_) + fixed_index;
  
  auto p = lower_bound(begin(expanded_panels_), f, center_x,
    [](const ref_ptr<Panel>& e, int x){ return e->cur_panel_center() < x; });
      
  rotate(p, f, f + 1);
  }

#if 0
  // Next, check if the panel has moved to the other side of another panel.
  const int center_x = fixed_panel->cur_panel_center();
  for (size_t i = 0; i < expanded_panels_.size(); ++i) {
    Panel* panel = expanded_panels_[i].get();
    if (center_x <= panel->cur_panel_center() ||
        i == expanded_panels_.size() - 1) {
      if (panel != fixed_panel) {
        // If it has, then we reorder the panels.
        ref_ptr<Panel> ref = expanded_panels_[fixed_index];
        expanded_panels_.erase(expanded_panels_.begin() + fixed_index);
        if (i < expanded_panels_.size()) {
          expanded_panels_.insert(expanded_panels_.begin() + i, ref);
        } else {
          expanded_panels_.push_back(ref);
        }
      }
      break;
    }
  }
#endif
#if 0

  // Next, check if the panel has moved to the other side of another panel.
  const int center_x = fixed_panel->cur_panel_center();
  for (size_t i = 0; i < expanded_panels_.size(); ++i) {
    Panel* panel = expanded_panels_[i].get();
    if (center_x <= panel->cur_panel_center() || i == expanded_panels_.size() - 1) {
      if (panel != fixed_panel) {
        // If it has, then we reorder the panels.
        ref_ptr<Panel> ref = expanded_panels_[fixed_index];
        expanded_panels_.erase(expanded_panels_.begin() + fixed_index);
        expanded_panels_.insert(expanded_panels_.begin() + i, ref);
      }
      break;
    }
  }
  
#endif
    
  // Find the total width of the panels to the left of the fixed panel.
  int total_width = 0;
  fixed_index = -1;
  for (int i = 0; i < static_cast<int>(expanded_panels_.size()); ++i) {
    Panel* panel = expanded_panels_[i].get();
    if (panel == fixed_panel) {
      fixed_index = i;
      break;
    }
    total_width += panel->panel_width();
  }
  CHECK_NE(fixed_index, -1);
  int new_fixed_index = fixed_index;

  // Move panels over to the right of the fixed panel until all of the ones
  // on the left will fit.
  int avail_width = max(fixed_panel->cur_panel_left() - kBarPadding, 0);
  while (total_width > avail_width) {
    new_fixed_index--;
    CHECK_GE(new_fixed_index, 0);
    total_width -= expanded_panels_[new_fixed_index]->panel_width();
  }

  // Reorder the fixed panel if its index changed.
  if (new_fixed_index != fixed_index) {
    Panels::iterator it = expanded_panels_.begin() + fixed_index;
    ref_ptr<Panel> ref = *it;
    expanded_panels_.erase(it);
    expanded_panels_.insert(expanded_panels_.begin() + new_fixed_index, ref);
    fixed_index = new_fixed_index;
  }

  // Now find the width of the panels to the right, and move them to the
  // left as needed.
  total_width = 0;
  for (Panels::iterator it = expanded_panels_.begin() + fixed_index + 1;
       it != expanded_panels_.end(); ++it) {
    total_width += (*it)->panel_width();
  }

  avail_width = max(wm_->width() - (fixed_panel->cur_right() + kBarPadding),
                    0);
  while (total_width > avail_width) {
    new_fixed_index++;
    CHECK_LT(new_fixed_index, expanded_panels_.size());
    total_width -= expanded_panels_[new_fixed_index]->panel_width();
  }

  // Do the reordering again.
  if (new_fixed_index != fixed_index) {
    Panels::iterator it = expanded_panels_.begin() + fixed_index;
    ref_ptr<Panel> ref = *it;
    expanded_panels_.erase(it);
    expanded_panels_.insert(expanded_panels_.begin() + new_fixed_index, ref);
    fixed_index = new_fixed_index;
  }

  // Finally, push panels to the left and the right so they don't overlap.
  int boundary = expanded_panels_[fixed_index]->cur_panel_left() - kBarPadding;
  for (Panels::reverse_iterator it =
         // Start at the panel to the left of 'new_fixed_index'.
         expanded_panels_.rbegin() +
         (expanded_panels_.size() - new_fixed_index);
       it != expanded_panels_.rend(); ++it) {
    Panel* panel = it->get();
    if (panel->cur_right() > boundary) {
      panel->Move(boundary, kAnimMs);
    } else if (panel->cur_panel_left() < 0) {
      panel->Move(min(boundary, panel->panel_width() + kBarPadding), kAnimMs);
    }
    boundary = panel->cur_panel_left() - kBarPadding;
  }

  boundary = expanded_panels_[fixed_index]->cur_right() + kBarPadding;
  for (Panels::iterator it = expanded_panels_.begin() + new_fixed_index + 1;
       it != expanded_panels_.end(); ++it) {
    Panel* panel = it->get();
    if (panel->cur_panel_left() < boundary) {
      panel->Move(boundary + panel->panel_width(), kAnimMs);
    } else if (panel->cur_right() > wm_->width()) {
      panel->Move(max(boundary + panel->panel_width(),
                      wm_->width() - kBarPadding),
                  kAnimMs);
    }
    boundary = panel->cur_right() + kBarPadding;
  }
}

int main()
{
    int a[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto r = slide(&a[5], &a[8], &a[6]);
    copy(begin(a), end(a), ostream_iterator<int>(cout, ", "));
    auto r2 = gather(begin(a), end(a), &a[5], is_odd());
    copy(begin(a), end(a), ostream_iterator<int>(cout, ", "));
    copy(r2.first, r2.second, ostream_iterator<int>(cout, ", "));
    
}

#endif

/**************************************************************************************************/

#if 0

#include <functional>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <numeric>
#include <deque>

using namespace std;

template <typename I, typename N>
void advance_slow(I& i, N n)
{
    while (n != 0) { --n; ++i; };
}

template <typename I>
size_t distance_slow(I f, I l)
{
    size_t n = 0;
    while (f != l) { ++f; ++n; }
    return n;
}

template <typename I, typename T>
I lower_bound_slow(I f, I l, T x)
{
    size_t n = distance_slow(f, l);
    while (n != 0) {
        size_t h = n / 2;
        I m = f;
        advance_slow(m, h);
        if (*m < x) { f = m; ++f; n -= h + 1; }
        else n = h;
    }
    return f;
}

template <typename I, typename T>
I lower_bound_fast(I f, I l, T x)
{
    size_t n = l - f;
    while (n != 0) {
        size_t h = n / 2;
        I m = f;
        m += h;
        if (*m < x) { f = m; ++f; n -= h + 1; }
        else n = h;
    }
    return f;
}

using container = vector<int>; // Change this to a deque to see a huge difference

long long __attribute__ ((noinline)) time_slow(vector<int>& r, const container& a, const container& b)
{
    auto start = chrono::high_resolution_clock::now();
    
    for (const auto& e : b) r.push_back(*lower_bound_slow(begin(a), end(a), e));
    
    return chrono::duration_cast<chrono::microseconds>
        (chrono::high_resolution_clock::now()-start).count();
}

long long __attribute__ ((noinline)) time_fast(vector<int>& r, const container& a, const container& b)
{
    auto start = chrono::high_resolution_clock::now();
    
    for (const auto& e : b) r.push_back(*lower_bound_fast(begin(a), end(a), e));
    
    return chrono::duration_cast<chrono::microseconds>
        (chrono::high_resolution_clock::now()-start).count();
}


int main() {
    container a(10000);
    iota(begin(a), end(a), 0);
    
    container b = a;
    random_shuffle(begin(b), end(b));
    
    vector<int> r;
    r.reserve(b.size() * 2);
    
    cout << "fast: " << time_fast(r, a, b) << endl;
    cout << "slow: " << time_slow(r, a, b) << endl;
}


#endif

#if 0

#include <string>
#include <iostream>
#include <memory>

using namespace std;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
};

// base class definition

struct base {
    template <typename T> base(T x) : self_(make_shared<model<T>>(move(x))) { }
    
    friend inline void foo(const base& x) { x.self_->foo_(); }
  private:
    struct concept {
        virtual void foo_() const = 0;
    };
    template <typename T> struct model : concept {
        model(T x) : data_(move(x)) { }
        void foo_() const override { foo(data_); }
        T data_;
    };
    shared_ptr<const concept> self_;
    annotate _;
};

// non-intrusive "inheritance"

class derived { };
void foo(const derived&) { cout << "foo of derived" << endl; }

struct non_random {
    int a = 0xAAAA;
    int b = 0xBBBB;
};

struct contain_annotate {
    contain_annotate() : m_(m_) { }
    int m_;
};

// demonstration

    void print(int x) { cout << x << endl; }

int main() {
    contain_annotate w;
    cout << "----" << endl;


    int a[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    
    auto x_ = new non_random;
    cout << "new non_random:" << hex << x_->a << endl;
    auto y_ = new non_random();
    cout << "new non_random():" << hex << y_->a << endl;
    cout << "--------------" << endl;
    
    for_each(&a[0], &a[0] + (sizeof(a) / sizeof(int)), &print);
    for_each(begin(a), end(a), [](int x) { cout << x << endl; });
    
    for (int *f = &a[0], *l = &a[0] + (sizeof(a) / sizeof(int)); f != l; ++f) cout << *f << endl;
    
    for (const auto& e : a) cout << e << endl;
    
    transform(begin(a), end(a), begin(a), [](int x) { return x + 1; });
    
    for (auto& e : a) e += 1;
    for (const auto& e : a) cout << e << endl;

    cout << endl << "-- ctor setup --" << endl << endl;
    base x = derived(); // ctor
    base y = derived();
    
    cout << endl << "-- test copy-ctor --" << endl << endl;
    {
    base z = x; // copy-ctor
    }
    cout << endl << "-- test move-ctor --" << endl << endl;
    {
    base z = move(x); // move-ctor
    }
    cout << endl << "-- test assign --" << endl << endl;
    {
    x = y; // assign
    }
    cout << endl << "-- test move-assign --" << endl << endl;
    {
    x = move(y); // assign
    }
    cout << endl << "-- dtor teardown --" << endl << endl;
}

#endif

#if 0

#include <functional>

namespace adobe {

/**************************************************************************************************/

namespace details {

template <typename F> // F models function
struct bind_direct_t {
    using type = F;
    
    static inline type bind(F f) { return f; }
};

template <typename R, typename T>
struct bind_direct_t<R T::*> {
    using type = decltype(std::bind(std::declval<R T::*&>(), std::placeholders::_1));
    
    static inline type bind(R T::* f) { return std::bind(f, std::placeholders::_1); }
};

template <typename R, typename T>
struct bind_direct_t<R (T::*)()> {
    using type = decltype(std::bind(std::declval<R (T::*&)()>(), std::placeholders::_1));
    
    static inline type bind(R (T::*f)()) { return std::bind(f, std::placeholders::_1); }
};

template <typename R, typename T, typename A>
struct bind_direct_t<R (T::*)(A)> {
    using type = decltype(std::bind(std::declval<R (T::*&)(A)>(), std::placeholders::_1,
        std::placeholders::_2));
    
    static inline type bind(R (T::*f)(A)) { return std::bind(f, std::placeholders::_1,
        std::placeholders::_2); }
};

template <typename R, typename T>
struct bind_direct_t<R (T::*)() const> {
    using type = decltype(std::bind(std::declval<R (T::*&)() const>(), std::placeholders::_1));
    
    static inline type bind(R (T::*f)() const) { return std::bind(f, std::placeholders::_1); }
};

template <typename R, typename T, typename A>
struct bind_direct_t<R (T::*)(A) const> {
    using type = decltype(std::bind(std::declval<R (T::*&)(A) const>(), std::placeholders::_1,
        std::placeholders::_2));
    
    static inline type bind(R (T::*f)(A) const) { return std::bind(f, std::placeholders::_1,
        std::placeholders::_2); }
};

} // namespace details;
  
/**************************************************************************************************/

template <typename F> // F models function
typename details::bind_direct_t<F>::type make_callable(F&& f) {
    return details::bind_direct_t<F>::bind(std::forward<F>(f));
}

} // namespace adobe

/**************************************************************************************************/

#include <iostream>

using namespace adobe;
using namespace std;

struct test {
    test() { } 
    int member_ = 3;
    const int cmember_ = 5;
    int member() { cout << "member()" << endl; return 7; }
    int member2(int x) { cout << "member(int)" << endl; return x; }
};

int main() {
    cout << adobe::make_callable(&test::member_)(test()) << endl;
    cout << adobe::make_callable(&test::cmember_)(test()) << endl;
    cout << adobe::make_callable(&test::member)(test()) << endl;
    cout << adobe::make_callable(&test::member2)(test(), 9) << endl;
    cout << adobe::make_callable([]{ cout << "lambda" << endl; return 11; } )() << endl;
}


#endif

/**************************************************************************************************/

#if 0
#include <algorithm>
#include <iostream>
#include <list>
#include <future>
#include <exception>

#include <unistd.h>

using namespace std;

/**************************************************************************************************/

#include <dispatch/dispatch.h>

namespace adobe {

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args) -> std::future<typename std::result_of<F (Args...)>::type> {
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;
    
    auto p = new packaged_type(std::bind(f, std::forward<Args>(args)...));
    auto r = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* p_) {
                auto p = static_cast<packaged_type*>(p_);
                (*p)();
                delete p;
            });
    
    return r;
}

} // namespace adobe

#if 0

template <typename F, typename ...Args>
struct share {
    using result_type = typename result_of<F (Args...)>::type;
    
    share(F&& f) : f_(move(f)) { }
    
    ~share() {
        cout << "dtor" << endl;
        try {
            promise_.set_value(future_.get());
        } catch(...) {
            promise_.set_exception(current_exception());
        }
    }
    
    result_type invoke(Args&&... args) const { return f_(forward<Args>(args)...); }
    
    promise<result_type> promise_;
    future<result_type> future_;
    F f_;
};

template <typename F, typename ...Args>
auto async_(F&& f, Args&&... args) -> future<typename result_of<F (Args...)>::type>
{
    
    auto shared = make_shared<share<F, Args...>>(move(f));
    shared->future_ = async(bind(&share<F, Args...>::invoke, shared));
    return shared->promise_.get_future();
}

#endif


template <typename T> // models regular
class event_task {
public:
    template <typename F> // models void(T)
    event_task(F receiver) : shared_(make_shared<shared>(std::move(receiver))) { }
    
    void send(T x) {
        shared& s = *shared_;
        list<T> l;
        l.push_back(move(x));
        
        bool flag;
        {
            lock_t lock(s.mutex_);
            s.queue_.splice(s.queue_.end(), l);
            flag = s.flag_; s.flag_ = true;
        }
        if (!flag) {
            wait(); // check for exceptions
            auto task = packaged_task<void()>(bind(&shared::call_receiver, shared_ptr<shared>(shared_)));
            future_ = task.get_future();
            thread(move(task)).detach();
        }
    }
    
    void wait() {
        if (future_.valid()) future_.get(); // check for exceptions
    }
    
private:
    typedef lock_guard<mutex> lock_t;
    
    struct shared {
        
        template <typename F> // models void(T)
        shared(F receiver)
            : receiver_(std::move(receiver)),
              flag_(false)
              { }
        
        ~shared() { cout << "shared dtor" << endl; }


        void call_receiver() {
            list<T> l;
            do {
                {
                lock_t lock(mutex_);
                swap(queue_, l);    // receive
                }
                for (auto&& e : l) receiver_(move(e));
            } while (!l.empty());
            lock_t lock(mutex_);
            flag_ = false;
        }

        std::function<void(T x)>    receiver_;
        std::list<T>                queue_;
        bool                        flag_;
        std::mutex                  mutex_;
    };
    
    future<void>            future_;
    std::shared_ptr<shared> shared_;
};

class opaque {
    class imp;
    imp* self;
  private:
    opaque() noexcept : self(nullptr) { }
    
    // provide other ctors
    
    ~opaque(); // provide dtor
    
    opaque(const opaque&); // provide copy
    opaque(opaque&& x) noexcept : self(x.self) { x.self = nullptr; }
    
    opaque& operator=(const opaque& x) { return *this = opaque(x); }
    opaque& operator=(opaque&& x) noexcept { swap(*this, x); return *this; }
    
    friend inline void swap(opaque& x, opaque& y) noexcept { std::swap(x.self, y.self); }
};

struct talk {
    talk() { cout << "default-ctor" << endl; }
    
    ~talk() { cout << "dtor" << endl; }
    
    talk(const talk&) { cout << "copy-ctor" << endl; }
    talk(talk&& x) noexcept { cout << "move-ctor" << endl; }
    
    talk& operator=(const talk& x) { return *this = talk(x); }
    talk& operator=(talk&& x) noexcept { cout << "move-assign" << endl; return *this; }
};

void sink(talk x) {
    static talk x_;
    x_ = move(x);
}

int main() {
#if 0
    auto f = async_([]{
        sleep(3);
        cout << "testing" << endl;
        return 42;
    });
    
    cout << "sync" << endl;
    
    f.get();
    
    cout << "done" << endl;
#endif

    {
    event_task<std::string> task([](string x){
        cout << x << endl;
        sleep(3);
        // throw "fail";
    });
    
    task.send("Hello");
    task.send("World");
    sleep(1);
    cout << "Doing Work" << endl;
    }
    
    cout << "Waiting" << endl;
    sleep(9);
    cout << "done" << endl;
    
    #if 0
    try {
        task.wait();
    } catch(const char* x) {
        cout << "exception: " << x << endl;
    }
    #endif


    #if 0
    auto&& x = talk();
    x = talk();
    
    cout << "holding x" << endl;
    sink(move(x));
    cout << "done" << endl;
    #endif

}

#endif
#if 0

#include <iostream>
#include <memory>
#include <utility>

using namespace std;

class some_class {
  public:
    some_class(int x) : remote_(new int(x)) { } // ctor
    
    some_class(const some_class& x) : remote_(new int(*x.remote_)) { cout << "copy" << endl; }
    some_class(some_class&& x) noexcept = default;
    
    some_class& operator=(const some_class& x) { *this = some_class(x); return *this; }
    some_class& operator=(some_class&& x) noexcept = default;
    
  private:
    unique_ptr<int> remote_;
};

struct some_struct {
    some_class member_;
};

class some_constructor {
 public:
    some_constructor(string x) : str_(move(x)) { }
    string str_;
};

string append_world(string x) { return x += "World"; }

template <typename T, size_t N>
struct for_each_t {
    for_each_t(T (&r)[N]) : rp_(&r) { }
    T (*rp_)[N];
};

template <typename T, size_t N>
for_each_t<T, N> for_each(T (&r)[N]) { return for_each_t<T, N>(r); }

template <typename T, size_t N, typename F>
void operator|(const for_each_t<T, N>& fe, F f)
{
    for (const T& e : *fe.rp_) f(e);
}

int main() {
    int a[] = { 0, 1, 2, 3 };
    
    for_each(a) | [](int x) { cout << x << endl; };
    
    
    some_class x(42);
    
    x = some_class(53);

#if 0
    string x = "Hello";
    string y = append_world(x);
    
    cout << x << endl;
#endif


    cout << "done" << endl;
}

#endif


#if 0
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include <cassert>

namespace adobe {

typedef std::function<void ()>* cancellation_token_registration;

namespace details {

struct cancel_state {
    typedef std::list<std::function<void ()>> list_type;

    cancel_state() : canceled_(false) { }

    void cancel() {
        list_type list;
        {
            std::lock_guard<std::mutex> lock(guard_);
            if (!canceled_) {
                canceled_ = true;
                swap(list, callback_list_);
            }
        }
        for (const auto& e: list) e();
    }
    
    template <typename F> // F models void ()
    cancellation_token_registration register_callback(F f) {
        list_type list(std::move(f));
        cancellation_token_registration result = nullptr;
        bool canceled;
        {
            std::lock_guard<std::mutex> lock(guard_);
            canceled = canceled_;
            if (!canceled) {
                callback_list_.splice(end(callback_list_), list);
                result = &callback_list_.back();
            }
        }
        if (canceled) list.back()();
        return result;
    }
    
    void deregister_callback(const cancellation_token_registration& token) {
        std::lock_guard<std::mutex> lock(guard_);
        auto i = find_if(begin(callback_list_), end(callback_list_),
                [token](const std::function<void ()>& x) { return &x == token; });
        if (i != end(callback_list_)) callback_list_.erase(i);
    }
    
    bool is_canceled() const { return canceled_; }

    std::mutex guard_;
    volatile bool canceled_; // Write once, read many
    list_type callback_list_;
};

struct cancellation_scope;

template <typename T> // placeholder to allow header only library
cancellation_scope*& get_cancellation_scope() {
    __thread static cancellation_scope* cancellation_stack = nullptr;
    return cancellation_stack;
}

} // namespace details

class cancellation_token_source;

class cancellation_token {
  public:
    template <typename F> // F models void ()
    cancellation_token_registration register_callback(F f) const {
        return state_ ? state_->register_callback(move(f)) : nullptr;
    }
    
    void deregister_callback(const cancellation_token_registration& token) const {
        if (state_) state_->deregister_callback(token);
    }
    
    bool is_cancelable() const { return static_cast<bool>(state_); }
    
    
    bool is_canceled() const { return state_ ? state_->is_canceled() : false; }
    
    static cancellation_token none() { return cancellation_token(); }
    
    friend inline bool operator==(const cancellation_token& x, const cancellation_token& y) {
        return x.state_ == y.state_;
    }
    
  private:
    friend class cancellation_token_source;
    cancellation_token() { }
    explicit cancellation_token(std::shared_ptr<details::cancel_state> s) : state_(std::move(s)) { }
    
    std::shared_ptr<details::cancel_state> state_;
};

inline bool operator!=(const cancellation_token& x, const cancellation_token& y) {
    return !(x == y);
}

namespace details {

struct cancellation_scope {
    cancellation_scope(cancellation_token token) : token_(std::move(token)) {
        cancellation_scope*& scope = get_cancellation_scope<void>();
        prior_ = scope;
        scope = this;
    }
    
    ~cancellation_scope() { get_cancellation_scope<void>() = prior_; }
    
    cancellation_scope* prior_;
    const cancellation_token& token_;
};

} // namespace details

class cancellation_token_source {
  public:
    cancellation_token_source() : state_(std::make_shared<details::cancel_state>()) { }
    
    void cancel() const { state_->cancel(); }
    
    cancellation_token get_token() const { return cancellation_token(state_); }
    
    friend inline bool operator==(const cancellation_token_source& x, const cancellation_token_source& y) {
        return x.state_ == y.state_;
    }
        
  private:
    std::shared_ptr<details::cancel_state> state_;
};

inline bool operator!=(const cancellation_token_source& x, const cancellation_token_source& y) {
    return !(x == y);
}

class task_canceled : public std::exception {
  public:
    const char* what() const noexcept { return "task_canceled"; }
};

inline bool is_task_cancellation_requested() {
    const details::cancellation_scope* scope = details::get_cancellation_scope<void>();
    return scope ? scope->token_.is_canceled() : false;
}

[[noreturn]] inline void cancel_current_task() {
    throw task_canceled();
}

template <typename F> // F models void ()
inline void run_with_cancellation_token(const F& f, cancellation_token token) {
    details::cancellation_scope scope(std::move(token));
    if (is_task_cancellation_requested()) cancel_current_task();
    f();
}

template <typename T>
class optional {
  public:
    optional() : initialized_(false) { }
    
    optional& operator=(T&& x) {
        if (initialized_)reinterpret_cast<T&>(storage_) = std::forward<T>(x);
        else new(&storage_) T(std::forward<T>(x));
        initialized_ = true;
        return *this;
    }
    
    T& get() {
        assert(initialized_ && "getting unset optional.");
        return reinterpret_cast<T&>(storage_);
    }
    
    explicit operator bool() const { return initialized_; }
    
    ~optional() { if (initialized_) reinterpret_cast<T&>(storage_).~T(); }

  private:
    optional(const optional&);
    optional operator=(const optional&);
  
    alignas(T) char storage_[sizeof(T)];
    bool initialized_;
};

template <typename F, typename SIG> class cancelable_function;

template <typename F, typename R, typename ...Arg>
class cancelable_function<F, R (Arg...)> {
  public:
    typedef R result_type;
  
    cancelable_function(F f, cancellation_token token) :
            function_(std::move(f)), token_(std::move(token)) { }
    
    R operator()(Arg&& ...arg) const {
        optional<R> r;
        
        run_with_cancellation_token([&]{
            r = function_(std::forward<Arg>(arg)...);
        }, token_);
        
        if (!r) cancel_current_task();
        return std::move(r.get());
    }
    
  private:
    F function_;
    cancellation_token token_;
};

#if 1
template <typename F, typename ...Arg>
class cancelable_function<F, void (Arg...)> {
  public:
    typedef void result_type;
  
  
    cancelable_function(F f, cancellation_token token) :
            function_(std::move(f)), token_(std::move(token)) { }
    
    void operator()(Arg&& ...arg) const {
        bool executed = false;
        
        run_with_cancellation_token([&]{
            executed = true;
            function_(std::forward<Arg>(arg)...);
        }, token_);
        
        if (!executed) cancel_current_task();
    }
    
  private:
    F function_;
    cancellation_token token_;
};
#endif

template <typename SIG, typename F> // F models void ()
cancelable_function<F, SIG> make_cancelable(F f, cancellation_token token) {
    return cancelable_function<F, SIG>(f, token);
}


} // namespace adobe

#include <iostream>
#include <unistd.h>
#include <future>
#include <string>

using namespace std;
using namespace adobe;

/**************************************************************************************************/

#include <dispatch/dispatch.h>

namespace adobe {

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args) -> std::future<typename std::result_of<F (Args...)>::type> {
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;
    
    auto p = new packaged_type(std::bind(f, std::forward<Args>(args)...));
    auto r = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* p_) {
                auto p = static_cast<packaged_type*>(p_);
                (*p)();
                delete p;
            });
    
    return r;
}

} // namespace adobe

/**************************************************************************************************/

template <typename R>
struct for_each_t {
    for_each_t(const R& r) : rp_(&r);
    const R* rp_;
};

template <typename R, // R models range
          typename F> // F models void (value_type(R))
for_each_t<R> for_each(const R& r) { }

template <typename F>
void operator|(const for_each_t& fe, F f)
{
    for (const auto& e : *fe.rp_) f;
}


int main() {
    int a[] = { 0, 1, 2, 3 };
    
    for_each(a) | [](int x) { cout << x << endl; }


    pair<int, int> x;

    cancellation_token_source cts;
    
    cout << "Creating task..." << endl;
    
    auto f = make_cancelable<int (const string& message)>([](const string& message){
        for (int count = 0; count != 10; ++count) {
            if (is_task_cancellation_requested()) cancel_current_task();
            
            cout << "Performing work:" << message << endl;
            usleep(250);
        }
        return 42;
    }, cts.get_token());
    
    
    // Create a task that performs work until it is canceled.
    auto t = adobe::async(f, "Boogie down!");
    
    cout << "Doing something else..." << endl;
    
    adobe::async(f, "Running Free!");
    
    cout << "Got:" << t.get() << endl;
    
    cout << "Completed Successfully..." << endl;

    cout << "Creating another task..." << endl;
    t = adobe::async(f, "Boogie up!");
    
    cout << "Doing something else..." << endl;
    usleep(1000);

    cout << "Canceling task..." << endl;
    cts.cancel();
    
    try {
        t.get();
    } catch (const task_canceled&) {
        cout << "Cancel exception propogated." << endl;
    }

    cout << "Done." << endl;
}

#endif


#if 0
//
#include <vector>
#include <iostream>
using namespace std;

vector<int> func() {
	vector<int> a = { 0, 1, 2, 3 };
	//...
	return a;
}

struct type {
	explicit type(vector<int> x) : member_(move(x)) { }
	vector<int> member_;
};

int main() {
	vector<int> b = func();
	vector<int> c;
	c = func();
	
	type d(func());
	type e(vector<int>({ 4, 5, 6 }));
}
#endif

#if 0
#include <cstddef>

template <typename I, // I models InputIterator
          typename N> // N models Incrementable
N count_unique(I f, I l, N n) {
    if (f == l) return n;
    I p = f; ++f; ++n;
    
    while (f != l) {
        if (*f != *p) { p = f; ++n; }
        ++f;
    }
    return n;
}

template <typename I> // I models InputIterator
std::size_t count_unique(I f, I l) {
    return count_unique(f, l, std::size_t());
}

#include <iostream>

using namespace std;

int main() {
    int a[] = { 1, 2, 1, 3, 1, 4, 1, 5 };
    sort(begin(a), end(a));
    cout << count_unique(begin(a), end(a)) << endl;
}
#endif 



#if 0

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <iostream>
#include <algorithm>
#include <string>

#include <adobe/lua.hpp>


using namespace std;
using namespace adobe;

class type {
  public:
    type(string x) : member_(move(x)) { }
    const string& what() const { return member_; }

  private:
    string member_;
};

any user(lua_State* L, int index) {
    return *static_cast<const int*>(lua_touserdata(L, index));
}

extern "C" {

int f_(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        auto s = adobe::to_any(L, i, &user);
        cout << "<cpp>" << endl;
        if (s.type() == typeid(type)) {
            cout << '\t' << adobe::dynamic_cast_<const type&>(s).what() << endl;
        } else if (s.type() == typeid(int)) {
            cout << '\t' << adobe::dynamic_cast_<const int&>(s) << endl;
        }
        cout << "</cpp>" << endl;
    #if 0
        auto s = adobe::to<type>(L, i);
        cout << "<cpp>" << endl;
        cout << '\t' << s.what() << endl;
        cout << "</cpp>" << endl;
    #endif
    }
    return 0;
}

} // extern "C"

int main() {
    auto L = luaL_newstate();
    luaL_openlibs(L);
    
    luaL_dostring(L,
(R"lua(
                  
function luaFunction(arg1, f)
    print('<lua>')
    print('\t' .. type(arg1))
    print('</lua>/')
    f(arg1)
end

)lua")
    );
    
    lua_getglobal(L, "luaFunction");
    adobe::push(L, type("Testing"));
    lua_pushcfunction(L, f_);
    lua_pcall(L, 2, 0, 0);
    
    lua_getglobal(L, "luaFunction");
    new (lua_newuserdata(L, sizeof(int))) int(42);
    lua_pushcfunction(L, f_);
    lua_pcall(L, 2, 0, 0);
    
    lua_close(L);
    cout.flush();
}

#endif


