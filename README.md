# SimFlow

**Visual Automation for Flight Simulators**

SimFlow connects your hardware controllers and flight simulator through a visual flow-based interface. Build automation workflows, manage checklists, and calibrate your controllers—all without writing code.

> **⚠️ Early Alpha Software**  
> SimFlow is currently in early alpha development. You may encounter bugs, incomplete features, or unexpected behavior. Please report any issues you find. [here](https://github.com/TimoHocker/SimFlow/issues)

## Features

### Visual Flow Editor

- Drag-and-drop nodes to create automation workflows
- Completely open flow structure for maximum flexibility
  Build a checklist for your A320 or automate your toilet flush at home, whatever you come up with

### Hardware Integration

- **Serial Devices**: Connect to custom Arduino based controllers or other programmable serial hardware

To connect to a serial device, a simple handshake has to be completed before the device becomes available in the flow editor.

```txt
-> identify simflow serial
<- IDENT radiopanel
```

SimFlow will send `identify simflow serial` to any applicable serial port. If a device responds with `IDENT <name>`, it will be registered as a device with the given name. It's important that the `IDENT` response is the first response sent by the device after receiving the identify command, otherwise SimFlow will ignore the device.

### Flight Simulator Integration

- Read and write simulator variables
- Trigger simulator events
- Monitor aircraft state in real-time

### Interactive Checklists

- Create dynamic checklists that respond to simulator state

## License

Copyright © 2026 Timo Hocker. All rights reserved.

This software is provided free of charge for personal use only. You may not reverse engineer, decompile, or modify the software. No warranty is provided.
