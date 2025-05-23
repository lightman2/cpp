#include <iostream>
#include <vector>

template <typename Derived>
class Countable {
private:
    static inline int count = 0;
public:
    Countable() { ++count; }
    ~Countable() { --count; }
    static int getCount() { return count; }
};

class MyClass : public Countable<MyClass> {
    // MyClass获得了计数功能
};

class Document : public Countable<Document> {
    // Document特有功能
};

class User : public Countable<User> {
    // User特有功能
};

int main() {
    Document doc1, doc2, doc3;
    User user1, user2;
    
    std::cout << "Documents: " << Document::getCount() << std::endl; // 输出3
    std::cout << "Users: " << User::getCount() << std::endl; // 输出2
}