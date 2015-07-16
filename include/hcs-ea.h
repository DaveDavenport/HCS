#ifndef __HCS_EA_H__
#define __HCS_EA_H__

/**
 * Elektro Automatik Powersupply
 */
class EAPS2K : public PSU
{
private:
    const int baudrate = B115200;
    enum ErrorTypes
    {
        NO_ERROR              = 0x0,
        CRC_INVALID           = 0x3,
        DELIMITER_INVALID     = 0x4,
        OUTPUT_ADDR_INVALID   = 0x5,
        OBJECT_INVALID        = 0x7,
        OBJECT_LENGTH_INVALID = 0x8,
        ACCESS_VIOLATION      = 0x9,
        DEVICE_LOCKED         = 0x15,
        OBJECT_OVERFLOW       = 0x30,
        OBJECT_UNDERFLOW      = 0x31
    };
    struct
    {
        ErrorTypes type;
        const char *name;
    } ErrorTypeStr[10] =
    {
        { NO_ERROR,              "No error"                                   },
        { CRC_INVALID,           "Check sum incorrect"                        },
        { DELIMITER_INVALID,     "Start delimiter incorrect"                  },
        { OUTPUT_ADDR_INVALID,   "Wrong address for output"                   },
        { OBJECT_INVALID,        "Object not defined"                         },
        { OBJECT_LENGTH_INVALID, "Object length incorrect"                    },
        { ACCESS_VIOLATION,      "Read/Write permissions violated, no access" },
        { DEVICE_LOCKED,         "Device is in \"Lock\" state"                },
        { OBJECT_OVERFLOW,       "Upper limit of object exceeded"             }
    };
    // See object table manual.
    enum ObjectTypes
    {
        DEVICE_TYPE          = 0,
        DEVICE_SERIAL_NO     = 1,
        NOMINAL_VOLTAGE      = 2,
        NOMINAL_CURRENT      = 3,
        NOMINAL_POWER        = 4,
        DEVICE_ARTICLE_NO    = 6,
        MANUFACTURER         = 8,
        SOFTWARE_VERSION     = 9,
        DEVICE_CLASS         = 19,
        OVP_THRESHOLD        = 38,
        OCP_THRESHOLD        = 39,
        SET_VOLTAGE          = 50,
        SET_CURRENT          = 51,
        POWER_SUPPLY_CONTROL = 54,
        STATUS_ACTUAL        = 71,
        STATUS_SET           = 72,
    };
    // These values are needed to convert the result.
    float nominal_voltage = 1;
    float nominal_current = 1;
    float nominal_power   = 1;

    /** Telegram functions */

    // Max length is SD (1) + DN (1) + OBJ (2) + CS(2) + DATA (0-16)
    uint8_t _telegram[22]  = { 0, };
    uint8_t _telegram_size = 0;

    void telegram_validate ();
    enum SendType
    {
        SEND   =0xC0,
        RECEIVE=0x40
    };
    const int cast_type = 0x20;
    const int direction = 0x10;
    void telegram_start ( SendType dir, int size );
    void telegram_set_object ( ObjectTypes object );
    void telegram_push ( uint8_t val );
    const char *telegram_get_error ( ErrorTypes type ) const;
    void telegram_send ();
    void telegram_receive ();

    /**
     * Set the crc of the telegram.
     */
    void telegram_crc_set ();

    /**
     * Check the crc of the telegram.
     */
    bool telegram_crc_check ();

    void init ();

    void uninitialize ();

public:

    static bool check_supported_type ( const char *vendor_id, const char *product_id );
    void enable_remote () throw( PSUError & );
    void disable_remote () throw( PSUError & );
    void state_enable () throw( PSUError & );
    void state_disable () throw( PSUError & );
    bool get_state () throw( PSUError & );
    float get_current () throw( PSUError & );
    float get_voltage () throw( PSUError & );
    float get_current_actual () throw( PSUError & );

    float get_voltage_actual () throw( PSUError & );

    float get_over_voltage () throw ( PSUError & );
    float get_over_current () throw ( PSUError & );

    OperatingMode get_operating_mode () throw( PSUError & );
    void set_voltage ( float value ) throw( PSUError & );
    void set_current ( float value ) throw( PSUError & );
    void set_over_voltage ( float value ) throw( PSUError & );
    void set_over_current ( float value ) throw( PSUError & );

    void print_device_info () throw( PSUError & );

    EAPS2K();
    ~EAPS2K();
};
#endif // __HCS_EA_H__
