/*
TinyGPS - a small GPS library for Arduino providing basic NMEA parsing
Based on work by and "distance_to" and "course_to" courtesy of Maarten Lamers.
Suggestion to add satellites(), course_to(), and cardinal(), by Matt Monson.
Precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "tinygps.h"

#include <QtMath>

#define GPRMC_TERM   "GPRMC"
#define GPGGA_TERM   "GPGGA"
#define TWO_PI      (3.1416 * 2)
#define radians(x)  ((x) * 3.1416 / 180)
#define sq(z)       ((z) * (z))
#define degrees(y)  ((y) * 180 / 3.1416)

TinyGPS::TinyGPS(QObject *parent)
    : QObject(parent)
    , m_time(GPS_INVALID_TIME)
    ,  m_date(GPS_INVALID_DATE)
    ,  m_latitude(GPS_INVALID_ANGLE)
    ,  m_longitude(GPS_INVALID_ANGLE)
    ,  m_altitude(GPS_INVALID_ALTITUDE)
    ,  m_speed(GPS_INVALID_SPEED)
    ,  m_course(GPS_INVALID_ANGLE)
    ,  m_hdop(GPS_INVALID_HDOP)
    ,  m_numsats(GPS_INVALID_SATELLITES)
    ,  m_last_time_fix(GPS_INVALID_FIX_TIME)
    ,  m_last_position_fix(GPS_INVALID_FIX_TIME)
    ,  m_parity(0)
    ,  m_is_checksum_term(false)
    ,  m_sentence_type(GPS_SENTENCE_OTHER)
    ,  m_term_number(0)
    ,  m_term_offset(0)
    ,  m_gps_data_good(false)
    #ifndef _GPS_NO_STATS
    ,  m_encoded_characters(0)
    ,  m_good_sentences(0)
    ,  m_failed_checksum(0)
    #endif
{
    m_term[0] = '\0';
}

//
// public methods
//

bool TinyGPS::encode(char c)
{
    bool valid_sentence = false;

#ifndef _GPS_NO_STATS
    ++m_encoded_characters;
#endif
    switch(c)
    {
    case ',': // term terminators
        m_parity ^= c;
    case '\r':
    case '\n':
    case '*':
        if (m_term_offset < sizeof(m_term))
        {
            m_term[m_term_offset] = 0;
            valid_sentence = term_complete();
        }
        ++m_term_number;
        m_term_offset = 0;
        m_is_checksum_term = c == '*';
        return valid_sentence;

    case '$': // sentence begin
        m_term_number = m_term_offset = 0;
        m_parity = 0;
        m_sentence_type = GPS_SENTENCE_OTHER;
        m_is_checksum_term = false;
        m_gps_data_good = false;
        return valid_sentence;
    }

    // ordinary characters
    if (m_term_offset < sizeof(m_term) - 1)
        m_term[m_term_offset++] = c;
    if (!m_is_checksum_term)
        m_parity ^= c;

    return valid_sentence;
}

#ifndef GPS_NO_STATS
void TinyGPS::stats(unsigned long *chars, unsigned short *sentences, unsigned short *failed_cs)
{
    if (chars) *chars = m_encoded_characters;
    if (sentences) *sentences = m_good_sentences;
    if (failed_cs) *failed_cs = m_failed_checksum;
}
#endif

//
// internal utilities
//
int TinyGPS::from_hex(char a)
{
    if (a >= 'A' && a <= 'F')
        return a - 'A' + 10;
    else if (a >= 'a' && a <= 'f')
        return a - 'a' + 10;
    else
        return a - '0';
}

unsigned long TinyGPS::parse_decimal()
{
    char *p = m_term;
    bool isneg = *p == '-';
    if (isneg) ++p;
    unsigned long ret = 100UL * gpsatol(p);
    while (gpsisdigit(*p)) ++p;
    if (*p == '.')
    {
        if (gpsisdigit(p[1]))
        {
            ret += 10 * (p[1] - '0');
            if (gpsisdigit(p[2]))
                ret += p[2] - '0';
        }
    }
    return isneg ? -ret : ret;
}

// Parse a string in the form ddmm.mmmmmmm...
unsigned long TinyGPS::parse_degrees()
{
    char *p;
    unsigned long left_of_decimal = gpsatol(m_term);
    unsigned long hundred1000ths_of_minute = (left_of_decimal % 100UL) * 100000UL;
    for (p=m_term; gpsisdigit(*p); ++p);
    if (*p == '.')
    {
        unsigned long mult = 10000;
        while (gpsisdigit(*++p))
        {
            hundred1000ths_of_minute += mult * (*p - '0');
            mult /= 10;
        }
    }
    return (left_of_decimal / 100) * 1000000 + (hundred1000ths_of_minute + 3) / 6;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
bool TinyGPS::term_complete()
{
    if (m_is_checksum_term)
    {
        quint8 checksum = 16 * from_hex(m_term[0]) + from_hex(m_term[1]);
        if (checksum == m_parity)
        {
            if (m_gps_data_good)
            {
#ifndef _GPS_NO_STATS
                ++m_good_sentences;
#endif
                m_last_time_fix = m_new_time_fix;
                m_last_position_fix = m_new_position_fix;

                switch(m_sentence_type)
                {
                case GPS_SENTENCE_GPRMC:
                    m_time      = m_new_time;
                    m_date      = m_new_date;
                    m_latitude  = m_new_latitude;
                    m_longitude = m_new_longitude;
                    m_speed     = m_new_speed;
                    m_course    = m_new_course;
                    break;
                case GPS_SENTENCE_GPGGA:
                    m_altitude  = m_new_altitude;
                    m_time      = m_new_time;
                    m_latitude  = m_new_latitude;
                    m_longitude = m_new_longitude;
                    m_numsats   = m_new_numsats;
                    m_hdop      = m_new_hdop;
                    break;
                }

                return true;
            }
        }

#ifndef _GPS_NO_STATS
        else
            ++m_failed_checksum;
#endif
        return false;
    }

    // the first term determines the sentence type
    if (m_term_number == 0)
    {
        if (!gpsstrcmp(m_term, GPRMC_TERM))
            m_sentence_type = GPS_SENTENCE_GPRMC;
        else if (!gpsstrcmp(m_term, GPGGA_TERM))
            m_sentence_type = GPS_SENTENCE_GPGGA;
        else
            m_sentence_type = GPS_SENTENCE_OTHER;
        return false;
    }

    if (m_sentence_type != GPS_SENTENCE_OTHER && m_term[0])
        switch(COMBINE(m_sentence_type, m_term_number))
        {
        case COMBINE(GPS_SENTENCE_GPRMC, 1): // Time in both sentences
        case COMBINE(GPS_SENTENCE_GPGGA, 1):
            m_new_time = parse_decimal();
//            m_new_time_fix = millis();
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 2): // GPRMC validity
            m_gps_data_good = m_term[0] == 'A';
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 3): // Latitude
        case COMBINE(GPS_SENTENCE_GPGGA, 2):
            m_new_latitude = parse_degrees();
//            m_new_position_fix = millis();
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 4): // N/S
        case COMBINE(GPS_SENTENCE_GPGGA, 3):
            if (m_term[0] == 'S')
                m_new_latitude = -m_new_latitude;
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 5): // Longitude
        case COMBINE(GPS_SENTENCE_GPGGA, 4):
            m_new_longitude = parse_degrees();
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 6): // E/W
        case COMBINE(GPS_SENTENCE_GPGGA, 5):
            if (m_term[0] == 'W')
                m_new_longitude = -m_new_longitude;
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
            m_new_speed = parse_decimal();
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
            m_new_course = parse_decimal();
            break;
        case COMBINE(GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
            m_new_date = gpsatol(m_term);
            break;
        case COMBINE(GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
            m_gps_data_good = m_term[0] > '0';
            break;
        case COMBINE(GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
            m_new_numsats = (unsigned char)atoi(m_term);
            break;
        case COMBINE(GPS_SENTENCE_GPGGA, 8): // HDOP
            m_new_hdop = parse_decimal();
            break;
        case COMBINE(GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
            m_new_altitude = parse_decimal();
            break;
        }

    return false;
}

long TinyGPS::gpsatol(const char *str)
{
    long ret = 0;
    while (gpsisdigit(*str))
        ret = 10 * ret + *str++ - '0';
    return ret;
}

int TinyGPS::gpsstrcmp(const char *str1, const char *str2)
{
    while (*str1 && *str1 == *str2)
        ++str1, ++str2;
    return *str1;
}

/* static */
float TinyGPS::distance_between (float lat1, float long1, float lat2, float long2)
{
    // returns distance in meters between two positions, both specified
    // as signed decimal-degrees latitude and longitude. Uses great-circle
    // distance computation for hypothetical sphere of radius 6372795 meters.
    // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
    // Courtesy of Maarten Lamers
    float delta = radians(long1-long2);
    float sdlong = sin(delta);
    float cdlong = cos(delta);
    lat1 = radians(lat1);
    lat2 = radians(lat2);
    float slat1 = sin(lat1);
    float clat1 = cos(lat1);
    float slat2 = sin(lat2);
    float clat2 = cos(lat2);
    delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
    delta = sq(delta);
    delta += sq(clat2 * sdlong);
    delta = sqrt(delta);
    float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
    delta = atan2(delta, denom);
    return delta * 6372795;
}

