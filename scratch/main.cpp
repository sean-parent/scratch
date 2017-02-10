#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

/*
Simulate this data call

    String^ input = TextInput->Text;

    ... input->Data() ...

*/
wstring Data() { return L"Hello World!"; }


 
wstring /* TestFuncs:: */ReverseString(wstring input)
{
    reverse(begin(input), end(input));
    return input;
}

int main() {

    /*
        To avoid a copy, we pass the temproary directly to ReverString()
        
        You could also call as:
        {
        auto tmp = Data();
        auto str = ReverseString(move(tmp)); // Last use of tmp
        }
    */

    auto str = /* SharedLib::TestFuncs:: */ ReverseString(Data());

    wcout << str << endl;
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
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

annotate f() {
    annotate x;
    return x;
}

int main() {

    auto x = f();
}
#endif

#if 0


#include <future>
#include <memory>

#include <dispatch/dispatch.h>

namespace stlab {

template <class Sig, class F>
auto cancelable_task(F&& f) {
    using shared_t = std::packaged_task<Sig>;

    auto p = std::make_shared<shared_t>(std::forward<F>(f));
    auto r = p->get_future();

    return std::make_pair([_p = std::weak_ptr<shared_t>(p)] (auto&&... args) {
        auto p = _p.lock();
        if (!p) return;
        (*p)(std::forward<decltype(args)>(args)...);
    },
    std::async(std::launch::deferred, [_p = std::move(p), _r = std::move(r)] () mutable {
        return _r.get();
    }));
}

template <class T>
    struct shared_type {
        T _task;
        std::atomic_flag _flag = ATOMIC_FLAG_INIT;

        template <class U>
        explicit shared_type(U&& x) : _task(std::forward<U>(x)) { }
    };

template <class F, class... Args>
auto promotable_task(F&& f, Args&&... args) {
    using result_t = std::result_of_t<std::decay_t<F>(std::decay_t<Args>...)>;
    using packaged_t = std::packaged_task<result_t()>;

    using shared_t = shared_type<packaged_t>;


#if 0
    struct shared_t {
        packaged_t _task;
        std::atomic_flag _flag = ATOMIC_FLAG_INIT;

        explicit shared_t(packaged_t&& x) : _task(std::move(x)) { }
    };
#endif

    auto p = std::make_shared<shared_t>(std::bind([_f = std::forward<F>(f)](Args&... args) {
        return _f(std::move(args)...);
    }, std::forward<Args>(args)...));

    auto r = p->_task.get_future();

    return std::make_pair([_p = std::weak_ptr<shared_t>(p)] () {
        auto p = _p.lock();
        if (!p || p->_flag.test_and_set()) return;
        p->_task();
    },
    std::async(std::launch::deferred, [_p = std::move(p), _r = std::move(r)] () mutable {
        if (!_p->_flag.test_and_set()) _p->_task();
        return _r.get();
    }));
}

template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    auto task_future = promotable_task(std::forward<Function>(f), std::forward<Args>(args)...);

    using task_t = decltype(task_future.first);

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new task_t(std::move(task_future.first)), [](void* _p){
            auto p = static_cast<task_t*>(_p);
            (*p)();
            delete p;
        });

    return std::move(task_future.second);
}


} // namespace stlab

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

int main() {
    {
    cout << "test 1" << endl;
    auto task_future = stlab::cancelable_task<unique_ptr<int>(unique_ptr<int>)>([](unique_ptr<int> x) {
        ++(*x);
        cout << "executed: " << *x << endl;
        return x;
    });

    task_future.first(make_unique<int>(3));
    cout << *task_future.second.get() << endl;
    }

    {
    cout << "test 2" << endl;
    auto task_future = stlab::cancelable_task<unique_ptr<int>(unique_ptr<int>)>([](unique_ptr<int> x) {
        ++(*x);
        cout << "executed: " << *x << endl;
        return x;
    });

    cout << "canceling" << endl;

    task_future.second = future<unique_ptr<int>>();

    task_future.first(make_unique<int>(3));
    }

    
    {
    cout << "test 3" << endl;
    auto task_future = stlab::promotable_task([](unique_ptr<int> x) {
        ++(*x);
        cout << "executed: " << *x << endl;
        return x;
    }, make_unique<int>(3));

    task_future.first();

    cout << *task_future.second.get() << endl;
    }

    {
    cout << "test 4" << endl;
    auto task_future = stlab::promotable_task([](unique_ptr<int> x) {
        ++(*x);
        cout << "executed: " << *x << endl;
        return x;
    }, make_unique<int>(3));

    cout << "promoting" << endl;

    cout << *task_future.second.get() << endl;
    }

    {
    cout << "test 5" << endl;
    auto task_future = stlab::promotable_task([](unique_ptr<int> x) {
        ++(*x);
        cout << "executed: " << *x << endl;
        return x;
    }, make_unique<int>(3));

    cout << "canceling" << endl;
    task_future.second = future<unique_ptr<int>>();
    task_future.first();
    }

    {
    auto x = stlab::async([](int x){ return x; }, 42);
    cout << x.get() << endl;
    }

    {
    auto x = stlab::async([](annotate x){ return x; }, annotate());
    x.get();
    }

}

#endif

#if 0

#include <algorithm>
#include <iterator>

namespace stlab {

/*
    NOTE (sparent) : This routine requires forward ranges and is non-optimial unless
    called with random access ranges.
*/

template <class R1, class R2>
auto copy_bounded(const R1& src, R2& dst) {
    auto size = std::min(std::distance(std::begin(src), std::end(src)),
                         std::distance(std::begin(dst), std::end(dst)));

    return std::make_pair(std::begin(src) + size,
                          std::copy_n(std::begin(src), size, std::begin(dst)));
}

} // namespace stlab

#include <iostream>

int main() {
    char test[] = "this is a test";
    char output1[256];
    stlab::copy_bounded(test, output1);

    std::cout << output1 << std::endl;
}

#endif



#if 0

#include <functional>
#include <future>
#include <memory>
#include <type_traits>

#include <boost/thread/future.hpp>

#include <dispatch/dispatch.h>

namespace stlab {

#if 0
template <class> class cancelable_task;

template <class R, class ...Args>
class cancelable_task<R(Args...)> {

    struct shared_t {
        std::packaged_task<R(Args...)>   _f;
        std::future<R>                  _result;
        std::atomic_flag                _start = ATOMIC_FLAG_INIT;

        explicit shared_t(packaged_type&& f) : _f(std::move(f)), _result(_f.get_future()) { }

        void run(Args... args) {
            if (!_start.test_and_set()) _f(std::forward<Args>(args)...);
        }
    };

    std::weak_ptr<shared_t> _shared;
    std::future<R> _future;

public:
    using result_type = R;

    // construction and destruction
    cancelable_task() noexcept = default;
    template <class F>
        explicit cancelable_task(F&& f);
    ~cancelable_task() = default;

    // no copy
    cancelable_task(const cancelable_task&) = delete;
    cancelable_task& operator=(const cancelable_task&) = delete;

    // move support
    cancelable_task(cancelable_task&&) noexcept = default;
    cancelable_task& operator=( cancelable_task&&) noexcept = default;

    void swap(cancelable_task& other) noexcept { std::swap(_shared, other._shared); }

    bool valid() const noexcept { return _shared; }

    // result retrieval
    std::future<R> get_future() {
        return std::move(_future);
    }

    // execution
    void operator()(Args...);

    void reset();
};

template< class Function, class... Args >
void swap( cancelable_task<Function(Args...)> &lhs,
           cancelable_task<Function(Args...)> &rhs ) noexcept {
    return lhs.swap(rhs);
}

#endif

namespace detail {

template <typename>
struct result_of_;

template <typename R, typename... Args>
struct result_of_<R(Args...)> { using type = R; };

template <typename F>
using result_of_t_ = typename result_of_<F>::type;


template <typename Sig>
struct shared {
    using packaged_t = std::packaged_task<Sig>;

    packaged_t _f;
    std::future<result_of_t_<Sig>> _result;
    std::atomic_flag _start = ATOMIC_FLAG_INIT;
};

} // namespace detail

template <class> struct packaged_task;

template <class R, class... Args>
class packaged_task<R(Args...)> {
    std::weak_ptr<detail::shared<R(Args...)>> _p;

  public:
    void operator()(Args... args) {
        auto p = _p.lock();
        if (p) (*p)(std::move(args)...);
    }
};


template <class Sig, class S, class F>
auto cancelable_task(F f) {
    auto p = std::make_shared<detail::shared<Sig>>(std::move(f));
    return std::make_pair(packaged_task<Sig>(p), std::future<detail::result_of_t_<Sig>>(p));
}

#if 0

template <class Function, class... Args>
auto make_cancelable_task(Function&& f, Args&&... args) {
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;

    struct shared_t {
        packaged_type               _f;
        std::future<result_type>    _result;
        std::atomic_flag            _start = ATOMIC_FLAG_INIT;

        explicit shared_t(packaged_type&& f) : _f(std::move(f)), _result(_f.get_future()) { }

        void run() {
            if (!_start.test_and_set()) _f();
        }
    };

    auto shared = std::make_shared<shared_t>(packaged_type(std::bind([](Function& f, Args&... args) {
        return f(std::move(args)...);
    }, std::forward<Function>(f), std::forward<Args>(args)...)));

    return std::make_pair([_p = std::weak_ptr<shared_t>(shared)]{
        auto p = _p.lock();
        if (p) p->run();
    }, std::async(std::launch::deferred, [_p = shared] {
        _p->run();
        return _p->_result.get();
    }));
}

#endif

template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    auto task_future = cancelable_task(std::forward<Function>(f), std::forward<Args>(args)...);

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new decltype(task_future.first)(std::move(task_future.first)), [](void* _p){
            auto p = static_cast<decltype(task_future.first)*>(_p);
            (*p)();
            delete p;
        });

    return std::move(task_future.second);
}

#if 0
template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;

    struct shared_t {
        packaged_type               _f;
        std::future<result_type>    _result;
        std::atomic_flag            _start = ATOMIC_FLAG_INIT;

        explicit shared_t(packaged_type f) : _f(std::move(f)), _result(_f.get_future()) { }

        void run() { if (!_start.test_and_set()) _f(); }
    };

    auto shared = std::make_shared<shared_t>(packaged_type(std::bind([_f = std::forward<Function>(f)](Args&... args) {
        return _f(std::move(args)...);
    }, std::forward<Args>(args)...)));

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new std::weak_ptr<shared_t>(shared), [](void* _p){
            auto p = static_cast<std::weak_ptr<shared_t>*>(_p);
            auto lock = p->lock();
            if (lock) lock->run();
            delete p;
        });

    return std::async(std::launch::deferred, [_shared = shared] {
        _shared->run(); return _shared->_result.get();
    });
}
#endif

} // namespace stlab

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

int main() {
    {
    auto x = stlab::async([_a = annotate()]{
        cout << "executed" << endl;
    });

    x.get();
    cout << "about to dtor" << endl;
    }

    cout << "cancel" << endl;

    stlab::async([_a = annotate()]{
        cout << "executed" << endl;
    });

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
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

int main() {
    auto f = std::async(std::launch::deferred, [_a = annotate()]{
        cout << "invoked" << endl;
    });

    f.get();
}
#endif

#if 0

#include <functional>
#include <future>
#include <type_traits>

#include <dispatch/dispatch.h>

namespace stlab {

template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;
    
    auto _p = new packaged_type(std::bind([_f = std::forward<Function>(f)](Args&... args) {
        return _f(std::move(args)...);
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

} // namespace stlab

int main() {
    mutex _mutex;
    condition_variable _condition;
    bool _done = false;

    auto _future = async(std::launch::async, [&]{
        {
        unique_lock<mutex> _lock{_mutex};
        _done = true;
        }
        _condition.notify_one();
    });

    {
    unique_lock<mutex> _lock{_mutex};
    while (!_done) _condition.wait(_lock);
    }

    _future.wait();
}
#endif

#if 0

#include <iostream>

using namespace std;

#if 0
// If C++ had tail recursion

int helper(int n, int result) {
    return n <= 1 ? result : helper(n - 1, n * result);
}

int factorial(int n) {
    return helper(n, 1);
}

int main() {

    cout << factorial(10) << endl;
}
#endif

int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

#endif


#if 0

#include <deque>
#include <memory>
#include <mutex>
#include <future>
#include <boost/optional.hpp>
#include <boost/thread/future.hpp>

namespace stlab {

class process {
    struct concept {
        virtual ~concept() = default;
    };

    template <typename T>
    struct model : concept {
        template <typename... Args>
        model(Args&&... args) : _self(std::forward<Args>(args)...) { }
        T _self;
    };

    std::shared_ptr<concept> _p;

    template <class T, class... Args>
    friend  process make_process(Args&&... args);

    process(std::shared_ptr<concept> p) : _p(std::move(p)) { }
  public:
    process() = default;
};

namespace detail {

template <class T>
struct shared_sender {
    virtual boost::future<void> send(T x) = 0;
    virtual void close() = 0;
    virtual void set_process(process) = 0;
};

template <class T>
struct shared_receiver {
    virtual boost::future<boost::optional<T>> receive() = 0;
};

template <class T>
struct shared_channel final : shared_sender<T>, shared_receiver<T> {
    process _process;

    std::mutex _mutex;
    std::deque<T> _q;
    bool _closed = false;
    std::size_t _buffer_size = 1;
    boost::optional<boost::promise<boost::optional<T>>> _receive_promise;
    boost::optional<boost::promise<void>> _send_promise;

    /*
        REVISIT : No flow control. Send should return a future<void> which is auto resolved
        if there is space in the queue, otherwise it isn't resolved until there is space.
    */

    boost::future<void> send(T x) override {
        std::lock_guard<std::mutex> lock(_mutex);
        // REVISIT : set_value() under the lock might deadlock if immediate continuation?
        if (_q.empty() && _receive_promise) {
            _receive_promise->set_value(std::move(x));
            _receive_promise.reset();
            return boost::make_ready_future();
        }
        _q.emplace_back(std::move(x));
        if (_q.size() == _buffer_size) {
            _send_promise = boost::promise<void>();
            return _send_promise.get().get_future();
        }
        return boost::make_ready_future();
    }

    void close() override {
        std::lock_guard<std::mutex> lock(_mutex);
        // REVISIT : set_value() under the lock might deadlock if immediate continuation?
        if (_q.empty() && _receive_promise) _receive_promise.get().set_value(boost::optional<T>());
        else _closed = true;
    }

    void set_process(process p) override {
        // Should not need to lock, can only be set once and controls lifetime of process bound
        // to this sender
        _process = std::move(p);
    }

    boost::future<boost::optional<T>> receive() override {
        {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_q.empty()) {
            auto result = boost::make_ready_future<boost::optional<T>>(std::move(_q.front()));
            _q.pop_front();
            if (_send_promise) {
                _send_promise->set_value();
                _send_promise.reset();
            }
            return result;
        }
        if (_closed) {
            return boost::make_ready_future<boost::optional<T>>();
        }
        _receive_promise = boost::promise<boost::optional<T>>();
        return _receive_promise.get().get_future();
        }
    }
};

} // namespace detail

template <class T> class receiver;

template <class T>
class sender {
    std::weak_ptr<detail::shared_sender<T>> _p;

    template <class U>
    friend std::pair<sender<U>, receiver<U>> channel();

    sender(std::weak_ptr<detail::shared_sender<T>> p) : _p(std::move(p)) { }

  public:
    sender() = default;

    boost::future<void> operator()(T x) const {
        auto p = _p.lock();
        if (!p) return boost::make_ready_future();
        return p->send(std::move(x));
    }
    void close() const {
        auto p = _p.lock();
        if (!p) return;
        p->close();
    }

    /*
        REVISIT : the process being set must hold the sender (this completes a strong/weak cycle).
        Is there a better way?
    */
    void set_process(process x) const {
        auto p = _p.lock();
        if (!p) return;
        p->set_process(std::move(x));
    }
};

template <class T>
class receiver {
    std::shared_ptr<detail::shared_receiver<T>> _p;

    template <class U>
    friend std::pair<sender<U>, receiver<U>> channel();

    receiver(std::shared_ptr<detail::shared_receiver<T>> p) : _p(std::move(p)) { }
  public:
    receiver() = default;
    boost::future<boost::optional<T>> operator()() const {
        return _p->receive();
    }
};

template <class T>
std::pair<sender<T>, receiver<T>> channel() {
    auto p = std::make_shared<detail::shared_channel<T>>();
    return std::make_pair<sender<T>, receiver<T>>(sender<T>(p), receiver<T>(p));
}

template <class T, class... Args>
process make_process(Args&&... args) {
    return process(std::make_shared<process::model<T>>(std::forward<Args>(args)...));
}


} // namespace stlab

struct mul2 {
    stlab::receiver<int> _receive;
    stlab::sender<int> _send;

    mul2(stlab::receiver<int> receive, stlab::sender<int> send) :
        _receive(std::move(receive)), _send(std::move(send)) {

        run();
    }

    void run() {
        _receive().then([this](auto x){
            auto opt = x.get();
            if (!opt) _send.close();
            _receive().then([this, _x = *opt](auto y){
                auto opt = y.get();
                if (!opt) _send(_x).then([this](auto){ _send.close(); });
                else _send(_x * *opt).then([this](auto){ run(); });
            });
        });
    }
};

struct iota {
    stlab::sender<int> _send;

    int _min;
    int _max;

    iota(stlab::sender<int> send, int min, int max) :
        _send(std::move(send)), _min(min), _max(max) {

        run();
    }

    void run() {
        if (_min == _max) {
            _send.close();
            return;
        }
        _send(_min).then([this](auto){
            ++_min;
            run();
        });
    }
};

struct sum {
    stlab::receiver<int> _receive;
    stlab::sender<int> _send;
    int _result = 0;

    sum(stlab::receiver<int> receive, stlab::sender<int> send) :
        _receive(std::move(receive)), _send(std::move(send)) {

        run();

    }

    void run() {
        _receive().then([this](auto x){
            auto opt = x.get();
            if (!opt) {
                _send(_result).then([this](auto){
                    _send.close();
                });
                return;
            }
            _result += *opt;
            run(); // continue
        });
    }
};

template <class F, class T> struct map;
template <class F, class R, class A>
struct map<F, R(A)> {
    stlab::receiver<A> _receive;
    stlab::sender<R> _send;
    F _f;

    map(stlab::receiver<A> receive, stlab::sender<R> send, F f) :
        _receive(std::move(receive)), _send(std::move(send)), _f(std::move(f))
    {
        run();
    }

    void run() {
        _receive().then([this](auto x){
            auto opt = x.get();
            if (!opt) _send.close();
            else _send(_f(*opt)).then([this](auto){ run(); });
        });
    }
};

template <class T, class F, class R, class S>
auto make_map(R&& r, S&& s, F&& f) {
    return stlab::make_process<map<std::decay_t<F>, T>>(std::forward<R>(r), std::forward<S>(s), std::forward<F>(f));
}

int main() {
    stlab::sender<int> send1;
    stlab::receiver<int> receive1;

    std::tie(send1, receive1) = stlab::channel<int>();

    send1.set_process(stlab::make_process<iota>(send1, 0, 10));

    
    stlab::sender<int> send2;
    stlab::receiver<int> receive2;

    std::tie(send2, receive2) = stlab::channel<int>();

    send2.set_process(make_map<int(int)>(receive1, send2, [](int x){ return x * 10; }));

    stlab::sender<int> send3;
    stlab::receiver<int> receive3;

    std::tie(send3, receive3) = stlab::channel<int>();

    send3.set_process(stlab::make_process<sum>(receive2, send3));

    for (auto value = receive3().get(); value; value = receive3().get()) {
        std::cout << *value << std::endl;
    }
}

#endif

#if 0
#include <tuple>
#include <iostream>
#include <cmath>

#include <stlab/future.hpp>
#include <stlab/channel.hpp>

using namespace stlab;
using namespace std;
using namespace std::chrono;

using parameters = int;
using frame = double;


frame render_frame(parameters params, bool quality) {
    return quality ? sqrt(params) : round(sqrt(params));
}

struct render {
    process_state_scheduled _state = await_forever;

    bool _final = false;

    parameters _params;

    void await(parameters params) {
        _final = false;
        _state = await_immediate;
        _params = params;
    }

    frame yield() {
        auto result = render_frame(_params, _final);
        _final = !_final;
        _state = _final ?  await_immediate : await_forever;
        return result;
    }

    void close() { if (_state == await_immediate) _state = yield_immediate; }

    const auto& state() const {
        return _state;
    }
};

int main() {

    sender<int> send;
    receiver<int> receive;

    tie(send, receive) = channel<int>(default_scheduler());

    send(3);
    send(5);
    send(6);

    auto hold = receive
            | (buffer_size(10) & render())
            | [](double x){ cout << x << '\n'; };

    receive.set_ready();

#if 0
    send(1);
    send(2);
    send(3);
#endif
    this_thread::sleep_for(1s);
    send(7);
    send(8);
    this_thread::sleep_for(1s);
    send(10);
    send.close();

    sleep(20);
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
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

[[nodiscard]] int f() { return 5; }

int main() {
    cout << __VERSION__ << endl;
    cout << __clang__ << endl;
    cout << __clang_major__ << endl;
    cout << __clang_minor__ << endl;
    cout << __apple_build_version__ << endl;

    f();


    for(annotate (); true; ) {
        cout << "Here" << endl;
        break;
     }

}

#endif
#if 0

#include <tuple>
#include <iostream>
#include <cmath>

#include <stlab/future.hpp>
#include <stlab/channel.hpp>

using namespace stlab;
using namespace std;
using namespace std::chrono;

using parameters = int;
using frame = double;

enum quality {
    draft,
    final
};

frame render_frame(parameters, bool);

struct render {
    process_state_scheduled _state = await_forever;

    bool _final = false;

    parameters _params;

    void await(parameters params) {
        _final = false;
        _state = await_immediate;
        _params = params;
    }

    frame yield() {
        auto result = render_frame(_params, _final);
        _final = !_final;
        _state = _final ?  await_immediate : await_forever;
        return result;
    }

    void close() { if (_state == await_immediate) _state = yield_immediate; }

    const auto& state() const {
        return _state;
    }
};

int main() {

    sender<int> send;
    receiver<int> receive;

    tie(send, receive) = channel<int>(default_scheduler());

    send(1);
    send(2);
    send(3);

    auto hold = receive
        | (buffer_size(10) & render())
        | [](double x){ cout << x << '\n'; };

    receive.set_ready();

#if 0
    send(1);
    send(2);
    send(3);
#endif
    this_thread::sleep_for(1s);
    send(4);
    send(5);
    this_thread::sleep_for(1s);
    send(6);
    send.close();

    sleep(20);
}

#endif


#if 0

#include <functional>
#include <future>
#include <type_traits>

#include <dispatch/dispatch.h>

namespace stlab {

template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;
    
    auto _p = new packaged_type(std::bind([_f = std::forward<Function>(f)](Args&... args) {
        return _f(std::move(args)...);
    }, std::forward<Args>(args)...));
    
    auto result = _p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            _p, [](void* p) {
                auto _p = static_cast<packaged_type*>(p);
                (*_p)();
                delete _p;
            });
    
    return result;
}

} // namespace stlab

#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>
#include <forward_list>
#include <memory>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

template <class I, class N>
I reverse_n_forward(I f, N n) {
    if (n < 2) return next(f, n);

    auto h = n / 2;

    I m = reverse_n_forward(f, h);
    I m2 = next(m, n % 2);
    I l = reverse_n_forward(m2, h);
    swap_ranges(f, m, m2);
    return l;
}

template <class I>
void reverse_forward(I f, I l) {
    reverse_n_forward(f, distance(f, l));
}

template <class I>
void reverse_(I f, I l) {
    auto n = distance(f, l);
    if (n < 2) return;

    auto m1 = next(f, n / 2);
    auto m2 = next(m1, n % 2);
    reverse(f, m1);
    reverse(m2, l);
    swap_ranges(f, m1, m2);
}

template <class ForwardIterator, class N>
auto reverse_n(ForwardIterator f, N n) {
    if (n < 2) return next(f, n);

    auto h = n / 2;
    auto m1 = reverse_n(f, h);
    auto m2 = next(m1, n % 2);
    auto l = reverse_n(m2, h);
    swap_ranges(f, m1, m2);
    return l;
}

template <class ForwardIterator>
void reverse(ForwardIterator f, ForwardIterator l) {
    reverse_n(f, distance(f, l));
}

int main() {
    std::forward_list<int> x = { 0, 1, 2, 3, 4, 5, 6, 7 };
    ::reverse(begin(x), end(x));
    for (const auto& e : x) cout << e << endl;

    auto p = stlab::async([](int x){ cout << "begin:" << x << endl; this_thread::sleep_for(3s); cout << "end" << endl; }, 42);
    this_thread::sleep_for(1s);
    cout << "test" << endl;
    p.get();
    
    {
    auto p = stlab::async([](unique_ptr<int> x){ cout << "move_only:" << *x << endl; },
        make_unique<int>(128));
    p.wait();
    }
}



#endif


#if 0

#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <iterator>

#include <dispatch/dispatch.h>

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


class serial_queue {
    dispatch_queue_t _q = dispatch_queue_create("com.stlab.serial_queue", NULL);
    
  public:
    serial_queue() = default;
    serial_queue(const char* name) : _q{ dispatch_queue_create(name, NULL) } { }
    ~serial_queue() { dispatch_release(_q); }
    
    template <class Function, class... Args>
    auto async(Function&& f, Args&&... args )
    {
        using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
        using packaged_type = std::packaged_task<result_type()>;
        
        // Forward arguments through bind.
        
        auto p = new packaged_type(std::bind([_f = std::forward<Function>(f)](Args&... args) {
            return _f(std::move(args)...);
        }, std::forward<Args>(args)...));
        
        auto result = p->get_future();

        dispatch_async_f(_q,
                p, [](void* f_) {
                    packaged_type* f = static_cast<packaged_type*>(f_);
                    (*f)();
                    delete f;
                });
        
        return result;
    }
};

#if 0
class registry {
    mutex _mutex;
    unordered_map<string, string> _map;
  public:
    void set(string key, string value) {
        unique_lock<mutex> lock(mutex);
        _map.emplace(move(key), move(value));
    }
    
    auto get(const string& key) -> string {
        unique_lock<mutex> lock(mutex);
        return _map.at(key);
    }
};

#else


class registry {
    serial_queue _q;
    
    using map_t = unordered_map<string, string>;
    
    shared_ptr<map_t> _map = make_shared<map_t>();
public:
    void set(string key, string value) {
        _q.async([_map = _map](string key, string value) {
            _map->emplace(move(key), move(value));
        }, move(key), move(value)) /* .detatch() */;
    }
    
    auto get(string key) -> future<string> {
        return _q.async([_map = _map](string key) {
            return _map->at(key);
        }, move(key));
    }

    void set(vector<pair<string, string>> sequence) {
        _q.async([_map = _map](vector<pair<string, string>> sequence) {
            _map->insert(make_move_iterator(begin(sequence)), make_move_iterator(end(sequence)));
        }, move(sequence));
    }
};

template <typename T>
class bad_cow {
    struct object_t {
        explicit object_t(const T& x) : data_m(x) {}
        atomic<int> count_m{1};
        T           data_m; };
    object_t* object_m;
 public:
    explicit bad_cow(const T& x) : object_m(new object_t(x)) { }
    ~bad_cow() { if (0 == --object_m->count_m) delete object_m; }
    bad_cow(const bad_cow& x) : object_m(x.object_m) { ++object_m->count_m; }

    bad_cow& operator=(const T& x) {
        if (object_m->count_m == 1) object_m->data_m = x;
        else {
            object_t* tmp = new object_t(x);
            if (0 == --object_m->count_m) delete object_m;
            object_m = tmp;
        }
        return *this;
    }
};

#endif

int main() {

    std::future<string> result;
    {
    registry r;
    
    r.set("Hello", "world");
    result = r.get("Hello");
    }
    
    try {
        cout << result.get() << endl;
    } catch (std::exception& error) {
        cout << "error: " << error.what() << endl;
    }
}

#endif

#if 0
#include <iostream>

#include <boost/parameter/name.hpp>
#include <boost/parameter/preprocessor.hpp>

namespace stlab {

BOOST_PARAMETER_NAME(name)    // Note: no semicolon
BOOST_PARAMETER_NAME(bind)
BOOST_PARAMETER_NAME(action)

BOOST_PARAMETER_FUNCTION(
  (void),                // 1. parenthesized return type
  button,                // 2. name of the function template

  tag,                   // 3. namespace of tag types

  (optional              //    four optional parameters, with defaults
    (name,           (const char*), "")
    (bind,           *, nullptr)
    (action,         *, [](const char*){ })
  )
)
{
    action(name);
}

} // namespace stlab

using namespace stlab;

int main() {
    button(_name="Hello", _action=[](const char* name){ std::cout << "name=" << name << std::endl; });
}

#endif


#if 0

#include <functional>
#include <future>
#include <type_traits>

#include <dispatch/dispatch.h>

namespace stlab {

template <class Function, class... Args>
auto async(Function&& f, Args&&... args )
{
    using result_type = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
    using packaged_type = std::packaged_task<result_type()>;
    
    auto p = new packaged_type(std::bind(std::forward<Function>(f), std::forward<Args>(args)...));
    auto result = p->get_future();

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            p, [](void* f_) {
                packaged_type* f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

} // namespace stlab

#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>
#include <forward_list>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

template <class I, class N>
I reverse_n_forward(I f, N n) {
    if (n < 2) return next(f, n);

    auto h = n / 2;

    I m = reverse_n_forward(f, h);
    I m2 = next(m, n % 2);
    I l = reverse_n_forward(m2, h);
    swap_ranges(f, m, m2);
    return l;
}

template <class I>
void reverse_forward(I f, I l) {
    reverse_n_forward(f, distance(f, l));
}

template <class I>
void reverse_(I f, I l) {
    auto n = distance(f, l);
    if (n < 2) return;

    auto m1 = next(f, n / 2);
    auto m2 = next(m1, n % 2);
    reverse(f, m1);
    reverse(m2, l);
    swap_ranges(f, m1, m2);
}

template <class ForwardIterator, class N>
auto reverse_n(ForwardIterator f, N n) {
    if (n < 2) return next(f, n);

    auto h = n / 2;
    auto m1 = reverse_n(f, h);
    auto m2 = next(m1, n % 2);
    auto l = reverse_n(m2, h);
    swap_ranges(f, m1, m2);
    return l;
}

template <class ForwardIterator>
void reverse(ForwardIterator f, ForwardIterator l) {
    reverse_n(f, distance(f, l));
}

int main() {
    std::forward_list<int> x = { 0, 1, 2, 3, 4, 5, 6, 7 };
    ::reverse(begin(x), end(x));
    for (const auto& e : x) cout << e << endl;

    stlab::async([](int x){ cout << "begin:" << x << endl; this_thread::sleep_for(3s); cout << "end" << endl; }, 42);
    cout << "concurrent?" << endl;
    cout << "test" << endl;
    this_thread::sleep_for(3s);
}

#endif

#if 0

#include <thread>
#include <iostream>
#include <deque>
#include <vector>
#include <type_traits>

/**************************************************************************************************/

using namespace std;

/*
    The TEST_USE_ flags are mutually exclusive. Set one of them to 1 and the rest to 0 to enable
    that test.
*/

#if 1
#define TEST_USE_ASIO 1
#define TEST_USE_GCD 0
#define TEST_USE_QUEUE 0
#define TEST_USE_SMQUEUE 0
#define TEST_USE_MQUEUE 0
#endif


#if 0
// #define TEST_USE_GCD 1
#define TEST_USE_MQUEUE 1
#define K 56
#define K2 1
#endif

#if 0
#define TEST_USE_BITQUEUE 1
#define K 48
#define K2 1
#endif

/*
    This flag determines if we are testing empty tasks (just function pionters) or tasks with a
    small captured value.
*/

#define TEST_PAYLOAD 0

/**************************************************************************************************/

#if TEST_USE_ASIO

#include <boost/asio/io_service.hpp>

using namespace boost;
using namespace boost::asio;

/**************************************************************************************************/

class task_system {
    io_service                      _service;
    vector<thread>                  _threads;
    unique_ptr<io_service::work>    _work{make_unique<io_service::work>(_service)};

  public:
    task_system() {
        for (unsigned n = 0; n != thread::hardware_concurrency(); ++n) {
            _threads.emplace_back([&]{
                _service.run();
            });
        }
    }

    ~task_system() {
        _work.reset();
        for (auto& e : _threads) e.join();
    }

    template <typename F>
    void async_(F&& f) {
        _service.post(forward<F>(f));
    }
};

/**************************************************************************************************/

#elif TEST_USE_QUEUE

/**************************************************************************************************/

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;

  public:
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
         while (_q.empty() && !_done) _ready.wait(lock);
         if (_q.empty()) return false;
         x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    notification_queue          _q;

    void run(unsigned i) {
        while (true) {
            function<void()> f;
            if (!_q.pop(f)) break;
            f();
        }
    }

  public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }

    ~task_system() {
        _q.done();
        for (auto& e : _threads) e.join();
    }

    template <typename F>
    void async_(F&& f) {
        _q.push(forward<F>(f));
    }
};

/**************************************************************************************************/

#elif TEST_USE_SMQUEUE

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;

  public:
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
         while (_q.empty() && !_done) _ready.wait(lock);
         if (_q.empty()) return false;
         x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};

 void run(unsigned i) {
        while (true) {
            function<void()> f;
            if (!_q[i].pop(f)) break;
            f();
        }
    }

  public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }

    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }

    template <typename F>
    void async_(F&& f) {
        auto i = _index++;
        _q[i % _count].push(forward<F>(f));
    }
};

/**************************************************************************************************/

#elif TEST_USE_MQUEUE

/**************************************************************************************************/

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;
    
public:
    bool try_pop(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock || _q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }
    
    template<typename F>
    bool try_push(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }
    
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }
    
    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
        while (_q.empty() && !_done) _ready.wait(lock);
        if (_q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }
    
    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

/**************************************************************************************************/

