#ifndef TINYGPS_H
#define TINYGPS_H

#include <QObject>

#define GPS_VERSION            13 // software version of this library
#define GPS_MPH_PER_KNOT       1.15077945
#define GPS_MPS_PER_KNOT       0.51444444
#define GPS_KMPH_PER_KNOT      1.852
#define GPS_MILES_PER_METER    0.00062137112
#define GPS_KM_PER_METER       0.001

class TinyGPS : public QObject
{
    Q_OBJECT
public:
    enum {
        GPS_INVALID_AGE = 0xFFFFFFFF,
        GPS_INVALID_ANGLE = 999999999,
        GPS_INVALID_ALTITUDE = 999999999,
        GPS_INVALID_DATE = 0,
        GPS_INVALID_TIME = 0xFFFFFFFF,
        GPS_INVALID_SPEED = 999999999,
        GPS_INVALID_FIX_TIME = 0xFFFFFFFF,
        GPS_INVALID_SATELLITES = 0xFF,
        GPS_INVALID_HDOP = 0xFFFFFFFF
    };

    static const float GPS_INVALID_F_ANGLE, GPS_INVALID_F_ALTITUDE, GPS_INVALID_F_SPEED;

    explicit TinyGPS(QObject *parent = nullptr);

    bool encode(char c); // process one character received from GPS
    TinyGPS &operator << (char c) {encode(c); return *this;}

    // lat/long in MILLIONTHs of a degree and age of fix in milliseconds
    // (note: versions 12 and earlier gave lat/long in 100,000ths of a degree.
    void get_position(long *latitude, long *longitude, unsigned long *fix_age = nullptr);

    // date as ddmmyy, time as hhmmsscc, and age in milliseconds
    void get_datetime(unsigned long *date, unsigned long *time, unsigned long *age = nullptr);

    // signed altitude in centimeters (from GPGGA sentence)
    inline long altitude() { return m_altitude; }

    // course in last full GPRMC sentence in 100th of a degree
    inline unsigned long course() { return m_course; }

    // speed in last full GPRMC sentence in 100ths of a knot
    inline unsigned long speed() { return m_speed; }

    // satellites used in last full GPGGA sentence
    inline unsigned short satellites() { return m_numsats; }

    // horizontal dilution of precision in 100ths
    inline unsigned long hdop() { return m_hdop; }

    void f_get_position(float *latitude, float *longitude, unsigned long *fix_age = nullptr);
    void crack_datetime(int *year, quint8 *month, quint8 *day,
                        quint8 *hour, quint8 *minute, quint8 *second, quint8 *hundredths = nullptr, unsigned long *fix_age = nullptr);
    float f_altitude();
    float f_course();
    float f_speed_knots();
    float f_speed_mph();
    float f_speed_mps();
    float f_speed_kmph();

    static int library_version() { return GPS_VERSION; }

    static float distance_between (float lat1, float long1, float lat2, float long2);
    static float course_to (float lat1, float long1, float lat2, float long2);
    static const char *cardinal(float course);

#ifndef GPS_NO_STATS
    void stats(unsigned long *chars, unsigned short *good_sentences, unsigned short *failed_cs);
#endif


signals:

public slots:

private:
    enum {GPS_SENTENCE_GPGGA, GPS_SENTENCE_GPRMC, GPS_SENTENCE_OTHER};

    // properties
    unsigned long m_time, m_new_time;
    unsigned long m_date, m_new_date;
    long m_latitude, m_new_latitude;
    long m_longitude, m_new_longitude;
    long m_altitude, m_new_altitude;
    unsigned long  m_speed, m_new_speed;
    unsigned long  m_course, m_new_course;
    unsigned long  m_hdop, m_new_hdop;
    unsigned short m_numsats, m_new_numsats;

    unsigned long m_last_time_fix, m_new_time_fix;
    unsigned long m_last_position_fix, m_new_position_fix;

    // parsing state variables
    quint8 m_parity;
    bool m_is_checksum_term;
    char m_term[15];
    quint8 m_sentence_type;
    quint8 m_term_number;
    quint8 m_term_offset;
    bool m_gps_data_good;

#ifndef _GPS_NO_STATS
    // statistics
    unsigned long m_encoded_characters;
    unsigned short m_good_sentences;
    unsigned short m_failed_checksum;
    unsigned short m_passed_checksum;
#endif

    // internal utilities
    int from_hex(char a);
    unsigned long parse_decimal();
    unsigned long parse_degrees();
    bool term_complete();
    bool gpsisdigit(char c) { return c >= '0' && c <= '9'; }
    long gpsatol(const char *str);
    int gpsstrcmp(const char *str1, const char *str2);
};

#endif // TINYGPS_H