float TinyGPS::course_to (float lat1, float long1, float lat2, float long2)
{
    // returns course in degrees (North=0, West=270) from position 1 to position 2,
    // both specified as signed decimal-degrees latitude and longitude.
    // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
    // Courtesy of Maarten Lamers
    float dlon = radians(long2-long1);
    lat1 = radians(lat1);
    lat2 = radians(lat2);
    float a1 = sin(dlon) * cos(lat2);
    float a2 = sin(lat1) * cos(lat2) * cos(dlon);
    a2 = cos(lat1) * sin(lat2) - a2;
    a2 = atan2(a1, a2);
    if (a2 < 0.0)
    {
        a2 += TWO_PI;
    }
    return degrees(a2);
}

const char *TinyGPS::cardinal (float course)
{
    static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};

    int direction = (int)((course + 11.25f) / 22.5f);
    return directions[direction % 16];
}

// lat/long in MILLIONTHs of a degree and age of fix in milliseconds
// (note: versions 12 and earlier gave this value in 100,000ths of a degree.
void TinyGPS::get_position(long *latitude, long *longitude, unsigned long *fix_age)
{
    if (latitude) *latitude = m_latitude;
    if (longitude) *longitude = m_longitude;
//    if (fix_age) *fix_age = m_last_position_fix == GPS_INVALID_FIX_TIME ?
//                GPS_INVALID_AGE : millis() - m_last_position_fix;
}

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void TinyGPS::get_datetime(unsigned long *date, unsigned long *time, unsigned long *age)
{
    if (date) *date = m_date;
    if (time) *time = m_time;
//    if (age) *age = m_last_time_fix == GPS_INVALID_FIX_TIME ?
//                GPS_INVALID_AGE : millis() - m_last_time_fix;
}

