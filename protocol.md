# Node and controller protocol
## Format
Commands are seperated by `\n`. Considering that there are Regex patterns in place on the controller to stop anyone from putting in a value other than numbers 0-9, letters a-z and A-Z, there are codes that are in other than these values that tell the controller and esp what values mean what. The codes are listed in the section, [Codes](#codes). This kind of format is done to save space with sending regular messages since the ESP can only recieve 128 bytes at a time, making JSON very inefficient. Patterns will send json, but it will recieve it 128 bytes at a time, and only the pattern will be sent in that one message. The value for each command is a list, the seperator is the code 0x7f. To terminate the list, use 0x80.

## Codes

### Special codes

| Code | Meaning          | Sent by    | Additional information             |
|------|------------------|------------|------------------------------------|
| 0x7f | Seperator        | Both       | Seperates the values in a command. |
| 0x80 | Terminate        | Both       | Terminates a list of values        |

### General data
| Code | Meaning          | Sent by    | Additional information |
|------|------------------|------------|------------------------|
| 0x81 | Signal strength  | ESP        | WiFi Signal Strength   |

### Fireworks
| Code | Meaning          | Sent by    | Additional information                     |
|------|------------------|------------|--------------------------------------------|
| 0x82 | Trigger firework | Controller |                                            |
| 0x83 | Detector change  | ESP        | `[firework, value]`                        |
| 0x84 | Pattern start    | Controller | Sends pattern data, read [Format](#format) |
| 0x85 | Pattern end      | Controller | Sends pattern data, read [Format](#format) |
| 0x86 | Run pattern      | Controller |                                            |

## Example

`0x82` 1 `0x7f` 2 `0x80`
