#include <clocale>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <stdexcept>

class BufString {
private:
    std::shared_ptr<char[]> owner_;   // владелец памяти (ref-counted)
    char*                   Buf;      // указатель в пределах owner_
    size_t                  len_;     // длина строки от Buf
    bool                    shared_;  // флаг: буфер разделён с другим объектом

    BufString(std::shared_ptr<char[]> owner, char* buf,
              size_t len, bool shared)
        : owner_(std::move(owner)), Buf(buf), len_(len), shared_(shared) {}

    static std::shared_ptr<char[]> allocCopy(const char* src, size_t n) {
        auto p = std::shared_ptr<char[]>(new char[n + 1]);
        std::memcpy(p.get(), src, n);
        p.get()[n] = '\0';
        return p;
    }

public:
    explicit BufString(const std::string& s)
        : owner_(allocCopy(s.c_str(), s.size())),
          Buf(owner_.get()), len_(s.size()), shared_(false) {}

    explicit BufString(const char* buf)
        : shared_(false)
    {
        if (!buf) buf = "";
        len_   = std::strlen(buf);
        owner_ = allocCopy(buf, len_);
        Buf    = owner_.get();
    }

    BufString clone() const {
        auto p = allocCopy(Buf, len_);
        return BufString(p, p.get(), len_, false);
    }


    BufString substr(size_t i, size_t size) const {
        if (i + size > len_)
            throw std::out_of_range("BufString::substr — выход за границы");

        if (i + size == len_) {                        // хвост → share
            return BufString(owner_, Buf + i, size, true);
        }
        auto p = allocCopy(Buf + i, size);             // середина → copy
        return BufString(p, p.get(), size, false);
    }

    BufString add(const BufString& s) const {
        size_t newLen = len_ + s.len_;
        auto p = std::shared_ptr<char[]>(new char[newLen + 1]);
        std::memcpy(p.get(),         Buf,   len_);
        std::memcpy(p.get() + len_,  s.Buf, s.len_);
        p.get()[newLen] = '\0';
        return BufString(p, p.get(), newLen, false);
    }

    BufString trim() const {
        size_t start = 0;
        while (start < len_ && Buf[start] == ' ') ++start;

        size_t end = len_;
        while (end > start && Buf[end - 1] == ' ') --end;

        size_t newLen = end - start;

        if (end == len_) {                             // нет правых пробелов
            return BufString(owner_, Buf + start, newLen, true);
        }
        auto p = allocCopy(Buf + start, newLen);       // есть правые → copy
        return BufString(p, p.get(), newLen, false);
    }

    char at(size_t i) const {
        if (i >= len_)
            throw std::out_of_range("BufString::at — выход за границы");
        return Buf[i];
    }

    char operator[](size_t i) const { return Buf[i]; }

    size_t      length()   const { return len_;    }
    const char* c_str()    const { return Buf;     }
    bool        isShared() const { return shared_; }

    friend std::ostream& operator<<(std::ostream& os, const BufString& bs) {
        return os.write(bs.Buf, static_cast<std::streamsize>(bs.len_));
    }
};

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    using std::cout, std::endl;
    auto header = [](const char* title) {
        cout << "\n══════════ " << title << " ══════════\n";
    };
    header("Конструкторы");

    BufString fromStr(std::string("Hello, world!"));
    cout << "Из string:  \"" << fromStr << "\"  len=" << fromStr.length() << endl;

    BufString fromPC("PChar строка");
    cout << "Из PChar:   \"" << fromPC  << "\"  len=" << fromPC.length()  << endl;

    header("clone (Prototype)");

    BufString original("Prototype");
    BufString cloned = original.clone();
    cout << "Оригинал:   \"" << original << "\"  ptr=" << (void*)original.c_str() << endl;
    cout << "Клон:       \"" << cloned   << "\"  ptr=" << (void*)cloned.c_str()   << endl;
    cout << "Буферы разные? " << (original.c_str() != cloned.c_str() ? "Да" : "Нет") << endl;

    header("substr");

    BufString base("Hello, world!");

    BufString mid = base.substr(0, 5);     // "Hello"
    cout << "substr(0,5) = \"" << mid << "\"  shared=" << mid.isShared() << endl;

    BufString tail = base.substr(7, 6);    // "world!"
    cout << "substr(7,6) = \"" << tail << "\"  shared=" << tail.isShared() << endl;
    cout << "  base.c_str()+7 == tail.c_str()? "
         << (base.c_str() + 7 == tail.c_str() ? "Да (один буфер)" : "Нет") << endl;

    header("add");

    BufString a(std::string("Foo"));
    BufString b(std::string("Bar"));
    BufString ab = a.add(b);
    cout << "\"" << a << "\" + \"" << b << "\" = \"" << ab << "\"" << endl;

    BufString chain = BufString("A").add(BufString("B")).add(BufString("C"));
    cout << "Цепочка: \"" << chain << "\"" << endl;

    header("trim");

    BufString leftOnly("   trimmed");
    BufString tLeft = leftOnly.trim();
    cout << "\"   trimmed\"  => \"" << tLeft << "\"  shared=" << tLeft.isShared() << endl;

    BufString both("   both sides   ");
    BufString tBoth = both.trim();
    cout << "\"   both sides   \" => \"" << tBoth << "\"  shared=" << tBoth.isShared() << endl;

    BufString nospace("clean");
    BufString tNone = nospace.trim();
    cout << "\"clean\" => \"" << tNone << "\"  shared=" << tNone.isShared() << endl;

    header("at() и operator[]");

    BufString s("ABCDE");
    cout << "Строка: \"" << s << "\"" << endl;
    for (size_t i = 0; i < s.length(); ++i)
        cout << "  s.at(" << i << ")='" << s.at(i)
             << "'   s[" << i << "]='" << s[i] << "'" << endl;

    try {
        s.at(100);
    } catch (const std::out_of_range& e) {
        cout << "at(100) => exception: " << e.what() << endl;
    }

    try {
        base.substr(10, 100);
    } catch (const std::out_of_range& e) {
        cout << "substr(10,100) => exception: " << e.what() << endl;
    }

    header("Immutability");

    BufString immut("immutable");
    BufString added = immut.add(BufString("!"));
    BufString trimmed = BufString("  immutable  ").trim();
    cout << "Оригинал после add:  \"" << immut   << "\" (не изменился)" << endl;
    cout << "Результат add:       \"" << added   << "\"" << endl;
    cout << "Результат trim:      \"" << trimmed << "\"" << endl;

    cout << "\n✓ Все тесты пройдены.\n";
    return 0;
}
