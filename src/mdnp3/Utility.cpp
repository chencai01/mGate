#include "mdnp3/pch.h"
#include "mdnp3/Utility.h"
#include <stdio.h>
#include <time.h>


namespace Dnp3Master
{

unsigned short create_crc(byte frame[], int offset, int count)
{
    unsigned short __crc = 0x0000;
    for (int i = offset; i < count + offset; i++)
    {
        __crc = (__crc >> 8) ^ crc_table[(__crc ^ frame[i]) & 0x00FF];
    }
    return ~__crc;
}

bool check_crc(byte frame[], int offset, int count)
{
    if (count > 2)
    {
        unsigned short __crc = create_crc(frame, offset, count - 2);
        if (frame[offset + count - 2] == (__crc & 0x00ff))
        {
            if (frame[offset + count - 1] == (__crc >> 8 & 0x00ff)) return true;
        }
    }

    return false;
}

byte get_low_byte_uint16(unsigned short num)
{
    return (byte)(num & 0x00ff);
}

byte get_high_byte_uint16(unsigned short num)
{
    return (byte)(num >> 8 & 0x00ff);
}


byte get_byte_int32(int num, int pos)
{
    return (byte)(num >> ((pos - 1) * 8) & 0x000000ff);
}


byte reverse_bits_in_byte(byte num)
{
    byte __r = 0x00;

    __r = __r | (num >> 0 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 1 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 2 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 3 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 4 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 5 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 6 & 0x01);
    __r <<= 1;
    __r = __r | (num >> 7 & 0x01);
    __r <<= 1;

    return __r;
}

int calc_bytes_from_len(int len)
{
    // can than phep chia co du, phai de ((len - 5) / 16) thi moi dung
    int __crc = 2 + 2 * ((len - 5) / 16);
    if ((len - 5) % 16 != 0)  __crc += 2;
    return len + 3 + __crc;
}

bool check_frame(byte frame[], int length)
{
    int __len = (int)frame[2];

    if ((frame[0] != 0x05) || (frame[1] != 0x64) || (__len < 5)) return false;

    if (calc_bytes_from_len(__len) > MAXFRAME) return false;

    /*==== Check CRC ====*/

    if (!check_crc(frame, 0, 10)) return false;

    int __offset = 10;
    int __block16s = (length - 10) / 18;
    int __remain_bytes = (length - 10) % 18;

    for (int i = 1; i < __block16s; i++, __offset += 18)
        if (!check_crc(frame, __offset, 18)) return false;

    if (__remain_bytes != 0)
    {
        __offset = length - __remain_bytes;
        if (!check_crc(frame, __offset, __remain_bytes)) return false;
    }

    /*==== ====*/

    unsigned short __dest = (unsigned short)frame[5] << 8 | (unsigned short)frame[4];
    unsigned short __source = (unsigned short)frame[7] << 8 | (unsigned short)frame[6];

    // 65520(0xfff0), 65535(0xffff)
    // Gia tri cua dest va source khong duoc nam trong khoang:
    // [65520 - 65535]
    if (__dest >= 0xfff0)
    {
        return false;
    }
    if (__source >= 0xfff0)
    {
        return false;
    }

    byte __prm = (frame[3] >> 6) & 0x01;
    byte __dir = (frame[3] >> 7) & 0x01;
    byte __fc = frame[3] & 0x0f;

    if (__dir == 0x01)
    {
        return false;
    }

    bool __err = false;
    switch (__prm)
    {
    case 0x01:
        __err = (__fc != 0x00) && (__fc != 0x02) && (__fc != 0x03) && (__fc != 0x04) && (__fc != 0x09);
        break;
    case 0x00:
        __err = (__fc != 0x00) && (__fc != 0x01) && (__fc != 0x0b);
        break;
    }
    if (__err)
    {
        return false;
    }

    return true;
}

bool check_frame(byte frame[], int length, std::stringstream& debug_stream)
{
    int __len = (int)frame[2];

    if ((frame[0] != 0x05) || (frame[1] != 0x64) || (__len < 5))
    {
        debug_stream << "[check_frame] ERROR: start or len < 5\n";
        return false;
    }


    if (calc_bytes_from_len(__len) > MAXFRAME)
    {
        debug_stream << "[check_frame] ERROR: len > 292\n";
        return false;
    }


    /*==== Check CRC ====*/

    if (!check_crc(frame, 0, 10))
    {
        debug_stream << "[check_frame] CRC ERROR:";
        debug_stream << std::hex << std::setfill('0');

        for (int j = 0; j < 10; j++)
            debug_stream << " " << std::setw(2) << (int)frame[j];

        debug_stream << "\n";
        return false;
    }

    int __offset = 10;
    int __block16s = (length - 10) / 18;
    int __remain_bytes = (length - 10) % 18;

    for (int i = 1; i < __block16s; i++, __offset += 18)
        if (!check_crc(frame, __offset, 18))
        {
            debug_stream << "[check_frame] CRC ERROR:";
            debug_stream << std::hex << std::setfill('0');

            for (int j = 0; j < 18; j++)
                debug_stream << " " << std::setw(2) << (int)frame[j + __offset];

            debug_stream << "\n";
            return false;
        }

    if (__remain_bytes != 0)
    {
        __offset = length - __remain_bytes;
        if (!check_crc(frame, __offset, __remain_bytes))
        {
            debug_stream << "[check_frame] CRC ERROR:";
            debug_stream << std::hex << std::setfill('0');

            for (int j = 0; j < __remain_bytes; j++)
                debug_stream << " " << std::setw(2) << (int)frame[j+__offset];

            debug_stream << "\n";
            return false;
        }
    }

    /*==== ====*/

    unsigned short __dest = (unsigned short)frame[5] << 8 | (unsigned short)frame[4];
    unsigned short __source = (unsigned short)frame[7] << 8 | (unsigned short)frame[6];

    // 65520(0xfff0), 65535(0xffff)
    // Gia tri cua dest va source khong duoc nam trong khoang:
    // [65520 - 65535]
    if (__dest >= 0xfff0)
    {
        debug_stream << "[check_frame] dest addr error\n";
        return false;
    }
    if (__source >= 0xfff0)
    {
        debug_stream << "[check_frame] src addr error\n";
        return false;
    }

    byte __prm = (frame[3] >> 6) & 0x01;
    byte __dir = (frame[3] >> 7) & 0x01;
    byte __fc = frame[3] & 0x0f;

    if (__dir == 0x01)
    {
        debug_stream << "[check_frame] DIR error = 1\n";
        return false;
    }

    bool __err = false;
    switch (__prm)
    {
    case 0x01:
        __err = (__fc != 0x00) && (__fc != 0x02) && (__fc != 0x03) && (__fc != 0x04) && (__fc != 0x09);
        break;
    case 0x00:
        __err = (__fc != 0x00) && (__fc != 0x01) && (__fc != 0x0b);
        break;
    }
    if (__err)
    {
        debug_stream
                << std::dec << "[check_frame] FC ERROR: "
                << "PRM= " << (int)__prm
                << ", FC= " << (int)__fc
                << "\n";
        return false;
    }

    return true;
}

void split_string(std::vector<std::string>& result, std::string& s, char delim)
{
    using namespace std;
    size_t __start = 0;
    size_t __next = 0;
    while ((__next = s.find_first_of(delim, __start)) != string::npos)
    {
        if (__next != __start) result.push_back(s.substr(__start, __next - __start));
        __start = __next + 1;
    }
    if (__start < s.length()) result.push_back(s.substr(__start, __next - __start));
}

int is_little_endian()
{
    unsigned int x = 1;
    unsigned char *c = (unsigned char*) &x;
    return (int)*c; // return 1 neu la little endian
}

float bytes_to_float(byte bytes[])
{
    //Input sample: 0x40    0x00    0x00    0x00 = 2.0

    static int __is_little_endian = -1;
    if (__is_little_endian == -1) __is_little_endian = is_little_endian();
    byte a[4];

    if (__is_little_endian == 1)
    {
        a[0] = bytes[3]; // 0x00
        a[1] = bytes[2]; // 0x00
        a[2] = bytes[1]; // 0x00
        a[3] = bytes[0]; // 0x40
    }
    else
    {
        a[0] = bytes[0]; // 0x40
        a[1] = bytes[1]; // 0x00
        a[2] = bytes[2]; // 0x00
        a[3] = bytes[3]; // 0x00
    }

    return *(float*)(&a);
}

double bytes_to_double(byte bytes[])
{
    static int __is_little_endian = -1;
    if (__is_little_endian == -1) __is_little_endian = is_little_endian();
    byte a[8];

    if (__is_little_endian == 1)
    {
        a[0] = bytes[7];
        a[1] = bytes[6];
        a[2] = bytes[5];
        a[3] = bytes[4];
        a[4] = bytes[3];
        a[5] = bytes[2];
        a[6] = bytes[1];
        a[7] = bytes[0];
    }
    else
    {
        a[0] = bytes[0];
        a[1] = bytes[1];
        a[2] = bytes[2];
        a[3] = bytes[3];
        a[4] = bytes[4];
        a[5] = bytes[5];
        a[6] = bytes[6];
        a[7] = bytes[7];
    }

    return *(double*)(&a);
}

std::string timestamp_to_str(unsigned long long timestamp)
{
    if (timestamp <= 0) return std::string("0");
    time_t t = (time_t)(timestamp/1000);
    struct tm* timeinfo = gmtime(&t);
    char buffer[30];
    strftime(buffer, 30, "%F %T", timeinfo);
    sprintf(buffer, "%s.%d",buffer, (int)(timestamp%1000));
    return std::string(buffer);
}

byte convert_dnp3F_to_iecQ(byte flag)
{
    byte __iecQ = 0x00;
    __iecQ = (~flag << 7) & 0x80; // ONLINE -> INVALID
    __iecQ |= (flag << 4) & 0x40; // COMM_LOST -> NOT TOPICAL
    __iecQ |= ((flag << 2) | (flag << 1)) & 0x20; // FORCED -> SUBSTITUTED
    __iecQ |= ( flag >> 5) & 0x01; // OVER_RANGE -> OVERFLOW
    return __iecQ;
}

} // Dnp3Master namespace

