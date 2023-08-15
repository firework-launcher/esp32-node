# Node and controller protocol
## Format
Commands are sent in JSON. It is an object. The code key is the only required key for each command. The code key is to show what to do, or what information is presented. Unless the [documentation of each code](#codes) says otherwise, there is another key, payload which holds an array of values. The additional information column of the code documentation explains more about what is expected for each code.

## Codes

### ESP codes

| Code | Meaning             | Additional information                                                                                    |
|------|---------------------|-----------------------------------------------------------------------------------------------------------|
| 1    | Trigger Firework    | For the payload, multiple numbers can be passed in the array which set of every firework in series.       |
| 2    | Arm                 | This code does not have any payload.                                                                      |
| 3    | Disarm              | This code does not have any payload.                                                                      |

## Example

```JSON
{"code": 1, "payload": [5, 6]}
```

This example uses the code to trigger a firework, and passes 2 values in payload, 5 and 6. These values are taken as firework numbers and are both launched in series.