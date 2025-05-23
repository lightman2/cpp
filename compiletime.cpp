// compile_time_example.cpp
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <string>
#include <type_traits>
#include <chrono>
#include <algorithm>
#include <thread>
// 基类标记接口，用于编译期识别
class SortedContainer {};

// 已排序的容器实现
template <typename T>
class SortedVector : public SortedContainer {
    std::vector<T> data;
public:
    SortedVector(std::initializer_list<T> init) : data(init) {
        std::sort(data.begin(), data.end());
    }
    
    const std::vector<T>& getData() const { return data; }
};

// 针对不同容器类型的算法实现
// 专门针对vector<int>的高效实现
template <typename T>
int specializedVectorAlgorithm(const std::vector<T>& vec) {
    std::cout << "Using specialized vector algorithm\n";
    // 假设这是一个高度优化的算法
    auto start = std::chrono::high_resolution_clock::now();
    
    int sum = 0;
    for (const auto& v : vec) {
        sum += v;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 模拟处理
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Processing time: " << duration.count() << " ms\n";
    
    return sum;
}

// 针对已排序容器的优化算法
template <typename T>
int sortedContainerAlgorithm(const T& container) {
    std::cout << "Using sorted container algorithm\n";
    // 利用已排序的特性优化
    auto start = std::chrono::high_resolution_clock::now();
    
    int sum = 0;
    for (const auto& v : container.getData()) {
        sum += v;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 模拟处理
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Processing time: " << duration.count() << " ms\n";
    
    return sum;
}

// 通用算法实现，性能较低
template <typename T>
int genericAlgorithm(const T& container) {
    std::cout << "Using generic algorithm\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    int sum = 0;
    for (const auto& v : container) {
        sum += v;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟处理
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Processing time: " << duration.count() << " ms\n";
    
    return sum;
}

// 使用编译期if根据类型选择最优算法
template <typename T>
int optimizedProcess(const T& container) {
    if constexpr (std::is_same_v<T, std::vector<int>>) {
        return specializedVectorAlgorithm(container);
    } 
    else if constexpr (std::is_base_of_v<SortedContainer, T>) {
        return sortedContainerAlgorithm(container);
    }
    else {
        return genericAlgorithm(container);
    }
}

// 编译期常量计算示例
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N-1>::value;
};

template <>
struct Factorial<0> {
    static constexpr int value = 1;
};

int main() {
    std::cout << "=== 编译期计算与类型特性示例 ===\n\n";

    // 1. 编译期常量计算
    std::cout << "5! = " << Factorial<5>::value << "\n";
    std::cout << "10! = " << Factorial<10>::value << "\n\n";
    
    // 2. 类型特性判断与算法选择
    std::cout << "Testing with std::vector<int>:\n";
    std::vector<int> vec = {5, 3, 1, 4, 2};
    int sum1 = optimizedProcess(vec);
    std::cout << "Sum: " << sum1 << "\n\n";
    
    std::cout << "Testing with SortedVector<int>:\n";
    SortedVector<int> sorted_vec = {5, 3, 1, 4, 2};
    int sum2 = optimizedProcess(sorted_vec);
    std::cout << "Sum: " << sum2 << "\n\n";
    
    std::cout << "Testing with std::list<int>:\n";
    std::list<int> lst = {5, 3, 1, 4, 2};
    int sum3 = optimizedProcess(lst);
    std::cout << "Sum: " << sum3 << "\n";
    
    return 0;
}