void TinyGPS::f_get_position(float *latitude, float *longitude, unsigned long *fix_age)
{
    long lat, lon;
    get_position(&lat, &lon, fix_age);
    *latitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lat / 1000000.0);
    *longitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lon / 1000000.0);
}

void TinyGPS::crack_datetime(int *year, quint8 *month, quint8 *day,
                             quint8 *hour, quint8 *minute, quint8 *second, quint8 *hundredths, unsigned long *age)
{
    unsigned long date, time;
    get_datetime(&date, &time, age);
    if (year)
    {
        *year = date % 100;
        *year += *year > 80 ? 1900 : 2000;
    }
    if (month) *month = (date / 100) % 100;
    if (day) *day = date / 10000;
    if (hour) *hour = time / 1000000;
    if (minute) *minute = (time / 10000) % 100;
    if (second) *second = (time / 100) % 100;
    if (hundredths) *hundredths = time % 100;
}

float TinyGPS::f_altitude()
{
    return m_altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : m_altitude / 100.0;
}

float TinyGPS::f_course()
{
    return m_course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : m_course / 100.0;
}

float TinyGPS::f_speed_knots()
{
    return m_speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : m_speed / 100.0;
}

float TinyGPS::f_speed_mph()
{
    float sk = f_speed_knots();
    return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : GPS_MPH_PER_KNOT * sk;
}

float TinyGPS::f_speed_mps()
{
    float sk = f_speed_knots();
    return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : GPS_MPS_PER_KNOT * sk;
}

float TinyGPS::f_speed_kmph()
{
    float sk = f_speed_knots();
    return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : GPS_KMPH_PER_KNOT * sk;
}

const float TinyGPS::GPS_INVALID_F_ANGLE = 1000.0;
const float TinyGPS::GPS_INVALID_F_ALTITUDE = 1000000.0;
const float TinyGPS::GPS_INVALID_F_SPEED = -1.0;