class task_system {
    const unsigned              _count{thread::hardware_concurrency() +
                                       thread::hardware_concurrency() / 2};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};
    
    void run(unsigned i) {
        while (true) {
            function<void()> f;

            for (unsigned n = 0; n != _count * K; ++n) {
                if (_q[(i + n) % _count].try_pop(f)) break;
            }
            if (!f && !_q[i].pop(f)) break;
            
            f();
        }
    }
    
public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }
    
    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }
    
    template <typename F>
    void async_(F&& f) {
        auto i = _index++;

        for (unsigned n = 0; n != _count * K2; ++n) {
            if (_q[(i + n) % _count].try_push(forward<F>(f))) return;
        }

        _q[i % _count].push(forward<F>(f));
    }
};

/**************************************************************************************************/

#elif TEST_USE_BITQUEUE

/**************************************************************************************************/

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;
    
public:
    enum class state { success, empty, busy };

    state try_pop(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock) return state::busy;
        if (_q.empty()) return state::empty;
        x = move(_q.front());
        _q.pop_front();
        return state::success;
    }
    
    template<typename F>
    bool try_push(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }
    
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }
    
    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
        while (_q.empty() && !_done) _ready.wait(lock);
        if (_q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }
    
    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

/**************************************************************************************************/

#if 1
int popcnt(int x) {
    int result = 0;
    while (x) {
        ++result;
        x &= x - 1;
    }
    return result;
}
#else
int popcnt(int x) {
    int result = 0;
    while (x &= x - 1) ++result;
    return result;
}
#endif

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};
    
    void run(unsigned i) {
        while (true) {
            function<void()> f;

            int empty = 0;


            for (unsigned n = 0; n != _count * K; ++n) {
                auto state = _q[(i + n) % _count].try_pop(f);
                if (state == notification_queue::state::success) break;
                if (state == notification_queue::state::empty) {
                    empty |= 1 << ((i + n) % _count);
                    if (popcnt(empty) == (_count + 1)) break; // break will never happen
                }
            }

            if (!f && !_q[i].pop(f)) break;
            f();
        }
    }
    
public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }
    
    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }
    
    template <typename F>
    void async_(F&& f) {
        auto i = _index++;

        for (unsigned n = 0; n != _count * K2; ++n) {
            if (_q[(i + n) % _count].try_push(forward<F>(f))) return;
        }

        _q[i % _count].push(forward<F>(f));
    }
};

#endif

/**************************************************************************************************/

#if TEST_USE_GCD

/**************************************************************************************************/

#include <dispatch/dispatch.h>

/**************************************************************************************************/

template <typename F>
auto async_(F&& f) -> std::enable_if_t<!std::is_convertible<F, void(*)()>::value>
{
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new F(move(f)), [](void* p){
            auto f = static_cast<F*>(p);
            (*f)();
            delete(f);
        });
}

void async_(void (*f)()) {
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        nullptr, (void(*)(void*))(f));
}

/**************************************************************************************************/

#else

/**************************************************************************************************/

task_system _system;

template <typename F>
void async_(F&& f) {
    _system.async_(forward<F>(f));
}

/**************************************************************************************************/

#endif // TEST_USE_GCD

/**************************************************************************************************/


/*
    This test floods the tasking system with a million small tasks followed by a single task that
    it waits on. There is no guarantee that all prior tasks are processed, however with 1M tasks
    it will be a small delta from the end. This task is repeated multiple times and the times
    averaged.
*/

__attribute__ ((noinline)) void time() {
    for (int n = 0; n != 1000000; ++n) {
    #if TEST_PAYLOAD
        async_([n]{ });
    #else
        async_([]{ });
    #endif
    }

    mutex block;
    condition_variable ready;
    bool done = false;

    async_([&]{
        {
            unique_lock<mutex> lock{block};
            done = true;
        }
        ready.notify_one();
    });

    unique_lock<mutex> lock{block};
    while (!done) ready.wait(lock);
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    for (int n = 0; n != 10; ++n) time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() / 10 << endl;

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
#include <thread>
#include <future>

using namespace std;

/******************************************************************************/
// Library

template <typename T>
T copy(T&& x) { return std::forward<T>(x); }

template <typename T>
void draw(const T& x, ostream& out, size_t position)
{ out << string(position, ' ') << x << endl; }

class object_t {
  public:
    template <typename T>
    object_t(T x) : self_(make_unique<model<T>>(move(x)))
    { }
    
    object_t(const object_t& x) : self_(x.self_->copy_())
    { cout << "copy" << endl; }
    object_t(object_t&&) noexcept = default;
    
    object_t& operator=(const object_t& x) { return *this = copy(x); }
    object_t& operator=(object_t&&) noexcept = default;
    
    friend void draw(const object_t& x, ostream& out, size_t position)
    { x.self_->draw_(out, position); }
    
  private:
    struct concept_t {
        virtual ~concept_t() = default;
        virtual unique_ptr<concept_t> copy_() const = 0;
        virtual void draw_(ostream&, size_t) const = 0;
    };
    template <typename T>
    struct model final : concept_t {
        model(T x) : data_(move(x)) { }
        unique_ptr<concept_t> copy_() const override { return make_unique<model>(*this); }
        void draw_(ostream& out, size_t position) const override
        { draw(data_, out, position); }
        
        T data_;
    };
    
   unique_ptr<const concept_t> self_;
};

using document_t = vector<object_t>;

void draw(const document_t& x, ostream& out, size_t position)
{
    out << string(position, ' ') << "<document>" << endl;
    for (auto& e : x) draw(e, out, position + 2);
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
    
    current(h).emplace_back(current(h));

    auto saving = async([document = current(h)]() {
        this_thread::sleep_for(chrono::seconds(3));
        cout << "--------- 'save' ---------" << endl;
        draw(document, cout, 0);
    });


    current(h).emplace_back(my_class_t());
    current(h)[1] = string("World");
    
    draw(current(h), cout, 0);
    cout << "--------------------------" << endl;
    
    undo(h);
    
    draw(current(h), cout, 0);
}


#endif

#if 0

#include <tuple>
#include <iostream>

#include <stlab/future.hpp>
#include <stlab/channel.hpp>

using namespace stlab;
using namespace std;
using namespace std::chrono;

struct sum {
    process_state_scheduled _state = await_forever;
    int _sum = 0;

    void await(int n) { _sum += n; }

    int yield() { _state = await_forever; return _sum; }

    void close() { _state = yield_immediate; }

    const auto& state() const { return _state; }
};

int main() {

    sender<int> send;
    receiver<int> receive;

    tie(send, receive) = channel<int>(default_scheduler());

    auto hold = receive
        | sum()
        | [](int x){ cout << x << '\n'; };

    receive.set_ready();

    send(1);
    send(2);
    send(3);
    send.close();

    sleep(5);
}

#endif

#if 0

#include <iostream>

#include <test/test.hpp>

int main() {
    test::test();
}
#endif

#if 0

//
//  main.cpp
//  div255
//
//  Created by Jack Sisson on 9/11/16.

#include "assert.h"

#include <iostream>
#include <string>
#include <functional>

using namespace std;

// Wrong!
inline uint8_t Oldmul8x8Div255(unsigned a, unsigned b) {
    uint32_t temp = a * b + 128;
    return (uint8_t)((temp +(temp>>8))>>8);
}

// Let's fix it:

// Integer division by N is equivalent to repeated subtraction by
// N until the result is < N.
inline uint8_t Mul8x8Div255_1(unsigned a, unsigned b) {
    uint32_t temp = a * b; // + 128;
    uint32_t count = 0;
    while (temp >= 255) {
        temp -= 255;
        ++count;
    }
    return count;
}

// temp - 255 = temp + 1 - 256. No need to worry about overflow, given our type selection.
inline uint8_t Mul8x8Div255_2(unsigned a, unsigned b) {
    uint32_t temp = a * b;// + 128;
    auto count = 0;
    while (temp >= 255) {
        temp += 1;
        temp -= 256;
        ++count;
    }
    return (uint8_t)count;
}

// We know that one will be added (a * b) / 255 times, since that's exactly how many iterations the while
// loop body will execute. We can therefore add temp/255 to temp up front and remove the temp += 1
// from the while loop.
inline uint8_t Mul8x8Div255_3(unsigned a, unsigned b) {
    uint32_t temp = a * b;// + 128;
    uint32_t count = 0;
    
    temp += temp / 255;
    while (temp >= 255) {
        temp -= 256;
        ++count;
    }
    
    return (uint8_t)count;
}

// We can now add one to temp and the condition expression. This will allow
// us to replace the while loop with a division by 256.
inline uint8_t Mul8x8Div255_4(unsigned a, unsigned b) {
    uint32_t temp = a * b;// + 128;
    uint32_t count = 0;
    
    temp += temp / 255;
    temp += 1;
    while (temp >= 256) {
        temp -= 256;
        ++count;
    } // this is equivalent to integer division by 256
    
    return (uint8_t)count;
}

// Let's swap in the division, and replace temp / 255 with
// an equivalent expression.
inline uint8_t Mul8x8Div255_5(unsigned a, unsigned b) {
    uint32_t temp = a * b;// + 128;
    
    temp += temp / 256 + (temp / 255 - temp / 256);
    temp += 1;
    temp /= 256;
    
    return (uint8_t)temp;
}

// Now, we can remove the (temp/255 - temp/256) if we can show:
//    temp/255 != temp/256 implies that
//    (temp + 1 + temp/256) / 256 == (temp + 1 + temp/255) / 256
//
// Given an integer a, a * 255 = a * (256 - 1), so a * 255 = a * 256 - a. Thus, given integer
// b in range [0, 255 * 255], b/255 != b/256 iff (0xFF & b) >= (0x100 - b/255). In these cases,
// b/255 - b/256 = 1. Thinking about this like division by 9 is helpful (i.e. diving by 9 is not the same
// as dividing by 10 for 9; 18, 19; 27, 28, 29, etc.).
//
// If b/255 - b/256 = 1, 0 <= (0xFF & (b + b/255)) <= b/255 - 1, since the lower limit for
// (0xFF & b) is 0x100 - b/255, and the upper limit is 0x100 - b/255 + (b/255 - 1). Additionally, since
// b/255 <= 255 it follows that 1 <= 0xFF & (b + b/255 + 1) <= 255. Since b/256 is exactly one less than b/255,
// 0 <= 0xFF & (b + b/256 + 1) <= 254. Therefore, the subraction by one that comes from swapping in temp/256
// never takes the least significant byte from 0 to 255, which is the only subraction by 1 that could change
// the result of the division. We can remove the (temp/255 - temp/256).
void testProof() {
    for (unsigned a = 0; a <= 255 * 255; ++a) {
        if ((0xFF & a) >= 0x100 - a/255) {
            assert(a/255 - a/256 == 1);
            assert((0xFF & (a + a/255)) >= 0 && (0xFF & (a + a/255)) <= a/255 - 1);
            assert((0xFF & (a + a/255 + 1)) >= 1 && (0xFF & (a + a/255 + 1)) <= 255);
            assert((0xFF & (a + a/256 + 1)) >= 0 && (0xFF & (a + a/256 + 1)) <= 254);
        } else {
            assert(a/255 == a/256);
        }
    }
}

// With this proof in hand, we can get rid of the (temp/255 - temp/256) term, and we might as well
// go ahead and replace the divisions by 256 with >> 8 and collapse into one statement.
inline uint8_t Mul8x8Div255(unsigned a, unsigned b) {
    uint32_t temp = a * b + 127;
    return (uint8_t)((temp + 1 + (temp >> 8)) >> 8);
}

// Now, let's test all of our assertions
void test(function<unsigned(unsigned, unsigned)> func, string testName) {
    int errorCount = 0;
    for (unsigned a = 0; a < 255; ++a) {
        for (unsigned b = 0; b < 255; ++b) {
            if (func(a, b) != (a * b + 127) / 255) {
                ++errorCount;
            }
        }
    }
    
    cout << testName << endl;
    if (errorCount > 0) {
        cout << "\tFailure: " << errorCount << " errors." << endl;
    } else {
        cout << "\tSuccess!" << endl;
    }
}

int main(int argc, const char * argv[]) {
    test(Oldmul8x8Div255, "Oldmul8x8Div255");
    test(Mul8x8Div255_1, "Mul8x8Div255_1");
    test(Mul8x8Div255_2, "Mul8x8Div255_2");
    test(Mul8x8Div255_3, "Mul8x8Div255_3");
    test(Mul8x8Div255_4, "Mul8x8Div255_4");
    test(Mul8x8Div255_5, "Mul8x8Div255_5");
    testProof();
    test(Mul8x8Div255, "Mul8x8Div255");

    return 0;
}

#endif

#if 0

#include <adobe/selection.hpp>
#include <tuple>

namespace adobe {

template <typename I, typename N> // I models ForwardIterator
std::pair<I, N> find_sequence_end(I f, I l, N n) {
    while (f != l && n == *f) {
        ++n;
        ++f;
    }
    return { f, n };
}

/*
    \pre [f, l) is a sorted (strictly increasing) set of of indices
*/

template <typename I, // I models ForwardIterator
          typename O> // O models OutputIterator
O index_set_to_selection(I f, I l, O out) {
    while (f != l) {
        auto n = *f;
        *out++ = n;
        ++f; ++n;
        std::tie(f, n) = find_sequence_end(f, l, n);
        *out++ = n;
    }
    return out;
}

} // namespace adobe

#include <iostream>

using namespace std;

int x = []{
    cout << "Hello World!" << endl;
    return 52;
}();

int main() {
    int a[] = { 3, 4, 5, 10, 15, 16, 27, 28 };
    adobe::selection_t s;
    adobe::index_set_to_selection(std::begin(a), std::end(a), std::back_inserter(s));
    for (const auto& e : s) {
        cout << e << endl;
    }
}

#endif





#if 0
#include <string>
#include <vector>
#include <memory>
#include <iostream>

using namespace std;

struct foo // implements ProviderInterface
{
    static string getProviderId() { return "foo::getProviderId"; }
};
 
struct bar // also implements ProviderInterface
{
    static string getProviderId() { return "bar::getProviderId"; }
};

class any_provider_interface {
 public:
    any_provider_interface(int type) {
        switch(type) {
        case 0: _model = make_shared<model<foo>>(); break;
        case 1: _model = make_shared<model<bar>>(); break;
        }
    }

    string getProviderId() const { return _model->getProviderId(); }
 private:
    struct concept {
        virtual ~concept() { }
        virtual string getProviderId() const = 0;
    };

    template <typename T>
    struct model : concept {
        string getProviderId() const override {
            return T::getProviderId();
        }
    };

    shared_ptr<const concept> _model;

};
 

// caller

int main() {
    any_provider_interface pi(0);
    cout << pi.getProviderId() << endl;
    cout << (0 ? true : false) << endl;
    cout << ((0) ? true : false) << endl;
}

#endif

#if 0

#include <functional>
#include <vector>
#include <iostream>

int main() {
    std::vector<bool> x = {
        1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
        0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
        0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
    };

    auto y = std::hash<std::vector<bool>>()(x);
    std::cout << y << std::endl;
}



template <typename I, // I models RandomAccessIterator
          typename P> // P models UnaryPredicate
I stable_partition(I f, I l, P p) {
    auto n = distance(f, l);

    if (n == 0) return f;
    if (n == 1) return f + p(*f);

    auto m = f + n / 2;

    return rotate(stable_partition(f, m, p),
                  m,
                  stable_partition(m, l, p));
}

template <typename I,
          typename P>
auto stable_partition_position(I f, I l, P p) -> I
{
    auto n = l - f;
    if (n == 0) return f;
    if (n == 1) return f + p(f);
    
    auto m = f + (n / 2);

    return rotate(stable_partition_position(f, m, p),
                  m,
                  stable_partition_position(m, l, p));
}


template <typename I> // I models RandomAccessIterator
void sort_subrange(I f, I l, I sf, I sl)
{
    if (sf == sl) return;
    if (f != sf) {
        nth_element(f, sf, l);
        ++sf;
    }
    partial_sort(sf, sl, l);
}

#endif

#if 0

#include <deque>
#include <mutex>
#include <tuple>
#include <thread>
#include <iostream>

#include <stlab/channel.hpp>
#include <boost/optional.hpp>

using namespace stlab;
using namespace std;
using namespace boost;

struct sum {
    process_state _state = process_state::await;
    int _sum = 0;

    void await(int n) { _sum += n; }

    int yield() { _state = process_state::await; return _sum; }

    void close() { _state = process_state::yield; }

    auto state() const { return _state; }
};


int main() {

    sender<int> send;
    receiver<int> receive;

    tie(send, receive) = channel<int>();

    auto hold = receive
        | sum()
        | [](int x){ cout << x << '\n'; };

    receive.set_ready();

    send(1);
    send(2);
    send(3);
    send.close();

    sleep(5);
}

#endif


#if 0

int main() {

    channel<int> send;

    auto hold = send
        | [](const receiver<int>& r) {
            int sum = 0;
            while(auto v = await r) {
                sum += v.get();
            }
            return sum;
        }
        | [](int x){ cout << x << '\n'; };

    send(1);
    send(2);
    send(3);
    send.close();

    sleep(5);
}
#endif


#if 0

#include <adobe/forest.hpp>
#include <string>
#include <iostream>

using namespace adobe;
using namespace std;

int main() {
    forest<string> x;

    auto a = x.insert(end(x), "A");
    x.insert(end(x), "E");

    x.insert(end(a), "B");
    x.insert(end(a), "C");
    x.insert(end(a), "D");

    auto r = depth_range(x);
    for (auto f = begin(r), l = end(r); f != l; ++f) {
        cout << string(f.depth() * 4, ' ') << (f.edge() ? "<" : "</") << *f << ">\n";
    }
}

#endif


#if 0

#include <future>

using namespace std;

template <typename T> // T models Promise
struct coroutine_handle {
    T _promise;

    static
};

struct MyCoro {
    struct promise_type {
    };
};

/*
    The only way I can figure out how to construct a coroutine_handle is to inherit from the
    promise_type.
*/

template <typename T> // T models promise
struct manual_coroutine : T {
    void operator()() const {

    }
};

MyCoro coroutine(future<int> x) {
    using promise_type = MyCoro::promise_type;
    return coroutine_handle<promise_type>::from_promise(manual_coroutine<promise_type>());
}

#endif
#if 0
#include <iostream>

using namespace std;

// void f(int x) { x = 5; cout << "void f(int x) -> " << x << endl; }
void f(const int x){ cout << "void f(const int x) -> " << x << endl; }


int main() {
    f(10);
}
#endif


#if 0
using connection = void;

class surface {
    template <typename T>
    void signal(const char* property, T value);

    template <typename F> // F has signature void(T) where T is the property type
    connection connect(const char* property, F callback);
};

#endif

#if 0

#include <deque>
#include <mutex>
#include <tuple>
#include <thread>

#include <stlab/channel.hpp>

using namespace stlab;
using namespace std;

struct scheduler {
    using result_type = void;
    using lock_t = std::unique_lock<std::mutex>;
    using function_t = std::function<void()>;

    template <typename F>
    void operator()(F f) {
        {
        lock_t _lock(_mutex);
        _queue.emplace_back(f);
        }
        _ready.notify_one();
    }

    static function_t pop() {
        lock_t _lock(_mutex);
        while (_queue.empty()) _ready.wait(_lock);
        auto result = std::move(_queue.back());
        _queue.pop_back();
        return result;
    }

    [[noreturn]] static void run() {
        while (true) {
            pop()();
        }
    }

    static std::condition_variable      _ready;
    static std::mutex                   _mutex;
    static std::deque<function<void()>> _queue;
};

std::condition_variable      scheduler::_ready;
std::mutex                   scheduler::_mutex;
std::deque<function<void()>> scheduler::_queue;

int main() {

    sender<int> send;
    receiver<void> cap;

    {
    receiver<int> receive;

    tie(send, receive) = channel<int>(scheduler());
    }

    auto thread = std::thread(&scheduler::run);
    thread.join(); // never to return
}

#endif


#if 0
#include <cstdint>
#include <cassert>
#include <utility>

namespace std {
inline namespace stlab {

using max_align_t = double;

template <std::size_t Len, std::size_t Align>
struct aligned_storage {
    typedef struct {
        alignas(Align) unsigned char data[Len];
    } type;
};

template <std::size_t Len, std::size_t Align = alignof(double)>
using aligned_storage_t = typename std::stlab::aligned_storage<Len, Align>::type;

} // namespace stlab
} // namespace std

template <typename T, std::size_t Align = sizeof(T)>
class aligned_large {
    static constexpr auto _max_align = alignof(max_align_t);
    static constexpr auto _max_pad = (Align < _max_align) ? 0 : Align - _max_align ;

    std::stlab::aligned_storage_t<sizeof(T) + _max_pad, _max_align>  _data;

    auto pad() const { return Align - reinterpret_cast<std::uintptr_t>(this) % Align; }

  public:
    template <typename... Args>
    aligned_large(Args... args) {
        assert((pad() <= _max_pad) && "pad() is greater than _max_pad!");
        new(reinterpret_cast<char*>(&_data) + pad()) T(std::forward<Args>(args)...);
    }
    ~aligned_large() {
        get().~T();
    }

    T& get() {
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(&_data) + pad());
    }

    const T& get() const {
        return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(&_data) + pad());
    }
};

#include <iostream>
using namespace std;

int main() {
    struct avx_vector { char data_ [256]; };

    avx_vector unaligned;
    aligned_large<avx_vector> aligned;

    cout << "unaligned: " << &unaligned.data_  << endl;
    cout << "aligned: " << &aligned.get().data_ << endl;

};

#endif


#if 0

#include <iostream>
#include <tuple>

#include <stlab/channel.hpp>

using namespace stlab;
using namespace std;

int main() {

    sender<int> send;
    receiver<void> cap;

    {
    receiver<int> receive;

    tie(send, receive) = channel<int>();

    cap = receive
        | [](int a) { cout << "a: " << a << endl; return a * 2; }
        | [](int b) { sleep(1); cout << "b: " << b << endl; };
    }

    for (int n = 0; n != 10; ++n) {
        send(n);
    }

    send.close();

    // Wait for everthing to execute (just for demonstration)
    sleep(100);
}

#endif

#if 0

#include <iostream>

using namespace std;

void print_string(const char* s) {
    while (*s != '\0') {
        cout << *s++;
    }
}

int* lower_bound(int* first, int* last, int value)
{
    while (first != last) {
        int* middle = first + (last - first) / 2;
        if (*middle < value) first = middle + 1;
        else last = middle;
    }
    return first;
}

int Search(int N, const int* b, int x)
{
    return int(lower_bound(b, b + N, x) - b);
}

int main() {
    int a[] = { 0, 1, 1, 3, 3, 4, 5, 6, 6, 8 };
    int* p = lower_bound(begin(a), end(a), 6);
    cout << "lower_bound: a[" << p - begin(a) << "] == " << *p << endl;
}

#endif

#if 0
#include <iostream>
#include <algorithm>
#include <utility>
#include <string>
#include <cassert>
#include <iterator>
#include <cstdint>
#include <vector>
#include <adobe/unicode.hpp>

using namespace std;

namespace stlab {

template <class T, class InputIterator, class OutputIterator>
OutputIterator copy_utf(InputIterator first, InputIterator last, OutputIterator result);

template <class ForwardIterator, class T, class Compare>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last,
        const T& value, Compare comp)
{
    auto n = distance(first, last);

    while (n != 0)  {
        auto h = n / 2;
        auto m = next(first, h);

        if (comp(*m, value)) {
            first = next(m);
            n -= h + 1;
        } else { n = h; }
    }

    return first;
}

#if 0
template <class ForwardIterator, class T, class Compare>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last,
        const T& value, Compare comp)
{
    auto n = distance(first, last);

    while (n != 0)  {
        auto h = n / 2;
        auto m = next(first, h);

        if (comp(*m, value)) {
            first = next(m);
            n -= h + 1;
        } else { n = h; }
    }

    return first;
}
#endif

template <class ForwardIterator>
void reverse(ForwardIterator f, ForwardIterator l) {
    auto n = distance(f, l);

    if (n == 0 || n == 1) return;

    auto m = next(f, n / 2);

    reverse(f, m);
    reverse(m, l);
    rotate(f, m, l);
}

} // namespace

template <class T>
const T& max(const T& a) { return a; }

template <class T>
const T& max(const T& a, const T& b) { return (b < a) ? a : b; }

//template <class T> const T& max(const T& a, const T& b, const T& c);

template<class T, class... Args> const T& max(const T& a, const Args&... args) {
    return ::max(a, ::max(args...));
}

template <class T>
void a(T& x) { x = f(x); } // action from transformation

template <class T>
T f(T x) { a(x); return x; } // transformation from action

template<typename T, typename Compare>
const T& clamp(const T& a, const T& lo, const T& hi, Compare comp)
{
    return min(max(lo, a, comp), hi, comp);
}

