#include "Copter.h"
#include <utility>
#include <SRV_Channel/SRV_Channel.h>
#include <Filter/LowPassFilter2p.h>

//Humidity sensor Params
const int N_RH = 4;     //supports up to 4

//IMET sensor Params
const int N_imet = 4;   //supports up to 4

//Fan control params
//It will run the fan at SERVO_MAX, which can be set in the params list
uint16_t fan_pwm_on = 100; // 40
uint16_t fan_pwm_off = 0;
bool _fan_status;

//Wind estimator Params
float wsA = 32.8;               //Coefficient A of the linear wind speed equation, from calibration
float wsB = -4.5;               //Coefficient B of the linear wind speed equation, from calibration
//const int N = 60;               //filter order
float _wind_speed, _wind_dir;
float wind_x, wind_y;
float ang_rot;                  //UAS tilt about axis of rotation
//float var_temp_dir, var_temp_gamma;
//float _roll_sum, _pitch_sum, avgR, avgP;
//float mean_aux_dir[2],var_aux_dir[2];
//float mean_aux_speed[2],var_aux_speed[2];
uint8_t k;                      //Number of measured wind dir/speed
LowPassFilter2pFloat wind_x_filter;
LowPassFilter2pFloat wind_y_filter;
LowPassFilter2pFloat rot_trace_filter;

#ifdef USERHOOK_INIT
void Copter::userhook_init()
{
    // put your initialisation code here
    // this will be called once at start-up

    //Initialize Wind estimator
    //_wind_dir = 0.0f; _roll = 0.0f; _pitch = 0.0f; _yaw = 0.0f;
    //_roll_sum = 0.0f; _pitch_sum = 0.0f;
    //avgR = 0.0; avgP = 0.0;
    //var_temp_dir = 0.0f; var_temp_gamma = 0.0f;
    _wind_dir = 0.0f;  _wind_speed = 0.0f; k = 0;
    //k = 0;
    //memset(mean_aux_dir,0.0f,sizeof(mean_aux_dir));
    //memset(var_aux_dir,1000.0f,sizeof(var_aux_dir));
    //memset(mean_aux_speed,0.0f,sizeof(mean_aux_speed));
    //memset(var_aux_speed,1000.0f,sizeof(var_aux_speed));

    //Wind filter initialization
    wind_x_filter.set_cutoff_frequency(10,1);
    wind_y_filter.set_cutoff_frequency(10,1);
    rot_trace_filter.set_cutoff_frequency(10,1);

    // Initialize Fan Control
    SRV_Channels::set_output_scaled(SRV_Channel::k_egg_drop, fan_pwm_off);
    _fan_status = false;
}
#endif

#ifdef USERHOOK_FASTLOOP
void Copter::userhook_FastLoop()
{
    // put your 100Hz code here
}
#endif

#ifdef USERHOOK_50HZLOOP
void Copter::userhook_50Hz()
{
    // put your 50Hz code here
}
#endif

#ifdef USERHOOK_MEDIUMLOOP
void Copter::userhook_MediumLoop()
{
    // put your 10Hz code here
    #if CONFIG_HAL_BOARD != HAL_BOARD_SITL
    // Read Temperature, Resistance and Health. Write sensors packet into the SD card
    struct log_IMET pkt_temp = {
        LOG_PACKET_HEADER_INIT(LOG_IMET_MSG),
        time_stamp             : AP_HAL::micros64(),                //Store time in microseconds
        fan_status             : _fan_status,                       //Store Fan on/off status
        healthy1               : copter.CASS_Imet[0].healthy(),     //Store sensor health
        healthy2               : copter.CASS_Imet[1].healthy(),
        healthy3               : copter.CASS_Imet[2].healthy(),
        healthy4               : copter.CASS_Imet[3].healthy(),
        temperature1           : copter.CASS_Imet[0].temperature(), //Store iMet Temperature
        temperature2           : copter.CASS_Imet[1].temperature(),
        temperature3           : copter.CASS_Imet[2].temperature(),
        temperature4           : copter.CASS_Imet[3].temperature(),
        resist1                : copter.CASS_Imet[0].resistance(),  //Store iMet bead resistence
        resist2                : copter.CASS_Imet[1].resistance(),
        resist3                : copter.CASS_Imet[2].resistance(),
        resist4                : copter.CASS_Imet[3].resistance()
    }; 
    #else
        uint32_t m = AP_HAL::millis();
        float alt; float simT;
        copter.ahrs.get_relative_position_D_home(alt);
        alt = -1*alt;  
        if(alt < 50){
            simT = 300 - 2*alt*alt/2500;
        }
        else if(alt < 150){
            simT = 298 - 0.01*alt;
        }
        else if(alt < 200){
            simT = 296 + 2*alt*alt/40000;
        }
        else{
            simT = 298;
        }
        // Write simulated sensors packet into the SD card
        struct log_IMET pkt_temp = {
            LOG_PACKET_HEADER_INIT(LOG_IMET_MSG),
            time_stamp             : AP_HAL::micros64(),
            fan_status             : _fan_status,
            healthy1               : copter.CASS_Imet[0].healthy(),
            healthy2               : copter.CASS_Imet[1].healthy(),
            healthy3               : copter.CASS_Imet[2].healthy(),
            healthy4               : copter.CASS_Imet[3].healthy(),
            temperature1           : simT + sinf(0.0003f*m) * 0.001f,
            temperature2           : simT + sinf(0.0004f*m) * 0.001f,
            temperature3           : simT + sinf(0.0005f*m) * 0.001f,
            temperature4           : simT + sinf(0.0006f*m) * 0.001f,
            resist1                : copter.CASS_Imet[0].resistance(),
            resist2                : copter.CASS_Imet[1].resistance(),
            resist3                : copter.CASS_Imet[2].resistance(),
            resist4                : copter.CASS_Imet[3].resistance()
        };
    #endif
    copter.DataFlash.WriteBlock(&pkt_temp, sizeof(pkt_temp));   //Send package to SD card
}
#endif

