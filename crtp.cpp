// crtp_example.cpp
#include <iostream>
#include <vector>
#include <algorithm>

// CRTP基类
template <typename Derived>
class Base {
public:
    void interface() {
        static_cast<Derived*>(this)->implementation();
    }
    
    // 提供默认实现
    void implementation() {
        std::cout << "Base implementation\n";
    }
};

// 派生类1：覆盖实现
class Derived1 : public Base<Derived1> {
public:
    void implementation() {
        std::cout << "Derived1 implementation\n";
    }
};

// 派生类2：使用默认实现
class Derived2 : public Base<Derived2> {
    // 不覆盖implementation()，将使用Base的默认实现
};

// CRTP实际应用：静态多态的容器操作
template <typename Derived>
class Container {
public:
    // 通用算法，由派生类实现具体细节
    void sort() {
        Derived& derived = static_cast<Derived&>(*this);
        auto begin = derived.begin();
        auto end = derived.end();
        std::sort(begin, end);
        std::cout << "Container sorted using static polymorphism\n";
    }
};

class Vector : public Container<Vector> {
    std::vector<int> data;
public:
    Vector(std::initializer_list<int> init) : data(init) {}
    
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    
    void print() {
        for (int x : data) {
            std::cout << x << " ";
        }
        std::cout << "\n";
    }
};

int main() {
    std::cout << "=== CRTP基本示例 ===\n";
    Derived1 d1;
    Derived2 d2;
    
    d1.interface();  // 调用Derived1::implementation()
    d2.interface();  // 调用Base::implementation()
    
    std::cout << "\n=== CRTP实际应用 ===\n";
    Vector v = {5, 3, 1, 4, 2};
    std::cout << "Before sorting: ";
    v.print();
    
    v.sort();  // 使用CRTP静态多态调用排序
    
    std::cout << "After sorting: ";
    v.print();
    
    return 0;
}