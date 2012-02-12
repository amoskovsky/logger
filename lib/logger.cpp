#include "StdAfx.h"
#include "logger.h"

#include <Windows.h>

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>


namespace logger {

destination* destination::m_instance = 0;

buffer trace_logger(destination::instance(), ":T:");
buffer debug_logger(destination::instance(), ":D:");
buffer info_logger(destination::instance(), ":I:");
buffer warn_logger(destination::instance(), ":W:");
buffer error_logger(destination::instance(), ":E:");


destination::destination()
: m_file_output(false)
, m_console_output(false)
, m_level(0)
{
}

destination& destination::instance()
{
    if (!m_instance)
        m_instance = new destination();
    return *m_instance;
}

void destination::set_file_name( const string& file_name )
{
    mutex::scoped_lock lock(m_mutex);
    if (m_stream.is_open() && m_file_name != file_name)
        m_stream.close();
    m_file_name = file_name;
    m_file_output = !m_file_name.empty();
}

void destination::write( const string& data )
{
    mutex::scoped_lock lock(m_mutex);
    
    if (m_console_output) {
        cout.write(data.c_str(), data.length());
        cout.flush();
    }

    if (!m_file_output)
        return;

    if (!m_stream.is_open()) {
        m_stream.clear();
        m_stream.open(m_file_name.c_str(), ios::out|ios::binary|ios::app);
        if (!m_stream.is_open()) {
            cerr << "Failed to open log file '" << m_file_name << "'. Logging to file disabled." << endl;
            m_file_output = false;
            return;
        }
    }

    m_stream.write(data.c_str(), data.length());
    m_stream.flush();

    if (m_stream.fail()) {
        m_file_output = false;
    }
    m_stream.close();
}

void destination::file_output( bool value )
{
    m_file_output = value;
}

bool destination::file_output() const
{
    return m_file_output;
}

void destination::console_output( bool value )
{
    m_console_output = value;
}

bool destination::console_output() const
{
    return m_console_output;
}

void destination::truncate()
{
    mutex::scoped_lock lock(m_mutex);
    if (m_stream.is_open())
        m_stream.close();
    boost::filesystem::remove(m_file_name.c_str());
}

void destination::destroy()
{
    if (m_instance) {
        delete m_instance;
        m_instance = 0;
    }
}



void buffer::enabled( bool value )
{
    m_enabled = value;
}

buffer::buffer( destination& dest, const string& id )
: m_destination(dest)
, m_enabled(false)
, m_id(id)
{
}

void buffer::init_data()
{
    if (!m_data.get()) {
        m_data.reset(new data());
        m_data->has_data = false;
    }
    if (!m_data->has_data) {
        boost::posix_time::ptime now  = boost::posix_time::microsec_clock::local_time();
        m_data->buf << now << m_id;
        m_data->has_data = true;
    }
}

buffer& buffer::operator<<( StandardEndLine manip )
{
    if (!m_enabled)
        return *this;
    init_data();
    m_data->buf << manip;
    m_destination.write(m_data->buf.str());
    m_data->buf.str("");
    m_data->has_data = false;
    return *this;
}

buffer& buffer::operator<<( const std::wstring& s )
{
    return (*this) << string_cast<std::string>(s);
}

void setup( const string& file_name, bool console_output, int log_level )
{
    destination::instance().set_file_name(file_name);
    destination::instance().console_output(console_output);
    destination::instance().level(log_level);
    trace_logger.enabled(log_level >= 5);
    debug_logger.enabled(log_level >= 4);
    info_logger.enabled(log_level >= 3);
    warn_logger.enabled(log_level >= 2);
    error_logger.enabled(log_level >= 1);

}

void set_level( int log_level )
{
    destination::instance().level(log_level);
    trace_logger.enabled(log_level >= 5);
    debug_logger.enabled(log_level >= 4);
    info_logger.enabled(log_level >= 3);
    warn_logger.enabled(log_level >= 2);
    error_logger.enabled(log_level >= 1);
}


void truncate()
{
    destination::instance().truncate();
}

void cleanup()
{
    destination::destroy();
}

template <>
std::string string_cast(const std::wstring& src)
{
    string dest;
    if (!src.empty()) {
        size_t size = ::WideCharToMultiByte(
            CP_ACP,
            0, //WC_DEFAULTCHAR,
            src.c_str(),
            src.size(), // not incl 0
            NULL,
            0, // return buf size in bytes
            NULL, //"?", // def char
            NULL // &lb_UsedDefChar
            );
        assert_throw(size != 0);
        dest.resize(size);
        ::WideCharToMultiByte(
            CP_ACP,
            0, //WC_DEFAULTCHAR,
            src.c_str(),
            src.size(), // not incl 0
            &dest[0],
            size,
            NULL, //"?", // def char
            NULL  //&lb_UsedDefChar
            );
    }
    return dest;
}



std::string get_last_error()
{
    std::string message;
    DWORD error = GetLastError();
    char* msg;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg,
        0, NULL);
    if (size != 0) {
        message.resize(size);
        strncpy(&message[0], msg, size);
        LocalFree(msg);
    }
    else {
        message = "FormatMessage failed";
    }
    return message;
}


} //namespace logger