int main() {
    const char str[] = u8"Hello World!";
    vector<uint16_t> out;
    adobe::copy_utf<uint16_t>(begin(str), end(str), back_inserter(out));

    int a0[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    cout << "lower: " << *std::lower_bound(begin(a0), end(a0), 3, std::less<>()) << endl;
    stlab::reverse(begin(a0), end(a0));
    for(const auto& e : a0) cout << e << endl;

    cout << max(0, 1, 2, 5, 4, 3) << endl;
    
    using pair = pair<int, string>;

    pair a = { 1, "OK" };

    pair lo = { 1, "FAIL: LO" };
    pair hi = { 2, "FAIL: HI" };

    a = clamp(a, lo, hi, [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    cout << a.second << endl;
}

#endif

#if 0

#include <iostream>
#include <algorithm>
#include <utility>
#include <string>
#include <cassert>

using namespace std;

#if 0
template <typename T>
const T& max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

template <typename T>
const T& min(const T& a, const T& b) {
    return (b < a) ? b : a;
}
#endif


template<typename T, typename O>
const T& clamp(const T& a, const T& lo, const T& hi, Compare comp)
{
    return min(max(lo, a, comp), hi, comp);
}

template<class T> const T& min(const T& a, const T& b);
template<class T> const T& max(const T& a, const T& b);

#if 0
template <typename T, typename O>
const T& median(const T& a, const T& b, const T& c, const O& o) {
    // return min(max(a, b, o), c, o);
    return max(a, min(b, c, o), o);
}

#endif


int main() {
    using p = pair<int, string>;

    {
    p x = { 1, "OK" };

    p f = { 0, "WRONG (LO)" };
    p l = { 2, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 0, "OK" };

    p f = { 0, "WRONG (LO)" };
    p l = { 2, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 2, "OK" };

    p f = { 0, "WRONG (LO)" };
    p l = { 2, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 0, "OK" };

    p f = { 0, "WRONG (LO)" };
    p l = { 0, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 0, "WRONG" };

    p f = { 1, "OK (LO)" };
    p l = { 2, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 3, "WRONG" };

    p f = { 1, "WRONG (LO)" };
    p l = { 2, "OK (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 0, "WRONG" };

    p f = { 1, "OK (LO)" };
    p l = { 1, "WRONG (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }

    {
    p x = { 3, "WRONG" };

    p f = { 1, "WRONG (LO)" };
    p l = { 1, "OK (Hi)" };

    x = clamp(x, f, l, [](auto x, auto y){ return x.first < y.first; });
    cout << x.second << endl;
    }



#if 0
    {
    p a = { 0, "a" };
    p b = { 1, "b" };
    p c = { 2, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 0, "a" };
    p b = { 0, "b" };
    p c = { 2, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 0, "a" };
    p b = { 2, "b" };
    p c = { 2, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 0, "a" };
    p b = { 0, "b" };
    p c = { 0, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 0, "a" };
    p b = { 1, "b" };
    p c = { 0, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 2, "a" };
    p b = { 1, "mid" };
    p c = { 0, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 0, "a" };
    p b = { 2, "b" };
    p c = { 1, "mid" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 1, "mid" };
    p b = { 2, "b" };
    p c = { 0, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 1, "mid" };
    p b = { 0, "b" };
    p c = { 2, "c" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }

    {
    p a = { 2, "a" };
    p b = { 0, "b" };
    p c = { 1, "mid" };
    b = median(a, b, c, [](auto x, auto y){ return x.first < y.first; });
    cout << b.second << endl;
    }
#endif


}

#endif


#if 0
#include <iostream>
#include <vector>

struct test {
    int a[0];
};

struct empty { };

void f() {
    return void();
}

using namespace std;
int main() {
    f();
    // void x = f();
    {
    int x;
    cout << x << endl;
    }

    {
    int x = 1 / 0;
    cout << x << endl;
    }
    {
    double x = 0.0 / 0.0;
    cout << x << endl;
    }
    {
    double x = 1.0 / 0.0;
    cout << x << endl;
    }
    {
    int a[0];
    cout << a << endl;
    }
    {
    int a[0];
    int b;
    cout << a << "\n" << &b << "\n" << endl;
    }
    {
    struct empty {};
    cout << sizeof(empty) << endl;
    }
    {
    using empty = int[0];
    empty a[2];
    cout << sizeof(a) << endl;
    cout << &a[0] << endl;
    cout << &a[1] << endl;
    }
    {
    std::vector<int> x = { 1, 2, 3 };
    try {
        x.insert(x.begin(), 0);
    } catch (...) {
        std::cout << x.size() << std::endl;
    }

    std::vector<int> y = std::move(x);

    // cout << 10 / y[0] << endl;
    }

    {
    }

}

#endif


#if 0
#include <typeinfo>
#include <cassert>
#include <utility>
#include <memory>
#include <iostream>

template <typename T>
auto test_equal(decltype(std::declval<T>() == std::declval<T>())) -> std::true_type;

template <typename>
auto test_equal(...) -> std::false_type;

template <typename T>
constexpr bool has_equal = decltype(test_equal<T>(0))::value;

template <typename T, typename = void>
class indirect;

template <typename T>
class indirect<T, std::enable_if_t<has_equal<T>>> {
    struct concept {
        virtual ~concept() = default;
        virtual bool equal(const concept&) const = 0;
        virtual std::unique_ptr<concept> copy() const = 0;
    };
    template <typename U>
    struct model : concept {
        model(std::unique_ptr<U> x) : _value(std::move(x)) { }

        bool equal(const concept& x) const override {
            if (typeid(x) != typeid(model)) return false;
            return static_cast<const model&>(x)._value == _value;
        }
        std::unique_ptr<concept> copy() const override {
            return std::make_unique<model>(std::make_unique<U>(_value));
        }

        std::unique_ptr<U> _value;
    };

    std::unique_ptr<concept> _self;
  public:
    template <typename U>
    indirect(const U* p) : _self(std::make_unique<model<U>>(p)) {
        assert(typeid(*p) == typeid(U)
            && "WARNING (sparent): indirect of derived class will slice on copy.");
    }

    template <typename U>
    indirect(const U& x) : _self(std::make_unique<model<U>>(std::make_unique<U>(x))) { }
};

struct A { virtual ~A() = default; };
struct B : A { };

int main() {
    indirect<int> a = 10;

    std::cout << has_equal<A> << std::endl;

    A* x = new B();
    assert(typeid(*x) == typeid(B));
    // assert(typeid(*x) == typeid(A));
}

#endif

#if 0
#include <stlab/channel.hpp>
#include <stlab/future.hpp>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace stlab;

/*
    sum is an example of an accumulating "co-routine". It will await for values, keeping an 
    internal sum, until the channel is closed and then it will yield the result as a string.
*/
struct sum {
    process_state _state = process_state::await;
    int _sum = 0;

    void await(int n) { _sum += n; }

    int yield() { _state = process_state::await; return _sum; }

    void close() { _state = process_state::yield; }

    auto state() const { return _state; }
};

int main() {
    /*
        Create a channel to aggregate our values.
    */
    sender<int> aggregate;
    receiver<int> receiver;
    tie(aggregate, receiver) = channel<int>();

    /*
        Create a vector to hold all the futures for each result as it is piped to channel.
        The future is of type <void> because the value is passed into the channel.
    */
    vector<stlab::future<void>> results;

    for (int n = 0; n != 10; ++n) {
        // Asyncrounously generate a bunch of values.
        results.emplace_back(async(default_scheduler(), [_n = n]{ return _n; })
            // Then send those values into a copy of the channel
            .then([_aggregate = aggregate](int n) {
                _aggregate(n);
            }));
    }
    // Now it is safe to close (or destruct) this channel, all the copies remain open.
    aggregate.close();

    auto pipe = receiver
        /*
            The receiver is our common end point - we attach the vector of futures to it (another)
            inefficiency here - this is a lambda whose only purpose is to hold the vector of
            futures.
        */
        | [ _results = move(results) ](auto x){ return x; }
        // Then we can pipe the values to our accumulator
        | sum()
        // And pipe the final value to a lambda to print it.
        // Returning void from the pipe will mark it as ready.
        | [](auto x){ cout << x << endl; };

    receiver.set_ready(); // close this end of the pipe

    // Wait for everthing to execute (just for demonstration)
    sleep(100);
}
#endif


#if 0
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

using namespace std;

string get_field() {
    string result;
    bool quote = false;
    while (!cin.eof()) {
        char c = cin.get();
        if (c == '\0') continue;
        if (c == '\xC2') continue;
        if (c == '\t') break;
        if (c == '\xAC') {
            result += '\n'; quote = true;
        } else if (c == '\xBB') {
            result += '\t';
        } else if (c == ',') { result += ','; quote = true; }
        else if (c == '"') { result += "\"\""; quote = true; }
        else result += c;
    }
    if (quote) result = "\"" + result + "\"";
    return result;
}

void get_eol() {
    while (!cin.eof()) {
        char c = cin.get();
        if (c == '\n') break;
    }
}

int main() {
    sleep(5); // debugger attach

    cout << "url,username,password,extra,name,grouping,type,hostname" << endl;

    while (!cin.eof()) {
        string title = get_field();
        string url = get_field();
        string username = get_field();
        string password = get_field();
        string notes = get_field();
        string category = get_field();
        string browser = get_field();
        get_eol();
        if (url == "") {
            if (username == "" && password == "") url = "http://sn";
            else url = "http://unknown";
        }
        cout << url << "," << username << "," << password << "," << notes << "," << title << ","
            << category << "," /* type */ << "," /* hostname */ << endl;
    }

#if 0
    while (getline(cin, x, '\t')) {
        cout << x << endl;
    }
#endif
}

#endif

#if 0

#include <adobe/forest.hpp>
#include <iostream>
#include <algorithm>
#include <utility>
#include <iterator>

using namespace std;
using namespace adobe;

template <typename I> // I is a depth adaptor range iterator
void output(I& f, I& l)
{
    while (f != l) {
        for (auto i = f.depth(); i != 0; --i) cout << "\t";

        if (f.edge() == forest_leading_edge) cout << "<" << *f << ">" << endl;
        else cout << "</" << *f << ">" << endl;

        ++f;
    }
}

template <typename R>
void output_new(const R& r) {
    for (auto f = boost::begin(r), l = boost::end(r); f != l; ++f) {
        for (auto i = f.base().depth(); i != 0; --i) cout << "\t";
        cout << *f << endl;
    }
}

int main() {
    forest<string> f;
    auto i (f.begin());
    i = adobe::trailing_of(f.insert(i, "grandmother"));
    {
        auto p = adobe::trailing_of(f.insert(i, "mother"));
        f.insert(p, "me");
        f.insert(p, "sister");
        f.insert(p, "brother");
    }
    {
        auto p = adobe::trailing_of(f.insert(i, "aunt"));
        f.insert(p, "cousin");
    }
    f.insert(i, "uncle");

    output_new(preorder_range(depth_range(f)));
    //output(r.first, r.second);
}

#endif

#if 0

#include "adobe/forest.hpp"

#include <iostream>
#include <list>

bool activate = false;

class test {
public:
  test(int d) : data_(d) { std::cout << "create " << data_ << std::endl; }
  test(const test& o) : data_(o.data_) {
    std::cout << "copy c " << data_ << std::endl;
    if (activate && data_ == 2) {
      std::cout << "throw " << data_ << std::endl;
      throw 0;
    }
  }
  test& operator=(const test& o) {
    data_ = o.data_;
    std::cout << "copy = " << data_ << std::endl;
    return *this;
  }
  ~test() { std::cout << "delete " << data_ << std::endl; }
private:
  friend std::ostream& operator<<(std::ostream& out, const test& o);

  int data_;
};

std::ostream& operator<<(std::ostream& out, const test& o) {
  out << o.data_;
  return out;
}

template<class T>
void x() try {
  using forest = T;

  forest f1;
  f1.push_back(1);
  f1.push_back(2);
  f1.push_back(3);

  activate = true;

  forest f(f1);
} catch(...) { activate = false; }

int main() {
  std::cout << "adobe::forest" << std::endl;
  x<adobe::forest<test>>();
  std::cout << "std::list" << std::endl;
  x<std::list<test>>();
}

#endif

#if 0

#include <stlab/future.hpp>
#include <iostream>

using namespace stlab;
using namespace std;

/**************************************************************************************************/

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

struct identity_t {
    template <typename T>
    T operator()(T x) const { return x; };
};

constexpr auto identity = identity_t();

int main() {
    identity(annotate());

    auto x = package<int()>(default_scheduler(), []{ return 42; });
    auto y = package<int()>(default_scheduler(), []{ throw 42; return 3; });
    auto w = stlab::when_all(default_scheduler(), [](int x, int y) {
        cout << x << ", " << y << endl;
    }, x.second, y.second);

    x.first();
    y.first();

    try {
        while (!w.get_try()) ;
    } catch(...) {
        cout << "exception" << endl;
    }
}

#endif

#if 0

#include <iostream>
#include <type_traits>

// Define an attribute with default value

template <typename T, T x>
struct is_override : std::false_type { };

template <typename T, T x>
constexpr bool is_override_t = is_override<T, x>::value;

// Example useage

struct test {
    void member();
    void member2();
};

// Set the attribute for one member function

template <>
struct is_override<decltype(&test::member), &test::member> : std::true_type { };

int main() {
    std::cout << is_override_t<decltype(&test::member), &test::member> << std::endl;
    std::cout << is_override_t<decltype(&test::member2), &test::member2> << std::endl;
}


#endif

#if 0
#include <algorithm>
#include <iostream>

using namespace std;

template <typename I, // I models ForwardIterator
          typename P, // P models UnaryPredicate
          typename F, // F models UnaryFunction
          typename Final> // Final models UnaryFunction
void for_each_if_final(I f, I l, P pred, F op, Final final) {
    auto prior = find_if(f, l, pred);
    if (prior == l) return;
    f = find_if(next(prior), l, pred);

    while (f != l) {
        op(*prior);
        prior = f;
        f = find_if(next(prior), l, pred);
    }

    final(*prior);
}

int main() {
    {
    int a[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    cout << "(";
    for_each_if_final(begin(a), end(a), [](int x){ return x & 1; },
        [](int x) { cout << x << ", "; },
        [](int x) { cout << x; });
    cout << ")" << endl;;
    }
    {
    int a[] = { 0 };
    cout << "(";
    for_each_if_final(begin(a), end(a), [](int x){ return x & 1; },
        [](int x) { cout << x << ", "; },
        [](int x) { cout << x; });
    cout << ")" << endl;;
    }
    {
    int a[] = { 0, 1 };
    cout << "(";
    for_each_if_final(begin(a), end(a), [](int x){ return x & 1; },
        [](int x) { cout << x << ", "; },
        [](int x) { cout << x; });
    cout << ")" << endl;;
    }
}
#endif

#if 0

#include <stlab/channel.hpp>
#include <stlab/future.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace stlab;

/**************************************************************************************************/

struct iota {
	int _count;
	int _n;
	process_state _state = process_state::await;

	void await(int n) {
		_count = 0;
		_n = n;
		if (_count != _n) _state = process_state::yield;
	}

	int yield() {
		int result = _count;
		++_count;
		_state = (_count != _n) ? process_state::yield : process_state::await;
        return result;
	}

    void close() { }

	process_state state() const { return _state; }
};

struct word {
    string _word;
    process_state _state = process_state::await;

    void await(char x) {
        if (x == ' ' || x == '\0') _state = process_state::yield;
        else _word += x;
    }

    string yield() {
        _state = process_state::await;
        string result;
        swap(_word, result);
        return result;
    }

    void close() {
        if (!_word.empty()) _state = process_state::yield;
    }

    process_state state() const { return _state; }
};

struct line {
    const size_t length = 30;
    string _line;
    string _result;
    process_state _state = process_state::await;

    using signature = string(string);

    void await(string x) {
        if ((length - _line.size() < x.size()) || x.size() == 0) {
            _result = move(_line);
            _line = move(x);
            _state = process_state::yield;
        } else _line += x;
    }

    string yield() {
        _state = process_state::await;
        return move(_result);
    }

    void close() {
        if (!_line.empty()) {
            _result = move(_line);
            _state = process_state::yield;
        }
    }

    process_state state() const { return _state; }
};

class empty { };


/*
    sum is an example of an accumulating "co-routine". It will await for values, keeping an 
    internal sum, until the channel is closed and then it will yield the result as a string.
*/
struct sum {
    process_state _state = process_state::await;
    int _sum = 0;

    void await(int n) { _sum += n; }

    string yield() { _state = process_state::await; return to_string(_sum); }

    void close() { _state = process_state::yield; }

    process_state state() const { return _state; }
};

int main() {
    auto a1 = stlab::async(default_scheduler(), [](){ return 1; });
    auto a2 = stlab::async(default_scheduler(), [](){ return 2; });
    auto a4 = when_all(default_scheduler(), [](auto x, auto y){
        cout << x << ", " << y << endl;
    }, a1, a2);

    sleep(10);


    sender<int> aggregate; // create a channel that we will use to accumulate values
    receiver<int> receiver;

    tie(aggregate, receiver) = channel<int>();


    /*
        Create a vector to hold all the futures for each result as it is piped to channel.
        The future is of type <void> because the value is passed into the channel.
    */
    vector<stlab::future<void>> results;

    for (int n = 0; n != 100; ++n) {
        // Asyncrounously generate a bunch of values.
        results.emplace_back(async(default_scheduler(), [_n = n]{ return _n; })
            // Then send those values into a copy of the channel
            .then([_aggregate = aggregate](int n){ _aggregate(n); }));
    }
    // Now it is safe to close (or destruct) this channel, all the copies remain open.
    aggregate.close();

    /*
        REVISIT (sparent) : currently doesn't work - you can't append the pipe 
        after without losing data.
    */
    
    //sleep(3); // SHOW THE BUG!
    auto pipe = receiver
        /*
            The receiver is our common end point - we attach the vector of futures to it (another)
            inefficiency here - this is a lambda whose only purpose is to hold the vector of
            futures.
        */
        | [ _results = move(results) ](auto x){ return x; }
        // Then we can pipe the values to our accumulator
        | sum()
        // And pipe the final value to a lambda to print it.
        | [](string s){ cout << s << endl; };

    receiver.set_ready();

    // Wait for everthing to execute (just for demonstration)
    sleep(1000);


#if 0

    channel<char> sender;

    auto receiver = sender | word() | line() | [](auto x){
        cout << x << endl;
    };

    for (char e : "This is a very very very long, as in supper long string, that needs to be formatted.") {
        sender(e);
    }
    sender.close();
    
    sleep(100);
#endif
}

#endif

#if 0

template <typename I>
std::size_t maximum_overlap(I f, I l, std::size_t n) {
    std::vector<std::pair<std::tuple_element_t<0, I>, bool>> sorted;
    while (f != l && n != 0) {
        sorted.emplace_back(get<0>(*f), true);
        sorted.emplace_back(get<1>(*f), false);
        ++f; --n;
    }
    std::make_heap(begin(sorted), end(sorted));
    
}
#endif





#if 0
#include <iostream>
#include <memory>
#include <deque>
#include <thread>
#include <utility>
#include <stlab/future.hpp>

#include <boost/optional.hpp>

using namespace stlab;
using namespace std;
using namespace boost;

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

template <typename> class channel;
template <typename> class receiver;

/**************************************************************************************************/

namespace detail {

/**************************************************************************************************/

// REVISIT (sparent) : I have a make_weak_ptr() someplace already. Should be in memory.hpp

template <typename T>
auto make_weak_ptr(const std::shared_ptr<T>& x) { return std::weak_ptr<T>(x); }

/**************************************************************************************************/

template <typename> struct shared_channel_base;
template <typename> struct shared_channel;

/**************************************************************************************************/

// This is overwrite, we also need queue.
template <typename T>
struct shared_channel_base : std::enable_shared_from_this<shared_channel_base<T>> {
    using map_t = std::vector<std::function<void(T)>>;

    // how to propogate errors?
    std::mutex                          _map_mutex;
    map_t                               _map;

    std::function<void()>               _cts; // clear to send signal

    bool                                   _ready = true;
    bool                                   _hold = false;
    std::deque<std::function<void()>>      _message_queue;
    std::mutex                             _message_mutex;

    virtual ~shared_channel_base() { }
    virtual void task_continue() = 0;

    template<typename F>
    auto map(F&& f) {
        using signature_type = typename F::signature_type;
        auto p = std::make_shared<detail::shared_channel<signature_type>>(std::move(f),
                [_p = this->shared_from_this()](){ _p->task_continue(); });

        {
        std::unique_lock<std::mutex> lock(_map_mutex);
        _map.emplace_back([_p = make_weak_ptr(p), _hold = this->shared_from_this()](auto&& x){
            auto p = _p.lock();
            if (p) (*p)(std::forward<decltype(x)>(x));
        });
        }

        return receiver<result_of_t_<signature_type>>(p);
    }

    template <typename P>
    bool task_done(const P& p) {
        if (!p->_process.done()) return false;

        bool ready;
        {
        std::unique_lock<std::mutex> lock(_message_mutex);
        ready = _message_queue.empty();
        _ready = ready;
        }
        if (ready) return true;
        task_chain(p);
        return true;
    }

    template <typename P>
    void task_body(const P& p) {
        // interuptable - check for message at each step
        bool hold = false;
        std::function<void()> message;
        {
            std::unique_lock<std::mutex> lock(_message_mutex);
            if (_message_queue.size()) {
                message = std::move(_message_queue.front());
                _message_queue.pop_front();
            }
            std::swap(hold, _hold);
        }

        if (message) {
            message();
            if (task_done(p)) { return; }
        }

        send_value(p->_process.yield());

        if (task_done(p)) { return; }

        task_chain(p);
    }

    template <typename P>
    void task_chain(const P& p) {
        default_scheduler()([_p = make_weak_ptr(p)]{
            // check for cancelation
            auto p = _p.lock();
            if (!p) return;
            p->task_body(p);
        });
    }

    template <typename P>
    void task_continue(const P& p) {
        std::unique_lock<std::mutex> lock(_message_mutex);
        // STOP HERE
        _ready;
    }

    template <typename P, typename... Args>
    void set_value(const P& p, Args&&... args) {
        bool ready = false;
        {
            std::unique_lock<std::mutex> lock(_message_mutex);
            _message_queue.emplace_back([=]{ p->_process.await(args...); });
            swap(ready, _ready);
            _hold = true; // TODO (sparent) : adjustable on queue size. Currently 1.
        }
        if (!ready) return;
        task_chain(p);
    }

    template <typename U>
    void send_value(U&& x) {
        map_t map;
        {
            std::unique_lock<std::mutex> lock(_map_mutex);
            swap(map, _map);
        }
        for (const auto& e : map) {
            default_scheduler()([=]{ e(x); });
        }

        // REVISIT (sparent): throw on insert below is problematic.
        {
            std::unique_lock<std::mutex> lock(_map_mutex);
            if (_map.empty()) swap(map, _map);
            else _map.insert(end(_map),
                    make_move_iterator(begin(map)), make_move_iterator(end(map)));
        }
    }
};

/**************************************************************************************************/

template <>
struct shared_channel_base<void> : std::enable_shared_from_this<shared_channel_base<void>> {
    bool                                   _ready{ true };
    std::deque<std::function<void()>>      _message_queue;
    std::mutex                             _message_mutex;

    // REVISIT - not used...
    virtual ~shared_channel_base() { }
    virtual void task_continue() = 0;

    template <typename P>
    bool task_done(const P& p) {
        if (!p->_process.done()) return false;

        bool ready;
        {
        std::unique_lock<std::mutex> lock(_message_mutex);
        ready = _message_queue.empty();
        _ready = ready;
        }
        if (ready) return true;
        task_chain(p);
        return true;
    }

    template <typename P>
    void task_body(const P& p) {
        // interuptable - check for message at each step
        std::function<void()> message;
        {
            std::unique_lock<std::mutex> lock(_message_mutex);
            if (_message_queue.size()) {
                message = std::move(_message_queue.front());
                _message_queue.pop_front();
            }
        }

        if (message) {
            message();
            if (task_done(p)) { return; }
        }

        p->_process.yield();

        if (task_done(p)) { return; }

        task_chain(p);
    }

    template <typename P>
    void task_chain(const P& p) {
        default_scheduler()([_p = make_weak_ptr(p)]{
            // check for cancelation
            auto p = _p.lock();
            if (!p) return;
            p->task_body(p);
        });
    }

    template <typename P, typename... Args>
    void set_value(const P& p, Args&&... args) {
        bool ready = false;
        {
            std::unique_lock<std::mutex> lock(_message_mutex);
            _message_queue.emplace_back([=]{ p->_process.await(args...); });
            swap(ready, _ready);
        }
        if (!ready) return;
        task_chain(p);
    }
};

/**************************************************************************************************/

template <typename Sig>
struct shared_channel : shared_channel_base<result_of_t_<Sig>> {

    template <typename T> struct any_process;
    template <typename R, typename... Args>
    struct any_process<R (Args...)> {
        struct concept {
            virtual ~concept() { }
            virtual std::unique_ptr<concept> copy() const = 0;
            virtual void await(Args... args) = 0;
            virtual R yield() = 0;
            virtual bool done() = 0;

        };

        template <typename U>
        struct model : concept {
            U _process;
            model(U x) : _process(std::move(x)) { }
            std::unique_ptr<concept> copy() const { return std::make_unique<model>(_process); }
            void await(Args... args) { _process.await(args...); }
            R yield() { return _process.yield(); }
            bool done() { return _process.done(); }
        };

        std::unique_ptr<concept> _p;

        template <typename U>
        any_process(U&& x) : _p(std::make_unique<model<U>>(std::forward<U>(x))) { }
        any_process(const any_process& x) : _p(x._p->copy()) { }
        any_process(any_process&&) noexcept = default;
        any_process& operator=(const any_process& x) {
            auto tmp = x; *this = std::move(tmp); return *this;
        }
        any_process& operator=(any_process&&) noexcept = default;

        template <typename... A>
        void await(A&&... args) { _p->await(std::forward<A>(args)...); }
        auto yield() { return _p->yield(); }
        bool done() const { return _p->done(); }
    };

    using process_type = any_process<Sig>;
    process_type _process;

    template <typename F>
    explicit shared_channel(F&& process) : _process(std::forward<F>(process)) { }

    std::shared_ptr<shared_channel> shared_from_this() {
        return std::static_pointer_cast<shared_channel>(this->shared_channel_base<result_of_t_<Sig>>::shared_from_this());
    }

    void task_continue() override {
        this->task_continue(shared_from_this());
    }

    template <typename ...A>
    void operator()(A&&... args) {
        try {
            this->set_value(shared_from_this(), std::forward<A>(args)...);
        } catch(...) {
            // TODO: handle exception pipe
        }
    }
};

} // namespace detail

/**************************************************************************************************/

template <typename T>
class receiver {
    using ptr_t = std::shared_ptr<detail::shared_channel_base<T>>;
    ptr_t _p;

    explicit receiver(ptr_t p) : _p(std::move(p)) { }

    template <typename>
    friend class channel;

    template <typename>
    friend struct detail::shared_channel_base;
  public:

    receiver() = default;
    template <typename F>
    auto map(F&& f) const { return _p->map(std::forward<F>(f)); }
};

template <typename T, typename F>
auto operator|(const receiver<T>& x, F&& f) {
    return x.map(std::forward<F>(f));
}

/**************************************************************************************************/

template <typename F> struct function_process;

template <typename R, typename... Args>
struct function_process<R (Args...)> {
    std::function<R (Args...)> _f;
    std::function<R ()> _bound;
    bool _done = false;

    using signature_type = R(Args...);

    template <typename F>
    function_process(F&& f) : _f(std::forward<F>(f)) { }

    template <typename... A>
    void await(A&&... args) {
        _bound = std::bind(std::move(_f), std::forward<A>(args)...);
        _done = false;
    }

    R yield() { _done = true; return _bound(); }
    bool done() const { return _done; }
};

/**************************************************************************************************/

template <typename T>
class channel {
    using ptr_t = std::weak_ptr<detail::shared_channel<T (T)>>;

    ptr_t _p;
  public:
    channel() = default;

    channel(const channel&) = default;
    channel(channel&&) noexcept = default;

    channel& operator=(const channel& x) { auto tmp = x; *this = std::move(tmp); return *this; }
    channel& operator=(channel&& x) noexcept = default;

    ~channel() = default;

    receiver<T> get_receiver() {
        auto p = _p.lock();
        if (!p) {
            p = std::make_shared<detail::shared_channel<T(T)>>(function_process<T(T)>([](T x){ return x; }));
            _p = p;
        }
        return receiver<T>(p);
    }

    template <typename... A>
    void operator()(A&&... args) const {
        auto p = _p.lock();
        if (p) (*p)(std::forward<A>(args)...);
    }
};

template <typename T, typename F>
auto operator|(channel<T>& x, F&& f) {
    return x.get_receiver().map(std::forward<F>(f));
}

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif

#if 0

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/

#include <stlab/channel.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace stlab;

struct iota {
	int _count;
	int _n;
	process_state _state = process_state::await;

    using signature = int(int);

	void await(int n) {
		_count = 0;
		_n = n;
		if (_count != _n) _state = process_state::yield;
	}

	int yield() {
		int result = _count;
		++_count;
		_state = (_count != _n) ? process_state::yield : process_state::await;
        return result;
	}

    void close() { }

	process_state state() const { return _state; }
};

struct word {
    string _word;
    process_state _state = process_state::await;

    using signature = string(char);

    void await(char x) {
        if (x == ' ' || x == '\0') _state = process_state::yield;
        else _word += x;
    }

    string yield() {
        _state = process_state::await;
        string result;
        swap(_word, result);
        return result;
    }

    void close() {
        if (!_word.empty()) _state = process_state::yield;
    }

    process_state state() const { return _state; }
};

struct line {
    const size_t length = 30;
    string _line;
    string _result;
    process_state _state = process_state::await;

    using signature = string(string);

    void await(string x) {
        if ((length - _line.size() < x.size()) || x.size() == 0) {
            _result = move(_line);
            _line = move(x);
            _state = process_state::yield;
        } else _line += x;
    }

    string yield() {
        _state = process_state::await;
        return move(_result);
    }

    void close() {
        if (!_line.empty()) {
            _result = move(_line);
            _state = process_state::yield;
        }
    }

    process_state state() const { return _state; }
};

class empty { };

int main() {
    channel<char> sender;

    auto receiver = sender | word() | line() | function_process<empty(string)>([](auto x){
        cout << x << endl; return empty();
    });

    for (char e : "This is a very very very long, as in supper long string, that needs to be formatted.") {
        sender(e);
    }
    sender.close();
    
    sleep(1000);
}

#endif

#if 0

class empty { };

int main() {

    channel<int> sender;

#if 0
    auto receiver = sender | function_process<empty(int)>([](auto x){
        cout << x << endl; return empty();
    });
#endif

#if 1
    auto receiver = sender | iota() | function_process<empty(int)>([](auto x){
        cout << x << endl; return empty();
    });
#endif

#if 0
    auto receiver = sender.get_receiver();

    auto r0 = receiver.map(count_to_10_process());

    auto r = r0.map(function_process<void_(int)>([](auto x){
        cout << x << endl; return void_();
    }));
#endif

    sender(5);
    sender(42);
    
    sleep(100);

    cout << "done" << endl;


#if 0
    process p;
    stlab::future<int> start = make_ready_future(3);
    stlab::future<void_> serialize = make_ready_future(void_());
    join(p, serialize, start, [](int x){ cout << x << endl; });
#endif

#if 0
    {

    stlab::future<int> f1;
    stlab::future<void_> f2 = make_ready_future(void_());

    int f = 0;
    int l = 10;

    recursive(f, l, f1, f2);

    sleep(100);

    }
#endif
}

#endif


#if 0

#include <iostream>
#include <stlab/future.hpp>

#include <boost/optional.hpp>

using namespace stlab;
using namespace std;
using namespace boost;

struct sender {
    int _x = 0;

    auto yield() {
        if (_x == 10) return optional<int>();
        return optional<int>(_x++);
    }
};

struct receiver {
    void await(int x) {
        cout << x << endl;
    }
};

template <typename S, typename R>
auto async_coroutines(S s, R r) {
    return async(default_scheduler(), [=]() mutable { return s.yield(); }).then([=](auto e) mutable {
        if (!e) return;
        r.await(e.get());
        return async_coroutines(s, r);
    });
}

#if 0
stlab::future<optional<int>> recurse(coroutine cr) {
    return async(default_scheduler(), [=]() mutable { return cr.await(); }).then(
            [=](auto x) -> stlab::future<optional<int>> mutable {
                if (!x) return x;
                cout << x << endl;
                return recurse(cr);
            });
}
#endif

int main() {
    int     _shared = 0;
    mutex   _mutex;
    bool    _sent = false;
    bool    _done = false;
    condition_variable _ready;

    auto f = async(default_scheduler(), [&]{
        int x = 0;
        while (x != 10) {
            {
            unique_lock<mutex> lock(_mutex);
            while (_sent) _ready.wait(lock);
            _shared = x;
            _sent = true;
            }
            _ready.notify_one();
            ++x;
        }
        {
        unique_lock<mutex> lock(_mutex);
        while (_sent) _ready.wait(lock);
        _done = true;
        _sent = true;
        }
        _ready.notify_one();
    });

    while (true) {
        int _out = 0;
        {
        unique_lock<mutex> lock(_mutex);
        while (!_sent && !_done) _ready.wait(lock);
        if (_done) break; // => done
        _out = _shared;
        _sent = false;
        }
        _ready.notify_one();
        cout << _shared << endl;
    }

    sender cr;
    receiver r;
    for (auto e = cr.yield(); e; e = cr.yield()) {
        r.await(e.get());
    }

    {
        auto f = async_coroutines(sender(), receiver());
        while (!f.get_try());
    }

    while (!f.get_try());
}

#endif

#if 0

#include <iostream>
#include <set>
#include <bitset>
#include <string>

using namespace std;

int toggle_a(int x) {
    if (x & 0b10) x^= 0b0001110;
    else x^= 0b1010010;
    return x;
}

int toggle_b(int x) {
    if (x & 0b01) x^= 0b0110101;
    else x^= 0b0000101;
    return x;
}

bool try_value(int value, set<int>& tried, int indent) {
    if (tried.count(value)) {
        cout << string(indent, ' ') << "dup:" << bitset<7>(value) << endl;
        return false;
    }
    cout << string(indent, ' ') << "try:" << bitset<7>(value) << endl;
    tried.insert(value);
    if ((value & 0b1111100) == 0b1111100) return true;

    if (try_value(toggle_a(value), tried, indent + 1)) {
        cout << bitset<7>(toggle_a(value)) << endl;
        return true;
    }
    if (try_value(toggle_b(value), tried, indent + 1)) {
        cout << bitset<7>(toggle_b(value)) << endl;
        return true;
    }
    return false;
}

int main() {
    set<int> tried;
    int value = 0;
    int indent = 0;
    if (try_value(value, tried, indent + 1)) {
        cout << bitset<7>(value) << endl;
    } else {
        cout << "no solution found" << endl;
    }
}



#endif


#if 0

#include <iostream>

#define ADOBE_STD_SERIALIZATION

#include <adobe/dictionary.hpp>
#include <adobe/iomanip_pdf.hpp>
#include <adobe/name.hpp>
#include <adobe/array.hpp>

using namespace adobe;
using namespace adobe::literals;
using namespace std;

int main() {

    dictionary_t d;
    d["array"_name] = array_t({ "name"_name, false, empty_t() });
    d["key"_name] = 42;
    d["key2"_name] = "Hello World!";

    cout << begin_pdf << d << end_pdf << endl;
}

#endif

#if 0

#include <algorithm>
#include <random>
#include <iostream>

using namespace std;

template <typename I, typename N>
void sort_within_n(I f, I l, N n) {
    while ((l - f) < (2 * n)) {
        partial_sort(f, f + n, f + (2 * n));
        f += n;
    }
    sort(f, l);
}

template <typename I, typename O>
void for_each_pair(I f, I l, O o) {
    while (f != l) {
        auto n = f; ++n;
        if (n == l) return;
        o(*f, *n);
        ++f; ++f;
    }
}

template <typename I, typename N>
void shuffle_within_n(I f, I l, N n) {
    std::random_device rd;
    std::mt19937 gen(rd());
    auto r = uniform_int_distribution<int>(0, 1);

    while (n != 0) {
        auto s = f + (n % 2);
        for_each_pair(s, l, [&](auto& x, auto& y) {
            if (r(gen)) swap(x, y);
        });
        --n;
    }
}



int main() {
    int a[100];
    iota(begin(a), end(a), 0);
    shuffle_within_n(begin(a), end(a), 50);

    sort_within_n(begin(a), end(a), 50);

    for (const auto& e : a) {
        cout << e << endl;
    }

}

#endif


#if 0
#include <utility>
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <memory>


using namespace std;
using namespace std::chrono;



template <typename I,
          typename P>
auto stable_partition_position(I f, I l, P p) -> I
{
    auto n = l - f;
    if (n == 0) return f;
    if (n == 1) return f + p(f);
    
    auto m = f + (n / 2);

    return rotate(stable_partition_position(f, m, p),
                  m,
                  stable_partition_position(m, l, p));
}

int main() {

int  a[] = { 1, 2, 3, 4, 5, 5, 4, 3, 2, 1 };
bool b[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0 };

auto p = stable_partition_position(begin(a), end(a), [&](auto i) {
    return *(begin(b) + (i - begin(a))); });

for (auto f = begin(a), l = p; f != l; ++f) cout << *f << " ";
cout << "^ ";
for (auto f = p, l = end(a); f != l; ++f) cout << *f << " ";
cout << endl;

}


#endif


#if 0
#include <utility>
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <memory>


using namespace std;
using namespace std::chrono;

#if 1

class UIElement { };

class UIElementCollection {
  public:
    void Add(shared_ptr<UIElement>);
};

class Panel : public UIElement {
  public:
    shared_ptr<UIElementCollection> Children() const;
};

//...

void f() {
    shared_ptr<Panel> panel;
    shared_ptr<UIElement> element;

    panel->Children()->Add(element);
    panel->Children()->Add(element);
    panel->Children()->Add(panel);
}



#endif

const size_t num = 32;
pair<int, int> a[num];

__attribute__ ((noinline)) int binary(int* f, int* l, int n) {
    int* p = f;
    int r;
    for (; n != 0; --n) {
        r = lower_bound(begin(a), end(a), *p,
            [](auto x, auto y){ return x.first < y; })->second;
        if (++p == l) p = f;
    }
    return r;
}

__attribute__ ((noinline)) int linear(int* f, int* l, int n) {
    int* p = f;
    int r;
    for (; n != 0; --n) {
        r = find_if(begin(a), end(a),
            [&](auto x){ return x.first == *p; })->second;
        if (++p == l) p = f;
    }
    return r;
}

map<int, int> m_;

__attribute__ ((noinline)) int map_table(int* f, int* l, int n) {
    int* p = f;
    int r;
    for (; n != 0; --n) {
        r = m_.find(*p)->second;
        if (++p == l) p = f;
    }
    return r;
}

unordered_map<int, int> u_;

__attribute__ ((noinline)) int unordered_map_table(int* f, int* l, int n) {
    int* p = f;
    int r;
    for (; n != 0; --n) {
        r = u_.find(*p)->second;
        if (++p == l) p = f;
    }
    return r;
}

int main() {

    int n = 0;
    int m = 10;
    generate(begin(a), end(a), [&](){ return make_pair(n++, m++); });
    int key[num];

    iota(begin(key), end(key), 0);
    shuffle(begin(key), end(key), default_random_engine{});

    m_.insert(begin(a), end(a));
    u_.insert(begin(a), end(a));
    {
    auto start = high_resolution_clock::now();

    auto result = linear(begin(key), end(key), 100000);

    cout << "linear result: " << result << " time: "
        << duration_cast<microseconds>(high_resolution_clock::now()-start).count()
        << endl;
    }
    {
    auto start = high_resolution_clock::now();

    auto result = map_table(begin(key), end(key), 100000);

    cout << "map result: " << result << " time: "
        << duration_cast<microseconds>(high_resolution_clock::now()-start).count()
        << endl;
    }
    {
    auto start = high_resolution_clock::now();

    auto result = unordered_map_table(begin(key), end(key), 100000);

    cout << "unordered map result: " << result << " time: "
        << duration_cast<microseconds>(high_resolution_clock::now()-start).count()
        << endl;
    }

    {
    auto start = high_resolution_clock::now();

    auto result = binary(begin(key), end(key), 100000);

    cout << "binary result: " << result << " time: "
        << duration_cast<microseconds>(high_resolution_clock::now()-start).count()
        << endl;
    }

}

#endif


#if 0

#include <adobe/forest.hpp>
#include <iostream>
#include <algorithm>
#include <utility>
#include <iterator>

using namespace std;
using namespace adobe;

template <typename I> // I is a depth adaptor range iterator
void output(I& f, I& l)
{
    while (f != l) {
        for (auto i = f.depth(); i != 0; --i) cout << "\t";

        if (f.edge() == forest_leading_edge) cout << "<" << *f << ">" << endl;
        else cout << "</" << *f << ">" << endl;

        ++f;
    }
}

int main() {
    forest<string> f;

    f.insert(end(f), "A");
    f.insert(end(f), "E");

    auto a = trailing_of(begin(f));
    f.insert(a, "B");
    f.insert(a, "C");
    f.insert(a, "D");

    auto r = depth_range(f);
    output(r.first, r.second);
}


#endif

#if 0

#include <algorithm>
#include <utility>
#include <cassert>
#include <iostream>

using namespace std;

enum x { b, c, d };
enum y { e = 10, f, g };

y x_to_y(x x_) {
    const pair<x, y> a[] = { { b, e }, { c, f }, { d, g } };
    assert(is_sorted(begin(a), end(a),[](auto a, auto b){ return a.first < b.first; }));
    auto p = lower_bound(begin(a), end(a), x_, [](auto a, auto b){ return a.first < b; });
    assert(p != end(a));
    return p->second;
}

int main() {
    cout << x_to_y(c) << endl;
}

#endif


#if 0

#include <cstddef>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <map>

using namespace std;
using namespace std::chrono;

#define TEST_MAP 1

#if TEST_MAP
template <typename C, typename T>
__attribute__ ((noinline)) void insert(C& c, const T& a) {
    c.insert(c.end(), begin(a), end(a));
}

template <typename C, typename T>
__attribute__ ((noinline)) void extract(C& c, const T& a) {
    c.insert(begin(a), end(a));
}
#endif

template <typename I> // I models RandomAccessIterator
void sort_subrange(I f, I l, I sf, I sl)
{
    if (sf == sl) return;
    if (f != sf) {
        nth_element(f, sf, l);
        ++sf;
    }
    partial_sort(sf, sl, l);
}

template <typename T>
__attribute__ ((noinline)) void sort(T& x) {
    sort(begin(x), end(x));
}

template <typename T>
__attribute__ ((noinline)) void partial(T& x) {
    sort_subrange(begin(x), end(x), begin(x) + 5000, begin(x) + 5100);
}

int main() {
    int x[]={ 4, 13, 12, 7, 9, 5, 15, 14, 2, 11, 6, 16, 10, 1, 8, 3 };

    int *f = begin(x), *sf = f + 5, *sl = f + 9, *l = end(x);
    int *nl = sl + 3;

    sort_subrange(f, l, sf, sl);
    partial_sort(sl, nl, l);

    for (const auto& e : x) cout << e << "; ";

#if 0
    nth_element(begin(x), begin(x) + 5, end(x));
    for (const auto& e : x) cout << e << "; ";
    cout << endl;
    partial_sort(begin(x) + 6, begin(x) + 9, end(x));
    for (const auto& e : x) cout << e << "; ";
    cout << endl;
#endif


    #if 0
    sort_subrange(begin(x), end(x), begin(x) + 50, begin(x) + 60);
    for (const auto& e : x) cout << e << endl;
    #endif

#if 0
    {
    auto start = chrono::high_resolution_clock::now();

    partial(x);

    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;
    }

    shuffle(begin(x), end(x), default_random_engine{});

    {
    auto start = chrono::high_resolution_clock::now();

    sort(x);

    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;
    }
#endif

}


#endif


#if 0

#include <cstddef>
#include <iostream>
#include <list>
#include <forward_list>
#include <vector>

using namespace std;

struct counter {
    struct { size_t *_m, *_c, *_ma, *_ca; } _counts;

    counter(size_t& m, size_t& c, size_t& ma, size_t& ca) :
        _counts{&m, &c, &ma, &ca}
    { }
    counter(const counter& x) : _counts(x._counts) { ++(*_counts._c); }
    counter(counter&& x) noexcept : _counts(x._counts) { ++(*_counts._m); }
    counter& operator=(const counter& x) { _counts = x._counts; ++(*_counts._ca); return *this; }
    counter& operator=(counter&& x) noexcept { _counts = x._counts; ++(*_counts._ma); return *this; }
};

template <typename T>
void test(const char* name) {
    size_t m{0}, c{0}, ma{0}, ca{0};
    counter n(m, c, ma, ca);
    T fl(100, n);
    m = 0, c = 0, ma = 0, ca = 0;
    rotate(begin(fl), next(begin(fl), 20), end(fl));
    cout << name << "{ move:" << m << ", copy:" << c << ", move_assign:" << ma
        << ", copy_assign:" << ca << " }" << endl;
}

int main() {
    test<forward_list<counter>>("forward_list");
    test<list<counter>>("list");
    test<vector<counter>>("vector");
};

#endif


#if 0

#define NDEBUG

#include <iostream>
#include <cstdint>
#include <cstring>
#include <cassert>

constexpr bool str_equal(const char* x, const char * y)
{
    while (*x) {
        if (*x != *y) return false;
        ++x; ++y;
    }

    return !*y;
}

constexpr std::int32_t operator""_typeid(const char* s, std::size_t n) noexcept {
    if (str_equal(s, "Hello")) return 0;
    if (str_equal(s, "World")) return 1;
    throw 0;
}

int main()
{
    char a["Hello"_typeid];
    char b["World"_typeid];
    char c["Boogie"_typeid];

    std::cout << sizeof(a) << std::endl;
    std::cout << sizeof(b) << std::endl;
    std::cout << sizeof(c) << std::endl;
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
}

#endif



#if 0
#include <stlab/future.hpp>
#include <iostream>
#include <dispatch/dispatch.h>
#include <chrono>

using namespace std;
using namespace stlab;
using namespace std::chrono;

/**************************************************************************************************/

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

/**************************************************************************************************/

#if 0
auto schedule = [](auto f) // F is void() and movable
{
    using f_t = decltype(f);

    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
            new f_t(std::move(f)), [](void* f_) {
                auto f = static_cast<f_t*>(f_);
                (*f)();
                delete f;
            });
};
#else
auto schedule = [](auto f) // F is void() and movable
{
    thread(move(f)).detach();
};

auto schedule_immediate = [](auto f)
{
    cout << "now!" << endl;
    f();
};
#endif

/**************************************************************************************************/
namespace stlab {

template <typename T>
class promise {
    stlab::packaged_task<T(T)> _f;
  public:
    template <typename S>
    stlab::future<T> get_future(S&& s) {
        stlab::future<T> result;
        std::tie(_f, result) = package<T(T)>(std::move(s), [](T x){ return x; });
        return result;
    }

    void set_value(T value) {
        _f(std::move(value));
    }
};

} // namespace stlab

/**************************************************************************************************/

int main() {
{
    stlab::promise<int> p;
    auto f = p.get_future(schedule);
    f.then([](int x){ cout << "promise fulfilled:" << x << endl; }).detach();
    p.set_value(42);
    std::this_thread::sleep_for(seconds(3));
}

    cout << "begin-test" << endl;
    {
    async(schedule, [](){ cout << "detached" << endl; }).detach();
    std::this_thread::sleep_for(seconds(3));
    }



    auto p = async(schedule_immediate, [](){ cout << "async" << endl; return 42; });
    auto p2 = p.then(schedule_immediate,[](auto x){
        cout << "x = " << x << endl;
    });
    std::this_thread::sleep_for(seconds(3));

    cout << "end-test" << endl;


    {
    #if 1
        auto p = async(schedule, []()->unique_ptr<int> { return make_unique<int>(42); });
        std::this_thread::sleep_for(seconds(1));
        auto x = std::move(p.get_try().get());
        cout << *x << endl;
    #endif
    }

    {
    auto p = async(schedule, []{ cout << "1" << endl; }).then([]{ cout << "2" << endl; });

    if (p.get_try()) { cout << "got it" << endl; }
    else { cout << "not yet" << endl; }

    std::this_thread::sleep_for(seconds(1));

    if (p.get_try()) { cout << "got it" << endl; }
    else { cout << "not yet" << endl; }
    }
    {
    auto p = async(schedule, []{ return annotate(); }).then([](auto x) -> annotate { return move(x); });

    std::this_thread::sleep_for(seconds(3));

    auto a = std::move(std::move(p).get_try().get());
    }
    std::this_thread::sleep_for(seconds(1));
    {
    cout << "-----" << endl;
    auto x = async(schedule, []{ return 10; });
    auto y = async(schedule, []{ return 5; });
    auto r = when_all(schedule, [](auto x, auto y){ return x + y; }, x, y);
    std::this_thread::sleep_for(seconds(1));
    cout << r.get_try().get() << endl;
    }
}

/**************************************************************************************************/
/**************************************************************************************************/




#endif














#if 0

#include <vector>
#include <thread>
#include <functional>
#include <iostream>

using namespace std;

template <typename>
struct result_of_;

template <typename R, typename... Args>
struct result_of_<R(Args...)> { using type = R; };

template <typename F>
using result_of_t_ = typename result_of_<F>::type;

using lock_t = unique_lock<mutex>;

/**************************************************************************************************/

template <typename S, typename R>
struct shared_base {
    vector<R> _r; // optional
    mutex _mutex;
    vector<function<void()>> _then;
    S _s;

    explicit shared_base(S s) : _s(move(s)) { }
    
    virtual ~shared_base() { }
    
    void set(R&& r) {
        vector<function<void()>> then;
        {
            lock_t lock{_mutex};
            _r.push_back(move(r));
            swap(_then, then);
        }
        for (const auto& f : then) _s(move(f));
    }
    
    template <typename F>
    void then(F&& f) {
        bool resolved{false};
        {
            lock_t lock{_mutex};
            if (_r.empty()) _then.push_back(forward<F>(f));
            else resolved = true;
        }
        if (resolved) _s(move(f));
    }
};

template <typename> struct shared; // not defined

template <typename S, typename R, typename... Args>
struct shared<R(Args...)> : shared_base<S, R> {
    function<R(Args...)> _f;
    
    template<typename F>
    shared(F&& f) : _f(forward<F>(f)) { }
    
    template <typename... A>
    void operator()(A&&... args) {
        this->set(_f(forward<A>(args)...));
        _f = nullptr;
    }
};

template <typename> class packaged_task; //not defined
template <typename> class future;

template <typename S, typename F>
auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;

template <typename R>
class future {
    shared_ptr<shared_base<R>> _p;
    
    template <typename S, typename F>
    friend auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;
    
    explicit future(shared_ptr<shared_base<R>> p) : _p(move(p)) { }
 public:
    future() = default;
    
    template <typename F>
    auto then(F&& f) {
        auto pack = package<result_of_t<F(R)>()>([p = _p, f = forward<F>(f)](){
            return f(p->_r.back());
        });
        _p->then(move(pack.first));
        return pack.second;
    }
    
    const R& get() const { return _p->get(); }
};

template<typename R, typename ...Args >
class packaged_task<R (Args...)> {
    weak_ptr<shared<R(Args...)>> _p;
    
    template <typename S, typename F>
    friend auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;
    
    explicit packaged_task(weak_ptr<shared<R(Args...)>> p) : _p(move(p)) { }
    
 public:
    packaged_task() = default;
    
    template <typename... A>
    void operator()(A&&... args) const {
        auto p = _p.lock();
        if (p) (*p)(forward<A>(args)...);
    }
};

template <typename S, typename F>
auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>> {
    auto p = make_shared<shared<S>>(forward<F>(f));
    return make_pair(packaged_task<S>(p), future<result_of_t_<S>>(p));
}

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
{
    using result_type = result_of_t<F (Args...)>;
    using packaged_type = packaged_task<result_type()>;
    
    auto pack = package<result_type()>(bind(forward<F>(f), forward<Args>(args)...));

#if 0
    _system.async_(move(get<0>(pack)));
#else
    thread(move(get<0>(pack))).detach(); // Replace with task queue
#endif
    return get<1>(pack);
}

int main() {
    future<int> x = async([]{ return 100; });
    
    future<int> y = x.then([](const int& x){ return int(x * 2); });
    future<int> z = x.then([](const int& x){ return int(x / 15); });
                                                            
    cout << y.get() << endl;
    cout << z.get() << endl;
}

#endif


#if 0

#include <iostream>

//#include <future>

#include <boost/thread/future.hpp>

using namespace std;
using namespace boost;



int main() {

    
    promise<int> x;
    {
        auto y = x.get_future().map([](auto x) { });
    }




#if 0
    auto z = when_all(x, y).then([](auto _x, auto _y){
        cout << _x + _y << endl;
    });
#endif


#if 0
    future<csbl::tuple<future<int>, future<int>>> w = when_all(std::move(x), std::move(y));

    future<void> z = w.then([](auto result) {
        auto tup = result.get();
        cout << get<0>(tup).get() + get<1>(tup).get() << endl;
    });
    z.wait();
#endif

}



#endif















#if 0

#define BOOST_THREAD_PROVIDES_EXECUTORS 1

#include <iostream>
#include <boost/thread/future.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <chrono>
#include <array>

#include <boost/thread/executors/basic_thread_pool.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <dispatch/dispatch.h>

using namespace std;
using namespace boost;
using namespace std::chrono;
using namespace boost::multiprecision;

namespace adobe {

template <typename F, typename ...Args>
auto async_main(F&& f, Args&&... args)
        -> boost::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = boost::packaged_task<result_type ()>;

    auto p = new packaged_type(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto result = p->get_future();
    dispatch_async_f(dispatch_get_main_queue(),
            p, [](void* f_) {
                auto f = static_cast<packaged_type*>(f_);
                (*f)();
                delete f;
            });
    
    return result;
}

template <typename T>
class detached_future {
    struct shared {
        boost::future<T> _future;
        ~shared() {
            if (_future.valid()) async_main([_f = std::move(_future)]{});
        }
    };

    std::shared_ptr<shared> _shared;
  public:
    detached_future() : _shared(std::make_shared<shared>()) { }
    detached_future& operator=(boost::future<T>&& x) { _shared->_future = std::move(x); return *this; }
};

class executor_main
{
  public:
    executor_main(basic_thread_pool const&) = delete;
    executor_main& operator=(executor_main const&) = delete;

    constexpr executor_main() { }

    void close() { }
    bool closed() { return false; }

    template <typename Closure>
    void submit(Closure&& closure) const {
        using closure_t = std::remove_reference_t<Closure>;
        auto p = new closure_t(std::forward<Closure>(closure));
        dispatch_async_f(dispatch_get_main_queue(),
                p, [](void* f_) {
                    auto f = static_cast<closure_t*>(f_);
                    (*f)();
                    delete f;
                });
    }

    bool try_executing_one() { return false; }

    template <typename Pred>
    bool reschedule_until(Pred const& pred) { return false; }
};

constexpr executor_main execute_main;

class executor_default
{
  public:
    executor_default(basic_thread_pool const&) = delete;
    executor_default& operator=(executor_default const&) = delete;

    constexpr executor_default() { }

    void close() { }
    bool closed() { return false; }

    template <typename Closure>
    void submit(Closure&& closure) const {
        using closure_t = std::remove_reference_t<Closure>;
        auto p = new closure_t(std::forward<Closure>(closure));
        dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                p, [](void* f_) {
                    auto f = static_cast<closure_t*>(f_);
                    (*f)();
                    delete f;
                });
    }

    bool try_executing_one() { return false; }

    template <typename Pred>
    bool reschedule_until(Pred const& pred) { return false; }
};

constexpr executor_default execute_default;

} // namespace adobe

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

int main() {
    promise<int> x;
    {
        auto y = x.get_future().then(launch::deferred, [](auto x){});
    }
    x.set_value(10);


#if 0

    auto x = boost::async(adobe::execute_default, [](){ return 0; });

        auto y = x.then(adobe::execute_default, [](auto f){
            cout << "begin" << endl;
            f.get(); // deadlock here
            cout << "end" << endl;
        });
    cout << "done" << endl;
#endif
}

#endif


#if 0

#include <iostream>
#include <boost/thread/future.hpp>
#include <thread>
#include <chrono>
#include <array>

#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost;
using namespace std::chrono;
using namespace boost::multiprecision;

/**************************************************************************************************/

template <typename T>
auto split(future<T>& x) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future(); // replace x with new future
    return tmp.then([_p = std::move(p)](auto _tmp) mutable {
        if (_tmp.has_exception()) {
            auto error = _tmp.get_exception_ptr();
            _p.set_exception(error);
            rethrow_exception(error);
        }

        auto value = _tmp.get();
        _p.set_value(value); // assign to new "x" future
        return value; // return value through future result
    });
}

/**************************************************************************************************/

template <typename T, typename N, typename O>
T power(T x, N n, O op)
{
    if (n == 0) return identity_element(op);
    
    while ((n & 1) == 0) {
        n >>= 1;
        x = op(x, x);
    }
    
    T result = x;
    n >>= 1;
    while (n != 0) {
        x = op(x, x);
        if ((n & 1) != 0) result = op(result, x);
        n >>= 1;
    }
    return result;
}

/**************************************************************************************************/

template <typename N>
struct multiply_2x2 {
    std::array<N, 4> operator()(const std::array<N, 4>& x, const std::array<N, 4>& y)
    {
        return { x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
            x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3] };
    }
};

template <typename N>
std::array<N, 4> identity_element(const multiply_2x2<N>&) { return { N(1), N(0), N(0), N(1) }; }

template <typename R, typename N>
R fibonacci(N n) {
    if (n == 0) return R(0);
    return power(std::array<R, 4>{ 1, 1, 1, 0 }, N(n - 1), multiply_2x2<R>())[0];
}

/**************************************************************************************************/




int main() {
    promise<int> x;
    future<int> y = x.get_future();

    x.set_value(42);
    cout << y.get() << endl;

    
    future<cpp_int> z = async([]{ return fibonacci<cpp_int>(1'000); });
    cout << z.get() << endl;



#if 0
    future<cpp_int> x = async([]{
        throw runtime_error("failure");
        return fibonacci<cpp_int>(1'000'000);
    });
        
    // Do Something

    try {
        cout << x.get() << endl;
    } catch (const runtime_error& error) {
        cout << error.what() << endl;
    }
#endif


#if 0
    future<cpp_int> x = async([]{ return fibonacci<cpp_int>(100); });
    
    future<cpp_int> y = split(x).then([](future<cpp_int> x){ return cpp_int(x.get() * 2); });
    future<cpp_int> z = x.then([](future<cpp_int> x){ return cpp_int(x.get() / 15); });

    future<void> done = when_all(std::move(y), std::move(z)).then([](auto f){
        auto t = f.get();
        cout << get<0>(t).get() << endl;
        cout << get<1>(t).get() << endl;
    });

    done.wait();

#endif
}

#endif

















#if 0

#include <memory>
#include <iostream>

using namespace std;

struct cancel_base_ {
    virtual ~cancel_base_() {}
};

template <typename F>
struct cancel_model_ : cancel_base_ {
    cancel_model_(F f) : _f(move(_f)) { }
    F _f;
};


template <typename F>
class cancelable_task {
    weak_ptr<cancel_model_<F>> _shared;
public:
    cancelable_task(weak_ptr<cancel_model_<F>> x) : _shared(move(x)) { }

    void operator()() const {
        auto shared = _shared.lock();
        if (shared) shared->_f();
    }
};

class cancelation_token {
    template <typename F> friend auto make_cancelable(F f);

    cancelation_token(shared_ptr<cancel_base_> x) : _shared(move(x)) { }
    shared_ptr<cancel_base_> _shared;
};

template <typename F>
auto make_cancelable(F f) {
    auto x = make_shared<cancel_model_<F>>(move(f));
    return make_pair(cancelation_token(x), cancelable_task<F>(x));
}

int main() {
    auto p = make_cancelable([](){ cout << "called" << endl; });
    auto f = move(p.second);
    {
        auto x = move(move(p.first));
        f();
    }
    f();


}

#endif


#if 0
#define BOOST_THREAD_PROVIDES_EXECUTORS 1

#include <boost/thread/future.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>


#include <iostream>
#include <chrono>
#include <thread>

#include <utility>

using namespace boost;
using namespace std;
using namespace std::chrono;

template <typename T>
auto split(future<T>& x) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future();
    return tmp.then([_p = std::move(p)](auto x) mutable {
        if (x.has_exception()) {
            auto error = x.get_exception_ptr();
            _p.set_exception(error);
            rethrow_exception(error);
        }

        auto value = x.get();
        _p.set_value(value);
        return value;
    });
}


int main() {

    promise<int> x;
    auto a = x.get_future().share();

    auto f1 = a.then([](auto x){ cout << "then 1:" << x.get() << endl; });
    auto f2 = a.then([](auto x){ cout << "then 2:" << x.get() << endl; });

    x.set_value(42);

    std::this_thread::sleep_for(seconds(10));

#if 0
    basic_thread_pool pool;

    promise<int> x;
    auto a = x.get_future();

    auto f1 = split(a).then(pool, [](auto x){ cout << "then 1:" << x.get() << endl; });
    auto f2 = split(a).then(pool, [](auto x){ cout << "then 2:" << x.get() << endl; });

    x.set_value(42);

    std::this_thread::sleep_for(seconds(10));
#endif

}


#endif


#if 0

#include <iostream>
#include <future>

using namespace std;

int main() {
    future<int> y;


    {
        promise<int> x;
        y = x.get_future();
        try {
            throw 42;
        } catch (...) {
            x.set_exception(current_exception());
        }
    }
    try {
        cout << y.get() << endl;
    } catch (const int& error) {
        cout << error << endl;
    }
}

#endif

















#if 0


#include <cstddef>
#include <functional>
#include <tuple>

/**************************************************************************************************/

namespace stlab {
namespace details {

/**************************************************************************************************/

template <typename F, typename T, typename Args, std::size_t... I>
auto apply_1_(F&& f, T&& t, const Args& args, std::index_sequence<I...>) {
    return f(std::forward<T>(t),
        std::forward<typename std::tuple_element<I, Args>::type>(std::get<I>(args))...);
}

template <typename F, typename T, typename Args>
auto apply_1(F&& f, T&& x, const Args& args) {
    return apply_1_(std::forward<F>(f), std::forward<T>(x), args,
        std::make_index_sequence<std::tuple_size<Args>::value>());
}

/**************************************************************************************************/

template <typename F, typename T>
struct pipeable {
    F&& _f;
    T _args;

    template <typename U>
    auto operator()(U&& x) { return apply_1(std::forward<F>(_f), std::forward<U>(x), _args); }
};

template <typename T, typename F, typename... Args>
auto operator>>(T&& x, pipeable<F, Args...>&& op) { return op(std::forward<T>(x)); }

/**************************************************************************************************/

} // namespace details

/**************************************************************************************************/

template <typename F, typename... Args>
auto make_pipeable(F&& f, Args&&... args) {
    return details::pipeable<F, std::tuple<Args&&...>>{ std::forward<F>(f),
        std::forward_as_tuple(std::forward<Args>(args)...) };
}

/**************************************************************************************************/

} // namespace stlab

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

template <typename T>
auto add(T&& x) {
    return stlab::make_pipeable(
        [](auto&&, T&& x){
            return std::forward<T>(x);
        }, std::forward<T>(x));
}

int main() {
    auto x = annotate() >> add(annotate());
    x >> add(annotate());
    annotate() >> add(x);
}

#endif


#if 0

#include <functional>

using namespace std;

template <typename T>
class future_ {
    T _x;
  public:
    future_(T x) : _x(move(x)) { }
    T&& get() { return move(_x); }
};

int main() {
    // bind([](future_<int> x, auto f) { return f(x.get()); }, placeholders::_1, [](auto y){ return y + 10; })(future_<int>(42));

    bind([](future_<int> x, auto f) { return f(x.get()); }, placeholders::_1, [](auto y){ return y.get(); })(future_<int>(42));
}

#endif

#if 0
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <tuple>

#include <boost/thread/future.hpp>

using namespace std;
using namespace boost;
using namespace std::chrono;

/**************************************************************************************************/

template <typename F, typename T, typename Args, std::size_t... I>
auto apply_1_(F f, T&& t, Args&& args, std::index_sequence<I...>) {
    return f(std::forward<T>(t), std::get<I>(args)...);
}

template <typename F, typename T, typename Args>
auto apply_1(F f, T&& x, Args&& args) {
    return apply_1_(f, x, args,
        std::make_index_sequence<std::tuple_size<std::remove_reference_t<Args>>::value>());
}

/**************************************************************************************************/

template <typename F, typename... Args>
struct pipeable {
    F&& _f;
    std::tuple<Args&&...> _args;

    template <typename T>
    auto operator()(T&& x) { return apply_1(std::forward<F>(_f), std::forward<T>(x), _args); }
};

template <typename F, typename... Args>
auto make_pipeable(F&& f, Args&&... args) {
    return pipeable<F, Args...>{ std::forward<F>(f),
        std::forward_as_tuple(std::forward<Args>(args)...) };
}

template <typename T, typename F, typename... Args>
auto operator|(T&& x, pipeable<F, Args...>&& op) { return op(std::forward<T>(x)); }

/**************************************************************************************************/



template <typename F, typename... T>
auto apply(F f, future<std::tuple<future<T>...>>& x) {
    return apply([_f = move(f)](auto&... x){ return _f(x.get()...); }, x.get());
};

template <typename F, typename T>
auto apply(F f, future<T>& x) {
    return f(x.get());
}

template <typename... T>
auto unpack_future(future<std::tuple<future<T>...>>& x) {
    return apply([](auto&... x){ return make_tuple(x.get()...); }, x.get());
}

template <typename T>
auto unpack_future(future<T>& x) { return x.get(); }

template <typename T, typename F>
auto then(future<T>& x, F&& f) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future();
    return tmp.then([_f = std::forward<F>(f), _p = std::move(p)](auto x) mutable {
        auto value = [&](){
            try {
                return x.get();
            } catch(...) {
                _p.set_exception(boost::current_exception());
                throw;
            }
        }();
        _p.set_value(value);
        return _f(std::move(value));
    });
}

template <typename T, typename F>
auto then(future<T>&& x, F&& f) {
    return x.then([_f = std::forward<F>(f)](auto x) { return _f(x.get()); });
}

template <typename F>
auto then(F&& f) {
    return make_pipeable([](auto&& x, auto&& f) {
        return then(std::forward<decltype(x)>(x), std::forward<decltype(f)>(f));
    }, std::forward<F>(f));
}

#if 0
template <typename T, typename F>
void error(future<T>& x, F&& f) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future();
    tmp.then(boost::launch::deferred, [_f = move(f), _p = std::move(p)](auto x) mutable {
        auto value = [&](){
            try {
                return x.get();
            } catch(...) {
                auto error = boost::current_exception();
                _p.set_exception(error);
                _f(error);
                throw;
            }
        }();
        _p.set_value(value);
    });
}
#endif

int main() {
    promise<int> a_;

    auto x = a_.get_future() | then([](int y){ return y + 10; }) | then([](int x){ return x + 5; });
    auto y = x | then([](int x){ return x == 57 ? "success" : "fail"; });
    auto z = x | then([](int x){ return x / 3; });

    a_.set_value(42);

    cout << x.get() << " " << y.get() << " " << z.get() << endl;

#if 0
    future<int> a = a_.get_future();

#if 0
    error(a, [](auto x) {
        cout << "error called" << endl;
        try { rethrow_exception(x); }
        catch (const std::exception& x) {
            cout << "error: " << x.what() << endl;
        }
    });
#endif

    auto x = a_.get_future() | then([](auto x){ return x + 10; }) | then([](auto x){ return x + 5; });

#if 0
    auto x = then(a, [](auto x) {
        if (x == 42) return "Hello";
        return "fail";
    });

    auto y = then(a, [](auto x) {
        if (x == 42) return " World!";
        return "fail";
    });
#endif

    a_.set_value(42);

   // a_.set_exception(std::runtime_error("runtime_error"));

    try {
        cout << x.get();
    } catch (const std::exception& e) {
        cout << "catch 1: " << e.what() << endl;
    }

    try {
        cout << y.get() << endl;
    } catch (const std::exception& e) {
        cout << "catch 2: " << e.what() << endl;
    }

    std::this_thread::sleep_for(seconds(3));

#endif
#if 0
    promise<int> a_;
    promise<double> b_;
    future<int> a = a_.get_future();
    future<double> b = b_.get_future();

    //future<tuple<future<int>, future<int>>> x;
    //auto y = unpack_future(x);
    auto c = when_all(std::move(a), std::move(b));
    auto d = c.then([](auto x){ return apply([](int x, double y){ return x + y; }, x); });
   a_.set_value(42);
   b_.set_value(68);

   cout << d.get() << endl;
#endif

}

#endif


#if 0
#include <iostream>

using namespace std;

int main() {
    int x = 0;
    const auto f = [&](){ return ++x; };
    cout << f() << " " << f() << " " << f() << endl;

    const auto f2 = [](){ return make_shared<int>(42); };
    auto a = f2();
    auto b = f2();
}
#endif

#if 0

#include <iostream>
#include <typeinfo>
#include <string>
#include <memory>
#include <vector>

using namespace std;

template <typename T>
void draw(const T&);

class any {
    struct concept {
        virtual ~concept() { }
        virtual concept* copy() const = 0;
        virtual const type_info& type() const = 0;
        virtual bool equal(const concept& x) const = 0;
        virtual void draw_() const = 0;
    };

    template <typename T>
    struct model : concept {
        model(T x) : _data(move(x)) { }
        concept* copy() const { return new model(*this); }
        const type_info& type() const { return typeid(_data); }
        bool equal(const concept& x) const {
            return _data == static_cast<const model&>(x)._data;
        }
        void draw_() const { draw(_data); }
        T _data;
    };

    unique_ptr<concept> _model;
  public:

    template <typename T>
    any(T x) : _model(new model<T>(move(x))) { }
    any() = default;
    any(const any& x) : _model(x._model->copy()) { }
    any(any&&) noexcept = default;
    any& operator=(const any& x) { any tmp(x); *this = move(tmp); return *this; }
    any& operator=(any&&) noexcept = default;

    const type_info& type() const { return _model->type(); }

    template <typename T>
    T& get() { return dynamic_cast<model<T>&>(*_model)._data; }

    friend inline bool operator==(const any& x, const any& y) {
        return (x.type() == y.type()) && (x._model->equal(*y._model));
    }

    void draw_() const { _model->draw_(); }
};


template <typename T>
void draw(const T& x) { cout << x << endl; }

template <>
void draw<string>(const string& x) { cout << "string: " << x << endl; }

int main() {
    function<void()> f = []() { cout << "hello!" << endl; };
    int a = 10;
    any b = (ref(a));
    b.get<reference_wrapper<int>>().get() = 22;
    cout << a << endl;

#if 0
    any x(42.5);
    any y(string("Hello World"));
    // any z(vector<any>{ 27.3, string("nifty"), true});

    cout << (x == x) << endl;

    cout << x.get<double>() << endl;
    cout << y.get<string>() << endl;

    x.draw_();
    y.draw_();
#endif

#if 0
    cout << z.get<vector<any>>()[0].get<double>() << z.get<vector<any>>()[1].get<string>()
        << z.get<vector<any>>()[2].get<bool>() << endl;
#endif
}

#endif



















#if 0

#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>
#include <chrono>
#include <thread>

#include <boost/thread/future.hpp>

using std::decay_t;
using std::get;
using std::index_sequence;
using std::make_index_sequence;
using std::size_t;
using std::tuple;
using std::tuple_size;
using std::move;
using std::make_tuple;
using std::cout;
using std::endl;
using std::result_of_t;
using boost::future;
using boost::promise;
using boost::when_all;
using boost::packaged_task;
using boost::make_ready_future;

using namespace std::chrono;



template <typename F> // F models UnaryFunction
struct pipeable {
    F op_;

    template <typename T>
    auto operator()(T&& x) const { return op_(forward<T>(x)); }
};

template <typename F>
auto make_pipeable(F x) { return pipeable<F>{ move(x) }; }

template <typename F, typename... Args>
auto make_pipeable(F x, Args&&... args) {
    return make_pipeable(bind(move(x), placeholders::_1, forward<Args>(args)...));
}

template <typename F, typename T, size_t... I>
auto apply_(F f, T&& t, index_sequence<I...>) {
    return f(get<I>(t)...);
}

template <typename F, typename T>
auto apply(F f, T&& t) {
    return apply_(f, t, make_index_sequence<tuple_size<T>::value>());
}

template <typename F, typename... T>
auto apply(F f, future<tuple<future<T>...>>& x) {
    return apply([_f = move(f)](auto&... x){ return _f(x.get()...); }, x.get());
};

template <typename F, typename T>
auto apply(F f, future<T>& x) {
    return f(x.get());
}

template <typename... T>
auto unpack_future(future<tuple<future<T>...>>& x) {
    return apply([](auto&... x){ return make_tuple(x.get()...); }, x.get());
}

template <typename T>
auto unpack_future(future<T>& x) { return x.get(); }

template <typename T, typename F>
auto operator|(T&& x, const pipeable<F>& op) { return op(forward<T>(x)); }

template <typename T, typename F>
auto then(future<T>& x, F&& f) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future();
    return tmp.then([_f = move(f), _p = std::move(p)](auto x) mutable {
        auto value = [&](){
            try {
                return x.get();
            } catch(...) {
                _p.set_exception(boost::current_exception());
                throw;
            }
        }();
        _p.set_value(value);
        return _f(move(value));
    });
}

#if 0
template <typename T, typename F>
void error(future<T>& x, F&& f) {
    auto tmp = std::move(x);
    promise<T> p;
    x = p.get_future();
    tmp.then(boost::launch::deferred, [_f = move(f), _p = std::move(p)](auto x) mutable {
        auto value = [&](){
            try {
                return x.get();
            } catch(...) {
                auto error = boost::current_exception();
                _p.set_exception(error);
                _f(error);
                throw;
            }
        }();
        _p.set_value(value);
    });
}
#endif

int main() {
    promise<int> a_;
    future<int> a = a_.get_future();

#if 0
    error(a, [](auto x) {
        cout << "error called" << endl;
        try { rethrow_exception(x); }
        catch (const std::exception& x) {
            cout << "error: " << x.what() << endl;
        }
    });
#endif

    auto x = then(a, [](auto x) {
        if (x == 42) return "Hello";
        return "fail";
    });

    auto y = then(a, [](auto x) {
        if (x == 42) return " World!";
        return "fail";
    });

    a_.set_value(42);

   // a_.set_exception(std::runtime_error("runtime_error"));

    try {
        cout << x.get();
    } catch (const std::exception& e) {
        cout << "catch 1: " << e.what() << endl;
    }

    try {
        cout << y.get() << endl;
    } catch (const std::exception& e) {
        cout << "catch 2: " << e.what() << endl;
    }

    std::this_thread::sleep_for(seconds(3));


#if 0
    promise<int> a_;
    promise<double> b_;
    future<int> a = a_.get_future();
    future<double> b = b_.get_future();

    //future<tuple<future<int>, future<int>>> x;
    //auto y = unpack_future(x);
    auto c = when_all(std::move(a), std::move(b));
    auto d = c.then([](auto x){ return apply([](int x, double y){ return x + y; }, x); });
   a_.set_value(42);
   b_.set_value(68);

   cout << d.get() << endl;
#endif

}


#endif


#if 0

//
//  main.cpp
//  Homework3
//
//  Created by Russell Williams on 3/12/15.
//  Copyright (c) 2015 Russell Williams. All rights reserved.
//

#include <iostream>
#include <string.h>

// parse_exception captures the error message and current parse location

class parse_exception : public std::runtime_error {
public:
	std::string _parse_position;

	parse_exception (std::string msg, std::string parse_position) :
		std::runtime_error (msg), _parse_position (parse_position) { }
	};

// parse a simplified version of json: no escapes in strings and numbers
// are non-range-checked integers. White space can tokens otherwise not
// separated. Input is a pointer to a null-terminated UTF-8 string which
// must not be modified until the parse is complete. Errors cause a
// parse_exception exception to be thrown.
//
// All recognized grammar elements are printed to cout when recognized,
// but no other action is taken.
//
// It would be easy to modify this code to return lower level values back up
// to the object and array routines for insertion in a dictionary.
//
// may throw parse_exception

class json_parser {
public:
	// Construct a parser given a pointer to a null-terminated UTF-8 string

	explicit json_parser (const char* p) : _p(p) { }

	// parse the input

	void parse_json() {
		require (is_object (), "not object");
		skip_space ();
		require (!*_p, "extra characters after end of object");
		}

private:
	const char* _p;	// Anything that advances p must check for end of string

	// object = "{" {string : value {,string:value}} "}"
	// Yes, I could have added an initializer list rule and had two simpler rules

	bool is_object () {

		if (!is_token ("{"))
			return false;

		// empty object
		if (is_token ("}")) {
			recognized ("object");
			return true;
			}

		// first item in list
		require (is_string ("member name"), "member name required after \"{\"");
		require (is_token (":"), "\":\" required after member name");
		require (is_value (), "value required for member");

		while (is_token (",")) {
			require (is_string ("member name"), "member name required after \",\"");
			require (is_token (":"), "\":\" required after member name");
			require (is_value (), "value required for member");
			}

		require (is_token ("}"), "expected \"}\" to close object");
		recognized ("object");
		return true;
		}

	// array = "[" {value {, value} } "]"

	bool is_array () {
		if (!is_token ("["))
			return false;

		if (is_token ("]")) {
			recognized ("array");
			return true;
			}

		if (!is_value ())
			return false;

		while (is_token (",")) {
			require (is_value (), "expected value after \",\" in array");
			}

		require (is_token ("]"), "expected \"]\" to close array");
		recognized ("array");
		return true;
		}

	// string = """ { non-quote-character } """

	bool is_string (const char* label) {
		if (!is_token ("\""))
			return false;

		const char* start_compare = _p;
		while (*_p && !is_token ("\""))
			++_p;

		require (*_p, "unterminated string");
		recognized (label, start_compare, _p - start_compare-1);
		return true;
		}

	// number = ["-" digit {digit}

	bool is_number () {
		skip_space ();

		int  result = 0;
		bool positive = true;

		if (is_token ("-"))
			positive = false;

		bool found_digit = false;

		while (*_p && std::isdigit (*_p)) {
			found_digit = true;
			result = result * 10 + (*_p++ - '0');
			}

		if (positive && !found_digit)
			return false;

		require (found_digit, "expected digit");

		if (!positive)
			result = -result;

		recognized ("number", result);
		return true;
		}

	// true = "true"

	bool is_true () {
		if (is_token ("true")) {
			recognized ("true");
			return true;
			}
		else
			return false;
		}

	// false = "false"

	bool is_false () {
		if (is_token ("false")) {
			recognized ("false");
			return true;
			}
		else
			return false;
		}

	// null = "null"

	bool is_null () {
		if (is_token ("null")) {
			recognized ("null");
			return true;
			}
		else
			return false;
		}

	// value = string | number | object | array | true | false | null

	bool is_value () {
		return is_string ("string value") || is_number () || is_object () ||
			   is_array () || is_true () || is_false () || is_null ();

		}

	// look for specified token, possibly preceded by whitespace
	// advance _p only if it's found

	bool is_token (const char* token) {
		skip_space ();

		size_t compare_length = 0;

		const char* p_copy		=_p;
		const char* token_copy	= token;

		while (*p_copy && *token_copy && *p_copy == *token_copy)
			++p_copy, ++compare_length, ++token_copy;

		// See if we got to the end of the token

		if (*token_copy)
			return false;

		_p += compare_length;

		recognized ("token", token);
		return true;
		}

	void skip_space () {
		while (*_p && isspace (*_p))
			++_p;
		}

	void require (bool condition, const char* errorMsg) {
		if (!condition)
			throw parse_exception (errorMsg, _p);
		}

	// The recognized methods just print a message describing what has
	// been recognized; overloads print messages for different items

	void recognized (const char* label) {
		std::cout << "Recognized " << label << std::endl;
		}

	void recognized (const char* label, const char* value) {
		std::cout << "Recognized " << label << " " << value << std::endl;
		}

	void recognized (const char* label, const char* value, size_t n) {
		std::cout << "Recognized " << label << " ";
		std::cout.write (value, n);
		std::cout << std::endl;
		}

	void recognized (const char* label, int value) {
		std::cout << "Recognized " << label << " " << value << std::endl;
		}
	};

// Parse the specified string by constructing and then running a json_parser,
// then print the parse error (if any). Other exceptions pass through

void parse (const char* p) {
	json_parser parser(p);
	try {
		parser.parse_json ();
		}
	catch (const parse_exception& err)
		{
		std::cout << "Parse error: " << err.what () << " at \'" <<
			err._parse_position << "\'" << std::endl;
		}
	}

// Read and parse input lines until an empty line is entered

int main() {
		parse (R"(
            { "number" : 42, "array" : [ 42, "string", true] }
        )");

	}

#endif

#if 0
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

/*

expression = add_term.
add_term = mul_term { '+' mul_term }.
mul_term = number { '*' number }.
number = digit { digit }.

*/

int parse(const char*);

class parser {

    friend inline int parse(const char* p) {
        parser tmp(p);
        return tmp.expression();
    }

    const char* _p;

    explicit parser(const char* p) : _p(p) { }

    int expression() {
        int result;
        require(is_add_term(result), "add_term required");
        skip_space();
        if (*_p != '\0') throw runtime_error("not at eof");
        return result;
    }
    bool is_add_term(int& x) {
        int result;
        if (!is_mul_term(result)) return false;
        int arg;
        while (is_token("+")) {
            require(is_mul_term(arg), "mul_term required");
            result += arg;
        }
        x = result;
        return true;
    }
    bool is_mul_term(int& x) {
        int result;
        if (!is_number(result)) return false;
        int arg;
        while (is_token("*")) {
            require(is_number(arg), "number required");
            result *= arg;
        }
        x = result;
        return true;
    }

    bool is_number(int& x) {
       skip_space();
       if (!isdigit(*_p)) return false;
       int result = *_p - '0'; ++_p;
       while (isdigit(*_p)) { result = result * 10 + (*_p - '0'); ++_p; }
       x = result;
       return true;
    }

    bool is_token(const char* x) {
        skip_space();
        if (!(*mismatch(x, x + strlen(x), _p).first == '\0')) return false;
        _p += strlen(x);
        return true;
    }

    void skip_space() {
        while (isspace(*_p)) ++_p;
    }

    void require(bool x, const char* error) {
        if (!x) throw runtime_error(error);
    }

};

int main() {

    cout << parse("  3 + 67 * 100   ") << endl;

}

#endif





























#if 0

#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <cstring>
#include <iostream>

using namespace std;

int parse(const char* x);

class parse_expression {
    const char* _p;

    explicit parse_expression(const char* p) : _p(p) { }
    int evaluate() {
        int r;
        if (!is_add(r)) throw runtime_error("add required");
        while (isspace(*_p)) ++_p;
        if (*_p) throw runtime_error("end not reached");
        return r;
    }
    bool is_add(int& x) {
        int r;
        if (!is_mul(r)) return false;
        while (is_token("+")) {
            int a;
            if (!is_mul(a)) throw runtime_error("mul required");
            r += a;
        }
        x = r;
        return true;
    }
    bool is_mul(int& x) {
        int r;
        if (!is_number(r)) return false;
        while (is_token("*")) {
            int a;
            if (!is_number(a)) throw runtime_error("number required");
            r *= a;
        }
        x = r;
        return true;
    }
    bool is_number(int& x) {
        while (isspace(*_p)) ++_p;
        if (!isdigit(*_p)) return false;
        int result = *_p - '0'; ++_p;
        while (isdigit(*_p)) { result = result * 10 + (*_p - '0'); ++_p; }
        x = result;
        return true;
    }
    bool is_token(const char* x) {
        while (isspace(*_p)) ++_p;
        if (*mismatch(x, x + strlen(x), _p).first == '\0') return false;
        _p += strlen(x);
        return true;
    }

    friend inline int parse(const char* x) {
        parse_expression tmp(x);
        return tmp.evaluate();
    }
};

int main() {
    cout << parse("  5 *   10 + 3*2   ") << endl;
}

#endif




#if 0
// GOOD for talk

#include <deque>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

/**************************************************************************************************/

using namespace std;
using namespace boost::multiprecision;

/**************************************************************************************************/

template <typename T, typename N, typename O>
T power(T x, N n, O op)
{
    if (n == 0) return identity_element(op);
    
    while ((n & 1) == 0) {
        n >>= 1;
        x = op(x, x);
    }
    
    T result = x;
    n >>= 1;
    while (n != 0) {
        x = op(x, x);
        if ((n & 1) != 0) result = op(result, x);
        n >>= 1;
    }
    return result;
}

/**************************************************************************************************/

template <typename N>
struct multiply_2x2 {
    array<N, 4> operator()(const array<N, 4>& x, const array<N, 4>& y)
    {
        return { x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
            x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3] };
    }
};

template <typename N>
array<N, 4> identity_element(const multiply_2x2<N>&) { return { N(1), N(0), N(0), N(1) }; }

template <typename R, typename N>
R fibonacci(N n) {
    if (n == 0) return R(0);
    return power(array<R, 4>{ 1, 1, 1, 0 }, N(n - 1), multiply_2x2<R>())[0];
}

/**************************************************************************************************/

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;
    
public:
    bool try_pop(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock || _q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }
    
    template<typename F>
    bool try_push(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }
    
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }
    
    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
        while (_q.empty() && !_done) _ready.wait(lock);
        if (_q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }
    
    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

/**************************************************************************************************/

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};
    
    void run(unsigned i) {
        while (true) {
            function<void()> f;
            
            for (unsigned n = 0; n != _count * 32; ++n) {
                if (_q[(i + n) % _count].try_pop(f)) break;
            }
            if (!f && !_q[i].pop(f)) break;
            
            f();
        }
    }
    
