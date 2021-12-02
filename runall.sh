#!/bin/sh
echo "Deleting Old CSV files..."
rm Outputs/BMEoutput.csv && echo "Deleted BME688 CSV File"
rm Outputs/CO2output.csv && echo "Deleted SCD40 CSV File"
rm Outputs/PMoutput.csv && echo "Deleted SPS30 CSV File"
sleep 3

echo "Running BME688 Sensor..."
sleep 3
echo "***********************************************"
cd bme68x-python-library && python3 airqualitycsv.py &
echo "***********************************************"


echo "Running SCD40 Sensor..."
sleep 3
echo "***********************************************"
cd raspberry-pi-i2c-scd4x && make && ./scd4x_i2c_example_usage && fg
echo "***********************************************"


echo "Running SPS30 Sensor..."
sleep 3
echo "***********************************************"
cd sps30-uart-3.3.0 && make && ./sps30_example_usage && fg
echo "***********************************************"