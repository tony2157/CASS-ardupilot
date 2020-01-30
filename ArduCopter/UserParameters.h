#pragma once

#include <AP_Param/AP_Param.h>

class UserParameters {

public:
    UserParameters() {}
    static const struct AP_Param::GroupInfo var_info[];
    
    // Put accessors to your parameter variables here
    // UserCode usage example: g2.user_parameters.get_int8Param()
    AP_Int8 get_int8Param() const { return _int8; }
    AP_Int16 get_int16Param() const { return _int16; }
    AP_Float get_floatParam() const { return _float; }

    //CASS custom parameters accessors
    AP_Int32 get_user_sensor1() const{return _sensor1; }
    AP_Int32 get_user_sensor2() const{return _sensor2; }
    AP_Int32 get_user_sensor3() const{return _sensor3; }
    AP_Int32 get_user_sensor4() const{return _sensor4; }
    AP_Int32 get_user_sensor5() const{return _sensor5; }
    AP_Int32 get_user_sensor6() const{return _sensor6; }
    AP_Int32 get_user_sensor7() const{return _sensor7; }
    AP_Int32 get_user_sensor8() const{return _sensor8; }
    
private:
    // Put your parameter variable definitions here
    AP_Int8 _int8;
    AP_Int16 _int16;
    AP_Float _float;

    // CASS custom sensor parameters
    AP_Int32 _sensor1;
    AP_Int32 _sensor2;
    AP_Int32 _sensor3;
    AP_Int32 _sensor4;
    AP_Int32 _sensor5;
    AP_Int32 _sensor6;
    AP_Int32 _sensor7;
    AP_Int32 _sensor8;
};
