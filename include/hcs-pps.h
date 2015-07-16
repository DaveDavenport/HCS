#ifndef __HCS_PPS_H__
#define __HCS_PPS_H__

class PPS11360 : public PSU
{
public:
    static bool check_supported_type ( const char *vendor_id, const char *product_id );

    PPS11360();

    bool get_state () throw ( PSUError & );

    void state_enable ( void ) throw ( PSUError & );
    void state_disable ( void ) throw ( PSUError & );
    float get_voltage_actual () throw( PSUError & );
    float get_current_actual () throw( PSUError & );
    void print_device_info ( void ) throw ( PSUError & );
    void set_voltage ( float value )  throw ( PSUError & );
    void set_current ( float value )  throw ( PSUError & );
    float get_voltage () throw ( PSUError & );
    float get_current () throw ( PSUError & );


private:
    void init ();
    void uninitialize ();
    void get_voltage_current ( float &voltage, float &current );

    void send_cmd ( const char *command, const char *arg );

    size_t read_cmd ( char *buffer, size_t max_length );
};
#endif // __HCS_PPS_H__
