#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <ios>
#include <stdexcept>
#include <iostream>

#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>

namespace logger 
{

using namespace std;

using boost::mutex;
using boost::thread_specific_ptr;

template <class T, class S> T string_cast(const S& src);
template <> std::string string_cast(const std::wstring& src);

class destination {
public:
    static destination& instance();
    static void destroy();
    void file_output(bool value);
    bool file_output() const;
    void console_output(bool value);
    bool console_output() const;
    void set_file_name(const string& file_name);
    void write(const string& data);
    void truncate();
private:
    destination();

private:
    string m_file_name;
    ofstream m_stream;
    mutex m_mutex;
    static destination* m_instance;
    bool m_file_output;
    bool m_console_output;
};

class buffer {
public:
    buffer(destination& dest, const string& id);
    void enabled(bool value);
    bool enabled() const { return m_enabled; }
    const string& id() const;
    template <class T> buffer& operator << (const T& value) {
        if (!m_enabled)
            return *this;
        init_data();
        m_data->buf << value;
        return *this;
    }
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);
    // define an operator<< to take in std::endl
    buffer& operator<<(StandardEndLine manip);
    buffer& operator<<(const std::wstring& s);
    buffer& operator<<(const wchar_t* s) { return (*this) << std::wstring(s);}
private:
    void init_data();

private:
    struct data {
        ostringstream buf;
        bool has_data;
    };
    destination& m_destination;
    bool m_enabled;
    string m_id;
    thread_specific_ptr<data> m_data;
};

extern buffer trace_logger;
extern buffer debug_logger;
extern buffer info_logger;
extern buffer warn_logger;
extern buffer error_logger;

void setup(const string& file_name, bool console_output, int log_level);
void truncate();
void cleanup();
void set_level(int log_level);

std::string get_last_error();

} // namespace logger 

#define trace_log if (logger::trace_logger.enabled()) logger::trace_logger
#define debug_log if (logger::debug_logger.enabled()) logger::debug_logger
#define info_log if (logger::info_logger.enabled()) logger::info_logger
#define warn_log if (logger::warn_logger.enabled()) logger::warn_logger
#define error_log if (logger::error_logger.enabled()) logger::error_logger


#define assert_throw(cond) if (!(cond)) \
    do { \
        std::stringstream os; os << "assert failed at " \
        << __FILE__ << " line " << __LINE__ << ": " << #cond; \
        std::string msg = os.str(); \
        error_log << msg << std::endl; \
        throw std::runtime_error(msg); \
    } while (0)
