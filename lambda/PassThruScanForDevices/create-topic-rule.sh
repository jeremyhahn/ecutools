#!/bin/bash

aws iot delete-topic-rule --rule-name PassThruScanForDevices
aws iot create-topic-rule --rule-name PassThruScanForDevices --topic-rule-payload file://iot-rule.json
