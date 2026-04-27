#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>


std::string toHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return ss.str();
}


std::string toBase64(const unsigned char* data, size_t len) {
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            out.push_back(lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}


class Object {
public:
    virtual ~Object() {}
    virtual std::string getType() const = 0;
    virtual std::string serializeBin() const = 0; 
};

class Integer : public Object {
public:
    int value;
    Integer(int v) : value(v) {}
    std::string getType() const override { return "int"; }
    std::string serializeBin() const override {
        return std::string(reinterpret_cast<const char*>(&value), sizeof(int));
    }
};

class Float : public Object {
public:
    float value;
    Float(float v) : value(v) {}
    std::string getType() const override { return "float"; }
    std::string serializeBin() const override {
        return std::string(reinterpret_cast<const char*>(&value), sizeof(float));
    }
};

class StringObj : public Object {
public:
    std::string value;
    StringObj(std::string v) : value(v) {}
    std::string getType() const override { return "string"; }
    std::string serializeBin() const override { return value; }
};



class ObjectList {
private:
    std::vector<std::unique_ptr<Object>> items;
    const std::string SIGN = "MY_FORMAT";
    const std::string VERSION = "1.0";

public:
    void addInt(int v) { items.push_back(std::make_unique<Integer>(v)); }
    void addFloat(float v) { items.push_back(std::make_unique<Float>(v)); }
    void addString(std::string v) { items.push_back(std::make_unique<StringObj>(v)); }

    size_t count() const { return items.size(); }

    
    void saveJSON(const std::string& filename) {
        std::ofstream file(filename);
        file << "{\n";
        file << "  \"sign\": \"" << SIGN << "\",\n";
        file << "  \"version\": \"" << VERSION << "\",\n";
        file << "  \"items\": [\n";
        for (size_t i = 0; i < items.size(); ++i) {
            std::string bin = items[i]->serializeBin();
            file << "    {\"type\": \"" << items[i]->getType() << "\", \"data\": \"" 
                 << toHex((unsigned char*)bin.data(), bin.size()) << "\"}";
            if (i < items.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ]\n}";
        file.close();
    }

    
    void saveXML(const std::string& filename) {
        std::ofstream file(filename);
        file << "<root>\n";
        file << "  <sign>" << SIGN << "</sign>\n";
        file << "  <version>" << VERSION << "</version>\n";
        file << "  <size>" << items.size() << "</size>\n";
        file << "  <items>\n";
        for (auto& item : items) {
            std::string bin = item->serializeBin();
            file << "    <item type=\"" << item->getType() << "\">" 
                 << toBase64((unsigned char*)bin.data(), bin.size()) << "</item>\n";
        }
        file << "  </items>\n</root>";
        file.close();
    }

    
    bool loadFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        
        if (content.find(SIGN) == std::string::npos) {
            std::cerr << "Error: Wrong Signature!" << std::endl;
            return false;
        }

        if (content.find(VERSION) == std::string::npos) {
            std::cerr << "Error: Wrong Version!" << std::endl;
            return false;
        }

        std::cout << "File " << filename << " validated successfully." << std::endl;
        
        return true;
    }
};

int main() {
    ObjectList list;
    list.addInt(42);
    list.addFloat(3.14f);
    list.addString("Hello Binary");

    list.saveJSON("data.json");
    list.saveXML("data.xml");

    std::cout << "Files saved." << std::endl;

    if (list.loadFile("data.json")) {
        std::cout << "JSON Load Checked." << std::endl;
    }

    return 0;
}
