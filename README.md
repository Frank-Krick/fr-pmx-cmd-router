# Performance Mixer Control Router

- Set channel mapping with:
  `pw-cli s <node_id> Props '{params = [pmx.channel_mapping "<mappings>"]}'`
- Mappings is a list of arrays, the first entry is the midi channel, the second
  entry is the node id of the input node of the filter chain

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
