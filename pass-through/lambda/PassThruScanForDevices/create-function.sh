#!/bin/bash

zip PassThruScanForDevices.zip PassThruScanForDevices.js
aws lambda delete-function --function-name PassThruScanForDevices
aws lambda create-function \
  --function-name PassThruScanForDevices \
  --zip-file fileb://PassThruScanForDevices.zip \
  --role arn:aws:iam::899038310491:role/lambda-ecutools-execution-role \
  --handler PassThruScanForDevices.handler \
  --runtime nodejs4.3
rm PassThruScanForDevices.zip
