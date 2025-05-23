// expression_templates_example.cpp
#include <iostream>
#include <vector>
#include <chrono>

// 表达式基类模板
template <typename E>
class VecExpression {
public:
    // 使用CRTP访问派生表达式
    double operator[](size_t i) const { 
        return static_cast<const E&>(*this)[i]; 
    }
    
    size_t size() const { 
        return static_cast<const E&>(*this).size(); 
    }
    
    // 计算表达式的范数
    double norm() const {
        const E& e = static_cast<const E&>(*this);
        double sum = 0.0;
        for (size_t i = 0; i < e.size(); ++i)
            sum += e[i] * e[i];
        return std::sqrt(sum);
    }
};

// 真实向量类
class Vec : public VecExpression<Vec> {
    std::vector<double> data_;
public:
    Vec(size_t n) : data_(n, 0.0) {}
    
    Vec(std::initializer_list<double> init) : data_(init) {}
    
    // 从表达式构造
    template <typename E>
    Vec(const VecExpression<E>& expr) : data_(expr.size()) {
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = expr[i];
    }
    
    // 从表达式赋值
    template <typename E>
    Vec& operator=(const VecExpression<E>& expr) {
        if (data_.size() != expr.size())
            data_.resize(expr.size());
        
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = expr[i];
        
        return *this;
    }
    
    double operator[](size_t i) const { return data_[i]; }
    double& operator[](size_t i) { return data_[i]; }
    size_t size() const { return data_.size(); }
    
    // 显示向量内容
    void print(const std::string& name) const {
        std::cout << name << " = [";
        for (size_t i = 0; i < std::min(data_.size(), size_t(5)); ++i) {
            std::cout << data_[i];
            if (i < std::min(data_.size(), size_t(5)) - 1)
                std::cout << ", ";
        }
        if (data_.size() > 5)
            std::cout << ", ...";
        std::cout << "] (size: " << data_.size() << ")\n";
    }
};

// 加法表达式
template <typename E1, typename E2>
class VecSum : public VecExpression<VecSum<E1, E2>> {
    const E1& _u;
    const E2& _v;
public:
    VecSum(const E1& u, const E2& v) : _u(u), _v(v) {
        assert(u.size() == v.size());
    }
    
    double operator[](size_t i) const { return _u[i] + _v[i]; }
    size_t size() const { return _u.size(); }
};

// 减法表达式
template <typename E1, typename E2>
class VecDiff : public VecExpression<VecDiff<E1, E2>> {
    const E1& _u;
    const E2& _v;
public:
    VecDiff(const E1& u, const E2& v) : _u(u), _v(v) {
        assert(u.size() == v.size());
    }
    
    double operator[](size_t i) const { return _u[i] - _v[i]; }
    size_t size() const { return _u.size(); }
};

// 标量乘法表达式
template <typename E>
class VecScaled : public VecExpression<VecScaled<E>> {
    const E& _u;
    double _alpha;
public:
    VecScaled(const E& u, double alpha) : _u(u), _alpha(alpha) {}
    
    double operator[](size_t i) const { return _alpha * _u[i]; }
    size_t size() const { return _u.size(); }
};

// 运算符重载
template <typename E1, typename E2>
VecSum<E1, E2> operator+(const VecExpression<E1>& u, const VecExpression<E2>& v) {
    return VecSum<E1, E2>(static_cast<const E1&>(u), static_cast<const E2&>(v));
}

template <typename E1, typename E2>
VecDiff<E1, E2> operator-(const VecExpression<E1>& u, const VecExpression<E2>& v) {
    return VecDiff<E1, E2>(static_cast<const E1&>(u), static_cast<const E2&>(v));
}

template <typename E>
VecScaled<E> operator*(double alpha, const VecExpression<E>& v) {
    return VecScaled<E>(static_cast<const E&>(v), alpha);
}

// 传统实现进行性能对比
void traditionalImplementation(const std::vector<double>& a, 
                               const std::vector<double>& b,
                               const std::vector<double>& c,
                               const std::vector<double>& d,
                               std::vector<double>& result) {
    const size_t n = a.size();
    std::vector<double> temp1(n);
    std::vector<double> temp2(n);
    std::vector<double> temp3(n);
    
    // a + b
    for (size_t i = 0; i < n; ++i)
        temp1[i] = a[i] + b[i];
    
    // (a + b) - c
    for (size_t i = 0; i < n; ++i)
        temp2[i] = temp1[i] - c[i];
    
    // 2 * ((a + b) - c)
    for (size_t i = 0; i < n; ++i)
        temp3[i] = 2.0 * temp2[i];
    
    // 2 * ((a + b) - c) + d
    for (size_t i = 0; i < n; ++i)
        result[i] = temp3[i] + d[i];
}

int main() {
    std::cout << "=== 零开销抽象的Expression Templates示例 ===\n\n";
    
    const size_t n = 10000000; // 大向量用于性能测试
    
    // 创建测试向量
    Vec a(n), b(n), c(n), d(n);
    for (size_t i = 0; i < n; ++i) {
        a[i] = static_cast<double>(i % 100) / 100.0;
        b[i] = static_cast<double>((i + 30) % 100) / 100.0;
        c[i] = static_cast<double>((i + 60) % 100) / 100.0;
        d[i] = static_cast<double>((i + 90) % 100) / 100.0;
    }
    
    // 展示小向量运算
    Vec small_a = {1.0, 2.0, 3.0, 4.0, 5.0};
    Vec small_b = {5.0, 4.0, 3.0, 2.0, 1.0};
    Vec small_c = {0.5, 1.0, 1.5, 2.0, 2.5};
    Vec small_d = {0.1, 0.2, 0.3, 0.4, 0.5};
    
    small_a.print("a");
    small_b.print("b");
    small_c.print("c");
    small_d.print("d");
    
    // 使用表达式模板进行计算
    Vec result1 = 2.0 * (small_a + small_b - small_c) + small_d;
    result1.print("2*(a+b-c)+d");
    
    // 展示每个中间结果
    Vec temp1 = small_a + small_b;
    temp1.print("a+b");
    
    Vec temp2 = temp1 - small_c;
    temp2.print("(a+b)-c");
    
    Vec temp3 = 2.0 * temp2;
    temp3.print("2*((a+b)-c)");
    
    // 性能测试: 表达式模板
    std::cout << "\n性能测试 (n = " << n << "):\n";
    
    auto start1 = std::chrono::high_resolution_clock::now();
    Vec result_expr = 2.0 * (a + b - c) + d;
    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed1 = end1 - start1;
    
    // 性能测试: 传统实现
    std::vector<double> vec_a(n), vec_b(n), vec_c(n), vec_d(n), vec_result(n);
    for (size_t i = 0; i < n; ++i) {
        vec_a[i] = a[i];
        vec_b[i] = b[i];
        vec_c[i] = c[i];
        vec_d[i] = d[i];
    }
    
    auto start2 = std::chrono::high_resolution_clock::now();
    traditionalImplementation(vec_a, vec_b, vec_c, vec_d, vec_result);
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed2 = end2 - start2;
    
    std::cout << "表达式模板实现: " << elapsed1.count() << " 秒\n";
    std::cout << "传统实现: " << elapsed2.count() << " 秒\n";
    
    // 验证结果正确性
    double maxDiff = 0.0;
    for (size_t i = 0; i < n; ++i) {
        maxDiff = std::max(maxDiff, std::abs(result_expr[i] - vec_result[i]));
    }
    std::cout << "两种实现的最大误差: " << maxDiff << "\n";
    
    return 0;
}