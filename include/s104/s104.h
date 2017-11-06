#ifndef S104_H_INCLUDED
#define S104_H_INCLUDED

#include <iostream>
#include <string>
#include <sys/time.h>

using namespace std;

namespace S104
{
enum TYPEID
{
  SINGLE_POINT_INFORMATION = 1,
  SINGLE_POINT_INFORMATION_WITH_CP24TIME2A = 2,
  DOUBLE_POINT_INFORMATION = 3,
  DOUBLE_POINT_INFORMATION_WITH_CP24TIME2A = 4,
  STEP_POSITION_INFORMATION = 5,
  STEP_POSITION_INFORMATION_WITH_CP24TIME2A = 6,
  BITSTRING_OF_32_BITS = 7,
  BITSTRING_OF_32_BITSS_WITH_CP24TIME2A = 8,
  NORMALIZED_VALUE = 9,
  NORMALIZED_VALUE_WITH_CP24TIME2A = 10,
  SCALED_VALUE = 11,
  SCALED_VALUE_WITH_CP24TIME2A = 12,
  SHORT_FLOATING_POINT_NUMBER = 13,
  SHORT_FLOATING_POINT_NUMBER_WITH_CP24TIME2A = 14,
  INTEGRATED_TOTALS = 15,
  INTEGRATED_TOTALS_WITH_CP24TIME2A = 16,
  SINGLE_POINT_INFORMATION_WITH_CP56TIME2A = 30,
  DOUBLE_POINT_INFORMATION_WITH_CP56TIME2A = 31,
  STEP_POSITION_INFORMATION_WITH_CP56TIME2A = 32,
  BITSTRING_OF_32_BITS_WITH_CP56TIME2A = 33,
  NORMALIZED_VALUE_WITH_CP56TIME2A = 34,
  SCALED_VALUE_WITH_CP56TIME2A = 35,
  SHORT_FLOATING_NUMBER_WITH_CP56TIME2A = 36,
  INTEGRATED_TOTALS_WITH_CP56TIME2A = 37,
  SINGLE_COMMAND = 45,
  DOUBLE_COMMAND = 46,
  REGULATING_STEP_COMMAND = 47,
  SETPOINT_COMMAND_NORMALIZED_VALUE = 48,
  SETPOINT_COMMAND_SCALED_VALUE = 49,
  SETPOINT_COMMAND_SHORT_FLOATING_NUMBER = 50,
  BITSTRING_OF_32_BITS_COMMAND = 51,
  END_OF_INITIALIZATION = 52,
  INTERROGATION_COMMAND = 100,
  CLOCK_SYNCHRONIZATION_COMMAND = 103,
  RESET_PROCESS_COMMAND = 105
};

enum SEQUENCE
{
  oSQ = 0x00,
  SQ = 0x80
};

enum COT
{
  PERIODIC = 0x01,
  BACKGROUND_SCAN = 0x02,
  SPONTANEOUS = 0x03,
  INITIALIZED = 0x04,
  REQUEST, REQUESTED = 0x05,
  ACTIVATION = 0x06,
  ACTIVATION_CONFIRM = 0x07,
  DEACTIVATION = 0x08,
  DEACTIVATION_CONFIRM = 0x09,
  ACTIVATION_TERMINATION = 0x0A,
  INTERROGATED_BY_STATION_INTERROGATION = 0x14,
  UNKNOWN_TYPE_INDENTIFICATION = 0x2C,
  UNKNOWN_CAUSE_OF_TRANSMISSION = 0x2D,
  UNKNOWN_INFORMATION_OBJECT_ADDRESS = 0x2E,
  NEGATIVE_ACTIVATION_CONFIRM = 0x47,
  NEGATIVE_DEACTIVATION_CONFIRM = 0x49
};

enum PN
{
  POSITIVE = 0x00,
  NEGATIVE = 0x40
};

enum SE
{
  EXECUTE = 0x00,
  SELECT = 0x80
};

enum FRAME{
    SUPERVISORY = 0x01,
    STARTDT_act = 0x07,
    STARTDT_con = 0x0B,
    STOPDT_act = 0x13,
    STOPDT_con = 0x23,
    TESTFR_act = 0x43,
    TESTFR_con = 0x83
};

enum CTRL_CONFIRM{
    SINGLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM = 1,
    SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM = 2,
    SINGLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM = 3,

    DOUBLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM = 4,
    DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM = 5,
    DOUBLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM = 6,

    REGULATING_STEP_COMMAND_POSITIVE_ACTIVATION_CONFIRM = 7,
    REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM = 8,
    REGULATING_STEP_COMMAND_POSITIVE_DEACTIVATION_CONFIRM = 9,

    SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM = 10,
    SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM = 11,
    SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_DEACTIVATION_CONFIRM = 12,

    SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_ACTIVATION_CONFIRM = 13,
    SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM = 14,
    SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_DEACTIVATION_CONFIRM = 15,

    SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_ACTIVATION_CONFIRM = 16,
    SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM = 17,
    SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_DEACTIVATION_CONFIRM = 18
};

class infItem
{
    public:

        infItem();
        ~infItem();

    public:
        int address;
        TYPEID typeID;
        float a;
        float b;
        char value[4];
        char quality;
        timeval time;

    public:
        int update(string _value, char _quality);
        int update(string _value, char _quality, timeval _time);
};

class ctlItem
{
    public:
        ctlItem();
        ~ctlItem();
        ctlItem(const ctlItem& obj);
        ctlItem& operator=(const ctlItem& obj);

    public:
        string ref1, ref2;
        int address;
        char typeID;
        char value[4];
        bool sbo;
};

class SPI : public infItem
{
    public:
        SPI();
        ~SPI();
        SPI(const SPI& obj);
        SPI& operator=(const SPI& obj);
        SPI(int _address);
};

class DPI: public infItem
{
    public:
        DPI();
        ~DPI();
        DPI(const DPI& obj);
        DPI& operator=(const DPI& obj);
        DPI(int _address);
        DPI(int _address, bool _mk);
};

class VTI: public infItem
{
    public:
        VTI();
        ~VTI();
        VTI(const VTI& obj);
        VTI& operator=(const VTI& obj);
        VTI(int _address);
};

class NVA : public infItem
{
    public:
        NVA();
        ~NVA();
        NVA(const NVA& obj);
        NVA& operator=(const NVA& obj);
        NVA(int _address);
};

class SVA : public infItem
{
    public:
        SVA();
        ~SVA();
        SVA(const SVA& obj);
        SVA& operator=(const SVA& obj);
        SVA(int _address);
};

class FP : public infItem
{
    public:
        FP();
        ~FP();
        FP(const FP& obj);
        FP& operator=(const FP& obj);
        FP(int _address);
        FP(int _address, float _a, float _b);
};

class SCO : public ctlItem
{
    public:
        SCO();
        ~SCO();
        SCO(string _ref, int _address);
        SCO(string _ref, int _address, string _sbo);
};

class DCO : public ctlItem
{
    public:
        DCO();
        ~DCO();
        DCO(string _ref, int _address);
        DCO(string _ref1, string ref2, int _address);
        DCO(string _ref, int _address, string _sbo);
        DCO(string _ref1, string _ref2, int _address, string _sbo);
};

class RCO : public ctlItem
{
    public:
        RCO();
        ~RCO();
        RCO(string _ref, int _address);
        RCO(string _ref1, string _ref2, int _address);
        RCO(string _ref, int _address, string _sbo);
        RCO(string _ref1, string _ref2, int _address, string _sbo);
};

class NVO : public ctlItem
{
    public:
        NVO();
        ~NVO();
        NVO(string _ref, int _address);
        NVO(string _ref, int _address, string _sbo);
};

class SVO : public ctlItem
{
    public:
        SVO();
        ~SVO();
        SVO(string _ref, int _address);
        SVO(string _ref, int _address, string _sbo);
};

class FPO : public ctlItem
{
    public:
        FPO();
        ~FPO();
        FPO(string _ref, int _address);
        FPO(string _ref, int _address, string _sbo);
};

}

#endif // S104_H_INCLUDED
