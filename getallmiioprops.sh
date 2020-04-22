#!/bin/sh
echo "HumiSetvalue"
miio protocol call 237871184 get_prop '["HumiSet_Value"]'
echo "TemperatureValue"
miio protocol call 237871184 get_prop '["TemperatureValue"]'
echo "Humidity_Value"
miio protocol call 237871184 get_prop '["Humidity_Value"]'
#echo "soundstate"
#miio protocol call 237871184 get_prop '["TipSound_State"]'
#echo "onoffstate"
#miio protocol call 237871184 get_prop '["OnOff_State"]'
#echo "ledstate"
#miio protocol call 237871184 get_prop '["Led_State"]'
echo "gear"
miio protocol call 237871184 get_prop '["Humidifier_Gear"]'
echo "waterstatus"
miio protocol call 237871184 get_prop '["waterstatus"]'
echo "tankstatus"
miio protocol call 237871184 get_prop '["watertankstatus"]'