public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }
    
    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }
    
    template <typename F>
    void async_(F&& f) {
        auto i = _index++;
        
        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].try_push(forward<F>(f))) return;
        }
        
        _q[i % _count].push(forward<F>(f));
    }
};

/**************************************************************************************************/

task_system _system;

/**************************************************************************************************/

template <typename>
struct result_of_;

template <typename R, typename... Args>
struct result_of_<R(Args...)> { using type = R; };

template <typename F>
using result_of_t_ = typename result_of_<F>::type;

/**************************************************************************************************/

template <typename R>
struct shared_base {
    vector<R> _r; // optional
    mutex _mutex;
    condition_variable _ready;
    vector<function<void()>> _then;
    
    virtual ~shared_base() { }
    
    void set(R&& r) {
        vector<function<void()>> then;
        {
            lock_t lock{_mutex};
            _r.push_back(move(r));
            swap(_then, then);
        }
        _ready.notify_all();
        for (const auto& f : then) _system.async_(move(f));
    }
    
    template <typename F>
    void then(F&& f) {
        bool resolved{false};
        {
            lock_t lock{_mutex};
            if (_r.empty()) _then.push_back(forward<F>(f));
            else resolved = true;
        }
        if (resolved) _system.async_(move(f));
    }
    
    const R& get() {
        lock_t lock{_mutex};
        while (_r.empty()) _ready.wait(lock);
        return _r.back();
    }
};

