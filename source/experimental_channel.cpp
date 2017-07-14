#include <deque>
#include <memory>
#include <mutex>
#include <future>
#define BOOST_THREAD_PROVIDES_EXECUTORS 1
#include <boost/optional.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>

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
    using receive_promise_t = boost::optional<boost::promise<boost::optional<T>>>;
    using send_promise_t = boost::optional<boost::promise<void>>;

    process _process;

    std::mutex _mutex;
    std::deque<T> _q;
    bool _closed = false;
    std::size_t _buffer_size = 1;
    receive_promise_t _receive_promise;
    send_promise_t _send_promise;

    /*
        REVISIT : Make this a future<bool> to support cancelation of non-process senders?
    */

    boost::future<void> send(T x) override {
        receive_promise_t promise;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_q.empty() && _receive_promise) {
                promise = std::move(_receive_promise);
                _receive_promise.reset();
            } else {
                _q.emplace_back(std::move(x));
                if (_q.size() == _buffer_size) {
                    _send_promise = boost::promise<void>();
                    return _send_promise.get().get_future();
                }
            }
        }
        if (promise) promise->set_value(std::move(x));
        return boost::make_ready_future();
    }

    void close() override {
        receive_promise_t promise;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_q.empty() && _receive_promise) {
                promise = std::move(_receive_promise);
                _receive_promise.reset();
            }
            else _closed = true;
        }
        if (promise) promise->set_value(boost::optional<T>());
    }

    void set_process(process p) override {
        // Should not need to lock, can only be set once and controls lifetime of process bound
        // to this sender
        _process = std::move(p);
    }

    boost::future<boost::optional<T>> receive() override {
        send_promise_t promise;
        boost::future<boost::optional<T>> result;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_q.empty()) {
                result = boost::make_ready_future<boost::optional<T>>(std::move(_q.front()));
                _q.pop_front();
                promise = std::move(_send_promise);
                _send_promise.reset();
            } else if (_closed) {
                return boost::make_ready_future<boost::optional<T>>();
            } else {
                _receive_promise = boost::promise<boost::optional<T>>();
                return _receive_promise.get().get_future();
            }
        }
        // !_q.empty()
        if (promise) promise->set_value();
        return result;
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

    /*
        REVISIT (sparent) : Should a sender be copyable or only movable? If only movable should
        it close on destruction?
    */

    boost::future<void> operator()(T x) const {
        if (auto p = _p.lock()) return p->send(std::move(x));
        return boost::make_ready_future();
    }
    void close() const {
        if (auto p = _p.lock()) p->close();
    }

    /*
        REVISIT : the process being set must hold the sender (this completes a strong/weak cycle).
        Is there a better way?
    */
    void set_process(process x) const {
        if (auto p = _p.lock()) p->set_process(std::move(x));
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
            if (auto opt = x.get()) {
                _receive().then([this, _x = *opt](auto y){
                    if (auto opt = y.get()) _send(_x * *opt).then([this](auto){ run(); });
                    else _send(_x).then([this](auto){ _send.close(); });
                });
            } else _send.close();
        });
    }
};

#include <boost/thread/executors/inline_executor.hpp>

struct iota {
    boost::inline_executor immediate;
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
        _send(_min).then(immediate, [this](auto){
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
            if (auto opt = x.get()) {
                _result += *opt;
                run(); // continue
            } else {
                _send(_result).then([this](auto){
                    _send.close();
                });
            }
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
            if (auto opt = x.get()) _send(_f(*opt)).then([this](auto){ run(); });
            else _send.close();
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

    while (auto n = receive1().get()) {
        std::cout << *n << std::endl;
    }
    
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
