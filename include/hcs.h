#ifndef __HCS_H__
#define __HCS_H__

/***
 * DEFAULTS
 */
#define BAUDRATE       B9600
#define MODEMDEVICE    "/dev/ttyUSB0"

#define FALSE          0
#define TRUE           1

class PSUError : public std::exception
{
public:
    PSUError( const char* errMessage ) : errMessage_ ( errMessage )
    {
    }
    PSUError( const std::string errMessage ) : errMessage_ ( errMessage.c_str () )
    {
    }

    // overriden what() method from exception class
    const char* what () const throw( )
    {
        return errMessage_;
    }

private:
    const char* errMessage_;
};

/**
 * Base class a PSU implementation should inherit from.
 */
class PSU
{
private:
    struct termios oldtio;
protected:
    int            fd = -1;
    // Baudrate, set when needed.
    int            baudrate = B9600;

    PSU( int baudrate ) : baudrate ( baudrate )
    {
    }

    virtual void init ()         = 0;
    virtual void uninitialize () = 0;

public:
    /**
     * List of supported power supplies.
     */
    enum class PSUTypes
    {
    EAPS2K,
    PPS11360
    };

    enum class OperatingMode
    {
    OFF,
    CV,
    CC
    };
    const char *OperatingModeStr[3] = {
        "Off",
        "CV",
        "CC"
    };
    const char *get_mode_str ( OperatingMode type )
    {
        return OperatingModeStr[static_cast<int>( type )];
    }

    virtual ~PSU()
    {
        if ( fd >= 0 ) {
            close_device ();
        }
    }
    /**
     * Open a connection to the device.
     * Based on hostname, or hard coded default.
     */
    virtual void open_device ();

    bool is_open () noexcept
    {
        return fd >= 0;
    }
    /**
     * @param dev_node The device node to open.
     *
     * Open a connection to the device.
     */
    virtual void open_device ( const char *dev_node ) throw ( PSUError & );

    /**
     * Close connection to the device.
     */
    virtual void close_device ();

    /**
     * Get the set output voltage.
     */
    virtual float get_voltage () throw( PSUError & ) = 0;
    /**
     * Get the set current limit.
     */
    virtual float get_current () throw( PSUError & ) = 0;

    /**
     * @param value the output voltage (in Volts)
     * Set the output voltage.
     */
    virtual void set_voltage ( const float value ) throw( PSUError & ) = 0;

    /**
     * @param value
     *
     * Set the over voltage protection level.
     */
    virtual void set_over_voltage ( const float value ) throw( PSUError & )
    {
        throw PSUError ( "Current feature is not supported for this power supply" );
    }
    /**
     * @param value
     *
     * Set the over current protection level.
     */
    virtual void set_over_current ( const float value ) throw( PSUError & )
    {
        throw PSUError ( "Current feature is not supported for this power supply" );
    }
    /**
     * Get the over voltage protection level.
     * @returns the current over voltage protection level.
     */
    virtual float get_over_voltage ( ) throw( PSUError & )
    {
        throw PSUError ( "Current feature is not supported for this power supply" );
    }
    /**
     * Get the over current protection level.
     * @returns the current over current protection level.
     */
    virtual float get_over_current ( ) throw( PSUError & )
    {
        throw PSUError ( "Current feature is not supported for this power supply" );
    }
    /**
     * Get the current operating mode. (Voltage controller or Current Controlled).
     */
    virtual OperatingMode get_operating_mode () throw ( PSUError & )
    {
        throw PSUError ( "Current feature is not supported for this power supply" );
    }

    /**
     * @param value the output current (in Amps)
     * Set the output current.
     */
    virtual void set_current ( const float value ) throw( PSUError & ) = 0;

    /**
     * Get the actual output voltage.
     *
     * @returns the output voltage in Volts.
     */
    virtual float get_voltage_actual () throw( PSUError & ) = 0;

    /**
     * Get the actual output current.
     *
     * @returns the output current in Amps.
     */
    virtual float get_current_actual () throw( PSUError & ) = 0;

    /**
     * Enable output.
     *
     * Enable the output of the power supply.
     */
    virtual void state_enable () throw( PSUError & ) = 0;

    /**
     * Disable output.
     *
     * Disable the output of the power supply.
     */
    virtual void state_disable () throw( PSUError & ) = 0;

    /**
     * Get output state
     *
     * Get the output state of the power supply.
     *
     * @returns true when the output is enable, false otherwise.
     */
    virtual bool get_state () throw( PSUError & ) = 0;

    /**
     * Print device information.
     *
     * Print out information about the device.
     */
    virtual void print_device_info () throw( PSUError & ) = 0;
};

#endif