template <typename> struct shared; // not defined

template <typename R, typename... Args>
struct shared<R(Args...)> : shared_base<R> {
    function<R(Args...)> _f;
    
    template<typename F>
    shared(F&& f) : _f(forward<F>(f)) { }
    
    template <typename... A>
    void operator()(A&&... args) {
        this->set(_f(forward<A>(args)...));
        _f = nullptr;
    }
};

template <typename> class packaged_task; //not defined
template <typename> class future;

template <typename S, typename F>
auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;

template <typename R>
class future {
    shared_ptr<shared_base<R>> _p;
    
    template <typename S, typename F>
    friend auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;
    
    explicit future(shared_ptr<shared_base<R>> p) : _p(move(p)) { }
 public:
    future() = default;
    
    template <typename F>
    auto then(F&& f) {
        auto pack = package<result_of_t<F(R)>()>([p = _p, f = forward<F>(f)](){
            return f(p->_r.back());
        });
        _p->then(move(pack.first));
        return pack.second;
    }
    
    const R& get() const { return _p->get(); }
};

template<typename R, typename ...Args >
class packaged_task<R (Args...)> {
    weak_ptr<shared<R(Args...)>> _p;
    
    template <typename S, typename F>
    friend auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>>;
    
    explicit packaged_task(weak_ptr<shared<R(Args...)>> p) : _p(move(p)) { }
    
 public:
    packaged_task() = default;
    
    template <typename... A>
    void operator()(A&&... args) const {
        auto p = _p.lock();
        if (p) (*p)(forward<A>(args)...);
    }
};

template <typename S, typename F>
auto package(F&& f) -> pair<packaged_task<S>, future<result_of_t_<S>>> {
    auto p = make_shared<shared<S>>(forward<F>(f));
    return make_pair(packaged_task<S>(p), future<result_of_t_<S>>(p));
}

template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
{
    using result_type = result_of_t<F (Args...)>;
    using packaged_type = packaged_task<result_type()>;
    
    auto pack = package<result_type()>(bind(forward<F>(f), forward<Args>(args)...));

#if 0
    _system.async_(move(get<0>(pack)));
#else
    thread(move(get<0>(pack))).detach(); // Replace with task queue
#endif
    return get<1>(pack);
}



int main() {
    future<cpp_int> x = async([]{ return fibonacci<cpp_int>(100); });
    
    future<cpp_int> y = x.then([](const cpp_int& x){ return cpp_int(x * 2); });
    future<cpp_int> z = x.then([](const cpp_int& x){ return cpp_int(x / 15); });
                                                            
    cout << y.get() << endl;
    cout << z.get() << endl;
}

/**************************************************************************************************/

#endif

#if 0

#include <iostream>

#include <boost/multiprecision/integer.hpp>

using namespace std;

template <typename T, typename N, typename O>
T power(T x, N n, O op)
{
    if (n == 0) return identity_element(op);
    
    while ((n & 1) == 0) {
        n >>= 1;
        x = op(x, x);
    }
    
    T result = x;
    n >>= 1;
    while (n != 0) {
        x = op(x, x);
        if ((n & 1) != 0) result = op(result, x);
        n >>= 1;
    }
    return result;
}

template <typename N>
struct multiply_2x2 {
    array<N, 4> operator()(const array<N, 4>& x, const array<N, 4>& y)
    {
        return { x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
                 x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3] };
    }
};

template <typename N>
array<N, 4> identity_element(const multiply_2x2<N>&) { return { N(1), N(0), N(0), N(1) }; }

template <typename R, typename N>
R fibonacci(N n) {
    if (n == 0) return R(0);
    return power(array<R, 4>{ 1, 1, 1, 0 }, N(n - 1), multiply_2x2<R>())[0];
}

#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;


#include <boost/thread/future.hpp>

using namespace std::chrono;

int main() {
    
    auto x = boost::async([]{ return fibonacci<cpp_int>(1'000'000); });
    auto y = boost::async([]{ return fibonacci<cpp_int>(2'000'000); });
    
    auto z = when_all(std::move(x), std::move(y)).then([](auto f){
        auto t = f.get();
        return cpp_int(get<0>(t).get() * get<1>(t).get());
    });
    
    cout << z.get() << endl;
    
    // auto z = boost::when_all([](auto x){ }, x, y);
    
#if 0
    z.then([](auto r){
            return get<0>(r.get()).get() *  get<1>(r.get()).get();
    });
#endif
    
   // cout << z.get() << endl;

    
    // Do something
    
    //y.wait();
    //z.wait();

#if 0
    
    this_thread::sleep_for(seconds(60));
    // y.wait();
    auto start = chrono::high_resolution_clock::now();
    
    auto x = fibonacci<cpp_int>(1'000'000);
    
    cout << chrono::duration_cast<chrono::milliseconds>
    (chrono::high_resolution_clock::now()-start).count() << endl;
    
    cout << x << endl;
#endif
    
#if 0
    
    future<cpp_int> x = async(fibonacci<cpp_int, int>, 1'000'000);
    
    // Do something
    
    auto start = chrono::high_resolution_clock::now();
    
    x.wait();
    
    cout << chrono::duration_cast<chrono::milliseconds>
    (chrono::high_resolution_clock::now()-start).count() << endl;
    
    
    cout << x.get() << endl;
#endif

#if 0
    boost::promise<int> p;
    boost::future<int> x = p.get_future();
    auto y = x.then([](auto x){ cout << "then 1: " << x.get() << endl; });
    x.then([](auto x){ cout << "then 2: " << x.get() << endl; });
    p.set_value(5);
    y.wait();
#endif
}

#endif





#if 0

#include <iostream>

using namespace std;

int main() {

    auto body = [n = 10]() mutable {
        cout << n-- << endl;
        return n != 0;
    };
    
    while (body()) ;

}

#endif


#if 0

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <new>
#include <utility>
#include <string>
#include <vector>

#include <adobe/forest.hpp>
#include <adobe/algorithm/for_each_position.hpp>

using namespace std;


class cirque {
  public:
    class iterator { }; // write this part
    class const_iterator { }; // write this part

    template <typename I>
    iterator insert(const_iterator p, I f, I l) {
        return insert(p, f, l, typename iterator_traits<I>::iterator_category());
    }

  private:
    template <typename I>
    iterator insert(const_iterator p, I f, I l, input_iterator_tag) {
        /* write this part */
    }

    template <typename I>
    iterator insert(const_iterator p, I f, I l, forward_iterator_tag) {
        /* write this part */
    }
};

using namespace adobe;

template <typename R>
void output(const R& r)
{
    cout << "digraph 'forest' {" << endl;
        for_each_position(r, [&](const auto& n) {
            cout << "  " << *n << " -> { ";
            for(const auto& e : child_range(n.base())) cout << e << "; ";
            cout << "};" << endl;
        });
    cout << "}" << endl;
}

int main() {

    typedef adobe::forest<std::string> forest;
    typedef forest::iterator iterator;

    forest f;
    iterator i (f.begin());
    i = adobe::trailing_of(f.insert(i, "grandmother"));
    {
        iterator p (adobe::trailing_of(f.insert(i, "mother")));
        f.insert(p, "me");
        f.insert(p, "sister");
        f.insert(p, "brother");
    }
    {
        iterator p (adobe::trailing_of(f.insert(i, "aunt")));
        f.insert(p, "cousin");
    }
    f.insert(i, "uncle");
    
    output(adobe::preorder_range(f));
}


#endif


#if 0

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <new>
#include <utility>
#include <string>
#include <vector>

using namespace std;

template <typename I, typename F>
F uninitialized_move(I f, I l, F o) {
    return uninitialized_copy(make_move_iterator(f), make_move_iterator(l), o);
}

template <typename I, typename F>
F uninitialized_move_backward(I f, I l, F o) {
    return uninitialized_move(reverse_iterator<I>(l), reverse_iterator<I>(f),
        reverse_iterator<F>(o)).base();
}

template <typename I, typename F>
F uninitialized_copy_backward(I f, I l, F o) {
    return uninitialized_copy(reverse_iterator<I>(l), reverse_iterator<I>(f),
        reverse_iterator<F>(o)).base();
}

/*
    Modulo arithemtic: a and b are in the range [0, n), result is (a op b) mod n.
*/

size_t add_mod(size_t a, size_t b, size_t n) {
    return (a < n - b) ? a + b : a - (n - b);
}

size_t sub_mod(size_t a, size_t b, size_t n) {
    return (a < b) ? a + (n - b) : a - b;
}

size_t neg_mod(size_t a, size_t n) { return n - a; }

template <typename T, // T models container with capacity
          typename N> // N is size_type(T)
void grow(T& x, N n) { x.reserve(max(x.size() + n, x.capacity() * 2)); }

template <typename T>
class cirque {
    struct remote {
        size_t _allocated;
        size_t _size;
        size_t _begin;
        aligned_storage<sizeof(T)> _buffer;

        T* buffer() { return static_cast<T*>(static_cast<void*>(&_buffer));}
        size_t capacity() { return _allocated - 1; }

        auto data_index() const -> tuple<size_t, size_t, size_t, size_t> {
            auto n = _allocated;
            auto f = _begin;
            auto l = add_mod(f, _size, n);
            if (l < f) return { f, n, 0, l };
            return { f, l, 0, 0 };
        }
    };

    unique_ptr<remote> _object{nullptr};

    T* buffer() const { return _object ? _object->buffer() : nullptr; }
    size_t begin_index() const { return _object ? _object->_begin : 0; }
    void set_size(size_t n) { if (_object) _object->_size = n; }
    void set_begin(size_t n) { _object->_begin = n; }

    auto _data() const -> tuple<T*, T*, T*, T*> {
        if (!_object) return { nullptr, nullptr, nullptr, nullptr };
        auto n = _object->_allocated;
        auto f = _object->_begin;
        auto l = add_mod(f, _object->_size, n);
        auto p = _object->buffer();
        if ( l < f ) return { p + f, p + n, p, p + l };
        return { p + f, p + l, nullptr, nullptr };
    }

    void grow(size_t n) { reserve(max(size() + n, capacity() * 2)); }
  public:
    template <typename U>
    class iterator_ {
        friend class cirque;

        remote* _object{nullptr};
        size_t  _i{0};

        iterator_(remote* object, size_t index) :
            _object(object), _i{index} { }

        size_t add(ptrdiff_t x) {
            if (!_object) return 0;
            if (x < 0) return sub_mod(_i, -x, _object->_allocated);
            return add_mod(_i, x, _object->_allocated);
        }

      public:
        using difference_type = ptrdiff_t;
        using value_type = U;
        using pointer = U*;
        using reference = U&;
        using iterator_category = random_access_iterator_tag;

        iterator_() = default;
        template <typename V>
        iterator_(const iterator_<V>& x, enable_if_t<is_convertible<V*, U*>::value>* = 0) :
            _i{x._i}, _object{x._object} { }
        // copy and assignment defaulted

        reference operator*() const { return *(_object->buffer() + _i); }
        pointer operator->() const { return &*(*this); }

        iterator_& operator++() { return *this += 1; }
        iterator_ operator++(int) { auto result = *this; ++(*this); return result; }

        iterator_& operator--() { return *this -= 1; }
        iterator_ operator--(int) { auto result = *this; --(*this); return result; }

        iterator_& operator+=(difference_type x) { _i = add(x); return *this;}
        iterator_& operator-=(difference_type x) { return *this += -x;}

        reference operator[](difference_type n) const { return *(*this + n); }

        friend inline iterator_ operator+(iterator_ x, difference_type y) { return x += y; }
        friend inline iterator_ operator+(difference_type x, iterator_ y) { return y + x; }
        friend inline iterator_ operator-(iterator_ x, difference_type y) { return x -= y; }

        friend inline difference_type operator-(const iterator_& x, const iterator_& y) {
            if (!x._object) return 0;
            assert(y._object && "iterators must be in same container");

            size_t f, l;
            tie(f, l, ignore, ignore) = x._object->data_index();

            bool x_block = f <= x._i && x._i <= l;
            bool y_block = f <= y._i && y._i <= l;

            if (x_block == y_block) return x._i - y._i;
            return x._object->_allocated - (y._i - x._i);
        }

        friend inline bool operator==(const iterator_& x, const iterator_& y) {
            return x._i == y._i;
        }
        friend inline bool operator!=(const iterator_& x, const iterator_& y) { return !(x == y); }

        friend inline bool operator<(const iterator_& x, const iterator_& y) {
            if (!x._object) return false;
            assert(y._object && "iterators must be in same container");

            size_t f, l;
            tie(f, l, ignore, ignore) = x._object->data_index();

            bool x_block = f <= x._i && x._i <= l;
            bool y_block = f <= y._i && y._i <= l;

            if (x_block == y_block) return x._i < y._i;
            return x_block;
        }

        friend inline bool operator>(const iterator_& x, const iterator_& y) { return y < x; }
        friend inline bool operator<=(const iterator_& x, const iterator_& y) { return !(y < x); }
        friend inline bool operator>=(const iterator_& x, const iterator_& y) { return !(x < y); }
    };

    // types:
    using reference = T&;
    using const_reference = const T&;
    using iterator = iterator_<T>;
    using const_iterator = iterator_<const T>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // construct/copy/destroy:
    cirque() = default;
    explicit cirque(size_type n) : cirque(n, T()) { }
    cirque(size_type n, const T& x) {
        reserve(n);
        uninitialized_fill_n(buffer(), n, x);
        set_size(n);
    }

    template<typename I>
    cirque(I f, I l, enable_if_t<!is_integral<I>::value>* = 0) { insert(begin(), f, l); }

    cirque(const cirque& x) {
        reserve(x.size());
        const T *f0, *l0, *f1, *l1;
        tie(f0, l0, f1, l1) = x.data();
        uninitialized_copy(f1, l1, uninitialized_copy(f0, l0, buffer()));
        set_size(x.size());
    }
    cirque(cirque&& x) noexcept = default;
    cirque(initializer_list<T> init) : cirque(init.begin(), init.end()) { }

    ~cirque() { clear(); }

    cirque& operator=(const cirque& x) { *this = cirque(x); return *this; }
    cirque& operator=(cirque&& x) noexcept = default;

    void assign(size_type n, const T& x);
    template<typename I>
    void assign(I f, I l);
    void assign(std::initializer_list<T> init);

    auto data() -> tuple<T*, T*, T*, T*> { return _data(); }
    auto data() const -> tuple<const T*, const T*, const T*, const T*> { return _data(); }

    reference at(size_type p) {
        if (!(p < size())) throw out_of_range("cirque index out of range");
        return *(begin() + p);
    }
    const_reference at(size_type p) const {
        if (!(p < size())) throw out_of_range("cirque index out of range");
        return *(begin() + p);
    }

    reference operator[](size_type p) {
        assert(p < size() && "cirque index out of range");
        return *(begin() + p);
    }
    const_reference operator[](size_type p) const {
        assert(p < size() && "cirque index out of range");
        return *(begin() + p);
    }

    reference front() { return *begin(); }
    const_reference front() const { return *begin(); }

    reference back() { return *(end() - 1); }
    const_reference back() const { return *(end() - 1); }

    size_type capacity() const { return _object ? _object->capacity() : 0; }
    void reserve(size_type n) {
        if (!(capacity() < n)) return;
        unique_ptr<remote> p{static_cast<remote*>(operator new(sizeof(remote) + (n * sizeof(T))))};
        p->_size = size();
        p->_begin = 0;
        p->_allocated = n + 1;

        T *f0, *l0, *f1, *l1;
        tie(f0, l0, f1, l1) = data();
        uninitialized_move(f1, l1, uninitialized_move(f0, l0, p->buffer()));

        _object = move(p);
    }

    iterator begin() { return { _object.get(), begin_index() }; }
    iterator end() { return { _object.get(), add_mod(begin_index(), size(), capacity() + 1) }; }

    const_iterator begin() const { return { _object.get(), begin_index() }; }
    const_iterator end() const
    { return { _object.get(), add_mod(begin_index(), size(), capacity() + 1) }; }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return end(); }
    reverse_iterator rend() { return begin(); }
    const_reverse_iterator rbegin() const { return end(); }
    const_reverse_iterator rend() const { return begin(); }
    const_reverse_iterator crbegin() const { return end(); }
    const_reverse_iterator crend() const { return begin(); }

    /* Test this with uint8 for size_type and int8 for difference_type */
    size_type max_size() const { return numeric_limits<size_type>::max() / 2 - 1; }

    size_type size() const { return _object ? _object->_size : 0; }
    bool empty() const { return size() == 0; }

    void shrink_to_fit() {
        if (size() == capacity()) return;
        *this = *this;
    }

    void clear() { erase(begin(), end()); }

    iterator erase(const_iterator f, const_iterator l) {
        // minimize number of moves
        iterator f0{_object.get(), f._i};
        iterator l0{_object.get(), l._i};
        if (distance(begin(), f0) < distance(l0, end())) {
            auto p = move_backward(begin(), f0, l0);
            for_each(begin(), p, [](auto& e) { e.~T(); });
            set_size(size() - distance(f0, l0));
            set_begin(p._i);
            return {_object.get(), l._i};
        } else {
            auto p = move(l0, end(), f0);
            for_each(p, end(), [](auto& e) { e.~T(); });
            set_size(size() - distance(f0, l0));
            return {_object.get(), f._i};
        }
    }

    template <typename I>
    iterator insert(const_iterator p, I f, I l) {
        return insert(p, f, l, typename iterator_traits<I>::iterator_category());
    }

    iterator insert(const_iterator p, std::initializer_list<T> init) {
        return insert(p, init.begin(), init.end());
    }

    iterator insert(const_iterator p, const T& value) { return insert(p, &value, &value + 1); }
    iterator insert(const_iterator p, T&& value) {
        return insert(p, make_move_iterator(&value), make_move_iterator(&value + 1));
    }
    iterator insert(const_iterator p, size_type count, const T& value) {

    }

    iterator erase(const_iterator p) {
        return erase(p, p + 1);
    }

    void push_back(const T& x) { insert(end(), x); }
    void push_back(T&& x) { insert(end(), move(x)); }
    void push_front(const T& x) { insert(begin(), x); }
    void push_front(T&& x) { insert(begin(), move(x)); }

    void pop_back() {
        assert(size() && "pop_back on empty cirque");
        erase(end() - 1);
    }

    void pop_front() {
        assert(size() && "pop_front on empty cirque");
        erase(begin());
    }

    void swap(cirque& x) { std::swap(*this, x); }

  private:
    template <typename I>
    iterator insert(const_iterator p, I f, I l, forward_iterator_tag) {
        auto i = distance(cbegin(), p);
        auto n = distance(f, l);
        auto pos = begin() + i;

        if (capacity() < size() + n) {
            cirque tmp;
            tmp.reserve(max(size() + n, capacity() * 2));
            tmp.insert(tmp.end(), make_move_iterator(begin()), make_move_iterator(pos));
            tmp.insert(tmp.end(), f, l);
            tmp.insert(tmp.end(), make_move_iterator(pos), make_move_iterator(end()));
            swap(tmp);
            return begin() + i;
        }

        // FIXME - this is relying on iterators working outside the begin() end() range -
        // This could break if move() was implemented interms of move_n() for example (subtraction
        // of iterators not in the begin/end range is undefined.

        if (i < size() - i) {
            auto un = min(n, i);
            auto p0 = uninitialized_move(begin(), begin() + un, begin() - n);
            auto p1 = move(begin() + un, pos, p0);
            auto p2 = uninitialized_copy(f, f + (n - un), p1);
            copy(f + (n - un), l, p2);

            set_size(size() + n);
            set_begin(sub_mod(begin_index(), n, capacity() + 1));
            return pos - n;
        } else {
            auto un = min(n, distance(begin(), end()) - i);
            auto p0 = uninitialized_move_backward(end() - un, end(), end() + n);
            auto p1 = move_backward(pos, end() - un, p0);
            auto p2 = uninitialized_copy_backward(f, f + (n - un), p1);
            copy_backward(f + (n - un), l, p2);
            set_size(size() + n);
            return pos;
        }
    }

    template <typename I>
    iterator insert(const_iterator p, I f, I l, input_iterator_tag) {
        auto i = distance(cbegin(), p);
        auto s = size();
        if (i < s - i) {
            while (f != l) { push_front(*f); ++f; }
            auto n = size() - s;
            reverse(begin(), begin() + (n + i));
            reverse(begin(), begin() + i);
        } else {
            while (f != l) { push_back(*f); ++f; }
            rotate(begin() + i, begin() + s, end());
        }
        return begin() + i;
    }
};

int main() {

    cirque<int> x{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    x.insert(begin(x), { -3, -2, -1, });

    for (const auto& e : x) cout << e << endl;

}

#endif

#if 0

#include <iostream>
#include <utility>
#include <string>

using namespace std;

class example {
    string _title;
  public:
    const string& title() const& { return _title; }
    string&& title() && { return move(_title); }
};

template <typename... T>
struct comp_ : T... {
    template <typename... Args>
    comp_(Args&&... args) : T(args)... { }
};

template <typename... Args>
auto combine(Args&&... args) { return comp_<Args...>(forward<Args>(args)...); }


int main() {
    auto f = combine([](int x){ cout << "int:" << x << endl; },
                     [](const char* x){ cout << "string:" << x << endl; });

    f(10);
    f("Hello");
}

#endif

#if 0

#include <functional>
#include <iostream>
#include <initializer_list>

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

template <typename F, typename... Ts>
auto for_each_argument(F f, Ts&&... a) -> F {
 (void)initializer_list<int>{(f(forward<Ts>(a)), 0)...};
 return f;
}


#if 0
struct print {
    template <typename T>
    void operator()(const T& x) const { cout << x << endl; }
};
#endif

template <typename F>
F&& func(F&& x)
{
    (F&&)x += 1;
    return (F&&)x; }

int main() {
   // for_each_argument(print(), 10, "Hello", 42.5);
   for_each_argument([](const auto& x){ cout << x << endl; }, 10, "Hello", 42.5);

    int x{0};
    func(x);
    cout << x << endl;
}

#endif


#if 0
#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>
#include <deque>
#include <future>

#include <boost/lockfree/queue.hpp>

#include <dispatch/dispatch.h>

using namespace std;
using namespace chrono;

using lock_t = unique_lock<mutex>;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;

  public:
    bool try_pop(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock || _q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    bool try_push(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }

    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
         while (_q.empty() && !_done) _ready.wait(lock);
         if (_q.empty()) return false;
         x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }
};

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};

 void run(unsigned i) {
        while (true) {
            function<void()> f;

            for (unsigned n = 0; n != _count * 32; ++n) {
                if (_q[(i + n) % _count].try_pop(f)) break;
            }
            if (!f && !_q[i].pop(f)) break;

            f();
        }
    }

  public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }

    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }

    template <typename F>
    void async_(F&& f) {
        auto i = _index++;

        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].try_push(forward<F>(f))) return;
        }

        _q[i % _count].push(forward<F>(f));
    }
};

task_system _system;

#if 0
#include <array>

template <typename T, T... Args>
constexpr auto sort() -> array<T, sizeof...(Args)> {
    if
    return { Args... };
}
#endif

#if 1
template <typename F>
void async_(F&& f) {
    _system.async_(forward<F>(f));
}
#else
template <typename F>
void async_(F&& f) {
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new F(move(f)), [](void* p){
            auto f = static_cast<F*>(p);
            (*f)();
            delete(f);
        });
}
#endif

  void time() {

    for (int n = 0; n != 1000000; ++n) {
        async_([]{ });
    }

    mutex block;
    condition_variable ready;
    bool done = false;

    async_([&]{
        {
            unique_lock<mutex> lock{block};
            done = true;
        }
        ready.notify_one();
    });

    unique_lock<mutex> lock{block};
    while (!done) ready.wait(lock);
}

int main() {
#if 0
    future<int> r;
    {
    auto p = make_shared<packaged_task<int()>>([]{ return 42; });
    r = p->get_future();
    async_([_p = move(p)]{ (*_p)(); });
    }
    cout << r.get() << endl;

    //auto f = async(long_operation);
#endif



    auto start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

    start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

}

#endif




#if 0

#include <utility>
#include <iterator>
#include <iostream>
#include <algorithm>

using namespace std;



