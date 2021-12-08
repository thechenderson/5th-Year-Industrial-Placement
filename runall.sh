#!/bin/sh

# This script deletes all old csv files that appear in the Outputs folder, compiles the programs for running both Sensirion sensors and the Bosch sensor and then runs them.

echo "Deleting Old CSV files..."
echo "****************************************************"
rm Outputs/BMEoutput.csv && echo "Deleted BME688 CSV File"
rm Outputs/CO2output.csv && echo "Deleted SCD40 CSV File"
rm Outputs/PMoutput.csv && echo "Deleted SPS30 CSV File"
echo "****************************************************"

cd sps30-uart-3.3.0 && make && ./sps30_example_usage & cd ../raspberry-pi-i2c-scd4x && make && ./scd4x_i2c_example_usage && cd ../bme68x-python-library && python3 airqualitycsv.py 