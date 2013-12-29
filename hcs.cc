/**
 *    This file is part of HCS.
 *    Written by Qball Cow <qball@gmpclient.org> 2013
 *
 *    HCS is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License.
 *
 *    HCS is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with HCS.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <string>
#include <errno.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyUSB0"

#define _POSIX_SOURCE 1

#define FALSE 0
#define TRUE 1


class HCS
{
    private:
        int fd = 0;
        struct termios oldtio;


        void send_cmd (const char *command, const char *arg) {
            // Check command.
            if(command == nullptr ) return;
            // Write command to str. 
            write(fd, command, strlen(command));
            if(arg != nullptr ) {
                // Send argument.
                write(fd, arg, strlen(arg));
            }
            // End line
            write(fd, "\r" , 1);
        }

        size_t read_cmd (char *buffer, size_t max_length)
        {
            size_t size = 0;
            while(size < 3 ||
                    !(
                        buffer[size-3] == 'O' &&
                        buffer[size-2] == 'K' &&
                        buffer[size-1] == '\n'
                     )
                 )
            {
                ssize_t v = read(fd, &buffer[size], max_length-size);
                buffer[size+1] = '\0';
                if(buffer[size] == '\r') buffer[size] = '\n';
                if(v > 0){
                    size+=v;
                    if(size+1 >= max_length) {
                        return -1;
                    }
                }else {
                    printf("%i\n", errno);
                    printf("%s\n", strerror(errno));
                    return -1;
                }
            }
            return size;
        }


        void get_status ( void )
        {
            char buffer[1024];
            // Send getd mesg.
            this->send_cmd("GETD",NULL);

            // Get
            if( this->read_cmd(buffer, 1024) > 0 )
            {
                std::string b = buffer;
                double voltage = strtol(b.substr(0,3).c_str(), 0, 10)/10.0;
                double status = strtol(b.substr(4,7).c_str(), 0, 10)/1000.0;
                int limited = strtol(b.substr(8,8).c_str(), 0, 10);

                printf("Volt: %.2f V\n",voltage);
                printf("Curr: %.2f A\n",status);
                printf("V*C:  %.2f VA\n",voltage*status);
                printf("Lim:  %s\n",   (limited == 0)?"Voltage":"Current");
            }
        }

        void set_on ( void )
        {
            char buffer[1024];
            this->send_cmd("SOUT", "0");
            this->read_cmd(buffer, 1024);
        }
        void set_off ( void )
        {
            char buffer[1024];
            this->send_cmd("SOUT", "1");
            this->read_cmd(buffer, 1024);
        }

        void set_volt ( float value )
        {
            char buffer[1024];
            snprintf(buffer,1024,"%03d", (int)(value*10));
            this->send_cmd("VOLT", buffer);
            this->read_cmd(buffer, 1024);
        }
        void set_current ( float value )
        {
            char buffer[1024];
            snprintf(buffer,1024,"%03d", (int)(value*100));
            this->send_cmd("CURR", buffer);
            this->read_cmd(buffer, 1024);
        }

        void get_voltage_current ( double &voltage, double &current )
        {
            char buffer[1024];
            this->send_cmd("GETS", buffer);
            voltage = current = -1.0;
            if( this->read_cmd(buffer, 1024) > 5 )
            {
                std::string b = buffer;
                voltage = strtol(b.substr(0,3).c_str(), 0, 10)/10.0;
                current= strtol(b.substr(3,6).c_str(), 0, 10)/100.0;
            }
        }

    public:
        HCS()
        {
            fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );

            if(fd < 0) {
                printf("failed: %s\n", strerror(errno));
                // TODO: Error checker.
                exit(-1);
            }
            // save status port settings
            tcgetattr(fd,&oldtio);

            struct termios newtio;

            newtio.c_cflag = BAUDRATE | CS8;
            newtio.c_iflag = 0;
            newtio.c_oflag = 0;
            newtio.c_lflag = 0;       //ICANON;
            newtio.c_cc[VMIN]=1;
            newtio.c_cc[VTIME]=0;
            tcflush(fd, TCIFLUSH);
            tcsetattr(fd,TCSANOW,&newtio);
        }

        ~HCS()
        {
            if(fd != 0) {
                // Restore
                tcsetattr(fd, TCSANOW, &oldtio);
                close(fd);
                fd=0;
            }
        }


        int run ( int argc, char **argv )
        {
            for ( int i=0; i < argc; i++ )
            {
                const char *command = argv[i];

                if ( strncmp(command, "status", 6) == 0 )
                {
                    this->get_status();
                }
                else if ( strncmp(command, "on", 2) == 0 )
                {
                    this->set_on();
                }
                else if ( strncmp(command, "off", 3) == 0 )
                {
                    this->set_off();
                }
                else if ( strncmp(command, "voltage", 7) == 0 )
                {
                    if( argc > (i+1) ) {
                        // write
                        const char *value = argv[++i];
                        float volt = strtof(value, nullptr);
                        this->set_volt(volt);
                    }else {
                        double voltage, current;
                        // read
                        this->get_voltage_current(voltage, current);
                        printf("%.2f\n", voltage);
                    }
                }
                else if ( strncmp(command, "current", 7) == 0 )
                {
                    if( argc > (i+1) ) {
                        // write
                        const char *value = argv[++i];
                        float current = strtof(value, nullptr);
                        this->set_current(current);
                    }else {
                        double voltage, current;
                        // read
                        this->get_voltage_current(voltage, current);
                        printf("%.2f\n", current);
                    }
                }

            }

            return EXIT_FAILURE;
        }


};

int main ( int argc, char **argv )
{
    HCS hcs;
    return hcs.run(argc, argv);
}