template <typename I> // I models ForwardIterator
I rotate_n_plus_one_to_n(I f, I m, I l) {
    auto tmp = move(*f);
    while (m != l) {
        *f = move(*m);
        ++f;
        *m = move(*f);
        ++m;
    }
    *f = move(tmp);
    return f;
}

template <typename I> // I models ForwardIterator
void reverse_(I f, I l) {
    auto n = distance(f, l);

    if (n == 0 || n == 1) return;

    auto m = f;
    advance(m, n - (n / 2));

    reverse_(f, m);
    reverse_(m, l);

    if (n % 2) rotate_n_plus_one_to_n(f, m, l);
    else swap_ranges(f, m, m);
}

int main() {
    int a[] = { 0, 1, 2, 3, 4 };

    // auto f = begin(a), l = end(a);

    // rotate_n_plus_one_to_n(f, f + (l - f) / 2 + 1, l);

    reverse_(begin(a), end(a));
    for (const auto& e : a) cout << e << endl;
}


#if 0

    if (n % 2) rotate_n_plus_one_n(f, m, l);
    else rotate_n_n(f, m, l);
#endif

#endif

#if 0

#include <utility>
#include <iterator>
#include <iostream>


template <typename I, // I models RandomAccessIterator
          typename P> // P models UnaryPredicate
I stable_partition_(I f, I l, P p) {
    auto n = distance(f, l);

    if (n == 0) return f;
    if (n == 1) return f + p(*f);

    auto m = f + n / 2;

    return rotate(stable_partition_(f, m, p),
                  m,
                  stable_partition_(m, l, p));
}

template <typename I> // I models RandomAccessIterator
I rotate_(I f, I m, I l) {
    reverse(f, l);
    m = f + (l - m);
    reverse(f, m);
    reverse(m, l);
    return m;
}

template <typename I> // I models ForwardIterator
void reverse_(I f, I l) {
    auto n = distance(f, l);

    if (n == 0 || n == 1) return;

    auto m = f;
    advance(m, n  - (n / 2));

    reverse_(f, m);
    reverse_(m, l);
    if (n % 2) rotate_n_plus_one_n(f, m, l);
    else rotate_n_n(f, m, l);
}

template <typename I> // I models ForwardIterator
I rotate_n_plus_one_n(I f, I m, I l) {
    auto tmp = move(*f);
    while (m != l) {
        *f = move(*m);
        ++f;
        *m = move(*f);
        ++m;
    }
    *f = move(tmp);
    return f;
}

template <typename I> // I models ForwardIterator
I rotate_n_n(I f, I m, I l) {
    while (m != l) {
        swap(*f, *m);
        ++f, ++m;
    }
    return f;
}


#endif

#if 0

// GOOD for concurrency talk!

#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>
#include <deque>

#include <dispatch/dispatch.h>

using namespace std;
using namespace chrono;

class any {
    struct concept {
        virtual ~concept() = default;
        virtual unique_ptr<concept> copy() const = 0;
    };
    template <typename T>
    struct model : concept {
        explicit model(T x) : _data(x) { }
        unique_ptr<concept> copy() const override { return make_unique<model>(*this); }
        T _data;
    };

    unique_ptr<concept> _object;
  public:
    any() = default;
    template <typename T>
    any(T x) : _object(make_unique<model<T>>(move(x))) { }
    any(const any& x) : _object(x._object ? x._object->copy() : nullptr) { }
    any(any&&) noexcept = default;
    any& operator=(const any& x) { any tmp(x); *this = move(tmp); return *this; }
    any& operator=(any&&) noexcept = default;

    template <typename T, typename... Args>
    void emplace(Args&&... args) { _object = make_unique<model<T>>(forward<Args>(args)...); }

    template <typename T> friend T static_cast_(any& x);
    template <typename T> friend T static_cast_(const any& x);
};

template <typename T>
T static_cast_(any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<any::model<T>*>(x._object.get())->_data;
}

template <typename T>
T static_cast_(const any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<const any::model<T>*>(x._object.get())->_data;
}

/**************************************************************************************************/

template <typename F, typename... Args>
void for_each_argument(F f, Args&&... args) {
    using t = int[sizeof...(Args)];
    (void)t{(f(forward<Args>(args)), 0)...};
}

template <typename F, typename T, size_t... I>
void apply_(F f, T&& t, index_sequence<size_t, I...>) {
    f(get<I>(t)...);
}

template <typename F, typename T>
void apply(F f, T&& t) {
    apply_(f, t, make_index_sequence<size_t, tuple_size<T>::value>());
}
/**************************************************************************************************/

using lock_t = unique_lock<mutex>;

template <typename T>
struct task_ {
    any                         _f;
    vector<T>                   _r; // optional

    mutex _mutex;
    vector<function<void()>> _then;
    bool _resolved{false};

    // virtual ~task_() = default;

    template <typename F, typename... Args>
    void resolve(F& f, Args&&... args) {
        _r.emplace_back(f(forward<Args>(args)...));
        vector<function<void()>> tmp;
        {
        lock_t lock{_mutex};
        swap(tmp, _then);
        _resolved = true;
        }
        for(const auto& e : tmp) e();
    }

    template <typename F>
    void then(F f) {
        bool resolved{false};
        {
        lock_t lock{_mutex};
        if (_resolved) resolved = true;
        else _then.emplace_back(move(f));
        }
        if (resolved) f();
    }

    const T& get() const {
        return _r.back();
    }
};

template <typename T>
class future;


template <typename T, typename F>
struct task_f {
    F _f;
    weak_ptr<task_<T>> _r;

    template <typename... Args>
    explicit task_f(weak_ptr<task_<T>> r, Args&&... args) : _r(move(r)), _f(forward<Args>(args)...) { }

    template <typename... Args>
    void operator()(Args&&... args) {
        auto r = _r.lock();
        if (r) r->resolve(_f, forward<Args>(args)...);
    }
};

template <typename F, typename... Args>
struct task_group {
    F _f;
    atomic<size_t>  _n{sizeof...(Args)};

    explicit task_group(F f) : _f(move(f)) { }

    void operator()() {
        if (--_n == 0) _f();
    };
};


template <typename T>
class packaged;

template <typename T, typename F>
using packaged_task = packaged<task_f<T, F>>;

template <typename F, typename... Args>
auto when_all(F f, future<Args>... args) -> future<result_of_t<F(Args...)>>;

template <typename T, typename F>
auto package(F f) -> pair<packaged_task<T, F>, future<T>>;

template <typename T>
class future {
    shared_ptr<task_<T>> _p;

    template <typename T1, typename F1>
    friend auto package(F1 f) -> pair<packaged_task<T1, F1>, future<T1>>;

    template <typename F, typename... Args>
    friend auto when_all(F f, future<Args>... args) -> future<result_of_t<F(Args...)>>;

    explicit future(shared_ptr<task_<T>> p) : _p(move(p)) { }

    template <typename F>
    void then(F&& f) { _p->then(forward<F>(f)); }

  public:
    future() = default;

    const T& get() const { return _p->get(); }

};

template <typename T>
class packaged {
    weak_ptr<T> _p;

    template <typename T1, typename F1>
    friend auto package(F1 f) -> pair<packaged_task<T1, F1>, future<T1>>;

    template <typename F1, typename... Args>
    friend auto when_all(F1 f, future<Args>... args) -> future<result_of_t<F1(Args...)>>;

    explicit packaged(weak_ptr<T> p) : _p(move(p)) { }
  public:
    template <typename... Args>
    void operator()(Args&&... args) const {
        auto p = _p.lock();
        if (p) (*p)(forward<Args>(args)...);
    }
};


template <typename T, typename F>
auto package(F f) -> pair<packaged_task<T, F>, future<T>>
{
    auto p = make_shared<task_f<T, F>>(move(f));

    return make_pair(packaged_task<T, F>(p), future<T>(p));
}

template <typename F, typename... Args>
auto when_all(F f, future<Args>... args) -> future<result_of_t<F(Args...)>>
{
    using result_type = result_of_t<F(Args...)>;

    auto p = make_shared<task_group_<result_type>>(move(f), args...);

    for_each_argument([&](auto x){
        x.then([_f = packaged<task_group_<result_type>>(p)]{ _f(); });
    }, args...);

    return future<result_type>(p);
}

#if 0
inline constexpr size_t cstrlen(const char* p) {
    size_t n = 0;
    while (p[n] != '\0') ++n;
    return n;
}
#endif

template <size_t N>
size_t cstrlen(const char(&)[N]) { cout << "O(1)" << endl; return N - 1; }
inline size_t cstrlen(const char* p) {
    size_t n = 0;
    while (p[n] != '\0') ++n;
    return n;
}


int main() {

    for_each_argument([](const auto& x){ cout << x << endl; }, 10, "hello", 42.5);

    auto n = cstrlen("Hello World");
#if 0
    constexpr int a[cstrlen("Hello World")] = { 0 };
#endif

    function<void()> f;
    function<void()> g;
    future<int> g_f;

    {
    future<int> later;
    auto x = package<int>([]{ cout << "x" << endl; return 10; });
    auto y = package<int>([]{ cout << "y" << endl; return 20; });

    later = when_all([](int x, int y){ cout << "when_all: " << x << ", " << y << endl; return x + y; }, x.second, y.second);

    f = x.first; g = y.first;
    g_f = y.second;
    }
    f();
    g();
    when_all([](int x){ cout << "g: " << x << endl; return 1; }, g_f);


    [](auto... args){ int a[]{args...}; for (const auto& e : a) cout << e << endl; }(10, 20, 30);
}


#endif


#if 0

#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>

using namespace std;

template <typename I>
void reverse_(I f, I l) {
    auto n = distance(f, l);
    if (n == 0 || n == 1) return;

    auto m = f + (n / 2);

    reverse_(f, m);
    reverse_(m, l);
    rotate(f, m, l);
}


int main() {

    vector<int> a(1024 * 1024);
    iota(begin(a), end(a), 0);
    reverse_(begin(a), end(a));
    for (const auto& e : a) cout << e << endl;
}

#endif


#if 0
#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>
#include <deque>

#include <boost/lockfree/queue.hpp>

//#include <boost/thread/future.hpp>

#include <dispatch/dispatch.h>

using namespace std;
using namespace chrono;



class any {
    struct concept {
        virtual ~concept() = default;
        virtual unique_ptr<concept> copy() const = 0;
    };
    template <typename T>
    struct model : concept {
        explicit model(T x) : _data(x) { }
        unique_ptr<concept> copy() const override { return make_unique<model>(*this); }
        T _data;
    };

    unique_ptr<concept> _object;
  public:
    any() = default;
    template <typename T>
    any(T x) : _object(make_unique<model<T>>(move(x))) { }
    any(const any& x) : _object(x._object ? x._object->copy() : nullptr) { }
    any(any&&) noexcept = default;
    any& operator=(const any& x) { any tmp(x); *this = move(tmp); return *this; }
    any& operator=(any&&) noexcept = default;

    template <typename T> friend T static_cast_(any& x);
    template <typename T> friend T static_cast_(const any& x);
};

template <typename T>
T static_cast_(any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<any::model<T>*>(x._object.get())->_data;
}

template <typename T>
T static_cast_(const any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<const any::model<T>*>(x._object.get())->_data;
}

/**************************************************************************************************/

template <typename R>
struct shared_result;

template <typename R, typename F>
struct shared_task {
    F _f;
    weak_ptr<shared_result<R>> _r;

    explicit shared_task(F f) : _f(move(f)) { }

    template <typename... Args>
    void operator()(Args... args) const {
        auto r = _r.lock();
        if (r) (*r)(_f, forward<Args>(args)...);
    }
};

using lock_t = unique_lock<mutex>;

template <typename R>
struct shared_result {
    vector<R> _r; // optional
    any _f;

    mutex _mutex;
    vector<function<void()>> _then;
    bool _resolved{false};

    template <typename F, typename... Args>
    void operator()(F f, Args... args) {
        _r.emplace_back(f(forward<Args>(args)...));
        vector<function<void()>> tmp;
        {
        lock_t lock{_mutex};
        swap(tmp, _then);
        _resolved = true;
        }
        for(const auto& e : tmp) e();
    }

    template <typename F>
    void then(F f) {
        bool resolved{false};
        {
        lock_t lock{_mutex};
        if (_resolved) resolved = true;
        else _then.emplace_back(move(f));
        }
        if (resolved) f();
    }
};

template <typename R, typename F>
struct packaged_task {
    shared_ptr<shared_task<R, F>> _f;

    explicit packaged_task(shared_ptr<shared_task<R, F>>& f) : _f(move(f)) { }

    template <typename... Args>
    void operator()(Args... args) const {
        (*_f)(forward<Args>(args)...);
    }
};

template <typename R, typename F>
auto package(F f);

template <typename T>
class future {
    shared_ptr<shared_result<T>> _r;


    template <typename R, typename F>
    friend auto package(F f);
  public:
    // Need a friend template function?
    explicit future(shared_ptr<shared_result<T>>& r) : _r(move(r)) { }


    future() = default;

    const T& get() const { return _r->_r.back(); }

    template <typename F>
    auto then(F&& f) {
        auto p = package<T>(forward<F>(f));
        _r.then([_r = _r, _f = move(p.first)]{ _f(_r->_r.back()); });
        return move(p.second);
    }

    template <typename F>
    void then_(F&& f) { _r->then(forward<F>(f)); }
};

template <typename R, typename F>
auto package(F f) {
    auto f_ = make_shared<shared_task<R, F>>(move(f));
    auto r_ = make_shared<shared_result<R>>();
    r_->_f = f_;
    f_->_r = r_;

    return make_pair(packaged_task<R, F>(f_), future<R>(r_));
}

template <size_t N, typename F>
struct group {
    F                           _f;
    mutable atomic<size_t>      _n{N};

    explicit group(F f) : _f(move(f)) { }

    void operator()() const {
        if (--_n == 0) _f();
    };
};

template <size_t N, typename F>
auto make_group(F f) { return make_shared<group<N, F>>(move(f)); }

template <typename F, typename T, typename... Args>
auto when_all(F f, future<T> arg0, future<Args>... args) {
    auto p = package<result_of_t<F(T, Args...)>>(f);

    auto s = make_group<1 + sizeof...(Args)>([_f = move(p.first), arg0, args...]{ _f(arg0.get(), args.get()...); });
    arg0.then_([_s = s]{ (*_s)(); });
    [&](auto x){ x.then_([_s = s]{ (*_s)(); }); }(args...);

    return p.second;
}

int main() {
    auto p1 = package<string>([]{ return "Hello"; });
    auto p2 = package<string>([]{ return "World"; });
    auto f = when_all([](string x, string y){ cout << x << y << endl; return 0; }, p1.second, p2.second);
    p1.first();
    p2.first();

    cout << "end" << endl;
}

#endif



#if 0
#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>
#include <deque>

#include <boost/lockfree/queue.hpp>

#include <dispatch/dispatch.h>

using namespace std;
using namespace chrono;

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;

    using lock_t = unique_lock<mutex>;
  public:
    void done() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_one();
    }

    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
         while (_q.empty() && !_done) _ready.wait(lock);
         if (_q.empty()) return false;
         x = move(_q.front());
        _q.pop_front();
        return true;
    }

    bool pop_try(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock || _q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }

    template<typename F>
    bool push_try(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }
};

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};

  public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }

    ~task_system() {
        for (auto& e : _q) e.done();
        for (auto& e : _threads) e.join();
    }

    void run(unsigned i) {
        while (true) {
            function<void()> f;

            for (unsigned n = 0; n != _count; ++n) {
                if (_q[(i + n) % _count].pop_try(f)) break;
            }

            if (!f && !_q[i].pop(f)) break;
            f();
        }
    }

    template <typename F>
    void async_(F&& f) {
        unsigned i = _index++;
        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].push_try(forward<F>(f))) return;
        }
        _q[i % _count].push(forward<F>(f));
    }
};

class any {
    struct concept {
        virtual ~concept() = default;
        virtual unique_ptr<concept> copy() const = 0;
    };
    template <typename T>
    struct model : concept {
        explicit model(T x) : _data(x) { }
        unique_ptr<concept> copy() const override { return make_unique<model>(*this); }
        T _data;
    };

    unique_ptr<concept> _object;
  public:
    any() = default;
    template <typename T>
    any(T x) : _object(make_unique<model<T>>(move(x))) { }
    any(const any& x) : _object(x._object ? x._object->copy() : nullptr) { }
    any(any&&) noexcept = default;
    any& operator=(const any& x) { any tmp(x); *this = move(tmp); return *this; }
    any& operator=(any&&) noexcept = default;

    template <typename T> friend T static_cast_(any& x);
    template <typename T> friend T static_cast_(const any& x);
};

template <typename T>
T static_cast_(any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<any::model<T>*>(x._object.get())->_data;
}

template <typename T>
T static_cast_(const any& x) {
    static_assert(is_reference<T>::value, "result type must be a reference");
    return static_cast<const any::model<T>*>(x._object.get())->_data;
}

// ...

// namespace detail {

template <typename T>
struct shared {
    using cont_t = vector<function<void(T)>>;

    any         _f;
    vector<T>   _result;

    mutex       _mutex;
    bool        _resolved{false};
    cont_t      _cont;

    template <typename F>
    explicit shared(F f) : _f(move(f)) { }

    template <typename F, typename... Args>
    void resolve(F f, Args&&... args) {
        _result.emplace_back(f(forward<Args>(args)...));
        cont_t tmp;
        {
            unique_lock<mutex> lock(_mutex);
            swap(_cont, tmp);
            _resolved = true;
        }
        for (const auto& e : tmp) e(_result.back());
    }

    template <typename F>
    void then(F f) {
        bool resolved;
        {
            unique_lock<mutex> lock(_mutex);
            resolved = _resolved;
            if (!resolved) _cont.emplace_back(move(f));
        }
        if (resolved) f(_result.back());
    }
};

template <typename, typename> class group;
template <typename F, typename R, typename... Args>
struct group<F, R(Args...)> {
    atomic<size_t>  _count{sizeof...(Args)};
    F    _f;

    void available() {
        if (--_count == 0) _f();
    }
};

// Can I avoid void case in example code?

template <>
struct shared<void> {
    using cont_t = vector<function<void()>>;

    any     _f;
    mutex   _mutex;
    bool    _resolved{false};
    cont_t  _cont;

    template <typename F>
    explicit shared(F f) : _f(move(f)) { }
    // virtual ~shared() = default;

    template <typename F, typename... Args>
    void resolve(F f, Args&&... args) {
        f(forward<Args>(args)...);
        cont_t tmp;
        {
            unique_lock<mutex> lock(_mutex);
            swap(_cont, tmp);
            _resolved = true;
        }
        for (const auto& e : tmp) e();
    }

    template <typename F>
    void then(F f) {
        bool resolved;
        {
            unique_lock<mutex> lock(_mutex);
            resolved = _resolved;
            if (!resolved) _cont.emplace_back(move(f));
        }
        if (resolved) f();
    }
};

#if 0
// Better to write a quick any?

template <typename F, typename T>
struct shared_function : shared<T> {
    F _f;
    explicit shared_function(F f) : _f(move(f)) { }
};
#endif

template <typename F, typename T>
struct packaged {
    weak_ptr<shared<T>> _p;

    template <typename... Args>
    void operator()(Args&&... args) const {
        auto p = _p.lock();
        if (p) {
        p->resolve(static_cast_<const F&>(p->_f), forward<Args>(args)...);
        }
    }
};

// } // detail

#if 0
template <typename R, typename F, typename... Args>
auto when_all(F f, future<Args>... args) {
    package_task<R>([args...]() {
    });
}
#endif


// split a function into a function returning void and a future
template <typename T>
class future;

template <typename R, typename F>
auto package_task(F f) {
    auto p = make_shared<shared<R>>(move(f)); // one use of p could be a move

    return make_pair(packaged<F, R>{p}, future<R>{p});
}

template <typename T>
class future {
    shared_ptr<shared<T>> _shared;

  public:
    explicit future(shared_ptr<shared<T>> x) : _shared(move(x)) { }
    future() = default;

    template <typename F>
    auto then(F f) {
        auto p = package_task<result_of_t<F(T)>>([_hold = _shared, _f = move(f)](const T& x){ _f(x); });
        _shared->then(move(p.first));
        return move(p.second);
    }
};

task_system _system;

#if 0
#include <array>

template <typename T, T... Args>
constexpr auto sort() -> array<T, sizeof...(Args)> {
    if
    return { Args... };
}
#endif

#if 0
template <typename F>
void async_(F&& f) {
    _system.async_(forward<F>(f));
}
#else
template <typename F>
void async_(F&& f) {
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new F(move(f)), [](void* p){
            auto f = static_cast<F*>(p);
            (*f)();
            delete(f);
        });
}
#endif

  void time() {

    for (int n = 0; n != 1000000; ++n) {
        async_([]{
        #if 0
            int a[1000];
            iota(begin(a), end(a), 0);
            shuffle(begin(a), end(a), default_random_engine{});
            sort(begin(a), end(a));
        #endif
        });
    }

    mutex block;
    condition_variable ready;
    bool done = false;

    async_([&]{
        {
            unique_lock<mutex> lock{block};
            done = true;
        }
        ready.notify_one();
    });

    unique_lock<mutex> lock{block};
    while (!done) ready.wait(lock);
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

    start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

    function<void(int)> package;
    {
    future<void> y;

    auto x = package_task<int>([](int x) { cout << "call 1" << endl; return x; });
    package = move(x.first);
    y = x.second.then([](int x){ cout << "result:" << x << endl; });
    }
    package(42);
}

#endif


#if 0
#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>
#include <deque>

#include <boost/lockfree/queue.hpp>

#include <dispatch/dispatch.h>

using namespace std;
using namespace chrono;



#define VARIENT 99

#if VARIENT == 0
template <typename T>
class notification_queue {
    list<T> _q;
    bool    _done{false};

    mutex   _mutex;
    condition_variable _ready;

  public:
    void push_notify(T x) {
        list<T> tmp{move(x)};
        {
            unique_lock<mutex> lock{_mutex};
            _q.splice(_q.end(), tmp);
        }
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_mutex};
            while (_q.empty() && !_done) _ready.wait(lock);
            if (_q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }
};
#elif VARIENT == 1
template <typename T>
class notification_queue {
    struct cmp {
        template <typename U>
        bool operator()(const U& x, const U& y) const { return x.first > y.first; }
    };

    priority_queue<pair<uint64_t, T>, vector<pair<uint64_t, T>>, cmp> _q;
    uint64_t _count{0};
    bool    _done{false};

    mutex   _mutex;
    condition_variable _ready;

  public:
    void push_notify(T x) {
        {
            unique_lock<mutex> lock{_mutex};
            _q.emplace(_count, move(x));
            ++_count;
        }
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        unique_lock<mutex> lock{_mutex};
        while (_q.empty() && !_done) {  _ready.wait(lock); }
        if (_q.empty()) return false;
        x = move(_q.top().second);
        _q.pop();
        return true;
    }
};
#elif VARIENT == 2
template <typename T>
class notification_queue {
    list<T> _q;
    mutex   _q_mutex;
    bool    _done{false};

    mutex   _mutex;
    atomic<int> _idle{0};
    condition_variable _ready;

  public:
    bool pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_q_mutex};
            if (_q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }
    bool pop_try(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_q_mutex, try_to_lock};
            if (!lock || _q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }

    void push_notify(T x) {
        list<T> tmp{move(x)};
        {
            unique_lock<mutex> lock{_q_mutex};
            _q.splice(_q.end(), tmp);
        }
        if (_idle) _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        if (pop(x)) return true;

        ++_idle;
        bool result = pop(x);
        if (!result) {
            unique_lock<mutex> lock{_mutex};
            while (!result && !_done) {
                _ready.wait(lock);
                result = pop(x);
            }
        }
        --_idle;
        return result;
    }
};
#endif

class notification_queue {
    deque<function<void()>> _q;
    bool                    _done{false};
    mutex                   _mutex;
    condition_variable      _ready;

    using lock_t = unique_lock<mutex>;
  public:
    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(function<void()>& x) {
        lock_t lock{_mutex};
         while (_q.empty() && !_done) _ready.wait(lock);
         if (_q.empty()) return false;
         x = move(_q.front());
        _q.pop_front();
        return true;
    }

    bool pop_try(function<void()>& x) {
        lock_t lock{_mutex, try_to_lock};
        if (!lock || _q.empty()) return false;
        x = move(_q.front());
        _q.pop_front();
        return true;
    }

    template<typename F>
    void push(F&& f) {
        {
            lock_t lock{_mutex};
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
    }

    template<typename F>
    bool push_try(F&& f) {
        {
            lock_t lock{_mutex, try_to_lock};
            if (!lock) return false;
            _q.emplace_back(forward<F>(f));
        }
        _ready.notify_one();
        return true;
    }
};

class task_system {
    const unsigned              _count{thread::hardware_concurrency()};
    vector<thread>              _threads;
    vector<notification_queue>  _q{_count};
    atomic<unsigned>            _index{0};

  public:
    task_system() {
        for (unsigned n = 0; n != _count; ++n) {
            _threads.emplace_back([&, n]{ run(n); });
        }
    }

    ~task_system() {
        for (auto& e : _q) e.done_notify();
        for (auto& e : _threads) e.join();
    }

    void run(unsigned i) {
        while (true) {
            function<void()> f;

            for (unsigned n = 0; n != _count; ++n) {
                if (_q[(i + n) % _count].pop_try(f)) break;
            }

            if (!f && !_q[i % _count].pop(f)) break;
            f();
        }
    }

    template <typename F>
    void async_(F&& f) {
        unsigned i = _index++;
        for (unsigned n = 0; n != _count; ++n) {
            if (_q[(i + n) % _count].push_try(forward<F>(f))) return;
        }
        _q[i % _count].push(forward<F>(f));
    }
};

#if 0

class thread_pool {
    vector<thread> _pool;

  public:
    template <typename F>
    explicit thread_pool(F f) {
        for (auto n = thread::hardware_concurrency(); n != 0; --n) {
            _pool.emplace_back(move(f));
        }
    }
    ~thread_pool() {
        for (auto& e : _pool) e.join();
    }
};

class task_system {

    struct element_type {
        void (*_f)(void*);
        void* _x;
    };

    notification_queue<function<void()>> _q;
    thread_pool _pool;

  public:
    task_system() : _pool{[&]{
        function<void()> f;
        while (_q.wait_pop(f)) f();
    }} { }

    ~task_system() { _q.done_notify(); }

    template <typename F>
    void async_(F&& f) {
        _q.push_notify(forward<F>(f));
    }
};

#endif

task_system _system;

#if 0
template <typename F>
void async_(F&& f) {
    _system.async_(forward<F>(f));
}
#else
template <typename F>
void async_(F&& f) {
    dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        new F(move(f)), [](void* p){
            auto f = static_cast<F*>(p);
            (*f)();
            delete(f);
        });
}
#endif

__attribute__ ((noinline)) void time() {
    condition_variable ready;
    bool done = false;

    for (int n = 0; n != 1000000; ++n) {
        async_([]{
        #if 0
            int a[1000];
            iota(begin(a), end(a), 0);
            shuffle(begin(a), end(a), default_random_engine{});
            sort(begin(a), end(a));
        #endif
        });
    }

    async_([&]{ done = true; ready.notify_one(); });

    mutex block;
    unique_lock<mutex> lock{block};
    while (!done) ready.wait(lock);
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

    start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;
}

#endif


#if 0
#include <chrono>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <type_traits>
#include <queue>
#include <random>

#include <boost/lockfree/queue.hpp>

using namespace std;
using namespace chrono;

// namespace detail {

template <typename, typename> class group;
template <typename F, typename R, typename... Args>
struct group<F, R(Args...)> {
    atomic<size_t>  _count{sizeof...(Args)};
    F    _f;

    void available() {
        if (--_count == 0) _f();
    }
};

template <typename T>
struct shared {
    using queue_type = list<function<void()>>;
    aligned_storage_t<sizeof(T)> _result;

    mutex   _mutex;
    bool _resolved{false};
    queue_type _q;

    template <typename F, typename... Args>
    void resolve(F f, Args&&... args) {
        new (&_result)T(f(forward<Args>(args)...));
        queue_type tmp;
        {
            unique_lock<mutex> lock(_mutex);
            swap(_q, tmp);
            _resolved = true;
        }
        for (const auto& e : tmp) e();
    }

};

template <typename F, typename T>
struct packaged {
    F _f;
    weak_ptr<T> _p;

    template <typename... Args>
    void operator()(Args&&... args) const {
        auto p = _p.lock();
        if (p) p->resolve(_f, forward<Args>(args)...);
    }
};

// } // detail

template <typename T>
class future {
    shared_ptr<shared<T>> _shared;

  public:
    template <typename F>
    auto then(F f) { return when_all(f, *this); }
};

// split a function into a function returning void and a future

template <typename F, typename S>
auto package_task(F f) {
    using result_type = result_of_t<S>;

    auto p = make_shared<shared<result_type>>();

    return make_pair(packaged<F, result_type>{f, p}, future<result_type>{p});
}

// given a function return a function bound to the futures and a
// a future for  the result

/*
    begin
    end
*/

template <typename T>
class concurrent_queue {
    std::list<T>      _q;
    mutex             _mutex;

  public:
    bool pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_mutex};
            if (_q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }

    bool try_to_pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_mutex, try_to_lock};
            if (!lock || _q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }

    void push(T x) {
        list<T> tmp{move(x)};
        {
            unique_lock<mutex> lock{_mutex};
            _q.splice(_q.end(), tmp);
        }
    }
};

#define VARIANT 3

#if VARIANT == 0
template <typename T>
class notification_queue {
    list<T> _q;
    mutex   _q_mutex;
    bool    _done = false;
    mutex   _mutex;
    condition_variable _ready;

  public:
    bool pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_q_mutex};
            if (_q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }

    bool try_to_pop(T& x) {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_q_mutex, try_to_lock};
            if (!lock || _q.empty()) return false;
            tmp.splice(tmp.end(), _q, _q.begin());
        }
        x = move(tmp.front());
        return true;
    }

    void push_notify(T x) {
        list<T> tmp{move(x)};
        {
            unique_lock<mutex> lock{_q_mutex};
            _q.splice(_q.end(), tmp);
        }
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        if (try_to_pop(x)) return true;

        unique_lock<mutex> lock{_mutex};
        bool result = try_to_pop(x);
        while (!result && !_done) {
            _ready.wait(lock);
            result = pop(x);
        }
        return result;
    }
};
#elif VARIANT == 1
template <typename T>
class notification_queue {
    vector<pair<uint64_t, T>> _q;
    mutex   _q_mutex;
    uint64_t _count;
    bool    _done = false;
    mutex   _mutex;
    condition_variable _ready;

  public:
    bool pop(T& x) {
        unique_lock<mutex> lock{_q_mutex};
        if (_q.empty()) return false;
        pop_heap(begin(_q), end(_q), [](const auto& x, const auto& y) { return x.first > y.first; });
        x = move(_q.back().second);
        _q.pop_back();
        return true;
    }

    void push_notify(T x) {
        {
            unique_lock<mutex> lock{_q_mutex};
            _q.emplace_back(_count, move(x)); ++_count;
            push_heap(begin(_q), end(_q), [](const auto& x, const auto& y) { return x.first > y.first; });
        }
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        // if (pop(x)) return true;

        unique_lock<mutex> lock{_mutex};
        bool result = pop(x);
        while (!result && !_done) {
            _ready.wait(lock);
            result = pop(x);
        }
        return result;
    }
};
#elif VARIANT == 2
template <typename T>
class notification_queue {
    boost::lockfree::queue<T> _q{1000000 * 2};
    boost::lockfree::queue<T> _q_hi{1000000 * 2};
    bool    _done = false;
    mutex   _mutex;
    condition_variable _ready;

  public:
    void push_notify(T x) {
        _q.push(move(x));
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        if (_q_hi.pop(x) || _q.pop(x)) return true;

        unique_lock<mutex> lock{_mutex};
        bool result = _q_hi.pop(x) || _q.pop(x);
        while (!result && !_done) {
            _ready.wait(lock);
            result = _q_hi.pop(x) || _q.pop(x);
        }
        return result;
    }
};
#elif VARIANT == 3
template <typename T>
class notification_queue {
#if 1
    concurrent_queue<T> _q;
#else
    boost::lockfree::queue<T> _q{1000000 * 2};
#endif
    bool    _done = false;
    mutex   _mutex;
    condition_variable _ready;

  public:
    void push_notify(T x) {
        _q.push(move(x));
        
       { unique_lock<mutex> lock{_mutex}; }

        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool wait_pop(T& x) {
        // if (_q.try_to_pop(x)) return true;

        unique_lock<mutex> lock{_mutex};
        bool result = _q.pop(x);
        while (!result && !_done) {
            _ready.wait(lock);
            result = _q.pop(x);
        }
        return result;
    }
};
#endif

#define SIMPLE 1

class thread_pool {
    vector<thread> _pool;

  public:
    template <typename F>
    explicit thread_pool(F f) {
        for (auto n = thread::hardware_concurrency(); n != 0; --n) {
            _pool.emplace_back(f);
        }
    }
    ~thread_pool() {
        for (auto& e : _pool) e.join();
    }
};

class task_system {

    struct element_type {
        void (*_f)(void*);
        void* _x;
    };

#if SIMPLE
    notification_queue<element_type> _q;
#else
    notification_queue<function<void()>> _q;
#endif
    thread_pool _pool;

  public:
#if SIMPLE
    task_system() : _pool{[&]{
        element_type e;
        while (_q.wait_pop(e)) e._f(e._x);
    }} { }
#else
    task_system() : _pool{[&]{
        function<void()> f;
        while (_q.wait_pop(f)) f();
    }} { }
#endif


    ~task_system() { _q.done_notify(); }

#if SIMPLE
    template <typename F>
    void async_(F&& f) {
        _q.push_notify({[](void* x) {
            function<void()>* p = static_cast<function<void()>*>(x);
            (*p)();
            delete p;
        }, new function<void()>(forward<F>(f))});
    }
#else
    template <typename F>
    void async_(F&& f) {
        _q.push_notify(forward<F>(f));
    }

