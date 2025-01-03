# Performance Mixer Control Router

## Input Channels

Currently based on the factory settings for the Faderfox PC4. The Performance
Mixer currently supports 16 channels, each mapped to a midi channel of the
controller. The parameter mapping is as follows:

|Row 1|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|1|2|3|4|
|Device|SAT|SAT|SAT|SAT|
|Parameter|level in|drive|blend|level_out|

|Row 2|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|5|6|7|8|
|Device|COMP|COMP|COMP|COMP|
|Parameter|threshold|ratio|attack|release|

|Row 3|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|9|10|11|12|
|Device|COMP|COMP|COMP||
|Parameter|makeup|knee|mix||

|Row 4|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|13|14|15|16|
|Device|EQ|EQ|EQ|EQ|
|Parameter|low|mid|high|master|

|Row 5|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|17|18|19|20|
|Device|EQ|EQ|||
|Parameter|low_mid|mid_high| | |

|Row 6|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|21|22|23|24|
|Device|||||
|Parameter|||||

## Group Channels

The supported 4 Group Channels are currently mapped to a  Behringer CMD-1 DJ
Controller. The controller uses midi channel 5, but the channel is ignored.

|Pot Row 0|Out 1|Out 2|Cue Vol|Cue Mix|
|---|---|---|---|---|
|CC|1|2|4|5|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|send_1|send_1|send_1|send_1|

|Pot Row 1|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|6|7|8|9|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|send_2|send_2|send_2|send_2|

|Pot Row 2|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|10|11|12|13|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|high|high|high|high|

|Pot Row 3|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|14|15|16|17|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|mid|mid|mid|mid|

|Pot Row 4|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|18|19|20|21|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|low|low|low|low|

|Fader|Column 1|Column 2|Column 3|Column 4|
|---|---|---|---|---|
|CC|48|49|50|51|
|Device|EQ|EQ|EQ|EQ|
|Group Channel|1|2|3|4|
|Parameter|master|master|master|master|

## Selecting Layer A / B

Which Layer is currently active is controlled by the cross fader on the
Behringer CMD-1.

Control: Crossfader on CMD-1
Channel: *Ignored*
CC: 64
Logic:

- < 64, edit layer A
- >= 64, edit layer B

## Layer DJ Mixer

Mixing the layers is controlled by a Native Instruments DJ Controller.

## OSC Event Output

- All updates are sent out as osc messages on the `updates` port.
- The message is an osc bundle as raw bytes in a pod
- The time for the bundle is always `0L`, the time from the Pod should be used
  instead.
- The address for the messages is
  `/<channel_type>/<channel_number>/<device>`, where:
  - channel_type is one of:
    - I for input channels,
    - G for group channels,
    - L for layer channels
  - channel_number is one of:
    - `<layer_prefix><channel_number>` for channel type I and G
    - `<layer_prefix>` for channel type L
    - `<layer_prefix>` is either `A` or `B`
  - device is:
    - C for compressor,
    - S for saturator,
    - E for equalizer

## Notes

- Set channel mapping with:
  `pw-cli s <node_id> Props '{params = [pmx.channel_mapping "<mappings>"]}'`
- Mappings is a list of arrays, the first entry is the midi channel, the second
  entry is the node id of the input node of the filter chain
- Set group channel ids with
  `pw-cli s <node_id> Props '{params = [pmx.group_channel_node_ids "<list of group ids>"]}'`

## Example

```bash
pw-cli s 202 Props '{params = [pmx.channel_mapping "[0,205]"]}'

Object: size 72, type Spa:Pod:Object:Param:Props (262146), id Spa:Enum:ParamId:Props (2)
  Prop: key Spa:Pod:Object:Param:Props:params (524289), flags 00000000
    Struct: size 48
      String "pmx.channel_mapping"
      String "[0,205]"
```

The current setting can be checked with `pw-dump`.

```bash
pw-dump 202

[
  ...,
  {
    "id": 202,
    "type": "PipeWire:Interface:Node",
    "version": 3,
    "permissions": [ "r", "w", "x", "m" ],
    "info": {
      ...
      "props": {
        ...
        "pmx.channel_mapping": "[0,205]"
      },
      "params": {
        ...
      }
    }
  },
  ...
]
```
