#include <string>
#include <functional>
#include <map>
#include <variant>
#include <utility>
#include <optional>
#include <sstream>

#include <iostream>

// generic library code
template <typename... Bases>
struct overload : Bases...
{    
    template <typename... Args>
    overload(Args&&... args) : Bases(std::forward<Args>(args))...
    {
    }

    using Bases::operator()...;
};

template <typename... Args>
auto makeOverload(Args&&... args)
{
    return overload<Args...>(std::forward<Args>(args)...);
}

template <typename Head, typename... Tail>
void buildStringImpl(std::ostream& out, const Head& head, const Tail&... tail)
{
    out << head;

    if constexpr (sizeof...(Tail) > 0)
    {
        buildStringImpl(out, tail...);
    }
}

template <typename... Args>
std::string buildString(const Args&... args)
{
    std::stringstream stream;
    buildStringImpl(stream, args...);
    return stream.str();
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::optional<T>& value)
{
    if (!value)
        out << "--";
    else
        out << *value;

    return out;
}

// reflection library code

template <typename T>
void registerMembers(T* ptr)
{
}

template <typename CLASS_T>
struct Reflector
{
    using value_t = std::variant<int CLASS_T::*, 
                                 std::optional<double> CLASS_T::*,
                                 std::reference_wrapper<double> CLASS_T::*>;

    template<typename T>
    static void registerMember(const std::string& key, T&& member)
    {
        values[key] = member;
    }

    template <typename T>
    static void set(CLASS_T& obj, const std::string& key, const T& value)
    {
        static bool registered = (registerMembers<CLASS_T>(static_cast<CLASS_T*>(nullptr)), true);

        auto it = values.find(key);
        if (values.end() == it)
            throw std::runtime_error(buildString("Member ", key, " not found!"));

        std::visit(makeOverload(
            [&](auto& member) {
                assign(obj, member, value);
            }),
            values[key]);
    }

    template<typename F>
    static void visit(CLASS_T& obj, F&& f)
    {
        for (auto& [key, value] : values) 
        {
            std::visit([&](const auto& value){
                f(key, obj.*value);
            }, value);
        }
    }

protected:
    template <typename T, typename E>
    static void assign(CLASS_T& obj, std::reference_wrapper<T> CLASS_T::*member, const E& value)
    {
        (obj.*member).get() = value;
    }

    template <typename T, typename E>
    static void assign(CLASS_T& obj, T CLASS_T::*member, const E& value)
    {
        (obj.*member) = value;
    }

    static std::map<std::string, value_t> values;
};

template <typename CLASS_T>
std::map<std::string, typename Reflector<CLASS_T>::value_t> 
Reflector<CLASS_T>::values;

#define REFLECT_ENUMERATE(className) \
template<> \
void registerMembers<className>(className* ptr) \

#define REFLECT_MEMBER(name) \
    Reflector<std::decay_t<decltype(*ptr)>>::registerMember(#name, &std::decay_t<decltype(*ptr)>::name)

// client code
struct Subject
{
    int num;
    std::reference_wrapper<double> value;
    std::optional<double> optValue;
    std::string name;
};

REFLECT_ENUMERATE(Subject)
{
    REFLECT_MEMBER(num);
    REFLECT_MEMBER(value);
    REFLECT_MEMBER(optValue);
}

int main()
{
    double a = 5.0;
    Subject s{1, a};

    Reflector<Subject> reflector;

    reflector.set(s, "value", 3.15);
    reflector.set(s, "optValue", 8.88);

    std::cout << "Object: \n";
    reflector.visit(s, [](const auto& key, const auto& value){
        std::cout << key << ": " << value << '\n';
    });

    std::cout<< '\n';    
    std::cout << "num: " << s.num << '\n' << "value: " << s.value << ", a: " << a << '\n';
    std::cout << "optValue: " << s.optValue.value_or(0.0) << '\n';

    return 0;
}
