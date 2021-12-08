/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>  // printf
#include <time.h>   // add timestamp to measurements
#include <string.h> // remove the newline line ending from the built in C date function using strlen

#include "sensirion_uart.h"
#include "sps30.h"
#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

int main(void) {
    struct sps30_measurement m;
    char serial[SPS30_MAX_SERIAL_LEN];
    const uint8_t AUTO_CLEAN_DAYS = 4;
    int16_t ret;

    while (sensirion_uart_open() != 0) {
        printf("UART init failed\n");
        sensirion_sleep_usec(1000000); /* sleep for 1s */
    }

    /* Busy loop for initialization, because the main loop does not work without
     * a sensor.
     */
    while (sps30_probe() != 0) {
        printf("SPS30 sensor probing failed\n");
        sensirion_sleep_usec(1000000); /* sleep for 1s */
    }
    printf("SPS30 sensor probing successful\n");

    struct sps30_version_information version_information;
    ret = sps30_read_version(&version_information);
    if (ret) {
        printf("error %d reading version information\n", ret);
    } else {
        printf("FW: %u.%u HW: %u, SHDLC: %u.%u\n",
               version_information.firmware_major,
               version_information.firmware_minor,
               version_information.hardware_revision,
               version_information.shdlc_major,
               version_information.shdlc_minor);
    }

    ret = sps30_get_serial(serial);
    if (ret)
        printf("error %d reading serial\n", ret);
    else
        printf("SPS30 Serial: %s\n", serial);

    ret = sps30_set_fan_auto_cleaning_interval_days(AUTO_CLEAN_DAYS);
    if (ret)
        printf("error %d setting the auto-clean interval\n", ret);



    //BEGIN SCD40 SETUP ----------------------------------------------
    int16_t error = 0;

    sensirion_i2c_hal_init();

    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;
    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error) {
        printf("Error executing scd4x_get_serial_number(): %i\n", error);
    } else {
        printf("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
    }

    // Start Measurement

    error = scd4x_start_periodic_measurement();
    if (error) {
        printf("Error executing scd4x_start_periodic_measurement(): %i\n",
               error);
    }


    //Add headers to SPS30 CSV file. (Make sure a CSV file is not already present as the header will be added to the exisitg CSV otherwise)
    FILE *SPS30csv;
    SPS30csv = fopen("../Outputs/PMoutput.csv", "a");
    fprintf(SPS30csv,"Timestamp, PM1.0, PM2.5, PM4.0, PM10.0, nc0.5, nc1.0, nc2.5, nc4.5, nc10.0, Typical Particle Size\n");
    fclose(SPS30csv); 
    
    //Add headers to SCD40 CSV file. (Make sure a CSV file is not already present as the header will be added to the exisitg CSV otherwise)
    FILE *SCD40csv;
    SCD40csv = fopen("../Outputs/CO2output.csv", "a");
    fprintf(SCD40csv,"Timestamp, CO2, Temperature, Humidity\n");
    fclose(SCD40csv);

    while (1) {

        //Start SPS30 Measurements
        ret = sps30_start_measurement();
        if (ret < 0) {
            printf("error starting measurement\n");
        }

        //SPS30 measurements begin -------------------------------
        for (int i = 0; i < 60; ++i) {
            
            //Used to store the time for each measurement
            time_t currentTime;
            currentTime = time(NULL);
            char *measurementTime = ctime(&currentTime);
            //Remove the line ending \n from cdate output to allow for use in CSV
            measurementTime[strlen(measurementTime)-1] = '\0';

            ret = sps30_read_measurement(&m);
            if (ret < 0) {
                printf("error reading measurement\n");
            } else {
                if (SPS30_IS_ERR_STATE(ret)) {
                    printf(
                        "Chip state: %u - measurements may not be accurate\n",
                        SPS30_GET_ERR_STATE(ret));
                }
                printf("SPS30 MEASUREMENT TAKEN - %s\n", measurementTime);
                printf("********************************\n");

                SPS30csv = fopen("../Outputs/PMoutput.csv", "a");
                fprintf(SPS30csv,"%s, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f \n", measurementTime, m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5, m.nc_1p0, m.nc_2p5, m.nc_4p0, m.nc_10p0, m.typical_particle_size);
                fclose(SPS30csv);

            }
            sensirion_sleep_usec(1000000); /* sleep for 1s */
            
            sensirion_i2c_hal_sleep_usec(5000000);

            uint16_t co2;
            float temperature;
            float humidity;
            error = scd4x_read_measurement(&co2, &temperature, &humidity);
            if (error) {
                printf("Error executing scd4x_read_measurement(): %i\n", error);
            } else if (co2 == 0) {
                printf("Invalid sample detected, skipping.\n");
            } else {
                printf("SCD40 MEASUREMENT TAKEN - %s\n", measurementTime);
                printf("********************************\n");
                SCD40csv = fopen("../Outputs/CO2output.csv", "a");
                fprintf(SCD40csv,"%s, %u, %0.2f, %0.2f\n", measurementTime, co2, temperature, humidity);
                fclose(SCD40csv);
            }

            //Wake up SPS30
            if (version_information.firmware_major >= 2) {
                ret = sps30_wake_up();
                if (ret) {
                    printf("Error %i waking up sensor\n", ret);
                }
            }
        }

        /* Stop measurement for 1min to preserve power. Also enter sleep mode
             * if the firmware version is >=2.0.
             */
            ret = sps30_stop_measurement();
            if (ret) {
                printf("Stopping measurement failed\n");
            }

        
        //Sleep SPS30
        if (version_information.firmware_major >= 2) {
            ret = sps30_sleep();
            if (ret) {
                printf("Entering sleep failed\n");
            }
        }

        //No measurements for 1 minute
        sensirion_sleep_usec(1000000 * 60);

        //Wake up SPS30 after minute
        if (version_information.firmware_major >= 2) {
            ret = sps30_wake_up();
            if (ret) {
                printf("Error %i waking up sensor\n", ret);
            }
        }
    }

    if (sensirion_uart_close() != 0)
        printf("failed to close UART\n");

    return 0;
}