#ifdef USERHOOK_SLOWLOOP
void Copter::userhook_SlowLoop()
{
     #if CONFIG_HAL_BOARD != HAL_BOARD_SITL
        // Read Rel. Humidity, Temperature and Health. Write sensors packet into the SD card
        struct log_RH pkt_RH = {
            LOG_PACKET_HEADER_INIT(LOG_RH_MSG),
            time_stamp             : AP_HAL::micros64(),                        //Store time in microseconds
            healthy1               : copter.CASS_HYT271[0].healthy(),           //Store senors health
            healthy2               : copter.CASS_HYT271[1].healthy(),
            healthy3               : copter.CASS_HYT271[2].healthy(),
            healthy4               : copter.CASS_HYT271[3].healthy(),
            humidity1              : copter.CASS_HYT271[0].relative_humidity(), //Store Rel. humidity
            humidity2              : copter.CASS_HYT271[1].relative_humidity(),
            humidity3              : copter.CASS_HYT271[2].relative_humidity(),
            humidity4              : copter.CASS_HYT271[3].relative_humidity(),
            RHtemp1                : copter.CASS_HYT271[0].temperature(),       //Store temperature
            RHtemp2                : copter.CASS_HYT271[1].temperature(),
            RHtemp3                : copter.CASS_HYT271[2].temperature(),
            RHtemp4                : copter.CASS_HYT271[3].temperature()
        };
    #else
        uint32_t m = AP_HAL::millis();
        // Write sensors packet into the SD card
        struct log_RH pkt_RH = {
            LOG_PACKET_HEADER_INIT(LOG_RH_MSG),
            time_stamp             : AP_HAL::micros64(),
            healthy1               : copter.CASS_HYT271[0].healthy(),
            healthy2               : copter.CASS_HYT271[1].healthy(),
            healthy3               : copter.CASS_HYT271[2].healthy(),
            healthy4               : copter.CASS_HYT271[3].healthy(),
            humidity1              : (float)fabs(sinf(0.0003f*m) * 60.0f),
            humidity2              : (float)fabs(sinf(0.0004f*m) * 60.0f),
            humidity3              : (float)fabs(sinf(0.0005f*m) * 60.0f),
            humidity4              : (float)fabs(sinf(0.0006f*m) * 60.0f),
            RHtemp1                : 298.15f + sinf(0.0003f*m) * 2.0f,
            RHtemp2                : 298.15f + sinf(0.0004f*m) * 2.0f,
            RHtemp3                : 298.15f + sinf(0.0005f*m) * 2.0f,
            RHtemp4                : 298.15f + sinf(0.0006f*m) * 2.0f
        };
    #endif
    copter.DataFlash.WriteBlock(&pkt_RH, sizeof(pkt_RH));   //Send package to SD card
}
#endif

