#pragma once

#include "includes.h"

struct ben_code {
    virtual std::weak_ptr<ben_code>& get_prev() = 0;
    virtual std::map<std::string, std::shared_ptr<ben_code>>& get_dict() = 0;
    virtual std::list<std::shared_ptr<ben_code>>& get_list() = 0;
    virtual size_t get_int() = 0;
    virtual std::string& get_string() = 0;
    virtual std::vector<std::string>& get_keys() = 0;
    virtual ~ben_code() = default;

    std::string type_;
};
struct ben_code_dict: public ben_code {
    ben_code_dict();

    std::weak_ptr<ben_code>& get_prev() override;
    std::map<std::string, std::shared_ptr<ben_code>>& get_dict() override;
    std::vector<std::string>& get_keys() override;

    std::weak_ptr<ben_code> prev_;
    std::map<std::string, std::shared_ptr<ben_code>> dict_;
    std::vector<std::string> keys_;
private:
    std::list<std::shared_ptr<ben_code>>& get_list() override;
    size_t get_int() override;
    std::string& get_string() override;
};

struct ben_code_list: public ben_code {
    ben_code_list();

    std::weak_ptr<ben_code>& get_prev() override;
    std::list<std::shared_ptr<ben_code>>& get_list() override;

    std::weak_ptr<ben_code> prev_;
    std::list<std::shared_ptr<ben_code>> list_;
private:
    std::map<std::string, std::shared_ptr<ben_code>>& get_dict() override;
    size_t get_int() override;
    std::string& get_string() override;
    std::vector<std::string>& get_keys() override;
};
struct ben_code_int: public ben_code {
    ben_code_int(std::string value);

    size_t get_int() override;
    std::string& get_string() override;
    std::weak_ptr<ben_code>& get_prev() override;

    std::weak_ptr<ben_code> prev_;
    std::string val;
private:
    std::list<std::shared_ptr<ben_code>>& get_list() override;
    std::map<std::string, std::shared_ptr<ben_code>>& get_dict() override;
    std::vector<std::string>& get_keys() override;
};
struct ben_code_string: public ben_code {
    ben_code_string(std::string value);

    std::string& get_string() override;
    std::weak_ptr<ben_code>& get_prev() override;

    std::weak_ptr<ben_code> prev_;
    std::string str;
private:
    std::list<std::shared_ptr<ben_code>>& get_list() override;
    std::map<std::string, std::shared_ptr<ben_code>>& get_dict() override;
    size_t get_int() override;
    std::vector<std::string>& get_keys() override;
};

std::shared_ptr <ben_code> decode(const std::string& toDecode_);
void get_bencode(std::shared_ptr<ben_code> current, std::string& code);

