# Node and controller protocol
## Format
Commands are sent in JSON. It is an object. The code key is the only required key for each command. The code key is to show what to do, or what information is presented. Unless the [documentation of each code](#codes) says otherwise, there is another key, payload which holds an array of values. The additional information column of the code documentation explains more about what is expected for each code.

## Codes

### ESP codes

| Code | Meaning             | Additional information                                                                                        |
|------|---------------------|---------------------------------------------------------------------------------------------------------------|
| 1    | Trigger Firework    | For the payload, first number would be the firework to launch, and the second number would be the PWM to use. |
| 2    | Arm                 | This code does not have any payload. Will lock up web gui if ran in Advanced Settings and not connected to a controller. |
| 3    | Disarm              | This code does not have any payload.                                                                          |
| 4    | Run step            | There are 2 arrays within the payload array. the first one being an array of all the pins, and the second one being an array for the PWM values with the indexes lining up with the pins. |
| 5    | Enters OTA          | No payload                                                                                                    |
| 6    | Arm with no lights  | This code does not have any payload. Can be safely ran before it connects to the controller.                  |
| 7    | Disarm no lights    | No payload                                                                                                    |
## Example

```JSON
{"code": 1, "payload": [5, 1875]}
```

This example uses the code to trigger a firework, and passes 2 values in payload, 5 and 1875. The 5 is the pin number, the 1875 is the PWM.

# UDP Auto Discovery
There is a UDP broadcast from the controller every 5 seconds that reads NODE_DISCOVERY. When a node picks this up, it responds with NODE_RESPONSE, and the controller with see that, get the IP address and try to add the node.