# TinyENV nRF Clusters
### Power Source - 0x002F
#### Enabled Features
* Battery
* Replaceable
#### Enabled Attributes:
* BatChargeLevel
* BatPercentRemaining
* BatPresent - 1
* BatQuantity - 1
* BatReplaceability - 2
* BatReplacementDescription
* BatReplacementNeeded - 0
* BatVoltage
* ClusterRevision
* Description - "Battery"
* EndpointList
* FeatureMap - 10
* Order - 0
* Status

### Temperature Measurement - 0x0402
#### Enabled Features
N/A
#### Enabled Attributes
* MeasuredValue
* MinMeasuredValue - -2000 (negative integer)
* MaxMeasuredValue - 2000
* FeatureMap - 0
* ClusterRevision - 4


### Relative Humidity - 0x0405
#### Enabled Features
N/A
#### Enabled Attributes
* MeasuredValue
* MinMeasuredValue - 0
* MaxMeasuredValue - 10000
* FeatureMap - 0
* ClusterRevision - 3
