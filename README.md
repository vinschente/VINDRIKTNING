# VINDRIKTNING

# UART RX & TX Log
## Recoding on serial lines RX & TX
| Nr | Direction | Data |
| --- | --- | --- |
| #1 | RX | 0x11 0x02 0x0B 0x01 0xE1 |
| #1 | TX | 0x16 0x11 0x0B 0x00 0x00 0x00 0x23 0x00 0x00 0x06 0x01 0x00 0x00 0x00 0x2D 0x02 0x00 0x00 0x0B 0x6A |
| #2 | RX | 0x11 0x02 0x0B 0x01 0xE1 |
| #2 | TX | 0x16 0x11 0x0B 0x00 0x00 0x00 0x26 0x00 0x00 0x06 0x0E 0x00 0x00 0x00 0x30 0x02 0x00 0x00 0x0B 0x57 |
| #3 | RX | 0x11 0x02 0x0B 0x01 0xE1 |
| #3 | TX | 0x16 0x11 0x0B 0x00 0x00 0x00 0x27 0x00 0x00 0x06 0x12 0x00 0x00 0x00 0x31 0x02 0x00 0x00 0x0B 0x51 |
| #4 | RX | 0x11 0x02 0x0B 0x01 0xE1 |
| #4 | TX | 0x16 0x11 0x0B 0x00 0x00 0x00 0x26 0x00 0x00 0x06 0x10 0x00 0x00 0x00 0x30 0x02 0x00 0x00 0x0B 0x55 |

(see PM1006_uart_log.txt for more serial recordings)

## Description from PM1006 datasheet
| Direction | Data |
| --- | --- |
| Send | 11 02 0B 01 E1 |
| Response | 16 11 0B DF1-DF4 DF5-DF8 DF9-DF12 DF13 DF14 DF15 DF16[CS] |

Note: PM2.5(μg/m³)= DF3*256+DF4

## Description from PM1006K datasheet
| Direction | Data |
| --- | --- |
| Send | 11 01 02 EC |
| Response | 16 0d 02 DF1-DF4 DF5-DF8 DF9-DF12 [CS]  |

PM2.5(μg/m³)= DF3*256+DF4\
PM1.0(μg/m³)= DF7*256+DF8\
PM10(μg/m³)= DF11*256+DF12

Communication with **PM1006** module seems match what I recorded from from IKEA VINDRIKTNING. But when looking at Byte DF7/DF8 and DF11/DF12 we can see that also some interesting data is stored there. Maybe IKEA got a cutomized version of PM1006 sensor for their product and the particle sensor is also measuring PM1.0 and PM10.  So let's interpret the datasets that I have recorded from my device with additional help of **PM1006K** datasheet.

## interpreting serial log\
| Nr | PM2.5 | (PM1.0) | (PM10)|
| --- | --- | --- | --- |
| #1 | 35 | 1537 | 45 |
| #2 | 38 | 1550 | 48 |
| #3 | 39 | 1554 | 49 |
| #4 | 38 | 1552 | 48 |


##

**Next Steps (TODOs):**\
\+ Connect ESP8266\
\+ Use ESPHome or MQTT and update data on server\
\+ record data and produce some fancy graphs
\+ Test device is different scenarios (e.g. fireplace, celler and so on)