    template <typename F>
    auto async(F f) -> future<result_of_t<F()>> {
        auto p = package_task<result_of_t<F()>()>(move(f));
        _q.push_notify(move(p.first));
        return move(p.second);
    }
#endif
};


template <typename F, typename... Args>
auto when_all(F f, future<Args>&&...) {
    return package_task<result_of_t<F(Args...)>>(f);
}

task_system _system;

template <typename F>
void async_(F&& f) {
    _system.async_(forward<F>(f));
}

__attribute__ ((noinline)) void time() {
    condition_variable ready;
    bool done = false;

    for (int n = 0; n != 1000000; ++n) {
        async_([]{
        #if 0
            int a[1000];
            iota(begin(a), end(a), 0);
            shuffle(begin(a), end(a), default_random_engine{});
            sort(begin(a), end(a));
        #endif
        });
    }

    async_([&]{ done = true; ready.notify_one(); });

    mutex block;
    unique_lock<mutex> lock{block};
    while (!done) ready.wait(lock);
}

#include <future>

int main() {

#if 0
    condition_variable _ready;
    mutex              _hold;

    unique_lock<mutex> lock{_hold};
    auto t = async([&]{
        this_thread::sleep_for(seconds(1)); // time for pop
        {
        unique_lock<mutex> lock{_hold}; // get a lock for the push
        cout << "push()" << endl;
        }
        _ready.notify_one();
        cout << "notify_one()" << endl;
    }); // does this noification wait?

    cout << "pop()" << endl; // should be empty
    this_thread::sleep_for(seconds(2)); // time for push and notify before wait
    _ready.wait(lock); // hang...
    cout << "wait()" << endl;

#endif


    auto start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

    start = chrono::high_resolution_clock::now();
    time();
    cout << chrono::duration_cast<chrono::milliseconds>
        (chrono::high_resolution_clock::now()-start).count() << endl;

#if 0
    pair<int, int> a[100];
    int count = 0;

    auto cmp = [](const auto& x, const auto& y){ return x.first <= y.first; };
    transform(begin(a), end(a), begin(a), [&](pair<int, int>& x){ ++count; return make_pair(count % 10, count); });
    make_heap(begin(a), end(a), cmp);

    for (auto f = begin(a), l = end(a); f != l; --l) {
        pop_heap(f, l, cmp);
        cout << (l - 1)->first << ", " << (l - 1)->second << endl;
    }
#endif
}

#endif

#if 0
/*

promise - - - > shared
future  ------> shared


shared  } - - > group

group   }-----> shared (via futures)
group
    task
*/


template <typename T>
class packaged_task;

template <typename T>
class promise {
    struct shared_state {
        using queue_type = list<function<void(T)>>;
        aligned_storage_t<sizeof(T)> _result;

        mutex   _mutex;
        bool _resolved{false};
        queue_type _q;

        const T& get() const {
            return *static_cast<const T*>(static_cast<const void*>(&_result));
        }

        ~shared_state() {
            if (_resolved) get().~T();
        }

        void set_value(T x) {
            new (&_result)T(move(x));

            queue_type tmp;
            {
                unique_lock<mutex> lock{_mutex};
                _resolved = true;
                swap(_q, tmp);
            }

            execute(tmp);
        }

        void execute(const queue_type& q) {
            for (const auto& f : q) f(get());
        }

        template <typename F>
        auto then(F f) {
            packaged_task<result_of_t<F(T)>(T)> p{move(f)};
            auto result = p.get_future();
            queue_type tmp{move(p)};
            {
                unique_lock<mutex> lock{_mutex};
                if (!_resolved) _q.splice(_q.end(), tmp);
            }
            execute(tmp);
            return result;
        }
    };

    weak_ptr<shared_state> _shared;

  public:
    class future {
        shared_ptr<shared_state> _shared;

        friend promise;
        explicit future(shared_ptr<shared_state> shared) : _shared(move(shared)) { }
      public:
        future() = default;

        bool valid() const { return _shared; }

        template <typename F>
        auto then(F f) -> typename promise<result_of_t<F(const T&)>>::future {
            return _shared->then(move(f));
        }
    };

    future get_future() {
        shared_ptr<shared_state> shared = make_shared<shared_state>();
        _shared = shared;
        return future(shared);
    }

    template <typename F, typename... Args>
    void set_to_result(F f, Args&&... args) const {
        auto lock = _shared.lock();
        if (lock) lock->set_value(f(std::forward<Args>(args)...));
    }
};

template <typename R, typename... Args>
class packaged_task<R(Args...)> {
    function<R(Args...)> _f;
    promise<R> _p;

  public:
    using result_type = void;

    template <typename F>
    explicit packaged_task(F f) : _f(move(f)) { }

    auto get_future() { return _p.get_future(); }

    void operator()(Args&&... args) const {
        _p.set_to_result(_f, std::forward<Args>(args)...);
    }
};

template <typename T>
using future = typename promise<T>::future;

template <typename> class group;
template <typename... Args>
class group<void(Args...)> {
    atomic<size_t>      _count{sizeof...(Args)};
    function<void()>    _f;

    void available() {
        if (--_count == 0) _f();
    }
};

template <typename T>
class notification_queue {
    list<T> _q;
    bool    _done = false;
    mutex   _mutex;
    condition_variable _ready;

  public:
    void push_notify(T x) {
        list<T> tmp{move(x)};
        {
            unique_lock<mutex> lock{_mutex};
            _q.splice(_q.end(), tmp);
        }
        _ready.notify_one();
    }

    void done_notify() {
        {
            unique_lock<mutex> lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    list<T> wait_pop() {
        list<T> tmp;
        {
            unique_lock<mutex> lock{_mutex};
            while (_q.empty() && !_done) _ready.wait(lock);
            if (!_q.empty()) tmp.splice(tmp.end(), _q, _q.begin());
        }
        return tmp;
    }
};

class thread_pool {
    vector<thread> _pool;

  public:
    template <typename F>
    explicit thread_pool(F f) {
        for (auto n = thread::hardware_concurrency(); n != 0; --n) {
            _pool.emplace_back(f);
        }
    }
    ~thread_pool() {
        for (auto& e : _pool) e.join();
    }
};

class task_system {
    notification_queue<function<void()>> _q;
    thread_pool _pool;

  public:
    task_system() : _pool{[&]{
        while (true) {
            auto e = _q.wait_pop();
            if (e.empty()) break;
            e.front()();
        }
    }} { }

    ~task_system() { _q.done_notify(); }

    template <typename F>
    auto async(F f) -> future<result_of_t<F()>> {
        auto p = packaged_task<result_of_t<F()>()>(move(f));
        auto result = p.get_future();
        _q.push_notify(p);
        return result;
    }
};


int main() {

    mutex out_mutex;
    task_system tasks;

    auto x = tasks.async([&]{
        unique_lock<mutex> lock(out_mutex);
        cout << "initial" << endl;
        return 42;
    }).then([](int a){
        cout << "a:" << a << endl;
        return a * 2;
    }) /* .then([&](int a){
        unique_lock<mutex> lock(out_mutex);
        cout << "a:" << a << endl;
        return 1;
    }) */;

    vector<future<int>> a;

    for (auto n = 0; n != 20; ++n) {
        a.push_back(tasks.async([n,&out_mutex]{
            this_thread::sleep_for(seconds(n % 3));
            unique_lock<mutex> lock(out_mutex);
            cout << "pass one:" << n << endl;
            return 1;
        }));
    }
    for (auto n = 0; n != 20; ++n) {
        a[n] = tasks.async([n,&out_mutex]{
            this_thread::sleep_for(seconds(n % 3));
            unique_lock<mutex> lock(out_mutex);
            cout << "pass 2:" << n << endl;
            return 1;
        });
    }

   this_thread::sleep_for(seconds(30));
}


#endif



#if 0

#include <iostream>

using namespace std;

template <typename F>
void make_callable(F f) { cout << "non-member" << endl; }

template <typename R, typename T>
void make_callable(R (T::* m)) { cout << "data member" << endl; }

template <typename R, typename T, typename... Args>
void make_callable(R (T::* m)(Args...)) { cout << "member function" << endl; }

template <typename R, typename T, typename... Args>
void make_callable(R (T::* m)(Args...) const) { cout << "const member function" << endl; }

#if 0
struct test {
    int data;
    void function() const { }
};
#endif

template <typename R, typename O>
auto find_if(const R& r, O op)
{ return std::find(begin(r), end(r), std::bind(op, std::placeholders::_1)); }

struct test {
    bool is_selected() const { return true; }
};


int main() {
    test a[] = { test(), test(), test() };
    auto p = find_if(a, &test::is_selected);
}

#endif


#if 0

#include <algorithm>
#include <utility>
#include <cassert>
#include <iostream>
#include <functional>

#include <adobe/algorithm.hpp>

namespace details {

template <typename R, typename T, typename ...Args>
struct callable {
    using result_type = R;

    explicit callable(R (T::* p)(Args...)) : _p(p) { }

    template <typename... Brgs>
    auto operator()(T& x, Brgs&&... args) const { return (x.*_p)(std::forward<Brgs>(args)...); }

    template <typename... Brgs>
    auto operator()(T* x, Brgs&&... args) const { return (x->*_p)(std::forward<Brgs>(args)...); }

    R (T::* _p)(Args...);
};

template <typename T, typename ...Args>
struct callable<void, T, Args...> {
    using result_type = void;

    explicit callable(void (T::* p)(Args...)) : _p(p) { }

    template <typename... Brgs>
    void operator()(T& x, Brgs&&... args) const { (x.*_p)(std::forward<Brgs>(args)...); }

    template <typename... Brgs>
    void operator()(T* x, Brgs&&... args) const { (x->*_p)(std::forward<Brgs>(args)...); }

    void (T::* _p)(Args...);
};

template <typename R, typename T, typename ...Args>
struct callable_const{
    using result_type = R;

    explicit callable_const(R (T::* p)(Args...) const) : _p(p) { }

    template <typename... Brgs>
    auto operator()(const T& x, Brgs&&... args) const { return (x.*_p)(std::forward<Brgs>(args)...); }

    template <typename... Brgs>
    auto operator()(const T* x, Brgs&&... args) const { return (x->*_p)(std::forward<Brgs>(args)...); }

    R (T::* _p)(Args...) const;
};

template <typename T, typename ...Args>
struct callable_const<void, T, Args...> {
    using result_type = void;

    explicit callable_const(void (T::* p)(Args...) const) : _p(p) { }

    template <typename... Brgs>
    void operator()(T& x, Brgs&&... args) const { (x.*_p)(std::forward<Brgs>(args)...); }
    template <typename... Brgs>
    void operator()(const T& x, Brgs&&... args) const { (x.*_p)(std::forward<Brgs>(args)...); }

    template <typename... Brgs>
    void operator()(T* x, Brgs&&... args) const { (x->*_p)(std::forward<Brgs>(args)...); }
    template <typename... Brgs>
    void operator()(const T* x, Brgs&&... args) const { (x->*_p)(std::forward<Brgs>(args)...); }

    void (T::* _p)(Args...) const;
};

}

template <typename F>
auto make_callable(F f) { return f; }

template <typename R, typename T>
auto make_callable(R (T::* m)) { return std::bind(m, std::placeholders::_1); }

template <typename R, typename T, typename... Args>
auto make_callable(R (T::* m)(Args...)) { return details::callable<R, T, Args...>(m); }

template <typename R, typename T, typename... Args>
auto make_callable(R (T::* m)(Args...) const) { return details::callable_const<R, T, Args...>(m); }


using namespace std;

enum class color {
    red,
    green,
    blue,
    magenta,
    cyan
};

const pair<color, size_t> map_[] = {
    { color::red, 3 },
    { color::green, 1 },
    { color::blue, 2 },
    { color::magenta, 0 }};


const auto compare_project_2 = [](auto compare, auto project) {
    return [=](const auto& x, const auto& y) { return compare(project(x), project(y)); };
};

const auto compare_project_1 = [](auto compare, auto project) {
    return [=](const auto& x, const auto& y) { return compare(project(x), y); };
};

struct test {
    void member() const { cout << "called member" << endl; }
    const char* member1() const { return "member1"; }
    const char* member2(int) const { return "member2"; }
    const char* member3(int, int) const { return "member3"; }
    void member4(int, int) const { cout << "called member4" << endl; }
    void member5(int, int) { cout << "called member5" << endl; }

};

struct make_invokable_fn
{
   template<typename R, typename T>
   auto operator()(R T::* p) const -> decltype(std::mem_fn(p))
   {
       return std::mem_fn(p);
   }

   template<typename T, typename U = std::decay_t<T>>
   auto operator()(T && t) const ->
       enable_if_t<!std::is_member_pointer<U>::value, T>
   {
       return std::forward<T>(t);
   }
};

constexpr make_invokable_fn invokable {};

template<typename R, typename O>
auto find_if(const R& r, O op)
{ return std::find_if(begin(r), end(r), std::bind(op, std::placeholders::_1)); }

struct test2 {
   bool is_selected() const { cout << "is_selected" << endl; return true; }
};


int main() {
   test2 a[] = { test2(), test2(), test2() };
   //auto p = find_if(a, &test2::is_selected);

   //auto b = bind(&test2::is_selected, placeholders::_1);
   auto p = find_if(a, bind(&test2::is_selected, placeholders::_1));
}

#if 0

int main() {
    assert(adobe::is_sorted(map_, less<>(), &pair<color, size_t>::first) && "map must be sorted");

    color to_find = color::cyan;

    auto p2 = adobe::lower_bound(map_, to_find, less<>(), &pair<color, size_t>::first);
    if (p2 == end(map_) || p2->first != to_find) cout << "not found" << endl;
    else cout << p2->second << endl;


    auto p = &pair<color, size_t>::second;
    pair<color, size_t> x = { color::cyan, 42 };

    cout <<  x.*p << endl;


    cout << std::bind(&pair<color, size_t>::second, std::placeholders::_1)(make_pair(color::magenta, size_t(42))) << endl;

    cout << make_callable(&pair<color, size_t>::second)(make_pair(color::magenta, size_t(42))) << endl;

    make_callable(&test::member)(test());
    cout << make_callable(&test::member1)(test()) << endl;
    cout << make_callable(&test::member2)(test(), 42) << endl;
    cout << make_callable(&test::member3)(test(), 42, 42) << endl;
    make_callable(&test::member4)(test(), 10, 10);
    auto t = test();
    make_callable(&test::member5)(t, 10, 10);

    invokable(&test::member)(test());
    cout << invokable(&test::member1)(test()) << endl;
    cout << invokable(&test::member2)(test(), 42) << endl;
    cout << invokable(&test::member3)(test(), 42, 42) << endl;
    make_callable(&test::member4)(test(), 10, 10);
    invokable(&test::member5)(t, 10, 10);
}
#endif


#endif

























/*
    Can I build a complete, lock-free, micro tasking system? Using only C++14
    Thread pool / task queue
    Future, then, group
    task cancelation
    producer / consumer co-routine with await
*/





#if 0

#include <cassert>
#include <vector>

#define rcNULL 0

struct ZFileSpec {
};

bool operator!=(const ZFileSpec&, const ZFileSpec&);

struct TrackedFileInfo {
};

using PTrackedFileInfo = TrackedFileInfo*;

struct LinkedFileInfo;

using PLinkedFileInfo = LinkedFileInfo*;

struct LinkedFileInfo {
    PTrackedFileInfo GetTrackedFileInfo();
    ZFileSpec GetTrackedFileSpec(bool);

    bool TreatAsInstance(PLinkedFileInfo);
};

struct TrackedLinkedFileInfoList {
    using iterator = PLinkedFileInfo*;

    bool empty();
    iterator begin();
    iterator end();
};
struct TImageDocument {
    TrackedLinkedFileInfoList* GetTrackedLinkedFileInfoList();
};
struct ImageState {
};

struct ImageStateEditor {
    explicit ImageStateEditor(TImageDocument*);
    ImageState GetImageState();
    void Defloat();
};

bool DocumentExists(TImageDocument*);
bool HasFrontImage();
bool IsImageDocumentFront(TImageDocument*);
bool IsSimpleSheet(TImageDocument*);

using namespace std;


void UpdateLinkedFilesForDocument(TImageDocument* document, const ZFileSpec* onlyThisLink,
        bool forceUpdate )
{
    if (!DocumentExists(document)) return;

    assert(document);
    if (!forceUpdate && (!HasFrontImage() || !IsImageDocumentFront(document))) return;

    if ( IsSimpleSheet(document) ) return;

    bool didMakeChanges = false;
    TrackedLinkedFileInfoList* trackList = document->GetTrackedLinkedFileInfoList();

    if ( trackList->empty () ) return;

    vector<PLinkedFileInfo> updatedFileInfos;
    ImageStateEditor editor(document);

    ImageState oldState = editor.GetImageState ();
    editor.Defloat ();
    // check list of tracked attached to document's current (image) state
    for (TrackedLinkedFileInfoList::iterator item = trackList->begin();
        item != trackList->end();
        ++item )
    {
        PLinkedFileInfo existingInfo = *item;
        PTrackedFileInfo trackInfo = existingInfo->GetTrackedFileInfo ();

        if (trackInfo == rcNULL) continue;

        ZFileSpec itemSpec = existingInfo->GetTrackedFileSpec (true);

        if (onlyThisLink != NULL && itemSpec != *onlyThisLink) continue;

        bool processedPseudoInstance = false;
        for (std::size_t index = 0; index < updatedFileInfos.size(); ++index)
        {
            if (existingInfo->TreatAsInstance (updatedFileInfos[index]))
            {
                processedPseudoInstance = true;
                break;
            }
        }
        if (processedPseudoInstance) continue;

        bool externalFileStatusChanged = false;

        if (UpdateTrackedFileStatus (existingInfo, externalFileStatusChanged))
        {
            bool fileMatchesLastLoadFailure = (trackInfo->fFileStamp == existingInfo->fLastLoadFailureFileStamp);

            bool doUpdateFile = trackInfo->ShouldAutoUpdate()
                && (existingInfo->fExternalFileDiffers && !fileMatchesLastLoadFailure);

            if (doUpdateFile)
            {
                updatedFileInfos.push_back(existingInfo);
                if (externalFileStatusChanged) UpdateLayersPanel();

                CProgress progress ("$$$/Links/Progress/UpdateSmartObjects=Updating Smart Objects");
                AFile theFile( NewFile('    ', '    ', !kDataOpen) );
                theFile->SpecifyWithZFileSpec( itemSpec );
                FailOSErr (theFile->OpenFile ());

                bool isExternalLink = existingInfo->GetIsExternalLinkedFile();
                PLinkedFileInfo newLinkedFileInfo = TLinkedFileInfo::MakeFromFile( theFile.get(), isExternalLink, document, existingInfo );
                theFile.reset();
                bool didMakeChangesToThisItem = UpdateOnePlacedFile( existingInfo, newLinkedFileInfo, editor, document );
                didMakeChanges = didMakeChanges || didMakeChangesToThisItem;
            }

        }
    }

    if ( didMakeChanges )
    {
        document->SetImageState(oldState, csCurrent, isfPreserveVisibility, kRedraw);
        editor.InvalidateAllPixels();
        editor.InvalidateChannels(CompositeChannels ( editor.GetImageMode() ));
        editor.InvalidateSheetChannels();
        editor.InvalidateMerged();
        editor.InvalidateThumbnailCache();
        SetPendingChannelStateToAdjustedState (document, editor.GetImageState(), false);

        ValidatePendingChannelState (document, editor.GetImageState());
        editor.VerifyComps ();
        CommandInfo cmdInfo (cPlacedLayerUpdate,
                "$$$/Commands/UpdateSmartObjects=Update Smart Objects",
                GetStringID(kupdatePlacedLayerStr),
                gNullActionDescriptor,
                kDialogOptional);
                editor.PostCommand (cmdInfo, document, csPending);
                editor.PostNonUndoableCommand (cmdInfo, document, 0, csCurrent);
    }
}

#endif


#if 0

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <array>

#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace placeholders;

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

template <typename T>
class remote {
    unique_ptr<T> data_;
 public:
    remote() = default;

    template <typename... Args>
    remote(Args&&... args) : data_(new T(forward<Args>(args)...)) { }

  //   remote(T x) : data_(new T(move(x))) { }

    operator const T& () const { return *data_; }
    operator T& () { return *data_; }
    const T& get() const { return *data_; }
    T& get() { return *data_; }

    remote(const remote& x) : data_(new T(x)) { }

    remote(remote&& x) noexcept = default;
    remote& operator=(remote&& x) noexcept = default;

    remote& operator=(const remote& x) {
        remote tmp(x); *this = move(tmp); return *this;
    }
};

template <class T, class N, class O>
T power(T x, N n, O op)
{
    if (n == 0) return identity_element(op);

    while ((n & 1) == 0) {
        n >>= 1;
        x = op(x, x);
    }

    T result = x;
    n >>= 1;
    while (n != 0) {
        x = op(x, x);
        if ((n & 1) != 0) result = op(result, x);
        n >>= 1;
    }
    return result;
}

template <typename N>
struct multiply_2x2 {
    array<N, 4> operator()(const array<N, 4>& x, const array<N, 4>& y)
    {
        return { x[0] * y[0] + x[1] * y[2],
                 x[0] * y[1] + x[1] * y[3],
                 x[2] * y[0] + x[3] * y[2],
                 x[2] * y[1] + x[3] * y[3] };
    }
};

template <typename N>
array<N, 4> identity_element(const multiply_2x2<N>&) { return { N(1), N(0), N(0), N(1) }; }

template <typename N>
N fibonacci(N n) {
    if (n == 0) return N();
    return power(array<N, 4>{ 1, 1, 1, 0 }, N(n - 1), multiply_2x2<N>())[0];
}

using namespace boost::multiprecision;

int main() {
    cout << fibonacci(cpp_int(20000)) << endl;
}

int main() {
    // auto x = f(annotate());

    remote<pair<int, string>> x = { 52, "Hello World!" };


    auto y = remote<annotate>(annotate());
    auto z = remote<annotate>(annotate());

    assert(y == z);

    swap(y, z);
}

#endif




#if 0

#include <iostream>
#include <vector>

#include <range/v3/container/push_back.hpp>

using namespace std;
using namespace ranges;

int main() {

    vector<int> x { 0, 1, 2, 3, 4 };
    vector<int> y { 5, 6, 7, 8, 9 };

    push_back(x, y);

    for (const auto& e : x) cout << e << " ";
    cout << endl;


}

#endif

#if 0 // KEEP CLASS


#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;
using namespace placeholders;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }

    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { cout << "annotate dtor" << endl; }
    // friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
     friend inline bool operator==(const annotate&, const annotate&) { return true; }
     friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

template <typename T>
class remote {
    unique_ptr<T> data_;
 public:
    remote(); // WHAT SHOULD THIS DO?

    remote(T x) : data_(new T(move(x))) { }
    operator const T& () const { return *data_; }
    operator T& () { return *data_; }

    remote(const remote& x) : data_(new T(x)) { }

    remote(remote&& x) noexcept = default;
    remote& operator=(remote&& x) noexcept = default;

    remote& operator=(const remote& x) { *data_ = x; return *this; } // PROBLEM
};

remote<annotate> f(remote<annotate> x) { return x; }

int main() {
    // auto x = f(annotate());

    auto y = remote<annotate>(annotate());
    auto z = remote<annotate>(annotate());

    assert(y == z);

    swap(y, z);
}


#endif










#if 0

#include <iostream>
#include <functional>
#include <memory>
#include <cassert>
#include <utility>

using namespace std;

/*
    Problem: We want 'any' to be serializable. But 'any' can be implicitly constructed
    from a type T making it difficult to determine if T is serializable or if T converted to an
    'any' is serializable.
    
    Our stream operator is defined as:
    
    ostream& operator<<(ostream& out, const const_explicit_any& x);
    
    The const_explicit_any class provides a simple implicit conversion from 'any'.
    Only one implicit user conversion is allowed, so we cannot go T -> any -> const_explicit_any.
*/

struct any;

struct const_explicit_any {
    const_explicit_any(const any& x) : p_(&x) { }
    const any* p_;
};

class any {
  public:
    template <typename T>
    any(T x) : self(make_unique<model<T>>(move(x))) { }

    //...

    bool is_serializable() const { return self->is_serializable(); }

    friend inline ostream& operator<<(ostream& out, const const_explicit_any& x)
    { return x.p_->self->serialize(out); }

  private:

    struct concept {
        virtual ostream& serialize(ostream& out) const = 0;
        virtual bool is_serializable() const = 0;
    };

    template <typename T>
    struct model : concept {
        model(T x) : object(move(x)) { }

        ostream& serialize(ostream& out) const override { return out << "any:" << object; }

        bool is_serializable() const override { return true; }

        T object;
    };

    unique_ptr<concept> self;
};

int main() {
    cout << any(10) << endl;
    // cout << any(make_pair(10, 10)) << endl; // COMPILER ERROR!
    cout << 10 << endl;
    // cout << make_pair(10, 10) << endl;  // COMPILER ERROR!
}

#endif



#if 0

#include <iostream>
#include <functional>
#include <memory>
#include <cassert>
#include <utility>

using namespace std;

/*

    Problem: We want 'any' to be optionally serializable. But 'any' can be implicitly constructed
    from a type T making it difficult to determine if T is serializable or if T converted to an
    'any' is serializable.
    
    To detect we SFINAE on this expression:
    
        declval<ostream&>() << declval<T>()

    Our stream operator is defined as:
    
    ostream& operator<<(ostream& out, const const_explicit_any& x);
    
    The const_explicit_any class provides a simple implicit conversion from 'any'.
    Only one implicit user conversion is allowed, so we cannot go T -> any -> const_explicit_any.
*/

struct any;

// template <typename T> struct convert { convert(); operator T() const; };
template <typename T> false_type direct_serializable(...);
template <typename T> true_type direct_serializable(decltype(&(declval<ostream&>() << declval<T>())));

struct const_explicit_any {
    const_explicit_any(const any& x) : p_(&x) { }
    const any* p_;
};

class any {
  public:
    template <typename T>
    any(T x) : self(make_unique<model<T, decltype(direct_serializable<T>(0))::value>>(move(x))) { }

    //...

    bool is_serializable() const { return self->is_serializable(); }

    friend inline ostream& operator<<(ostream& out, const const_explicit_any& x)
    { return x.p_->self->serialize(out); }

  private:

    struct concept {
        virtual ostream& serialize(ostream& out) const = 0;
        virtual bool is_serializable() const = 0;
    };

    template <typename, bool> struct model;

    template <typename T>
    struct model<T, true> : concept {
        model(T x) : object(move(x)) { }

        ostream& serialize(ostream& out) const override { return out << "any:" << object; }

        bool is_serializable() const override { return true; }

        T object;
    };

    template <typename T>
    struct model<T, false> : concept {
        model(T x) : object(move(x)) { }

        ostream& serialize(ostream& out) const override { return out << "any: <not serializable>"; }
        bool is_serializable() const override { return false; }

        T object;
    };

    unique_ptr<concept> self;
};

int main() {
    cout << any(10) << endl;
    cout << any(make_pair(10, 10)) << endl; // Runtime failure
    cout << 10 << endl;

    // cout << make_pair(10, 10) << endl;  // COMPILER ERROR!
}

#endif

#if 0

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace placeholders;

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

template <typename T>
void g(T&& x) {
    f(x);
}

template <typename T>
void f(T&& x) {
    cout << is_reference<T>::value << is_const<typename remove_reference<T>::type>::value << endl;
}

const annotate f() { return annotate(); }


int main() {
    g(annotate());
    g(f());
    annotate x;
    g(x);
    const annotate y;
    g(y);

}

#endif


#if 0
// SAVE - class

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace placeholders;

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

template <typename T>
class remote {
    T* data_;
 public:
    remote(T x) : data_(new T(move(x))) { }
    operator const T& () const { return *data_; }
    operator T& () { return *data_; }
    ~remote() { delete data_; }

    remote(const remote& x) : data_(new T(x)) { }
    remote& operator=(const remote& x) { *data_ = x; return *this; }
};

annotate f(annotate x) { return x; }

int main() {
#if 0
    remote<annotate> x = annotate();

    x = annotate();
    // remote<int> y = 20;
#else
    auto x = f(annotate());
#endif



   // x = y;




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
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

template <typename T>
class remote {
    T* data_;

  public:
    remote(T x) : data_(new T(move(x))) { }
    ~remote() { delete data_; }
    operator const T&() const { return *data_; }

    remote(const remote&) = delete;
    remote operator=(const remote&) = delete;
};


int main() {
    remote<annotate> x{annotate()};
    x = annotate();
#if 0
    remote<annotate> x{annotate()};
    annotate y = x;
    remote<annotate> z(y);

    cout << (x == z) << endl;
#endif
};


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
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; }
    friend inline bool operator==(const annotate&, const annotate&) { return true; }
    friend inline bool operator!=(const annotate&, const annotate&) { return false; }
};

template <typename T>
T sink(T x) { return x; }

int main() {
    auto x = sink(annotate());

}


#endif


#if 0

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace std;

template <typename F> // F models UnaryFunction
struct pipeable {
    F op_;

    template <typename T>
    auto operator()(T&& x) const { return op_(forward<T>(x)); }
};

template <typename F>
auto make_pipeable(F x) { return pipeable<F>{ move(x) }; }

template <typename F, typename... Args>
auto make_pipeable(F x, Args&&... args) {
    return make_pipeable(bind(move(x), placeholders::_1, forward<Args>(args)...));
}

template <typename T, typename F>
auto operator|(T&& x, const pipeable<F>& op) { return op(forward<T>(x)); }

template <typename F>
auto sort(F op)
{
    return make_pipeable([](auto x, auto op) {
        sort(begin(x), end(x), move(op));
        return x;
    }, op);
}

auto sort()
{
    return sort(less<>());
}

auto unique() {
    return make_pipeable([](auto x) {
        auto p = unique(begin(x), end(x));
        x.erase(p, end(x));
        return x;
    });
}

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

    friend inline bool operator<(const annotate&, const annotate&) { return false; }
};

int main() {
    vector<annotate> c;
    c.push_back(annotate());
    c.push_back(annotate());

    auto x = move(c) | sort() | unique();


   //  auto x = vector<int>{10, 9, 9, 8, 6, 5, 5, 5, 2, 1 } | sort(greater<>()) | unique();

   // auto x = container::unique(container::sort(vector<int>{10, 9, 9, 8, 6, 5, 5, 5, 2, 1 }));

    //for (const auto& e : x) cout << e << " ";
    cout << endl;


}

#endif


#if 0

#include <memory>
#include <exception>

/*

future with then() taking function directly
exception propegation
task cancelation (throwing exception cancels subsequent tasks), destructing future cancels the task.

simple interface. Do we need promise future pair or just create future and copy it?
no get().

*/

namespace future_ {

template <typename T>
struct shared {

};

} // namespace future_

template <typename T>
class future {
    future();
};

template <typename T>
class promise {
    std::shared_ptr<future_::shared<T>> shared_;
public:
    void set_value(const T&);
    void set_value(T&&);
    void set_exception(std::exception_ptr);

    future<T> get_future();
};

#endif

#if 0

#include <memory>

namespace adobe {

template <typename T>
class remote {
    std::unique_ptr<T> part_;

public:
    template <typename... Args>
    remote(Args&&... args) : part_(new T(std::forward<Args>(args)...)) { }

    remote(const remote& x) : part_(new T(x)) { }
    remote(remote&&) noexcept = default;

    remote& operator=(const remote& x) { remote tmp = x; *this = std::move(tmp); return *this; }
    remote& operator=(remote&&) noexcept = default;

    //remote& operator=(T x) noexcept { *this = remote(std::move(x)); return *this; }

    operator T&() { return *part_; }
    operator const T&() const { return *part_; }

    friend inline bool operator==(const remote& x, const remote& y)
    { return static_cast<const T&>(x) == static_cast<const T&>(y); }

    friend inline bool operator!=(const remote& x, const remote& y)
    { return !(x == y); }

    friend inline bool operator==(const T& x, const remote& y)
    { return x == y.get(); }

    friend inline bool operator!=(const T& x, const remote& y)
    { return !(x == y); }

    friend inline bool operator==(const remote& x, const T& y)
    { return static_cast<const T&>(x) == y; }

    friend inline bool operator!=(const remote& x, const T& y)
    { return !(x == y); }

    T& get() { return *part_; }
    const T& get() const { return *part_; }
};

} // namespace adobe

#include <iostream>

using namespace adobe;
using namespace std;

int main() {
    remote<pair<int, double>> x(5, 3.0);
    remote<pair<int, double>> y = { 5, 32.0 };
    //x = "world";

    cout << x.get().first << y.get().second << endl;
}

#endif



#if 0
#include <iostream>

#include <cfenv>

using namespace std;

 __attribute__((noinline)) double neg_zero()
{ return -0.0; }

int main() {
    cout << 0.0 + neg_zero() << endl;

    fesetround(FE_DOWNWARD);

    cout << 0.0 + neg_zero() << endl;

    cout << neg_zero() << endl;

}
#endif


#if 0

#include <iostream>

using namespace std;

template <typename T> // T is ???
class remote {
    unique_ptr<T> part_;

  public:
    remote(T x) : part_(new T(move(x))) { }
    remote(const remote& x) : part_(new T(*x.part_)) { }
    remote& operator=(const remote& x) { *part_ = *x.part_;  return *this; }

    remote(remote&& x) noexcept = default;
    remote& operator=(remote&&) noexcept = default;

    T& operator*() { return *part_; }
    const T& operator*() const { return *part_; }
    T* operator->() { return &*part_; }
    const T* operator->() const { return &*part_; }
};

template <typename T>
bool operator==(const remote<T>& x, const remote<T>& y) {
    return *x == *y;
}

template <typename T>
bool operator!=(const remote<T>& x, const remote<T>& y) {
    return !(x == y);
}

template <typename T, typename... Args>
remote<T> make_remote(Args&&... args) { return remote<T>(T(forward<Args>(args)...)); }

int main() {

    auto x = make_remote<int>(5);
    auto y = x;
    auto z = move(x);
    x = y;

    cout << *z << endl;
    cout << *x << endl;
    cout << *y << endl;



}

#endif




























#if 0

#include <algorithm>
#include <list>
#include <iostream>

