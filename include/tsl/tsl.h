#ifndef TSL_TSL_H
#define TSL_TSL_H

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace tsl {
    namespace detail {
        template<typename> struct sfinae_true : std::true_type {};

        /*
         *  flush detection
         */
        template<typename T>
        auto test_flush(int)
        -> sfinae_true<decltype(std::declval<T>().flush())>;

        template<typename>
        auto test_flush(...) -> std::false_type;

        template<typename T> struct has_flush : decltype(test_flush<T>(0)) {};
        template<typename T> constexpr const bool has_flush_t = has_flush<T>::value;


        template<typename T, typename Processor, typename ... Processors>
        void process(T&& t, Processor&& processor, Processors&& ... processors) {
            processor(std::forward<T>(t), std::forward<Processors>(processors)...);
        };

        template<typename Processor>
        Processor last(Processor&& processor) { return std::move(processor); }

        template<typename Processor, typename ... Processors>
        decltype(auto) last(Processor&&, Processors&& ... processors) {
            return last(std::forward<Processors>(processors)...);
        };

        template<typename Processor>
        typename std::enable_if_t<has_flush_t<Processor>> flush(Processor&& processor) { processor.flush(); }

        template<typename Processor>
        typename std::enable_if_t<!has_flush_t<Processor>> flush(Processor&&) {}

        template<typename Processor, typename ... Processors>
        std::enable_if_t<has_flush_t<Processor>>
        flush(Processor&& processor, Processors&& ... processors) {
            processor.flush(std::forward<Processors>(processors)...);
            flush(std::forward<Processors>(processors)...);
        };

        template<typename Processor, typename ... Processors>
        std::enable_if_t<!has_flush_t<Processor>>
        flush(Processor&&, Processors&& ... processors) {
            flush(std::forward<Processors>(processors)...);
        };

        template<typename Iter> class stream_source {
        public:
            using value_type = typename Iter::value_type;

            stream_source(Iter begin, Iter end) : _current(begin), _end(end) {}
            bool empty() const { return _current == _end; }
            decltype(auto) next() { return *_current++; }

        private:
            Iter _current;
            Iter _end;
        };

        template<typename T, typename Compare> class sort {
        public:
            explicit sort(Compare compare = Compare{}) : _compare { std::move(compare) } {}

            template<typename U, typename ... Processors> void operator()(U&& t, Processors&& ...) {
                _values.push_back(std::forward<U>(t));
            }

            template<typename ... Processors> void flush(Processors&& ... processors) {
                std::sort(_values.begin(), _values.end(), _compare);
                for (auto&& e : _values) {
                    detail::process(std::move(e), std::forward<Processors>(processors)...);
                }
            }

        private:
            std::vector<T> _values;
            Compare _compare;
        };

        template<typename Function> class map {
        public:
            explicit map(Function f) : _f(std::move(f)) {}

            template<typename I, typename ... Processors>
            void operator()(I&& input, Processors&& ... processors) const {
                detail::process(_f(std::forward<I>(input)), std::forward<Processors>(processors)...);
            }

        private:
            mutable Function _f;
        };

        template<typename Function> class filter {
        public:
            explicit filter(Function f) : _f(std::move(f)) {}

            template<typename I, typename ... Processors>
            void operator()(I&& input, Processors&& ... processors) const {
                if (_f(std::forward<I>(input))) {
                    detail::process(std::forward<I>(input), std::forward<Processors>(processors)...);
                }
            }

        private:
            mutable Function _f;
        };

        template<typename Function, typename FlushFunction> class sink {
        public:
            explicit sink(Function f, FlushFunction flush) : _f(std::move(f)), _flush(std::move(flush)) {}

            template<typename I, typename ... Processors>
            void operator()(I&& input, Processors&& ...) const {
                _f(std::forward<I>(input));
            }

            template<typename ... Processors> void flush(Processors&& ...) {
                _flush();
            }

        private:
            mutable Function _f;
            mutable FlushFunction _flush;
        };

        template<typename T> struct sink_vector {
            template<typename U> void operator()(U&& input) {
                _v.push_back(std::forward<U>(input));
            }
            sink_vector() = default;
            sink_vector(sink_vector&&) = default;

            const T& operator[](typename std::vector<T>::size_type pos) const noexcept { return _v[pos]; }
            const std::vector<T>& value() const noexcept { return _v; }
            std::vector<T> _v;
        };
    }

    /**
     * stream
     */
    template<typename Source, typename ... Processors>
    decltype(auto) stream(Source&& src, Processors&& ... processors) {
        while (!src.empty()) {
            detail::process(src.next(), std::forward<Processors>(processors)...);
        }
        detail::flush(std::forward<Processors>(processors)...);
        return detail::last(std::forward<Processors>(processors)...);
    };

    /**
     * source
     */
    template<typename Iter> decltype(auto) source(Iter begin, Iter end) {
        return detail::stream_source<Iter>(begin, end);
    }

    template<typename V, typename ... Unused, template<typename, typename ...> class Container> decltype(auto)
    source(const Container<V, Unused...>& v) {
        return detail::stream_source<typename Container<V, Unused...>::const_iterator>(std::cbegin(v), std::cend(v));
    };

    template<typename V, typename ... Unused, template<typename, typename ...> class Container> decltype(auto)
    source(Container<V, Unused...>&& v) {
        return detail::stream_source<typename std::move_iterator<typename Container<V, Unused...>::iterator>>(
                std::make_move_iterator(std::begin(v)),
                std::make_move_iterator(std::end(v))
        );
    };

    /**
     * sort
     */
    template<typename T, template<typename> class Compare = std::less>
    detail::sort<T, Compare<T>> sort(Compare<T> compare = Compare<T>{}) {
        return detail::sort<T, Compare<T>>{ std::move(compare) };
    }

    template<typename T, typename Compare> detail::sort<T, Compare> sort(Compare compare = Compare{}) {
        return detail::sort<T, Compare>{ std::move(compare) };
    }

    /**
     * map
     */
    template<typename Function> detail::map<Function> map(Function f) {
        return detail::map<Function>(std::move(f));
    };

    /**
     * filter
     */

    template<typename Function> detail::filter<Function> filter(Function f) {
        return detail::filter<Function>(std::move(f));
    };

    /**
     * sink
     */

    template<typename Function, typename FlushFunction = std::function<void()>>
    detail::sink<Function, FlushFunction> sink(Function f, FlushFunction flush = [] {}) {
        return detail::sink<Function, FlushFunction>(std::move(f), std::move(flush));
    };

    /**
     * to_vector
     */
    template<typename T> auto to_vector() {
        return detail::sink_vector<T>();
    }
}

#endif //TSL_TSL_H