```
 _____ _____ __    _____    ___ ___ ___           _____ _         _    _____                 _   
|   __|   __|  |  |   __|  | | |  _|_  |   ___   |   __|_|___ ___| |  | __  |___ ___ ___ ___| |_ 
|   __|   __|  |__|   __|  |_  | . | | |  |___|  |   __| |   | .'| |  |    -| -_| . | . |  _|  _|
|_____|_____|_____|_____|    |_|___| |_|         |__|  |_|_|_|__,|_|  |__|__|___|  _|___|_| |_|  
                                                                                |_|              
```
# EELE 467 - Final Report

Brought to you by Grant Kirkland & Ken Vincent

## Table of Contents
* [System Overview](#system-overview)
* [Buzzer](#buzzer)
* [Water Sensor](#water-sensor)
* [Conclusion](#conclusion)

## System Overview

This project aims to create an enviornmental monitoring system, where water quality is monitored and the system alerts through both visual and auditory means if the quality drops below a set amount. The auditory alerts are implemented through a buzzer, that makes an awful sounds when triggered. The visual alerts are implemented through an led bar. These alerts are triggered by reading a water content sensor.

## Buzzer

The buzzer was implemented in VHDL by creating a square wave of variable period. This had a fixed 50% duty cycle, to more closely resemble a sine wave. This was further altered from the led control system by having the period value expanded to handle the higher frequencies required for auditory systems. This component was then connected to the avalon memory mapping interface and exported to the system level through a device tree and driver file. Further implementation of this component in software was not achieved during the project timeframe.

## Water Sensor

## Conclusion