namespace adobe {

template <typename I, // I models ForwardIterator
          typename N> // N is distance_type(I)
I reverse_n(I f, N n) // return f + n
{
    if (n < 2) return std::next(f, n);
    I m = reverse_n(f, n / 2);
    I l = reverse_n(m, n - (n / 2));
    std::rotate(f, m, l);
    return l;
}

template <typename I> // I models ForwardIterator
void reverse(I f, I l)
{
    reverse_n(f, std::distance(f, l));
}

} // namespace adobe


int main() {

    double x = -0.0;

    x += 0.0;

    std::cout << x << std::endl;

    std::list<int> a = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    adobe::reverse(begin(a), end(a));

    for (const auto& e : a) std::cout << e << " ";
    std::cout << std::endl;

}

#endif

#if 0

#include <future>
#include <functional>
#include <iostream>
#include <cmath>

#include <dispatch/dispatch.h>

namespace adobe {


template <typename F, typename ...Args>
auto async(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;
    using packaged_type = std::packaged_task<result_type ()>;

    auto p = new packaged_type(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
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

double foo(double x) { return std::sin(x*x); }


size_t search(int* a, size_t n, int x)
{
    size_t i = 0;

    while (n != 0)  {
        size_t h = n / 2;
        if (a[i + h] < x ) {
            i += h + 1;
            n -= h + 1;
        } else {
            n = h;
        }
    }
    return i;
}

using namespace std;

int main()
{
    int a[] = { 0, 1, 2, 3, 3, 3, 7, 10 };
    cout << search(a, end(a) - begin(a), -1) << endl;
    cout << search(a, end(a) - begin(a), 0) << endl;
    cout << search(a, end(a) - begin(a), 2) << endl;
    cout << search(a, end(a) - begin(a), 3) << endl;
    cout << search(a, end(a) - begin(a), 5) << endl;
    cout << search(a, end(a) - begin(a), 10) << endl;
    cout << search(a, end(a) - begin(a), 11) << endl;


  // compiles and runs ok
  auto result1=adobe::async([](){std::cout << "hello\n";});
  result1.get();
  // doesn't compile
  auto result2=adobe::async(foo,0.5);
  std::cout << result2.get() << "\n";
}

#endif

#if 0

#include <memory>
#include <chrono>
#include <iostream>
#include <functional>
#include <adobe/future.hpp>

using namespace adobe;
using namespace std;
using namespace chrono;

template <typename T>
void async_chain(T x) {
    auto shared =  make_shared<pair<T, function<void()>>>(x, function<void()>());
    shared->second = [=]{
        if (shared->first.while_()) { shared->first.body_(); adobe::async(shared->second); }
        else adobe::async(shared->first);
    };
    adobe::async(shared->second);
}


int main() {

    struct to_ten {
        int count = 0;

        bool while_() { return count != 10; }
        void body_() { ++count; }
        void operator()() { cout << "done:" << count; }
    };

    async_chain(to_ten());
    cout << "waiting..." << endl;
    this_thread::sleep_for(seconds(2));

}

#endif


#if 0
#include <cstddef>
#include <vector>

namespace adobe {

struct point {
    double x;
    double y;
};

struct rectangle {
    point top_left;
    point bottom_right;
};

struct image_t { };

class layer
{
    std::vector<layer> layers_;

  public:
    // REVISIT (sparent) : These operations are a 2x3 transform. Splitting them out for convienence
    // not as basis operations.
    void rotation(double);
    double rotation() const;

    void scale(double);
    double scale() const;

    void position(const point&);
    point position() const;

    void bounds(const rectangle&);
    const rectangle& bounds() const;

    // REVISIT (sparent) : Should use a forest or similar container. But to get going quickly
    // use a traditional approach.

    layer& parent() const;

    // count of sublayers
    std::size_t size() const { return layers_.size(); }
    void insert(std::size_t n, layer x) { layers_.insert(layers_.begin() + n, std::move(x)); }

    // layer content
    void image(image_t);
    const image_t& image() const;

    // non-basis operations (container conventions)
    void push_back(layer b) { layers_.push_back(std::move(b)); }
};

} // namespace adobe

using namespace adobe;

int main()
{
    layer root;
}
#endif

#if 0

#include <cstddef>
#include <iostream>

template <typename T> // T models integral type
std::size_t count_bits_set(T x) {
    std::size_t result = 0;
    while (x) { x &= x - 1; ++result; }
    return result;
}

using namespace std;

int main() {
    cout << count_bits_set(-1) << endl;
}

#endif


#if 0 // start of conceptual-c compiler

struct conceptual_c {

/*

    namespace   = identifier '{' declaration '}'.
    declaration = identifier '=' expression.
    expression  = lambda | ...
    lambda      = '[' ']' [ '(' argument-list ')' ] '{' '}'.
    
    max = [](&args...) require same_type(args...)
    {
    }
    
    swap = [](&x, &y) require type(x) == type(y)
    {
        tmp = unsafe::move(x);
        construct(&x, unsafe::move(y));
        y = tmp;
    }

*/

bool is_namespace()
{
    if (!is_keyword("namespace")) return false;
    auto identifier = require_identifier();
    require_token("{");

    while (is_declaration()) ;

    require_token("}");
}

}; // conceptual_c

#endif


#if 0 // demonstration of libc++ bug

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

using namespace std;

template <class T>
void check_not_copyable(T&& x)
{
    static_assert(!is_copy_constructible<typename remove_reference<T>::type>::value, "fail");
}

int main()
{
    unique_ptr<int> x(new int(42));
    check_not_copyable(bind([](const unique_ptr<int>& p) { return *p; }, move(x)));
}
#endif

#if 0

#include <iostream>
#include <type_traits>
#include <utility>

using namespace std;

template< typename T >
class M {
    T data;

	//...
 public:
    explicit M(T x) : data(move(x)) { }

    template <typename F>
    auto apply(F f) -> M<typename result_of<F(T)>::type> { cout << "case 3" << endl; return f(data); }


#if 0
	template< typename U >
	M< U > apply (M< U > (*f)( T )) { cout << "case 1" << endl; return f(data); }
		// Takes a function mapping T to M< U >

	template< typename V >	// Just to be different
	M< V > apply( V (*f)( T ) ) { cout << "case 2" << endl; return M<V>(f(data)); }
		// Takes a function mapping T to V
#endif

	//...

};

M<double> f1(int x) { cout << "f1:" << x << endl; return M<double>(x); }
double f2(int x) { cout << "f2:" << x << endl; return x; }


int main() {

    M<int> x(5);
    x.apply(f1);
    x.apply(f2);

}
#endif

#if 0

/**************************************************************************************************/

#include <dispatch/dispatch.h>

#include <adobe/future.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>

#include <Block.h>

using namespace std;
using namespace chrono;
using namespace adobe;

auto test() -> int {
    thread_local int x = rand();
    return x;
}

mutex io_;

template <typename R, typename... Args>
class function_;

enum storage_t {
    local,
    remote,
    shared
};

template <typename R, typename... Args>
class function_<R (Args...)>
{
    struct concept_t {
        virtual ~concept_t() { }
        virtual R call_(Args&&... args) = 0;
        virtual void copy_(void*) const = 0;
        virtual void move_(void*) = 0;
    };

    struct empty : concept_t {
        R call_(Args&&... args) override
        { return std::declval<R>(); }

        void copy_(void* x) const override
        { new (x) empty(); }

        void move_(void* x) noexcept override
        { new (x) empty(); this->~empty(); }
    };

    template <typename F, storage_t>
    struct model;

    template <typename F>
    struct local_store : concept_t { F f_; };
    template <typename F>
    struct remote_store :concept_t { unique_ptr<F> f_; };

    template <typename F>
    struct model<F, local> : concept_t {
        model(F&& f) : f_(move(f)) { }
        model(const F& f) : f_(f) { }

        R call_(Args&&... args) override
        { return f_(forward<Args>(args)...); }

        void copy_(void* x) const override
        { new (x) model(*this); }

        void move_(void* x) noexcept override
        { new (x) model(move(*this)); this->~model(); }

        F f_;
    };

    template <typename F>
    struct model<F, remote> : concept_t {
        model(F&& f) : f_(new F(move(f))) { }

        R call_(Args&&... args) override
        { return (*f_)(forward<Args>(args)...); }

        void copy_(void* x) const override
        { new (x) model(new F(f_)); }

        void move_(void* x) noexcept override
        { new (x) model(move(*this)); this->~model(); }

        unique_ptr<F> f_;
    };

    template <typename F>
    struct model<F, shared> : concept_t {
        model(F&& f) : f_(make_shared<F>(move(f))) { }
        model(const F& f) : f_(make_shared<F>(f)) { }

        R call_(Args&&... args) override
        { return (*f_)(forward<Args>(args)...); }

        void copy_(void* x) const override
        { new (x) model(*this); }

        void move_(void* x) noexcept override
        { new (x) model(move(*this)); this->~model(); }

        shared_ptr<F> f_;
    };

    void* data() const { return static_cast<void*>(&data_); }
    concept_t& concept() const { return *static_cast<concept_t*>(data()); }

    template <typename T, typename U>
    using disable_same_t =
        typename enable_if<!is_same<typename decay<T>::type, typename decay<U>::type>::value>::type*;

    using value_type = R (Args...);

    using max_size_t = integral_constant<size_t,
        sizeof(model<value_type, remote>) < sizeof(model<value_type, shared>) ? sizeof(model<value_type, shared>)
        : sizeof(model<value_type, remote>)>;

    template <typename T>
    using use_storage = integral_constant<storage_t,
        !is_copy_constructible<T>::value ? shared
        : (sizeof(local_store<T>) <= max_size_t::value ? local : remote)>;

    mutable typename aligned_storage<sizeof(max_size_t::value)>::type data_;

  public:

    template <typename F>
    function_(F&& f, disable_same_t<F, function_> = 0)
        {
        using type = typename remove_reference<F>::type;

        static_assert(!is_copy_constructible<type>::value, "");

        #if 0

        new (data()) model<type, use_storage<type>::value>(std::forward<F>(f));

        #endif
        }

    ~function_() { concept().~concept_t(); }

    function_(const function_& x) { x.concept().copy_(data()); }
    function_(function_&& x) { x.concept().move_(data()); }

    function_& operator=(const function_& x) { auto tmp = x; *this = move(tmp); return *this; }
    function_& operator=(function_&& x) noexcept { concept().~concept_t(); x.concept().move_(data()); }

    R operator()(Args&&... args) const { return concept().call_(forward<Args>(args)...); }
};

struct mutable_f {
    int x = 7;
    int operator()() { return ++x; }
};

template <class T>
void check_not_copyable(T&& x)
{
    static_assert(!is_copy_constructible<typename remove_reference<T>::type>::value, "copyable");
}

int main()
{
    unique_ptr<int> x(new int(42));
    //function_<int ()> f = bind([](const unique_ptr<int>& p) { return *p; }, move(x));

    check_not_copyable(x);
    //check_not_copyable(bind([](const unique_ptr<int>& p) { return *p; }, move(x)));

#if 0
    std::function<int()> fm = mutable_f();
    cout << fm() << endl;
    cout << fm() << endl;

    function_<int()> fm_ = mutable_f();
    cout << fm_() << endl;
    cout << fm_() << endl;

    function_<void ()> f1 = []{ };
    cout << f() << endl;
#endif

    shared_task_queue q;

    for (int t = 0; t != 10; ++t) {
        q.async([=]{
            this_thread::sleep_for(seconds(2));
            lock_guard<mutex> lock(io_);
            cout << "sequential: " << t << " seconds" << endl;
        });
    }

    {
    auto f = std::async([&]{
        this_thread::sleep_for(seconds(5));
        lock_guard<mutex> lock(io_);
        cout << "task done" << endl;
    });
    }
    {
    lock_guard<mutex> lock(io_);
    cout << "async?" << endl;
    }

    cout << "test: " << test() << endl;
    adobe::async([]() {
        lock_guard<mutex> lock(io_);
        cout << "test: " << test() << endl;
    });

    for (int t = 5; t != 0; --t) {
        adobe::async(steady_clock::now() + seconds(t),[=]{
            lock_guard<mutex> lock(io_);
            cout << "time: " << t << " seconds" << endl;
        });
    }

#if 0
    for (int task = 0; task != 100; ++task) {
        adobe::async([task]() {
            // sleep for 1 or 3 seconds
            this_thread::sleep_for(seconds(1 + 2 * (task & 1)));
            {
            lock_guard<mutex> lock(io_);

            auto now = system_clock::now();
            auto now_c = system_clock::to_time_t(now);
            cout << "task: " << setw(2) << task << " time: " << put_time(localtime(&now_c), "%c") << endl;
            }
        });
    }
#endif
    
    this_thread::sleep_for(seconds(60));
    cout << "Done!" << endl;


    // auto queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
}


#endif


/**************************************************************************************************/


#if 0
#include <algorithm>
#include <utility>
#include <memory>
#include <iterator>
#include <cassert>
#include <vector>
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>

using namespace std;

namespace adobe {

class unsafe { };

class int_array {
    size_t size_;
    unique_ptr<int[]> data_;
public:
    explicit int_array(size_t size) : size_(size), data_(new int[size]()) { }
    int_array(const int_array& x) : size_(x.size_), data_(new int[x.size_])
    { copy(x.data_.get(), x.data_.get() + x.size_, data_.get()); }

    int_array(int_array& x, unsafe) : size_(x.size_), data_(x.data_.get()) { }

    int_array(int_array&& x) noexcept = default;
    int_array& operator=(int_array&& x) noexcept = default;

    int_array& operator=(const int_array& x);  // **

#if 0
    int_array& operator=(const int_array& x)
    { int_array tmp = x; *this = move(tmp); return *this; }
#endif

    const int* begin() const { return data_.get(); }
    const int* end() const { return data_.get() + size_; }
    size_t size() const { return size_; }
};

bool operator==(const int_array& x, const int_array& y)
{ return (x.size() == y.size()) && equal(begin(x), end(x), begin(y)); }

template <typename T>
void move_unsafe(T& x, void* raw) { new (raw) T(x, unsafe()); }

template <typename T>
void move_unsafe(void* raw, T& x) { new (&x) T(*static_cast<T*>(raw), unsafe()); }


#if 0
void swap(int_array& x, int_array& y)
{
    aligned_storage<sizeof(int_array)>::type tmp;

    move_unsafe(x, &tmp);
    move_unsafe(y, &x);
    move_unsafe(&tmp, y);

    assert(
}
#endif

} // namespace adobe

using namespace adobe;

using container = vector<int_array>; // Change this to a deque to see a huge difference

long long __attribute__ ((noinline)) time(container& r)
{
    auto start = chrono::high_resolution_clock::now();
    
    reverse(begin(r), end(r));
    
    return chrono::duration_cast<chrono::microseconds>
        (chrono::high_resolution_clock::now()-start).count();
}



int main() {


    assert(log2f != log10f);

    this_thread::sleep_for(chrono::seconds(3));
    container a(1000000, int_array(10));
    cout << "time: " << time(a) << endl;

    int_array x(10);
    int_array y = x;
    int_array z = move(x);

    x = y;

    assert(x == y);
}

#if 0
int main() {
    int_array x(10);
    int_array y = x;
    int_array z = move(x);

    x = y;

    assert(x == y);
}
#endif

#endif

#if 0

#include <dispatch/dispatch.h>

/**************************************************************************************************/

#include <adobe/future.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>

using namespace std;

auto test() -> int {
    thread_local int x = rand();
    return x;
}

mutex io_;

int main()
{

    {
    auto f = std::async([&]{
        this_thread::sleep_for(chrono::seconds(5));
        lock_guard<mutex> lock(io_);
        cout << "task done" << endl;
    });
    }
    {
    lock_guard<mutex> lock(io_);
    cout << "async?" << endl;
    }

    cout << "test: " << test() << endl;
    adobe::async([]() {
        lock_guard<mutex> lock(io_);
        cout << "test: " << test() << endl;
    });

    for (int task = 0; task != 100; ++task) {
        adobe::async([task]() {
            // sleep for 1 or 3 seconds
            this_thread::sleep_for(chrono::seconds(1 + 2 * (task & 1)));
            {
            lock_guard<mutex> lock(io_);

            auto now = chrono::system_clock::now();
            auto now_c = chrono::system_clock::to_time_t(now);
            cout << "task: " << setw(2) << task << " time: " << put_time(localtime(&now_c), "%c") << endl;
            }
        });
    }
    cout << "Done!" << endl;


    // auto queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
}

#endif


#if 0
#include <iostream>
#include <adobe/dictionary.hpp>
#include <adobe/array.hpp>

using namespace std;
using namespace adobe;

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

int main() {
    //any_regular_t x = annotate();

    dictionary_t dict;
    //dict[name_t("hello")] = move(x);

    dict[name_t("world")].assign(annotate());

    //array_t array;
    //array.emplace_back(move(x));

    vector<pair<int, int>> v;
    v.emplace_back(0, 3);
    v.push_back(pair<int, int>(0, 3));
}
#endif

#if 0

/*
    Outline-
    
    Goal: Write complete types.
*/

#include <adobe/json.hpp>


/*
    Copyright 2005-2013 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/******************************************************************************/

// stdc++
#include <vector>
#include <iostream>


// asl
#include <adobe/any_regular.hpp>
#include <adobe/array.hpp>
#include <adobe/dictionary.hpp>
#include <adobe/iomanip.hpp>
#include <adobe/iomanip_asl_cel.hpp>
#include <adobe/json.hpp>

/******************************************************************************/

struct helper_t {
    typedef adobe::any_regular_t    value_type;
    typedef std::string             string_type;
    typedef string_type             key_type;
    typedef adobe::dictionary_t     object_type;
    typedef adobe::array_t          array_type;
    typedef object_type::value_type pair_type;

    static json_type type(const value_type& x) {
        const std::type_info& type(x.type_info());

        if (type == typeid(object_type))      return json_type::object;
        else if (type == typeid(array_type))  return json_type::array;
        else if (type == typeid(string_type)) return json_type::string;
        else if (type == typeid(double))      return json_type::number;
        else if (type == typeid(bool))        return json_type::boolean;
        else if (type == typeid(adobe::empty_t)) return json_type::null;

        assert(false && "invalid type for serialization");
    }
    
    template <typename T>
    static const T& as(const value_type& x) { return x.cast<T>(); }

    static void move_append(object_type& obj, key_type& key, value_type& value) {
        obj[adobe::name_t(key.c_str())] = std::move(value);
    }
    static void append(string_type& str, const char* f, const char* l) {
        str.append(f, l);
    }
    static void move_append(array_type& array, value_type& value) {
        array.push_back(std::move(value));
    }
};

/******************************************************************************/

int main() {
    std::cout << "-=-=- asl_json_helper_smoke -=-=-\n";
    adobe::any_regular_t x = json_parser_t<helper_t>(u8R"raw(
        [
            42,
            12.536,
            -20.5,
            -1.375e+112,
            3.1415926535897932384626433832795028841971693993751058209,
            null,
            {
                "Country": "US",
                "State": "CA",
                "Address": "",
                "Longitude": -122.3959,
                "Zip": "94107",
                "City": "SAN FRANCISCO",
                "Latitude": 37.7668,
                "precision": "zip"
            },
            {
                "precision": "zip",
                "Latitude": 37.371991,
                "City": "SUNNYVALE",
                "Zip": "94085",
                "Longitude": -122.02602,
                "Address": "",
                "State": "CA",
                "Country": "US"
            }
        ]
    )raw").parse();
    
    json_generator<helper_t, std::ostream_iterator<char>>(std::ostream_iterator<char>(std::cout)).generate(x);

    std::cout << "-=-=- asl_json_helper_smoke -=-=-\n" << std::endl;
}

#endif

/******************************************************************************/



#if 0

#include <iostream>
#include <memory>
#include <vector>

using namespace std;

template <class T>
class foo { };

typedef shared_ptr<foo<int>> fooPtr_t;
typedef vector<fooPtr_t> fooPtrVector_t;

int main() {
    fooPtrVector_t myFooPtrVector;
    fooPtrVector_t::iterator it = myFooPtrVector.begin();
}

#endif




#if 0

#include <utility>  // move
#include <memory>   // unique_ptr
#include <cassert>  // assert
#include <functional> // function

using namespace std;

class value {
public:
    template <typename T>
    value(T x) : object_(new model<T>(move(x))) { }
    
    template <typename R, typename A>
    value(function<R (const A&)> x) : object_(new function_model<R, A>(move(x))) { }
    
    value(const value& x) : object_(x.object_->copy()) { }
    value(value&& x) noexcept = default;
    value& operator=(const value& x) { value tmp(x); *this = move(tmp); return *this; }
    value& operator=(value&& x) noexcept = default;
    
    value operator()(const value& x) const { return (*object_)(x); }
    template <typename T>
    const T* cast() const {
        return typeid(T) == typeid(value) ? static_cast<const T*>(static_cast<const void*>(this))
            : static_cast<const T*>(object_->cast(typeid(T)));
    }
private:
    struct concept {
        virtual ~concept() { }
        virtual concept* copy() const = 0;
        virtual value operator()(const value&) const = 0;
        virtual const void* cast(const type_info&) const = 0;
    };
    
    template <typename T>
    struct model : concept {
        model(T x) : data_(move(x)) { }
        concept* copy() const override { return new model(data_); }
        value operator()(const value&) const override { assert(false); }
        const void* cast(const type_info& t) const override
        { return t == typeid(T) ? &data_ : nullptr; }
        
        T data_;
    };
    
    template <typename R, typename A>
    struct function_model : concept {
        function_model(function<R (const A&)> x) : data_(move(x)) { }
        concept* copy() const override { return new function_model(data_); }
        value operator()(const value& x) const override {
            const A* p = x.cast<A>();
            assert(p);
            return data_(*p);
        }
        const void* cast(const type_info& t) const override
        { return t == typeid(function<R (const A&)>) ? &data_ : nullptr; }
        
        function<R (const A&)> data_;
    };
    
    unique_ptr<concept> object_;
};

#include <iostream>

int main() {
    value x = 10;
    value f = function<int (const int&)>([](const int& x){ return x + 42; });
    
    cout << *f(x).cast<int>() << endl;
    cout << *f(12).cast<int>() << endl;
    
    typedef std::function<value(const value&)> Function; // lambda functions from Value to Value

    value s = function<int (const int&)>([](const int& x){ return x + 1; });
    value t = function<value (const value&)>([](const value& v) { // twice
        function<value(const value&)> f = v;
        function<value(const value&)> ff = [f](const value& x) -> value {return f(f(x));};
        return ff;
    });

    cout << "s(0) = " << *s(0).cast<int>() << "\n";
    cout << "t(s)(0) = " << *t(s)(0).cast<int>() << "\n";
    cout << "t(t)(s)(0) = " << *t(t)(s)(0).cast<int>() << "\n";
    cout << "t(t)(t)(s)(0) = " << *t(t)(t)(s)(0).cast<int>() << "\n";
    cout << "t(t)(t)(t)(s)(0) = " << *t(t)(t)(t)(s)(0).cast<int>() << "\n";
}

#endif


#if 0
#include <algorithm>
#include <forward_list>
#include <iostream>

namespace adobe {

/*
    \c Range is a semiopen interface of the form [position, limiter) where limiter is one of the
    following.
*/

/*!
    \c ConvertibleToRange denotes a sequence of the form <code>[begin, end)</code>. The elements of
    the range are the beginning
    
*/
template <typename T>
using iterator_t = decltype(std::begin(std::declval<T>()));

template <typename R>
using limit_t = decltype(std::end(std::declval<R>()));

template <typename T>
using enable_if_integral = typename std::enable_if<std::is_integral<T>::value>::type*;

template <typename T>
using disable_if_integral = typename std::enable_if<!std::is_integral<T>::value>::type*;

template <typename P, typename L>
struct range {
    range(P p, L l) : position(p), limit(l) { }

    P begin() const { return position; }
    L end() const { return limit; }

    P position;
    L limit;
};

template <typename R>
using range_t = range<iterator_t<R>, limit_t<R>>;

template <typename R> // R models ConvertibleToRange
auto make_range(R&& x) -> range<iterator_t<R>, iterator_t<R>>
{ return { std::begin(x), std::end(x) }; }

template <typename I, // I models InputIterator
          typename N, // N models Integral
          typename T>
range<I, N> find(I p, N n, const T& x, enable_if_integral<N> = 0) {
    while (n && *p != x) { ++p; --n; }
    return { p, n };
}

template <typename I, // I models InputIterator
          typename L, // L models sentinal
          typename T>
range<I, L> find(I f, L l, const T& x, disable_if_integral<L> = 0) {
    while (f != l && *f != x) { ++f; }
    return { f, l };
}

template <typename R,
          typename T>
range_t<R> find(R&& r, const T& x) {
    return adobe::find(std::begin(r), std::end(r), x);
}

template <typename I,
          typename N,
          typename O>
std::pair<I, O> copy(I p, N n, O o, enable_if_integral<N> = 0) {
    while (n) { *o++ = *p; ++p; --n; }
    return { p, o };
}

template <typename I,
          typename L,
          typename O>
std::pair<I, O> copy(I f, L l, O o, disable_if_integral<L> = 0) {
    while (f != l) { *o++ = *f; ++f; }
    return { f, o };
}

template <typename R,
          typename O>
std::pair<iterator_t<R>, O> copy(R&& r, O o) {
    return adobe::copy(std::begin(r), std::end(r), o);
}

template <typename CharT = char, typename Traits = std::char_traits<CharT>>
class ostream_iterator {
  public:
    using iterator_category = std::output_iterator_tag;
    using ostream_type = std::basic_ostream<CharT, Traits>;

    ostream_iterator(ostream_type& s) : stream_m(&s), string_m(nullptr) { }
    ostream_iterator(ostream_type& s, const CharT* c) : stream_m(&s), string_m(c) { }
    template <typename T>
    ostream_iterator& operator=(const T& x) {
        *stream_m << x;
        if (string_m) *stream_m << string_m;
        return *this;
    }
  ostream_iterator& operator*() { return *this; }
  ostream_iterator& operator++() { return *this; }
  ostream_iterator& operator++(int) { return *this; }
private:
  ostream_type* stream_m;
  const CharT* string_m;
};

template <class _Tp, 
          class _CharT = char, class _Traits = std::char_traits<_CharT>,
          class _Dist = ptrdiff_t> 
class istream_iterator {
public:
  typedef _CharT                                char_type;
  typedef _Traits                               traits_type;
  typedef std::basic_istream<_CharT, _Traits>   istream_type;

  typedef std::input_iterator_tag                    iterator_category;
  typedef _Tp                                   value_type;
  typedef _Dist                                 difference_type;
  typedef const _Tp*                            pointer;
  typedef const _Tp&                            reference;

  istream_iterator() : _M_stream(0), _M_ok(false) {}
  istream_iterator(istream_type& __s) : _M_stream(&__s) { _M_read(); }

  reference operator*() const { return _M_value; }
  pointer operator->() const { return &(operator*()); }

  istream_iterator& operator++() { 
    _M_read(); 
    return *this;
  }
  istream_iterator operator++(int)  {
    istream_iterator __tmp = *this;
    _M_read();
    return __tmp;
  }

  bool _M_equal(const istream_iterator& __x) const
    { return (_M_ok == __x._M_ok) && (!_M_ok || _M_stream == __x._M_stream); }

private:
  istream_type* _M_stream;
  _Tp _M_value;
  bool _M_ok;

  void _M_read() {
    _M_ok = (_M_stream && *_M_stream) ? true : false;
    if (_M_ok) {
      *_M_stream >> _M_value;
      _M_ok = *_M_stream ? true : false;
    }
  }
};

template <class _Tp, class _CharT, class _Traits, class _Dist>
inline bool 
operator==(const istream_iterator<_Tp, _CharT, _Traits, _Dist>& __x,
           const istream_iterator<_Tp, _CharT, _Traits, _Dist>& __y) {
  return __x._M_equal(__y);
}

template <class _Tp, class _CharT, class _Traits, class _Dist>
inline bool 
operator!=(const istream_iterator<_Tp, _CharT, _Traits, _Dist>& __x,
           const istream_iterator<_Tp, _CharT, _Traits, _Dist>& __y) {
  return !__x._M_equal(__y);
}



/*

Implement an istream range

template <typename T, typename charT>
range<> range(istream<charT>&)

*/


}

int main() {
    int a[] = { 0, 1, 2, 3, 4, 5 };
    adobe::copy(adobe::find(a, 3), adobe::ostream_iterator<>(std::cout, ", "));
}

#endif



#if 0


using namespace std;

size_t count = 0;

struct annotate {
    annotate() { cout << "annotate ctor" << endl; }
    annotate(const annotate&) { cout << "annotate copy-ctor" << endl; }
    annotate(annotate&&) noexcept { cout << "annotate move-ctor" << endl; }
    annotate& operator=(const annotate&) { cout << "annotate assign" << endl; return *this; }
    annotate& operator=(annotate&&) noexcept { cout << "annotate move-assign" << endl; return *this; }
    ~annotate() { /* cout << "annotate dtor" << endl; */ }
    friend inline void swap(annotate&, annotate&) { cout << "annotate swap" << endl; ++::count; }
};

template <typename I,
          typename N>
I reverse_n(I f, N n) {
    if (n == 0) return f;
    if (n == 1) return ++f;
    auto m = reverse_n(f, n / 2);
    auto l = reverse_n(m, n - (n / 2));
    std::rotate(f, m, l);
    return l;
}

template <typename I> // I models ForwardIterator
void reverse(I f, I l) {
    reverse_n(f, distance(f, l));
}

template <typename I> // I models ForwardIterator
void sort(I f, I l) {
    while (f != l) {

    }
}

int main() {
    forward_list<annotate> x(10000);
    ::reverse(begin(x), end(x));
    cout << "count: " << ::count << endl;
}

#endif

#if 0
#include <functional>
#include <iostream>

using namespace std;

struct type {
    bool member;
};

int main() {

    type x = { true };
    cout << !bind(&type::member, x)() << endl;

}
#endif


#if 0

#ifndef DRAWING
#define DRAWING

#include <memory>
#include <iostream>

namespace concepts{
    namespace drawing{
        struct concept_t{
            concept_t() = default;
            concept_t(concept_t const&) = delete;
            concept_t(concept_t&& ) = delete;
            concept_t& operator = (concept_t const&) = delete;
            concept_t& operator = (concept_t&&) = delete;
            virtual void draw() = 0;
        };
        
        template<class T>
        class model_t: public concept_t{
        public:
            model_t(T const& concept) : concept_(concept){
            }
            
            model_t(T&& concept) : concept_(std::move(concept)){
                
            }
            
            model_t(model_t const&) = delete;
            model_t(model_t&& ) = delete;
            model_t& operator = (model_t const&) = delete;
            model_t& operator = (model_t&&) = delete;
            
            void draw(){
                concept_.draw();
            }
        private:
            T concept_;
        };
        
        class drawable{
        public:
            template<class T>
            drawable(T const& concept) {
                auto ptr = new model_t<T>(concept);
                impl_ = std::shared_ptr<concept_t>(ptr);
                
            }
            
            drawable(drawable const&) = default;
            drawable(drawable&& ) = default;
            drawable& operator = (drawable const&) = default;
            drawable& operator = (drawable&&) = default;
            
            void draw(){
                impl_->draw();
            }
        private:
            std::shared_ptr<concept_t> impl_;
        };
    }
}

namespace graphics {
    class graphics_display{
    public:
        template<class T>
        graphics_display(T const& concept) : control_(concept){
            
        }
        
        void draw(){
            control_.draw();
        }
        
    private:
        concepts::drawing::drawable control_;
    };
    
    class rectangle{
        public:
        void draw(){
            std::cout << "Rectangle\n";
        }
    };
    
    class square{
        public:
        void draw(){
            std::cout << "Square\n";
        }
    };
    
    class circle{
        public:
        void draw(){
            std::cout << "Circle\n";
        }
    };
}


#endif

// Main file starts here
// #include "drawing.hpp"

int main(int argc, const char * argv[])
{

    graphics::circle c;
    graphics::graphics_display g(c);
    g.draw();

    return 0;
}

#endif




#if 0

#include <iostream>
#include <algorithm>

using namespace std;

#define SLOW

struct A {
  A() {}
  ~A() { std::cout << "A d'tor\n"; }
  A(const A&) { std::cout << "A copy\n"; }
  A(A&&) { std::cout << "A move\n"; }
  A &operator=(const A&) { std::cout << "A copy assignment\n"; return *this; }
  A &operator=(A&&) { std::cout << "A move assignment\n"; return *this; }
};

struct B {
  // Using move on a sink. 
  // Nice talk at Going Native 2013 by Sean Parent.
  B(A foo) : a_(std::move(foo)) {}  
  A a_;
};

A MakeA() {
  return A();
}

B MakeB() {  
 // The key bits are in here
#ifdef SLOW
  A a(MakeA());
  return B(move(a));
#else
  return B(MakeA());
#endif
}

int main() {
  std::cout << "Hello World!\n";
  B obj = MakeB();
  std::cout << &obj << "\n";
  return 0;
}

#endif


#if 0

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

#endif

#if 0

namespace adobe {

template <typename F, typename SIG> class continuable_function;

template <typename F, typename R, typename ...Arg>
class continuable_function<F, R (Arg...)> {
  public:
    typedef R result_type;

    template <typename R1>
    continuable_function(F f, R1(R));

    R1 operator()(Arg&& ...arg) const {
    }
};

}  // namespace adobe



int main() {
}

#endif


#if 0
#include <algorithm>
#include <string>

using namespace std;

tuple<I, I, I> name(I f, I l) {
    return { f, find_if(f + 1, l, [](char c){ return isupper(c) || isspace(c) },
             find_if(f, l, [](char c) { return isspace(c));
}

/*
    pair = name {!alpha} name
    name = upper lower {lower} space *upper lower {lower} *[upper lower {lower}]*;
*/


template <typename I>
I isoccpify_name_pair(I f, I l)
{
    auto n = [&]{f += 2; while(islower(*f++)); f += 1; I a = f; f += 2; while(islower(*f++));
                 I b = f; if (isupper(*f)) { f += 2; while(islower(*f++)); } return {a, b, f}};

    auto p = [&]{ auto a = n(); while(!alpha(*f++)); reuturn { a, n();

}
#endif


#if 0
struct move_only {
    move_only();
    move_only(move_only&&) { }
};

int main() {
    move_only x;
    move_only y;
    x = y;
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


