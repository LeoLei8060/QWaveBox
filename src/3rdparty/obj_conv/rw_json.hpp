#ifndef RW_JSON_HPP_
#define RW_JSON_HPP_

#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif
#include <algorithm>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#if (QT_VERSION < 0x060000)
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QQueue>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#endif // QT
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "rw_traits_type.hpp"

namespace reflexjson {
using namespace reflextraits;

template<typename _doc>
class Reader
{
protected:
    using doc_type = _doc;
    using reader_doc_tpye = Reader<_doc>;

protected:
    const doc_type *parent_;
    const char     *key_;
    int64_t         index_;

public:
    Reader(const doc_type *parent, const char *key)
        : parent_(parent)
        , key_(key)
        , index_(-1)
    {}
    Reader(const doc_type *parent, size_t index)
        : parent_(parent)
        , key_(nullptr)
        , index_(static_cast<int64_t>(index))
    {}
    ~Reader() {}

protected:
    decltype(auto) get_obj(const char *key, doc_type *doc_val)
    {
        doc_type *obj = static_cast<doc_type *>(this);
        if (nullptr != key)
            obj = obj->child(key, doc_val);
        return obj;
    }
    const std::string key() const { return key_ != nullptr ? key_ : std::to_string(index_); }
};
class JsonReader : public Reader<JsonReader>
{
    friend class Reader<JsonReader>;

private:
    std::string                                                    m_strData;
    std::shared_ptr<rapidjson::Document>                           doc_;
    const rapidjson::Value                                        *val_;
    mutable std::shared_ptr<rapidjson::Value::ConstMemberIterator> iter_;

public:
    JsonReader(const std::string &str, bool isfile = false)
        : reader_doc_tpye(nullptr, "")
        , doc_(new rapidjson::Document())
        , val_(doc_.get())
        , m_strData("")
    {
        std::string err;
        do {
            if (isfile) {
                std::ifstream fs(str.c_str(), std::ifstream::binary);
                if (!fs) {
                    err = "open file[" + str + "] fail.";
                    break;
                }
                m_strData = std::string((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
                doc_->Parse(m_strData);
            } else {
                doc_->Parse(str);
            }

            if (doc_->HasParseError()) {
                size_t offset = doc_->GetErrorOffset();
                if (isfile) {
                    std::string err_info = m_strData.substr(offset, offset + 32);
                    err = "parse json file [" + str + "] fail. " + err_info;
                    break;
                } else {
                    std::string err_info = str.substr(offset, offset + 32);
                    err = "parse json string [" + str + "] fail. " + err_info;
                    break;
                }
            }
            return;
        } while (false);

        doc_.reset();
        printf("error:%s\n", err.c_str());
    }
    ~JsonReader() { m_strData.clear(); }

public:
#define JSON_GETVAL(f, is_f, ...) \
    const rapidjson::Value *v = get_val(key); \
    if (NULL == v) { \
        return false; \
    } \
    try { \
        if (!v->is_f()) { \
            throw reflex_exption("key type error"); \
        } \
        val = __VA_ARGS__ v->f(); \
    } catch (const std::exception &e) { \
        printf("key<%s>:%s\n", key, e.what()); \
        return false; \
    } \
    return true;

    bool convert(const char *key, std::vector<char> &val)
    {
        std::string str;
        convert(key, str);
        val.resize(str.size());
        val.assign(str.begin(), str.end());
        return true;
    }
    bool convert(const char *key, std::string &val) { JSON_GETVAL(GetString, IsString); }
    bool convert(const char *key, int8_t &val) { JSON_GETVAL(GetInt, IsInt, (int8_t)); }
    bool convert(const char *key, uint8_t &val) { JSON_GETVAL(GetInt, IsInt, (uint8_t)); }
    bool convert(const char *key, int16_t &val) { JSON_GETVAL(GetInt, IsInt, (int16_t)); }
    bool convert(const char *key, uint16_t &val) { JSON_GETVAL(GetInt, IsInt, (uint16_t)); }
    bool convert(const char *key, int32_t &val) { JSON_GETVAL(GetInt, IsInt); }
    bool convert(const char *key, uint32_t &val) { JSON_GETVAL(GetUint, IsUint); }
    bool convert(const char *key, int64_t &val) { JSON_GETVAL(GetInt64, IsInt64); }
    bool convert(const char *key, uint64_t &val) { JSON_GETVAL(GetUint64, IsUint64); }
    bool convert(const char *key, double &val) { JSON_GETVAL(GetDouble, IsDouble); }
    bool convert(const char *key, float &val) { JSON_GETVAL(GetFloat, IsDouble); }
    bool convert(const char *key, bool &val)
    {
        const rapidjson::Value *v = get_val(key);
        if (NULL == v) {
            return false;
        } else if (v->IsBool()) {
            val = v->GetBool();
            return true;
        } else if (v->IsInt64()) {
            val = (0 != (v->GetInt64()));
            return true;
        } else {
            throw reflex_exption(std::string(key) + "wish bool, but not bool or int");
            return false;
        }
    }
    template<typename _type, typename = std::enable_if_t<std::is_enum_v<_type>>>
    bool convert(const char *key, _type &enum_val)
    {
        typename std::underlying_type<_type>::type val;
        if (!this->convert(key, val))
            return false;
        enum_val = static_cast<_type>(val);
        return true;
    }
    template<typename _type, typename = std::enable_if_t<!std::is_same_v<_type, char>>>
    bool convert(const char *key, std::vector<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        val.resize(num);
        for (size_t i = 0; i < num; ++i) {
            (*obj)[i].convert(nullptr, val[i]);
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, std::list<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        for (size_t i = 0; i < num; ++i) {
            _type elem;
            (*obj)[i].convert(nullptr, elem);
            val.emplace_back(elem);
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, std::set<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        for (size_t i = 0; i < num; ++i) {
            _type elem;
            (*obj)[i].convert(nullptr, elem);
            val.insert(elem);
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, std::map<std::string, _type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        for (auto data = obj->begin(); data; data = data.next()) {
            _type elem;
            data.convert(nullptr, elem);
            val[data.key()] = elem;
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, std::unordered_map<std::string, _type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;

        //data operator bool()
        for (auto data = obj->begin(); data; data = data.next()) {
            _type elem;
            data.convert(nullptr, elem);
            val[data.key()] = elem;
        }
        return true;
    }
    template<typename... _type>
    bool convert(const char *key, std::tuple<_type...> &data)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;

        int index = 0;
        for_each_tuple(data, [obj, &index](auto &&args) {
            (*obj)[index].convert(nullptr, args);
            index++;
        });

        return true;
    }
    template<typename _type>
    bool convert(const char *key, std::shared_ptr<_type> &val)
    {
        if (nullptr == val.get()) {
            val.reset(new _type());
        }
        return this->convert(key, *val);
    }
#if (QT_VERSION < 0x060000)
    bool convert(const char *key, QString &qval)
    {
        std::string val;
        //JSON_GETVAL(GetString, IsString);
        const rapidjson::Value *v = get_val(key);
        if (NULL == v) {
            return false;
        }
        try {
            if (!v->IsString()) {
                throw reflex_exption("key type error");
            }
            val = v->GetString();
        } catch (const std::exception &e) {
            printf("key<%s>:%s\n", key, e.what());
            return false;
        }
        qval = string2QString(val);
        return true;
    }
    template<typename _type>
    bool convert(const char *key, QVector<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        val.resize(num);
        for (size_t i = 0; i < num; ++i) {
            (*obj)[i].convert(nullptr, val[i]);
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, QList<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        val.clear();
        size_t num = obj->size();
        for (size_t i = 0; i < num; ++i) {
            _type elem;
            (*obj)[i].convert(nullptr, elem);
            val.push_back(elem);
        }
        return true;
    }
    template<typename _type>
    bool convert(const char *key, QQueue<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        val.clear();
        size_t num = obj->size();
        for (size_t i = 0; i < num; ++i) {
            _type elem;
            (*obj)[i].convert(nullptr, elem);
            val.append(elem);
        }
        return true;
    }
    bool convert(const char *key, QVariant &qval)
    {
        std::string             val;
        const rapidjson::Value *v = get_val(key);
        if (NULL == v) {
            return false;
        }
        try {
            if (!v->IsString()) {
                throw reflex_exption("key type error");
            }
            val = v->GetString();
        } catch (const std::exception &e) {
            printf("key<%s>:%s\n", key, e.what());
            return false;
        }
        QVariant tmp(string2QString(val));
        bool     bOk = false;
        int      iNum = tmp.toInt(&bOk);
        if (bOk)
            qval = iNum;
        double dNum = tmp.toDouble(&bOk);
        if (bOk)
            qval = dNum;
        return true;
    }

    // TODO: from ljb
    // TODO: 支持QList<对象指针>
    template<typename _type>
    bool convert(const char *key, QList<_type *> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        val.clear();
        for (size_t i = 0; i < num; ++i) {
            _type *elem = new _type;
            (*obj)[i].convert(nullptr, *elem);
            val.push_back(elem);
        }
        return true;
    }

    // TODO: from ljb
    // TODO: 支持QList<对象指针>
    template<typename _type>
    bool convert(const char *key, QVector<_type *> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        val.clear();
        for (size_t i = 0; i < num; ++i) {
            _type *elem = new _type;
            (*obj)[i].convert(nullptr, *elem);
            val.push_back(elem);
        }
        return true;
    }

    template<typename _type>
    bool convert(const char *key, QSet<_type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        size_t num = obj->size();
        for (size_t i = 0; i < num; ++i) {
            _type elem;
            (*obj)[i].convert(nullptr, elem);
            val.insert(elem);
        }
        return true;
    }

    template<typename _type>
    bool convert(const char *key, QMap<QString, _type> &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        if (nullptr == obj)
            return false;
        for (auto data = obj->begin(); data; data = data.next()) {
            _type elem;
            data.convert(nullptr, elem);
            val[QString::fromStdString(data.key())] = elem;
        }
        return true;
    }

    template<typename _type, typename... Args>
    bool convert(const char *key, QSharedPointer<_type> &val, Args &&...args)
    {
        if (nullptr == val.get()) {
            val.reset(new _type(std::forward<Args>(args)...));
        }
        return this->convert(key, *val);
    }

    template<typename _type, typename... Args>
    bool convert(const char *key, std::unique_ptr<_type> &val, Args &&...args)
    {
        if (nullptr == val.get()) {
            val.reset(new _type(std::forward<Args>(args)...));
        }
        return this->convert(key, *val);
    }

#endif
    template<typename _type, std::enable_if_t<has_member_condition<_type>::value, bool> = true>
    bool convert(const char *key, _type &val)
    {
        JsonReader  doc_val;
        JsonReader *obj = get_obj(key, &doc_val);
        bool        bret = false;
        if (nullptr == obj)
            return bret;

        size_t len = obj->size();
        if (len <= 1) {
            val.obj_to_struct(*obj);
            bret = true;
        } else if (nullptr == val.cond_t_.cond_func_) {
            return bret;
        } else {
            for (size_t i = 0; i < len; ++i) {
                JsonReader sub = (*obj)[i];
                if (val.cond_t_.cond_func_(val.cond_t_.parent_, (void *) &sub)) {
                    val.obj_to_struct(sub);
                    bret = true;
                    break;
                }
            }
        }
        val.cond_t_.set_value(nullptr, nullptr);
        return bret;
    }
    bool       has(const char *key) { return val_->HasMember(key) ? !(*val_)[key].IsNull() : false; }
    size_t     size() { return val_->IsArray() ? static_cast<size_t>(val_->Size()) : 0; }
    JsonReader operator[](const char *key)
    {
        if (!val_->HasMember(key)) {
            printf("key:%s not have", key);
            return JsonReader(nullptr, nullptr, "");
        }
        return JsonReader(&(*val_)[key], this, key);
    }
    JsonReader operator[](size_t index)
    {
        if (!val_->IsArray()) {
            printf("index:%llu out of index\n", index);
            return JsonReader(0, 0, "");
        }
        return JsonReader(&(*val_)[(rapidjson::SizeType) index], this, index);
    }
    JsonReader begin()
    {
        iter_.reset(new rapidjson::Value::ConstMemberIterator());
        *iter_ = val_->MemberBegin();
        return *iter_ != val_->MemberEnd() ? JsonReader(&(*iter_)->value, this, (*iter_)->name.GetString())
                                           : JsonReader(0, this, "");
    }
    JsonReader next()
    {
        if (nullptr == parent_) {
            printf("err:parent null\n");
            return JsonReader(0, parent_, "");
        } else if (nullptr == parent_->iter_) {
            printf("err:parent no iter\n");
            return JsonReader(0, parent_, "");
        } else {
            ++(*parent_->iter_);
        }
        return *parent_->iter_ != parent_->val_->MemberEnd()
                   ? JsonReader(&(*parent_->iter_)->value, parent_, (*parent_->iter_)->name.GetString())
                   : JsonReader(0, parent_, "");
    }
    operator bool() const { return nullptr != val_; }

    std::string get_json() { return m_strData; }

private:
    JsonReader()
        : reader_doc_tpye(0, "")
        , doc_(nullptr)
        , val_(nullptr)
        , iter_(nullptr)
    {}
    JsonReader(const rapidjson::Value *val, const JsonReader *parent, const char *key)
        : reader_doc_tpye(parent, key)
        , doc_(nullptr)
        , val_(val)
        , iter_(nullptr)
    {}
    JsonReader(const rapidjson::Value *val, const JsonReader *parent, size_t index)
        : reader_doc_tpye(parent, index)
        , doc_(nullptr)
        , val_(val)
        , iter_(nullptr)
    {}

    JsonReader *child(const char *key, JsonReader *js_reader)
    {
        rapidjson::Value::ConstMemberIterator iter;
        if (nullptr != val_ && (iter = val_->FindMember(key)) != val_->MemberEnd() && !(iter->value.IsNull())) {
            js_reader->key_ = key;
            js_reader->parent_ = this;
            js_reader->val_ = &iter->value;
            return js_reader;
        } else
            return nullptr;
    }
    const rapidjson::Value *get_val(const char *key)
    {
        if (nullptr == key) {
            return val_;
        } else if (nullptr != val_) {
            rapidjson::Value::ConstMemberIterator iter = val_->FindMember(key);
            return iter != val_->MemberEnd() && !(iter->value.IsNull()) ? &iter->value : nullptr;
        } else {
            return nullptr;
        }
    }
};
class JsonWriter
{
    using JsonStringBuffer = rapidjson::StringBuffer;
    using JsonWriterStringBuffer = rapidjson::Writer<JsonStringBuffer>;
    using JsonPrettyStringBuffer = rapidjson::PrettyWriter<JsonStringBuffer>;

private:
    std::unique_ptr<JsonStringBuffer>       json_buf_;
    std::unique_ptr<JsonWriterStringBuffer> json_writer_;
    std::unique_ptr<JsonPrettyStringBuffer> json_pretty_;

public:
    JsonWriter(int indetCount = 0, char indentChar = ' ')
        : json_buf_(new JsonStringBuffer())
    {
        if (indetCount < 0) {
            json_writer_ = std::make_unique<JsonWriterStringBuffer>(*json_buf_);
        } else {
            json_pretty_ = std::make_unique<JsonPrettyStringBuffer>(*json_buf_);
            json_pretty_->SetIndent(indentChar, indetCount);
        }
    }
    ~JsonWriter() {}

    std::string to_json_str() { return json_buf_->GetString(); }
    void        data_set_key(const char *key)
    {
        if (nullptr != key && key[0] != '\0') {
            json_writer_ != nullptr ? json_writer_->Key(key) : json_pretty_->Key(key);
        }
    }
    void        array_begin() { json_writer_ != nullptr ? json_writer_->StartArray() : json_pretty_->StartArray(); }
    void        array_end() { json_writer_ != nullptr ? json_writer_->EndArray() : json_pretty_->EndArray(); }
    void        object_begin() { json_writer_ != nullptr ? json_writer_->StartObject() : json_pretty_->StartObject(); }
    void        object_end() { json_writer_ != nullptr ? json_writer_->EndObject() : json_pretty_->EndObject(); }
    JsonWriter &convert(const char *key, const std::vector<char> &val)
    {
        std::string str;
        auto        iter = std::find(val.cbegin(), val.cend(), '\0');
        str.assign(val.begin(), iter);
        return convert(key, str);
    }
    JsonWriter &convert(const char *key, const std::string &val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->String(val) : json_pretty_->String(val);
        return *this;
    }
    JsonWriter &convert(const char *key, int16_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Int(val) : json_pretty_->Int(val);
        return *this;
    }
    JsonWriter &convert(const char *key, uint16_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Uint(val) : json_pretty_->Uint(val);
        return *this;
    }
    JsonWriter &convert(const char *key, int32_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Int(val) : json_pretty_->Int(val);
        return *this;
    }
    JsonWriter &convert(const char *key, uint32_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Uint(val) : json_pretty_->Uint(val);
        return *this;
    }
    JsonWriter &convert(const char *key, int64_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Int64(val) : json_pretty_->Int64(val);
        return *this;
    }
    JsonWriter &convert(const char *key, uint64_t val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Uint64(val) : json_pretty_->Uint64(val);
        return *this;
    }
    JsonWriter &convert(const char *key, double val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Double(val) : json_pretty_->Double(val);
        return *this;
    }
    JsonWriter &convert(const char *key, float val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Double(val) : json_pretty_->Double(val);
        return *this;
    }
    JsonWriter &convert(const char *key, bool val)
    {
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->Bool(val) : json_pretty_->Bool(val);
        return *this;
    }
    //add enum tpye check:https://stackoverflow.com/questions/9343329/how-to-know-underlying-type-of-class-enum
    template<typename _type, typename = typename std::enable_if_t<std::is_enum_v<_type>>>
    JsonWriter &convert(const char *key, const _type &val)
    {
        return convert(key, static_cast<typename std::underlying_type<_type>::type>(val));
    }
    template<typename _type, typename = std::enable_if_t<!std::is_same_v<_type, char>>>
    JsonWriter &convert(const char *key, const std::vector<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (size_t i = 0; i < data.size(); ++i) {
            this->convert("", data[i]);
        }
        this->array_end();
        return *this;
    }
    template<typename _type>
    JsonWriter &convert(const char *key, const std::list<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename std::list<_type>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", *it);
        }
        this->array_end();
        return *this;
    }
    template<typename _type>
    JsonWriter &convert(const char *key, const std::set<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename std::set<_type>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", *it);
        }
        this->array_end();
        return *this;
    }
    //template <typename _type>
    //void convert(const char* key, const std::map<std::string, _type>& data)
    //{
    //    data_set_key(key);
    //    this->object_begin();
    //    for (typename std::map<std::string, _type>::const_iterator iter = data.begin(); iter != data.end(); ++iter)
    //    {
    //        this->convert(iter->first.c_str(), iter->second);
    //    }
    //    this->object_end();
    //}
    //template<typename _key_type, typename _value_type>
    //void convert(const char* key, const std::map<_key_type, _value_type>& data)
    //{
    //    data_set_key(key);
    //    this->object_begin();
    //    for (typename std::map<_key_type, _value_type>::const_iterator iter = data.begin(); iter != data.end(); ++iter)
    //    {
    //        std::string key_str = std::to_string(iter->first);
    //        this->convert(key_str.c_str(), iter->second);
    //    }
    //    this->object_end();
    //}
#if (QT_VERSION < 0x060000)
    JsonWriter &convert(const char *key, const QString &qval)
    {
        std::string val = reflextraits::QString2string(qval);
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->String(val) : json_pretty_->String(val);
        return *this;
    }
    template<typename _type>
    JsonWriter &convert(const char *key, const QVector<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (size_t i = 0; i < data.size(); ++i) {
            this->convert("", data[i]);
        }
        this->array_end();
        return *this;
    }
    template<typename _type>
    JsonWriter &convert(const char *key, const QList<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename QList<_type>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", *it);
        }
        this->array_end();
        return *this;
    }
    template<typename _type>
    JsonWriter &convert(const char *key, const QQueue<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename QQueue<_type>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", *it);
        }
        this->array_end();
        return *this;
    }
    JsonWriter &convert(const char *key, const QVariant &qval)
    {
        std::string val = reflextraits::QString2string(qval.toString());
        data_set_key(key);
        json_writer_ != nullptr ? json_writer_->String(val) : json_pretty_->String(val);
        return *this;
    }
    // TODO: from ljb
    // TODO: 支持QList<对象指针>
    template<typename _type>
    JsonWriter &convert(const char *key, const QList<_type *> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename QList<_type *>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", **it);
        }
        this->array_end();
        return *this;
    }

    template<typename _type>
    JsonWriter &convert(const char *key, const QSet<_type> &data)
    {
        data_set_key(key);
        this->array_begin();
        for (typename QSet<_type>::const_iterator it = data.begin(); it != data.end(); ++it) {
            this->convert("", *it);
        }
        this->array_end();
        return *this;
    }

    template<typename _type>
    void convert(const char *key, const QMap<QString, _type> &data)
    {
        data_set_key(key);
        this->object_begin();
        for (typename QMap<QString, _type>::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
            this->convert(iter.key().toStdString().c_str(), iter.value());
        }
        this->object_end();
    }

    template<typename _type>
    void convert(const char *key, const QSharedPointer<_type> &val)
    {
        if (nullptr == val.get())
            return;
        this->convert(key, *val);
    }

    template<typename _type>
    void convert(const char *key, const std::unique_ptr<_type> &val)
    {
        if (nullptr == val.get())
            return;
        this->convert(key, *val);
    }

#endif
    template<typename _key, typename _type>
    void convert(const char *key, const std::map<_key, _type> &data)
    {
        if constexpr (std::is_same_v<std::string, _key> && std::is_same_v<QString, _key>) {
            data_set_key(key);
            this->object_begin();
            for (typename std::map<_key, _type>::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
                this->convert(iter->first.c_str(), iter->second);
            }
            this->object_end();
        } else
            static_assert("errror!");
    }
    template<typename _type>
    void convert(const char *key, const std::unordered_map<std::string, _type> &data)
    {
        data_set_key(key);
        this->object_begin();
        for (typename std::unordered_map<std::string, _type>::const_iterator iter = data.begin(); iter != data.end();
             ++iter) {
            this->convert(iter->first.c_str(), iter->second);
        }
        this->object_end();
    }
    //add tuple tpye support https://en.cppreference.com/w/cpp/utility/apply
    template<typename... _type>
    void convert(const char *key, const std::tuple<_type...> &data)
    {
        data_set_key(key);
        this->array_begin();
//C++17 implement
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
        std::apply([this](auto &&...args) { ((this->convert("", args)), ...); }, data);
#else
        for_each_tuple(data, [this](auto &&args) { this->convert("", args); });
#endif
        this->array_end();
    }
    template<typename _type>
    void convert(const char *key, const std::shared_ptr<_type> &val)
    {
        if (nullptr == val.get())
            return;
        this->convert(key, *val);
    }
    template<typename _type, std::enable_if_t<has_member_condition<_type>::value, bool> = true>
    void convert(const char *key, const _type &data)
    {
        data_set_key(key);
        this->object_begin();
        data.struct_to_obj(*this, key);
        this->object_end();
    }
};
} // namespace reflexjson
#endif //RW_JSON_HPP_
