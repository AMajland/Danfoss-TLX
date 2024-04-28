# Danfoss-TLX

RS485 Communication with Danfoss TLX solar inverter and REST server. Based on [TLX-ino by Torben](https://github.com/TorbenNor/Danfoss-TLX).

I use it for Home Assistant integration where each parameter/value can be read as a sensor.

See DanfossTLX-RS485.h for parameters.

[@exetico](https://github.com/exetico) have added support for Platform.io, and I'll recommend you to start with that, instead of the (a bit) aging Arduno IDE software.

```yaml
sensor:
  # Danfoss TLX
  # Energy
  - platform: rest
    resource: http://[IP-Address]/TotalE
    name: "TLX Total Energy Production"
    method: GET
    value_template: "{{ value_json.value |round(3)}}"
    unit_of_measurement: kWh
    device_class: "energy"
    state_class: total_increasing
```

But since HA would fire request for all entities almost at the same time it could'nt keep up and some would end up unavailable.
So instead of having each value returned in a seperat call i've added them all in stame same json return. See configuration.yaml for all rest sensors

```yaml
    rest:
      - resource: http://[IP-Address]/All
        sensor:
          # Info
          - name: "TLX Operation Mode"
            value_template: "{{ value_json.OpMode }}"
          - name: "TLX Operation Mode Text"
            value_template: "{{ value_json.OpModeTxt }}"
          - name: "TLX Product Number"
            value_template: "{{ value_json.Product }}"
          - name: "TLX Serial"
           value_template: "{{ value_json.Serial }}"
          # Produkction Energy
          - name: "TLX Total Energy Production"
            value_template: "{{ value_json.TotalE |round(3)}}"
            unit_of_measurement: kWh
            device_class: 'energy'
            state_class: total_increasing
            ...
```

For this project, I'll use pin 16 for RX (Serial2), and pin 17 for TX (Serial2).

If you face any problems with the communication between the Danfoss device and the ESP, try and swap TX and RX.

## Libraries

Used Libraries - output from verbose compile
Multiple libraries were found for "WiFi.h" Used: ...\packages\esp32\hardware\esp32\2.0.3\libraries\WiFi
Using library WiFi at version 2.0.0 in folder: ...\packages\esp32\hardware\esp32\2.0.3\libraries\WiFi
Using library WebServer at version 2.0.0 in folder: ...\packages\esp32\hardware\esp32\2.0.3\libraries\WebServer
Using library ArduinoJson at version 6.19.4 in folder: ...\libraries\ArduinoJson
Using library FS at version 2.0.0 in folder: ...\packages\esp32\hardware\esp32\2.0.3\libraries\FS

## How to use

### Platform.io

Open the project, and do whatever you'd normally do with Platform.io. Here you're also able to use OTA updates in the future, after the first flash by USB.

### Arduino IDE

Secure that you've downloaded the libs, and use the source-code from the `src` folder.

If you face any problems with compiling with Arduino IDE, kindly give Platform.io a try in VS Code.

With that said, try and rename the `main.ccp` file to `main.ino`.

## BOM

LOLIN D32 (ESP32): [AliExpres](https://www.aliexpress.com/item/32808551116.htm)

_The project can be used with other ESP32 boards._

R411A01 mini 3.3V Auto RS485 to TTL232 Converter Board: [AliExpress](https://www.aliexpress.com/item/32782552104.html)

_You can use other RS485 to TTL232 boards._

## Details about RS485 and ComLynx

Kindly check the `Assets` folder for more info. Here you'll find `ComLynx_User_Guide_16_20120817_A7.pdf`, a guide to accessing inverter parameters via RS485 using ComLynx protocol. The file are originally made by Danfoss.
