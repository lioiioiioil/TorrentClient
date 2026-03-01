#include "bencode.h"

ben_code_dict::ben_code_dict()
        : prev_()
        , dict_()
        , keys_()
{type_ = "dict";}

std::weak_ptr<ben_code>& ben_code_dict::get_prev() {
    return prev_;
}

std::map<std::string, std::shared_ptr<ben_code>>& ben_code_dict::get_dict() {
    return dict_;
}

std::vector<std::string>& ben_code_dict::get_keys() {
    return keys_;
}

std::list<std::shared_ptr<ben_code>>& ben_code_dict::get_list() {throw std::runtime_error("bad func");}
size_t ben_code_dict::get_int() {throw std::runtime_error("bad func");}
std::string& ben_code_dict::get_string() {throw std::runtime_error("bad func");}




ben_code_list::ben_code_list()
        : prev_()
        , list_()
{type_ = "list";}

std::weak_ptr<ben_code>& ben_code_list::get_prev() {
    return prev_;
}

std::list<std::shared_ptr<ben_code>>& ben_code_list::get_list() {
    return list_;
}

std::map<std::string, std::shared_ptr<ben_code>>& ben_code_list::get_dict() {throw std::runtime_error("bad func");}
size_t ben_code_list::get_int() {throw std::runtime_error("bad func");}
std::string& ben_code_list::get_string() {throw std::runtime_error("bad func");}
std::vector<std::string>& ben_code_list::get_keys() {throw std::runtime_error("bad func");}




ben_code_int::ben_code_int(std::string value)
        : prev_()
        , val(value)
{type_ = "int";}

size_t ben_code_int::get_int()  {
    return stoull(val);
}

std::string& ben_code_int::get_string()  {
    return val;
};

std::weak_ptr<ben_code>& ben_code_int::get_prev()  {
    return prev_;
}

std::list<std::shared_ptr<ben_code>>& ben_code_int::get_list()  {throw std::runtime_error("bad func");}
std::map<std::string, std::shared_ptr<ben_code>>& ben_code_int::get_dict()  {throw std::runtime_error("bad func");}
std::vector<std::string>& ben_code_int::get_keys()  {throw std::runtime_error("bad func");}




ben_code_string::ben_code_string(std::string value)
        : prev_()
        , str(value)
{type_ = "string";}

std::string& ben_code_string::get_string()  {
    return str;
}

std::weak_ptr<ben_code>& ben_code_string::get_prev()  {
    return prev_;
}

std::list<std::shared_ptr<ben_code>>& ben_code_string::get_list()  {throw std::runtime_error("bad func");}
std::map<std::string, std::shared_ptr<ben_code>>& ben_code_string::get_dict()  {throw std::runtime_error("bad func");}
size_t ben_code_string::get_int()  {throw std::runtime_error("bad func");}
std::vector<std::string>& ben_code_string::get_keys()  {throw std::runtime_error("bad func");}




std::shared_ptr <ben_code> decode(const std::string& toDecode_) {
    std::shared_ptr <ben_code> current = nullptr, root = nullptr, new_current;
    size_t l = 0, r, num;
    std::string key, val_string, val_int;
    std::vector<char> stack;
    stack.push_back('r');
    while (l < toDecode_.size()) {
        if (toDecode_[l] == 'e') {
            current = current->get_prev().lock();
            stack.pop_back();
            ++l;
            continue;
        }
        if (stack.back() == 'd') {
            r = l + 1;
            while (toDecode_[r] != ':') ++r;
            num = stoull(toDecode_.substr(l, r - l));
            key = toDecode_.substr(r + 1, num);
            l = r + num + 1;
        }
        if (toDecode_[l] == 'd') {
            new_current = std::dynamic_pointer_cast<ben_code>(std::make_shared<ben_code_dict>());
            new_current->get_prev() = current;
            if (stack.back() == 'l') current->get_list().push_back(new_current);
            else if (stack.back() == 'd') {
                current->get_dict()[key] = new_current;
                current->get_keys().push_back(key);
            }
            else if (stack.back() == 'r') root = new_current;
            current = new_current;
            stack.push_back('d');
            ++l;
        } else if (toDecode_[l] == 'l') {

            new_current = std::dynamic_pointer_cast<ben_code>(
                    std::make_shared<ben_code_list>());

            new_current->get_prev() = current;
            if (stack.back() == 'l') current->get_list().push_back(new_current);
            else if (stack.back() == 'd') {
                current->get_dict()[key] = new_current;
                current->get_keys().push_back(key);
            }
            current = new_current;
            stack.push_back('l');
            ++l;
        } else if (toDecode_[l] == 'i') {
            r = l + 1;
            while (toDecode_[r] != 'e') ++r;
            val_int = toDecode_.substr(l + 1, r - l - 1);
            l = r + 1;
            new_current = std::dynamic_pointer_cast<ben_code>(
                    std::make_shared<ben_code_int>(val_int));
            new_current->get_prev() = current;
            if (stack.back() == 'l') current->get_list().push_back(new_current);
            else if (stack.back() == 'd') {
                current->get_dict()[key] = new_current;
                current->get_keys().push_back(key);
            }
        } else {
            r = l + 1;
            while (toDecode_[r] != ':') ++r;
            num = stoull(toDecode_.substr(l, r - l));
            val_string = toDecode_.substr(r + 1, num);
            l = r + num + 1;
            new_current = std::dynamic_pointer_cast<ben_code>(
                    std::make_shared<ben_code_string>(val_string));
            new_current->get_prev() = current;
            if (stack.back() == 'l') current->get_list().push_back(new_current);
            else if (stack.back() == 'd') {
                current->get_dict()[key] = new_current;
                current->get_keys().push_back(key);
            }
        }
    }
    return root;
}

void get_bencode(std::shared_ptr<ben_code> current, std::string& code) {
    if (current->type_ == "dict") {
        code += 'd';
        for(const auto& x: current->get_keys()) {
            code += std::to_string(x.size()) + ':' + x;
            get_bencode(current->get_dict()[x], code);
        }
        code += 'e';
    }
    else if (current->type_ == "list") {
        code += 'l';
        for(const auto& x: current->get_list()) {
            get_bencode(x, code);
        }
        code += 'e';
    }
    else if (current->type_ == "int") {
        code += 'i' + current->get_string() + 'e';
    }
    else {
        code += std::to_string(current->get_string().size()) + ':' + current->get_string();
    }
}

