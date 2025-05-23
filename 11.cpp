// policy_based_design_example.cpp
#include <iostream>
#include <memory>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <thread>

// ============ 内存分配策略 ============
template <typename T>
struct MallocAllocator {
    static T* allocate(size_t n) {
        std::cout << "Using malloc to allocate " << n * sizeof(T) << " bytes\n";
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }
    
    static void deallocate(T* p) {
        std::cout << "Using free to deallocate memory\n";
        std::free(p);
    }
    
    static std::string name() { return "MallocAllocator"; }
};

template <typename T>
struct NewAllocator {
    static T* allocate(size_t n) {
        std::cout << "Using new[] to allocate " << n * sizeof(T) << " bytes\n";
        return new T[n];
    }
    
    static void deallocate(T* p) {
        std::cout << "Using delete[] to deallocate memory\n";
        delete[] p;
    }
    
    static std::string name() { return "NewAllocator"; }
};

// ============ 线程策略 ============
struct SingleThreaded {
    template <typename T>
    class Mutex {
    public:
        void lock() {
            // 无操作
        }
        
        void unlock() {
            // 无操作
        }
    };
    
    static std::string name() { return "SingleThreaded"; }
};

struct MultiThreaded {
    template <typename T>
    class Mutex {
        std::mutex mtx_;
    public:
        void lock() {
            //std::cout << "Locking mutex\n";
            mtx_.lock();
        }
        
        void unlock() {
            //std::cout << "Unlocking mutex\n";
            mtx_.unlock();
        }
    };
    
    static std::string name() { return "MultiThreaded"; }
};

// ============ 调试策略 ============
struct NoDebug {
    template <typename... Args>
    static void log(Args&&...) {
        // 不输出任何内容
    }
    
    static std::string name() { return "NoDebug"; }
};

struct DebugMode {
    template <typename... Args>
    static void log(Args&&... args) {
        (std::cout << ... << args) << '\n';
    }
    
    static std::string name() { return "DebugMode"; }
};

// ============ Vector实现 ============
template <
    typename T,
    template <typename> class AllocPolicy = MallocAllocator,
    typename ThreadPolicy = SingleThreaded,
    typename DebugPolicy = NoDebug
>
class Vector {
private:
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    typename ThreadPolicy::template Mutex<T> mutex_;
    
public:
    using value_type = T;
    using allocator_type = AllocPolicy<T>;
    
    Vector() {
        DebugPolicy::log("Vector constructed with policies: ", 
                          allocator_type::name(), ", ", 
                          ThreadPolicy::name(), ", ", 
                          DebugPolicy::name());
    }
    
    explicit Vector(size_t initial_size) : size_(initial_size), capacity_(initial_size) {
        DebugPolicy::log("Vector constructed with size ", initial_size);
        if (initial_size > 0) {
            data_ = allocator_type::allocate(initial_size);
            for (size_t i = 0; i < size_; ++i) {
                new (&data_[i]) T();
            }
        }
    }
    
    ~Vector() {
        DebugPolicy::log("Vector destructed, size=", size_, ", capacity=", capacity_);
        if (data_) {
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            allocator_type::deallocate(data_);
        }
    }
    
    Vector(const Vector& other) {
        DebugPolicy::log("Vector copy constructor");
        mutex_.lock();
        other.mutex_.lock();
        
        size_ = other.size_;
        capacity_ = other.capacity_;
        if (capacity_ > 0) {
            data_ = allocator_type::allocate(capacity_);
            for (size_t i = 0; i < size_; ++i) {
                new (&data_[i]) T(other.data_[i]);
            }
        }
        
        other.mutex_.unlock();
        mutex_.unlock();
    }
    
    Vector& operator=(const Vector& other) {
        DebugPolicy::log("Vector copy assignment");
        if (this != &other) {
            mutex_.lock();
            other.mutex_.lock();
            
            // 清理现有数据
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            if (data_) {
                allocator_type::deallocate(data_);
            }
            
            // 复制新数据
            size_ = other.size_;
            capacity_ = other.capacity_;
            if (capacity_ > 0) {
                data_ = allocator_type::allocate(capacity_);
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            } else {
                data_ = nullptr;
            }
            
            other.mutex_.unlock();
            mutex_.unlock();
        }
        return *this;
    }
    
    // 移动构造函数
    Vector(Vector&& other) noexcept {
        DebugPolicy::log("Vector move constructor");
        mutex_.lock();
        other.mutex_.lock();
        
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        
        other.mutex_.unlock();
        mutex_.unlock();
    }
    