#ifdef USERHOOK_SUPERSLOWLOOP
void Copter::userhook_SuperSlowLoop()
{
    //float var_gamma = 0.0f, var_wind_dir = 0.0f;
    //float body_wind_dir = 0.0f;
    float speed_xy = 0.0f, dist_to_wp = 0.0f;
    Vector3f e_angles, vel_xyz;
    float alt;

    //copter.EKF2.getHAGL(alt);
    //alt = copter.inertial_nav.get_altitude();
    //copter.ahrs.get_hagl(alt);
    copter.ahrs.get_relative_position_D_home(alt);
    alt = -100.0f*alt;           // get AGL altitude in cm
    //Vector3f _gyro_rate = copter.ins.get_gyro(0);
    //float _yaw_rate = _gyro_rate[2];
    //printf("Alt: %5.2f \n",alt);

    //Start estimation after Copter takes off
    if(!ap.land_complete && copter.position_ok()){ // !arming.is_armed(), !ap.land_complete, motors->armed()

        //Fan Control    
        if(alt > 185.0f && SRV_Channels::get_output_scaled(SRV_Channel::k_egg_drop) < 50){
            SRV_Channels::set_output_scaled(SRV_Channel::k_egg_drop, fan_pwm_on);
            _fan_status = true;
            #if CONFIG_HAL_BOARD == HAL_BOARD_SITL
                printf("FAN ON \n");  
            #endif
        }
        else{
            if(alt < 140.0f && SRV_Channels::get_output_scaled(SRV_Channel::k_egg_drop) > 50){
                SRV_Channels::set_output_scaled(SRV_Channel::k_egg_drop, fan_pwm_off);
                _fan_status = false;
                #if CONFIG_HAL_BOARD == HAL_BOARD_SITL
                    printf("FAN OFF \n");  
                #endif
            }
        }
        //uint16_t pwm;
        //SRV_Channels::get_output_pwm(SRV_Channel::k_egg_drop, pwm);
        //printf("PWM: %5.2f \n",(float)pwm);

        //Wind Estimator Algorithm
        //Current Attitude of the UAS
        const Matrix3f &rotMat = copter.ahrs.get_rotation_body_to_ned(); // rotMat.a.x ... rotMat.c.z
        float rot_trace = rotMat.a.x + rotMat.b.y + rotMat.c.z;
        wind_x = -1*rotMat.a.z;
        wind_y = -1*rotMat.b.z;
        float filtered_wind_x = wind_x_filter.apply(wind_x);
        float filtered_wind_y = wind_y_filter.apply(wind_y);
        //float filtered_rot_trace = rot_trace_filter.apply(rot_trace);
        _wind_dir = fabs(fmod(atan2f(filtered_wind_y,filtered_wind_x),2*M_PI));
        ang_rot = acosf((rot_trace-1)/2);
        //_wind_speed = wsA * sqrtf(tanf(ang_rot)) + wsB;

        //Estimated horizontal velocity calculated by the EKF2
        //copter.EKF2.getVelNED(-1,vel_xyz);
        vel_xyz = copter.inertial_nav.get_velocity();
        speed_xy = norm(vel_xyz.x,vel_xyz.y); // cm/s
        dist_to_wp = copter.wp_nav->get_wp_distance_to_destination(); // cm (horizontally)

        if(alt > 400.0f){
            if(speed_xy < 120.0f && dist_to_wp < 500){
                    _wind_dir = wrap_360_cd(_wind_dir*5729.6f);
                    // Send wind direction to the Flight control 
                    if(alt>4.0f && ang_rot>0.05f){
                        copter.cass_wind_direction = _wind_dir;
                    }
                    else{
                        // If wind speeds are very slow or none at all (TODO: use wind speed estimation)
                        copter.cass_wind_direction = copter.wp_nav->get_yaw();
                    }
            }
        }
    }
    else{
        //copter.wp_nav->turn_into_wind_heading = (float)copter.initial_armed_bearing;
        copter.cass_wind_direction = (float)copter.initial_armed_bearing;
        copter.cass_wind_speed = 0.0;
        SRV_Channels::set_output_scaled(SRV_Channel::k_egg_drop, fan_pwm_off);
        _fan_status = false;
    }

    // Write wind direction packet into the SD card
    struct log_WIND pkt_wind_est = {
        LOG_PACKET_HEADER_INIT(LOG_WIND_MSG),
        time_stamp             : AP_HAL::micros64(),// - _last_read_ms),
        _wind_dir              : _wind_dir/100,
        _wind_speed            : 0,
        _wind_x                : wind_x,
        _wind_y                : wind_y,
        _ang_rot               : ang_rot,
        _pitch_sum             : 0,
        _yaw                   : 0
    };
    copter.DataFlash.WriteBlock(&pkt_wind_est, sizeof(pkt_wind_est));

    // float data[5] = {0};
    // data[0] = _wind_dir/100.0f;
    // data[1] = _wind_speed;
    // data[2] = Vel_xy;
    // data[3] = var_wind_dir;
    // data[4] = alt;
    // copter.send_cass_data(3, data, 5);
}
#endif

#ifdef USERHOOK_AUXSWITCH
void Copter::userhook_auxSwitch1(uint8_t ch_flag)
{
    // put your aux switch #1 handler here (CHx_OPT = 47)
}

void Copter::userhook_auxSwitch2(uint8_t ch_flag)
{
    // put your aux switch #2 handler here (CHx_OPT = 48)
}

void Copter::userhook_auxSwitch3(uint8_t ch_flag)
{
    // put your aux switch #3 handler here (CHx_OPT = 49)
}
#endif
