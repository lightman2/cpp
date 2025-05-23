// type_erasure_example.cpp
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <any>

// 不同的图形类，没有共同基类
class Circle {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double getRadius() const { return radius; }
};

class Rectangle {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double getWidth() const { return width; }
    double getHeight() const { return height; }
};

class Text {
    std::string content;
public:
    Text(const std::string& s) : content(s) {}
    const std::string& getContent() const { return content; }
};

// 独立的绘制函数，使用ADL查找
void draw(const Circle& c) {
    std::cout << "Drawing Circle with radius " << c.getRadius() << "\n";
}

void draw(const Rectangle& r) {
    std::cout << "Drawing Rectangle with width " << r.getWidth() 
              << " and height " << r.getHeight() << "\n";
}

void draw(const Text& t) {
    std::cout << "Drawing Text: \"" << t.getContent() << "\"\n";
}

// ============ 方法1：使用基类和虚函数（传统多态）============
class DrawableBase {
public:
    virtual ~DrawableBase() = default;
    virtual void draw() const = 0;
    virtual std::unique_ptr<DrawableBase> clone() const = 0;
};

template <typename T>
class DrawableModel : public DrawableBase {
    T object_;
public:
    explicit DrawableModel(T obj) : object_(std::move(obj)) {}
    
    void draw() const override {
        // 使用ADL查找绘制函数
        ::draw(object_);
    }
    
    std::unique_ptr<DrawableBase> clone() const override {
        return std::make_unique<DrawableModel<T>>(object_);
    }
};

class Drawable {
    std::unique_ptr<DrawableBase> pImpl_;
public:
    template <typename T>
    Drawable(T&& obj) : pImpl_(std::make_unique<DrawableModel<std::decay_t<T>>>(
                                  std::forward<T>(obj))) {}
    
    Drawable(const Drawable& other) : pImpl_(other.pImpl_->clone()) {}
    
    Drawable& operator=(const Drawable& other) {
        if (this != &other) {
            pImpl_ = other.pImpl_->clone();
        }
        return *this;
    }
    
    Drawable(Drawable&&) = default;
    Drawable& operator=(Drawable&&) = default;
    
    void draw() const { pImpl_->draw(); }
};

// ============ 方法2：使用std::function进行类型擦除 ============
class FunctionBasedDrawable {
    std::function<void()> drawFunc_;
    std::function<FunctionBasedDrawable()> cloneFunc_;
    
public:
    template <typename T>
    explicit FunctionBasedDrawable(T obj) {
        drawFunc_ = [obj]() { ::draw(obj); };
        cloneFunc_ = [obj]() { return FunctionBasedDrawable(obj); };
    }
    
    void draw() const { drawFunc_(); }
    FunctionBasedDrawable clone() const { return cloneFunc_(); }
};

// ============ 方法3：使用std::any进行类型擦除 ============
class AnyBasedDrawable {
    std::any object_;
    std::function<void(const std::any&)> drawFunc_;
    
public:
    template <typename T>
    explicit AnyBasedDrawable(T obj) : object_(std::move(obj)) {
        drawFunc_ = [](const std::any& a) { 
            ::draw(std::any_cast<T>(a)); 
        };
    }
    
    void draw() const { drawFunc_(object_); }
};

int main() {
    std::cout << "=== 类型擦除与多态容器示例 ===\n\n";
    
    // 创建不同类型的对象
    Circle circle(5.0);
    Rectangle rectangle(4.0, 3.0);
    Text text("Hello, Type Erasure!");
    
    std::cout << "方法1: 使用类层次结构进行类型擦除\n";
    {
        std::vector<Drawable> drawables;
        drawables.emplace_back(Circle(5.0));
        drawables.emplace_back(Rectangle(4.0, 3.0));
        drawables.emplace_back(Text("Hello, Type Erasure!"));
        
        std::cout << "绘制所有对象:\n";
        for (const auto& d : drawables) {
            d.draw();
        }
    }
    
    std::cout << "\n方法2: 使用std::function进行类型擦除\n";
    {
        std::vector<FunctionBasedDrawable> drawables;
        drawables.emplace_back(FunctionBasedDrawable(Circle(5.0)));
        drawables.emplace_back(FunctionBasedDrawable(Rectangle(4.0, 3.0)));
        drawables.emplace_back(FunctionBasedDrawable(Text("Hello, Type Erasure!")));
        
        std::cout << "绘制所有对象:\n";
        for (const auto& d : drawables) {
            d.draw();
        }
    }
    
    std::cout << "\n方法3: 使用std::any进行类型擦除\n";
    {
        std::vector<AnyBasedDrawable> drawables;
        drawables.emplace_back(AnyBasedDrawable(Circle(5.0)));
        drawables.emplace_back(AnyBasedDrawable(Rectangle(4.0, 3.0)));
        drawables.emplace_back(AnyBasedDrawable(Text("Hello, Type Erasure!")));
        
        std::cout << "绘制所有对象:\n";
        for (const auto& d : drawables) {
            d.draw();
        }
    }
    
    return 0;
}