    // 移动赋值运算符
    Vector& operator=(Vector&& other) noexcept {
        DebugPolicy::log("Vector move assignment");
        if (this != &other) {
            mutex_.lock();
            other.mutex_.lock();
            
            // 清理现有数据
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            if (data_) {
                allocator_type::deallocate(data_);
            }
            
            // 移动资源
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            
            other.mutex_.unlock();
            mutex_.unlock();
        }
        return *this;
    }
    
    void push_back(const T& value) {
        DebugPolicy::log("push_back(", value, ")");
        mutex_.lock();
        
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            reserve(new_capacity);
        }
        
        new (&data_[size_]) T(value);
        ++size_;
        
        mutex_.unlock();
    }
    
    void pop_back() {
        DebugPolicy::log("pop_back()");
        mutex_.lock();
        
        if (size_ > 0) {
            --size_;
            data_[size_].~T();
        }
        
        mutex_.unlock();
    }
    
    void reserve(size_t new_capacity) {
        DebugPolicy::log("reserve(", new_capacity, ")");
        if (new_capacity <= capacity_) {
            return;
        }
        
        T* new_data = allocator_type::allocate(new_capacity);
        
        // 移动现有元素
        for (size_t i = 0; i < size_; ++i) {
            new (&new_data[i]) T(std::move(data_[i]));
            data_[i].~T();
        }
        
        if (data_) {
            allocator_type::deallocate(data_);
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    T& operator[](size_t index) {
        DebugPolicy::log("operator[](", index, ")");
        mutex_.lock();
        T& ref = data_[index];
        mutex_.unlock();
        return ref;
    }
    
    const T& operator[](size_t index) const {
        DebugPolicy::log("const operator[](", index, ")");
        mutex_.lock();
        const T& ref = data_[index];
        mutex_.unlock();
        return ref;
    }
    
    size_t size() const {
        return size_;
    }
    
    size_t capacity() const {
        return capacity_;
    }
    
    void print() const {
        mutex_.lock();
        std::cout << "[";
        for (size_t i = 0; i < size_; ++i) {
            std::cout << data_[i];
            if (i < size_ - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "] (size: " << size_ << ", capacity: " << capacity_ << ")\n";
        mutex_.unlock();
    }
};

// 测试函数
void testSingleThreaded() {
    std::cout << "\n=== 单线程 Vector 测试 ===\n";
    
    // 使用不同策略组合的Vector
    Vector<int, MallocAllocator, SingleThreaded, DebugMode> v1;
    
    std::cout << "添加元素:\n";
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    
    std::cout << "打印向量: ";
    v1.print();
    
    std::cout << "访问元素: v1[1] = " << v1[1] << "\n";
    
    std::cout << "修改元素: v1[1] = 25\n";
    v1[1] = 25;
    
    std::cout << "打印向量: ";
    v1.print();
    
    std::cout << "移除末尾元素\n";
    v1.pop_back();
    
    std::cout << "打印向量: ";
    v1.print();
    
    // 测试复制构造
    std::cout << "复制构造:\n";
    Vector<int, MallocAllocator, SingleThreaded, DebugMode> v2 = v1;
    
    std::cout << "原向量: ";
    v1.print();
    
    std::cout << "复制的向量: ";
    v2.print();
    
    // 测试移动构造
    std::cout << "移动构造:\n";
    Vector<int, MallocAllocator, SingleThreaded, DebugMode> v3 = std::move(v1);
    
    std::cout << "移动后的原向量: ";
    v1.print(); // 应该为空
    
    std::cout << "移动到的向量: ";
    v3.print();
}

void testMultiThreaded() {
    std::cout << "\n=== 多线程 Vector 测试 ===\n";
    
    // 使用多线程策略的Vector
    Vector<int, NewAllocator, MultiThreaded, DebugMode> v;
    
    // 创建多个线程同时访问Vector
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i, &v]() {
            std::cout << "线程 " << i << " 开始执行\n";
            
            // 添加一些元素
            for (int j = 0; j < 3; ++j) {
                int value = i * 100 + j;
                v.push_back(value);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            std::cout << "线程 " << i << " 完成执行\n";
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "所有线程完成，最终向量: ";
    v.print();
}

// 测试不同分配器策略
void testAllocatorPolicies() {
    std::cout << "\n=== 分配器策略测试 ===\n";
    
    std::cout << "使用 MallocAllocator:\n";
    Vector<double, MallocAllocator, SingleThreaded, DebugMode> v1(5);
    v1.push_back(3.14);
    
    std::cout << "\n使用 NewAllocator:\n";
    Vector<double, NewAllocator, SingleThreaded, DebugMode> v2(5);
    v2.push_back(2.71);
}

int main() {
    std::cout << "=== 模块化设计与Policy-Based类设计示例 ===\n";
    
    testSingleThreaded();
    testMultiThreaded();
    testAllocatorPolicies();
    
    return 0;
}