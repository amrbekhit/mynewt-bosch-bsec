# Bosch BSEC Sensor Fusion Library for Apache Mynewt

## Introduction
This repo packages the [Bosch BSEC Sensor Fusion Library](https://www.bosch-sensortec.com/bst/products/all_products/bsec) for the [BME680 Environmental Sensor](https://www.bosch-sensortec.com/bst/products/all_products/bme680) into an [Apache Mynewt](https://mynewt.apache.org/)-compatible package.

## Quick-Start
1. In a new or existing Mynewt project, add mynewt-bosch-bsec to the list of repos. The repo name is `mynewt-bosch-bsec`. Use the following for the repo details:
```yml
repository.mynewt-bosch-bsec:
    type: github
    vers: 1-latest
    user: amrbekhit
    repo: mynewt-bosch-bsec
```
2. Run `newt upgrade` to download and install the repo.
3. Download the [Bosch BSEC Sensor Fusion Library](https://www.bosch-sensortec.com/bst/products/all_products/bsec). After extracting the archive, identify the correct binary for your device and toolchain under `algo/bin/Normal_version`. Copy the `libalgobsec.a` file into the `repos/mynewtbosch_bsec/src` folder.
3. Make sure to include the BME680 package in your app. This can be found in `@apache-mynewt-core/hw/drivers/sensors/bme680`.
4. Configure the mynewt-bosch-bsec library. The most important syscfg values are `BOSCH_BSEC_TIMER_INIT` and `BOSCH_BSEC_CONFIG`. It is possible to reuse an existing timer rather then set up a new one - if so, simply set `BOSCH_BSEC_TIMER_INIT` to 0.
5. Persistence of the library state is configured using `BOSCH_BSEC_SAVE_STATE`. This uses the Mynewt fs library to save a file containing the state.
6. In order to access the data from the sensor, create a callback of type `output_ready_fct`, and register it with the BSEC library by calling the `bosch_bsec_output_cb` function. When new data is ready, the callback function will be called with all the new data values.

## How It Works
The mynewt-bosch-bsec library creates a thread, called `bsec`, which initialises the BSEC library and then starts the BSEC loop. This loop regularly polls the BME680 sensor and processes all of the sensor inputs, calling the `output_cb` function with the new data. If enabled, the library will periodically save the library state to disk. This state is loaded back into the library on bootup.

## Lower Power Recommendations
*The following is based on using the nRF series of micros with the BME680*
- In between polling the sensor, the bsec thread sleeps and thus allows the OS to run the idle task, putting the CPU to sleep and consuming minimal power.
- The main source of power consumption is the timer. It is recommended to use the RTC timer (and adjust `BOSCH_BSEC_TIMER_FREQ` accordingly), as this can run while the CPU is in sleep mode and consume very little power.
